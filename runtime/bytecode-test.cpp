#include "bytecode.h"

#include "gtest/gtest.h"

#include "ic.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using BytecodeTest = RuntimeFixture;

TEST_F(BytecodeTest, NextBytecodeOpReturnsNextBytecodeOpPair) {
  HandleScope scope(thread_);
  byte bytecode_raw[] = {
      NOP,          99, EXTENDED_ARG, 0xca, LOAD_ATTR,    0xfe, LOAD_GLOBAL, 10,
      EXTENDED_ARG, 1,  EXTENDED_ARG, 2,    EXTENDED_ARG, 3,    LOAD_ATTR,   4};
  Bytes original_bytecode(&scope, runtime_.newBytesWithAll(bytecode_raw));
  MutableBytes bytecode(
      &scope, runtime_.mutableBytesFromBytes(thread_, original_bytecode));
  word index = 0;
  BytecodeOp bc = nextBytecodeOp(bytecode, &index);
  EXPECT_EQ(bc.bc, NOP);
  EXPECT_EQ(bc.arg, 99);

  bc = nextBytecodeOp(bytecode, &index);
  EXPECT_EQ(bc.bc, LOAD_ATTR);
  EXPECT_EQ(bc.arg, 0xcafe);

  bc = nextBytecodeOp(bytecode, &index);
  EXPECT_EQ(bc.bc, LOAD_GLOBAL);
  EXPECT_EQ(bc.arg, 10);

  bc = nextBytecodeOp(bytecode, &index);
  EXPECT_EQ(bc.bc, LOAD_ATTR);
  EXPECT_EQ(bc.arg, 0x01020304);
}

TEST_F(BytecodeTest, OpargFromObject) {
  EXPECT_EQ(NoneType::object(),
            objectFromOparg(opargFromObject(NoneType::object())));
  EXPECT_EQ(Bool::trueObj(), objectFromOparg(opargFromObject(Bool::trueObj())));
  EXPECT_EQ(Bool::falseObj(),
            objectFromOparg(opargFromObject(Bool::falseObj())));
  EXPECT_EQ(SmallInt::fromWord(-1),
            objectFromOparg(opargFromObject(SmallInt::fromWord(-1))));
  EXPECT_EQ(SmallInt::fromWord(-64),
            objectFromOparg(opargFromObject(SmallInt::fromWord(-64))));
  EXPECT_EQ(SmallInt::fromWord(0),
            objectFromOparg(opargFromObject(SmallInt::fromWord(0))));
  EXPECT_EQ(SmallInt::fromWord(63),
            objectFromOparg(opargFromObject(SmallInt::fromWord(63))));
  EXPECT_EQ(Str::empty(), objectFromOparg(opargFromObject(Str::empty())));
  // Not immediate since it doesn't fit in byte.
  EXPECT_NE(SmallInt::fromWord(64),
            objectFromOparg(opargFromObject(SmallInt::fromWord(64))));
}

TEST_F(BytecodeTest, RewriteBytecodeWithMoreThanCacheLimitCapsRewriting) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[514];
  for (int i = 0; i < 257; i++) {
    bytecode[i * 2] = LOAD_ATTR;
    bytecode[(i * 2) + 1] = i * 3;
  }
  code.setCode(runtime_.newBytesWithAll(bytecode));
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[514];
  for (int i = 0; i < 257; i++) {
    expected[i * 2] = LOAD_ATTR_ANAMORPHIC;
    expected[(i * 2) + 1] = i;
  }

  // Bytecode inline cache rewritting is capped at 256 entires
  expected[512] = LOAD_ATTR;
  expected[513] = static_cast<byte>(256);

  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesLoadAttrOperations) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      NOP,          99,        EXTENDED_ARG, 0xca, LOAD_ATTR,    0xfe,
      NOP,          LOAD_ATTR, EXTENDED_ARG, 1,    EXTENDED_ARG, 2,
      EXTENDED_ARG, 3,         LOAD_ATTR,    4,    LOAD_ATTR,    77,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      NOP,          99,        EXTENDED_ARG,         0, LOAD_ATTR_ANAMORPHIC, 0,
      NOP,          LOAD_ATTR, EXTENDED_ARG,         0, EXTENDED_ARG,         0,
      EXTENDED_ARG, 0,         LOAD_ATTR_ANAMORPHIC, 1, LOAD_ATTR_ANAMORPHIC, 2,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  EXPECT_EQ(caches.length(), 3 * kIcPointersPerCache);
  for (word i = 0, length = caches.length(); i < length; i++) {
    EXPECT_TRUE(caches.at(i).isNoneType()) << "index " << i;
  }

  EXPECT_EQ(originalArg(*function, 0), 0xcafe);
  EXPECT_EQ(originalArg(*function, 1), 0x01020304);
  EXPECT_EQ(originalArg(*function, 2), 77);
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesZeroArgMethodCalls) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      NOP,          99,        EXTENDED_ARG,  0xca, LOAD_ATTR,     0xfe,
      NOP,          LOAD_ATTR, EXTENDED_ARG,  1,    EXTENDED_ARG,  2,
      EXTENDED_ARG, 3,         LOAD_ATTR,     4,    CALL_FUNCTION, 1,
      LOAD_ATTR,    4,         CALL_FUNCTION, 0};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));

  byte expected[] = {NOP,
                     99,
                     EXTENDED_ARG,
                     0,
                     LOAD_ATTR_ANAMORPHIC,
                     0,
                     NOP,
                     LOAD_ATTR,
                     EXTENDED_ARG,
                     0,
                     EXTENDED_ARG,
                     0,
                     EXTENDED_ARG,
                     0,
                     LOAD_ATTR_ANAMORPHIC,
                     1,
                     CALL_FUNCTION,
                     1,
                     LOAD_METHOD_ANAMORPHIC,
                     2,
                     CALL_METHOD,
                     0};
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  EXPECT_EQ(caches.length(), 3 * kIcPointersPerCache);
  for (word i = 0, length = caches.length(); i < length; i++) {
    EXPECT_TRUE(caches.at(i).isNoneType()) << "index " << i;
  }

  EXPECT_EQ(originalArg(*function, 0), 0xcafe);
  EXPECT_EQ(originalArg(*function, 1), 0x01020304);
  EXPECT_EQ(originalArg(*function, 2), 4);
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesLoadConstOperations) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      LOAD_CONST, 0, LOAD_CONST, 1, LOAD_CONST, 2, LOAD_CONST, 3,
      LOAD_CONST, 4, LOAD_CONST, 5, LOAD_CONST, 6,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));

  Tuple consts(&scope, runtime_.newTuple(10));
  // Immediate objects.
  consts.atPut(0, NoneType::object());
  consts.atPut(1, Bool::trueObj());
  consts.atPut(2, Bool::falseObj());
  consts.atPut(3, SmallInt::fromWord(0));
  consts.atPut(4, Str::empty());
  // Not immediate since it doesn't fit in byte.
  consts.atPut(5, SmallInt::fromWord(64));
  // Not immediate since it's a heap object.
  consts.atPut(6, runtime_.newTuple(4));
  code.setConsts(*consts);

  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));

  byte expected[] = {
      LOAD_IMMEDIATE, static_cast<byte>(opargFromObject(NoneType::object())),
      LOAD_IMMEDIATE, static_cast<byte>(opargFromObject(Bool::trueObj())),
      LOAD_IMMEDIATE, static_cast<byte>(opargFromObject(Bool::falseObj())),
      LOAD_IMMEDIATE, static_cast<byte>(opargFromObject(SmallInt::fromWord(0))),
      LOAD_IMMEDIATE, static_cast<byte>(opargFromObject(Str::empty())),
      LOAD_CONST,     5,
      LOAD_CONST,     6,
  };
  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesLoadMethodOperations) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      NOP,          99,          EXTENDED_ARG, 0xca, LOAD_METHOD,  0xfe,
      NOP,          LOAD_METHOD, EXTENDED_ARG, 1,    EXTENDED_ARG, 2,
      EXTENDED_ARG, 3,           LOAD_METHOD,  4,    LOAD_METHOD,  77,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      NOP,
      99,
      EXTENDED_ARG,
      0,
      LOAD_METHOD_ANAMORPHIC,
      0,
      NOP,
      LOAD_METHOD,
      EXTENDED_ARG,
      0,
      EXTENDED_ARG,
      0,
      EXTENDED_ARG,
      0,
      LOAD_METHOD_ANAMORPHIC,
      1,
      LOAD_METHOD_ANAMORPHIC,
      2,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  EXPECT_EQ(caches.length(), 3 * kIcPointersPerCache);
  for (word i = 0, length = caches.length(); i < length; i++) {
    EXPECT_TRUE(caches.at(i).isNoneType()) << "index " << i;
  }

  EXPECT_EQ(originalArg(*function, 0), 0xcafe);
  EXPECT_EQ(originalArg(*function, 1), 0x01020304);
  EXPECT_EQ(originalArg(*function, 2), 77);
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesStoreAttr) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {STORE_ATTR, 48};
  code.setCode(runtime_.newBytesWithAll(bytecode));
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {STORE_ATTR_CACHED, 0};
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  EXPECT_EQ(originalArg(*function, 0), 48);
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesBinaryOpcodes) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      BINARY_MATRIX_MULTIPLY,
      0,
      BINARY_POWER,
      0,
      BINARY_MULTIPLY,
      0,
      BINARY_MODULO,
      0,
      BINARY_ADD,
      0,
      BINARY_SUBTRACT,
      0,
      BINARY_FLOOR_DIVIDE,
      0,
      BINARY_TRUE_DIVIDE,
      0,
      BINARY_LSHIFT,
      0,
      BINARY_RSHIFT,
      0,
      BINARY_AND,
      0,
      BINARY_XOR,
      0,
      BINARY_OR,
      0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      BINARY_OP_CACHED, 0,  BINARY_OP_CACHED, 1,  BINARY_OP_CACHED, 2,
      BINARY_OP_CACHED, 3,  BINARY_OP_CACHED, 4,  BINARY_OP_CACHED, 5,
      BINARY_OP_CACHED, 6,  BINARY_OP_CACHED, 7,  BINARY_OP_CACHED, 8,
      BINARY_OP_CACHED, 9,  BINARY_OP_CACHED, 10, BINARY_OP_CACHED, 11,
      BINARY_OP_CACHED, 12,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  EXPECT_EQ(originalArg(*function, 0),
            static_cast<word>(Interpreter::BinaryOp::MATMUL));
  EXPECT_EQ(originalArg(*function, 1),
            static_cast<word>(Interpreter::BinaryOp::POW));
  EXPECT_EQ(originalArg(*function, 2),
            static_cast<word>(Interpreter::BinaryOp::MUL));
  EXPECT_EQ(originalArg(*function, 3),
            static_cast<word>(Interpreter::BinaryOp::MOD));
  EXPECT_EQ(originalArg(*function, 4),
            static_cast<word>(Interpreter::BinaryOp::ADD));
  EXPECT_EQ(originalArg(*function, 5),
            static_cast<word>(Interpreter::BinaryOp::SUB));
  EXPECT_EQ(originalArg(*function, 6),
            static_cast<word>(Interpreter::BinaryOp::FLOORDIV));
  EXPECT_EQ(originalArg(*function, 7),
            static_cast<word>(Interpreter::BinaryOp::TRUEDIV));
  EXPECT_EQ(originalArg(*function, 8),
            static_cast<word>(Interpreter::BinaryOp::LSHIFT));
  EXPECT_EQ(originalArg(*function, 9),
            static_cast<word>(Interpreter::BinaryOp::RSHIFT));
  EXPECT_EQ(originalArg(*function, 10),
            static_cast<word>(Interpreter::BinaryOp::AND));
  EXPECT_EQ(originalArg(*function, 11),
            static_cast<word>(Interpreter::BinaryOp::XOR));
  EXPECT_EQ(originalArg(*function, 12),
            static_cast<word>(Interpreter::BinaryOp::OR));
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesInplaceOpcodes) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      INPLACE_MATRIX_MULTIPLY,
      0,
      INPLACE_POWER,
      0,
      INPLACE_MULTIPLY,
      0,
      INPLACE_MODULO,
      0,
      INPLACE_ADD,
      0,
      INPLACE_SUBTRACT,
      0,
      INPLACE_FLOOR_DIVIDE,
      0,
      INPLACE_TRUE_DIVIDE,
      0,
      INPLACE_LSHIFT,
      0,
      INPLACE_RSHIFT,
      0,
      INPLACE_AND,
      0,
      INPLACE_XOR,
      0,
      INPLACE_OR,
      0,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      INPLACE_OP_CACHED, 0,  INPLACE_OP_CACHED, 1,  INPLACE_OP_CACHED, 2,
      INPLACE_OP_CACHED, 3,  INPLACE_OP_CACHED, 4,  INPLACE_OP_CACHED, 5,
      INPLACE_OP_CACHED, 6,  INPLACE_OP_CACHED, 7,  INPLACE_OP_CACHED, 8,
      INPLACE_OP_CACHED, 9,  INPLACE_OP_CACHED, 10, INPLACE_OP_CACHED, 11,
      INPLACE_OP_CACHED, 12,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  EXPECT_EQ(originalArg(*function, 0),
            static_cast<word>(Interpreter::BinaryOp::MATMUL));
  EXPECT_EQ(originalArg(*function, 1),
            static_cast<word>(Interpreter::BinaryOp::POW));
  EXPECT_EQ(originalArg(*function, 2),
            static_cast<word>(Interpreter::BinaryOp::MUL));
  EXPECT_EQ(originalArg(*function, 3),
            static_cast<word>(Interpreter::BinaryOp::MOD));
  EXPECT_EQ(originalArg(*function, 4),
            static_cast<word>(Interpreter::BinaryOp::ADD));
  EXPECT_EQ(originalArg(*function, 5),
            static_cast<word>(Interpreter::BinaryOp::SUB));
  EXPECT_EQ(originalArg(*function, 6),
            static_cast<word>(Interpreter::BinaryOp::FLOORDIV));
  EXPECT_EQ(originalArg(*function, 7),
            static_cast<word>(Interpreter::BinaryOp::TRUEDIV));
  EXPECT_EQ(originalArg(*function, 8),
            static_cast<word>(Interpreter::BinaryOp::LSHIFT));
  EXPECT_EQ(originalArg(*function, 9),
            static_cast<word>(Interpreter::BinaryOp::RSHIFT));
  EXPECT_EQ(originalArg(*function, 10),
            static_cast<word>(Interpreter::BinaryOp::AND));
  EXPECT_EQ(originalArg(*function, 11),
            static_cast<word>(Interpreter::BinaryOp::XOR));
  EXPECT_EQ(originalArg(*function, 12),
            static_cast<word>(Interpreter::BinaryOp::OR));
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesCompareOpOpcodes) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      COMPARE_OP, CompareOp::LT,        COMPARE_OP, CompareOp::LE,
      COMPARE_OP, CompareOp::EQ,        COMPARE_OP, CompareOp::NE,
      COMPARE_OP, CompareOp::GT,        COMPARE_OP, CompareOp::GE,
      COMPARE_OP, CompareOp::IN,        COMPARE_OP, CompareOp::NOT_IN,
      COMPARE_OP, CompareOp::IS,        COMPARE_OP, CompareOp::IS_NOT,
      COMPARE_OP, CompareOp::EXC_MATCH,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      COMPARE_OP_CACHED, 0,
      COMPARE_OP_CACHED, 1,
      COMPARE_OP_CACHED, 2,
      COMPARE_OP_CACHED, 3,
      COMPARE_OP_CACHED, 4,
      COMPARE_OP_CACHED, 5,
      COMPARE_OP,        CompareOp::IN,
      COMPARE_OP,        CompareOp::NOT_IN,
      COMPARE_IS,        0,
      COMPARE_IS_NOT,    0,
      COMPARE_OP,        CompareOp::EXC_MATCH,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  EXPECT_EQ(originalArg(*function, 0), static_cast<word>(CompareOp::LT));
  EXPECT_EQ(originalArg(*function, 1), static_cast<word>(CompareOp::LE));
  EXPECT_EQ(originalArg(*function, 2), static_cast<word>(CompareOp::EQ));
  EXPECT_EQ(originalArg(*function, 3), static_cast<word>(CompareOp::NE));
  EXPECT_EQ(originalArg(*function, 4), static_cast<word>(CompareOp::GT));
  EXPECT_EQ(originalArg(*function, 5), static_cast<word>(CompareOp::GE));
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesReservesCachesForGlobalVariables) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      LOAD_GLOBAL, 0, STORE_GLOBAL, 1, LOAD_ATTR, 9, DELETE_GLOBAL, 2,
      STORE_NAME,  3, DELETE_NAME,  4, LOAD_ATTR, 9, LOAD_NAME,     5,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setNames(runtime_.newTuple(12));
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      LOAD_GLOBAL,
      0,
      STORE_GLOBAL,
      1,
      // Note that LOAD_ATTR's cache index starts at 2 to reserve the first 2
      // cache lines for 12 global variables.
      LOAD_ATTR_ANAMORPHIC,
      2,
      DELETE_GLOBAL,
      2,
      STORE_NAME,
      3,
      DELETE_NAME,
      4,
      LOAD_ATTR_ANAMORPHIC,
      3,
      LOAD_NAME,
      5,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  Tuple caches(&scope, function.caches());
  // 12 for global names is round to 2 cache lines each of which is 8 entries.
  EXPECT_EQ(caches.length(), (2 + 2) * kIcPointersPerCache);
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesLoadFastAndStoreFastOpcodes) {
  HandleScope scope(thread_);
  Tuple varnames(&scope, runtime_.newTuple(3));
  varnames.atPut(0, Runtime::internStrFromCStr(thread_, "arg0"));
  varnames.atPut(1, Runtime::internStrFromCStr(thread_, "var0"));
  varnames.atPut(2, Runtime::internStrFromCStr(thread_, "var1"));
  Tuple freevars(&scope, runtime_.newTuple(1));
  freevars.atPut(0, Runtime::internStrFromCStr(thread_, "freevar0"));
  Tuple cellvars(&scope, runtime_.newTuple(1));
  cellvars.atPut(0, Runtime::internStrFromCStr(thread_, "cellvar0"));
  word argcount = 1;
  word nlocals = 3;
  byte bytecode[] = {
      LOAD_FAST,  2, LOAD_FAST,  1, LOAD_FAST,  0,
      STORE_FAST, 2, STORE_FAST, 1, STORE_FAST, 0,
  };
  Bytes code_code(&scope, runtime_.newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime_.emptyTuple());
  Object empty_string(&scope, Str::empty());
  Object lnotab(&scope, Bytes::empty());
  Code code(&scope,
            runtime_.newCode(argcount, /*posonlyargcount=*/0,
                             /*kwonlyargcount=*/0, nlocals,
                             /*stacksize=*/0, /*flags=*/0, code_code,
                             /*consts=*/empty_tuple, /*names=*/empty_tuple,
                             varnames, freevars, cellvars,
                             /*filename=*/empty_string, /*name=*/empty_string,
                             /*firstlineno=*/0, lnotab));
  code.setFlags(code.flags() | Code::Flags::kOptimized);

  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope, runtime_.newFunctionWithCode(thread_, empty_string,
                                                         code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      LOAD_FAST_REVERSE,  2, LOAD_FAST_REVERSE,  3, LOAD_FAST_REVERSE,  4,
      STORE_FAST_REVERSE, 2, STORE_FAST_REVERSE, 3, STORE_FAST_REVERSE, 4,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));
  EXPECT_EQ(Tuple::cast(function.originalArguments()).length(), 0);
  EXPECT_EQ(Tuple::cast(function.caches()).length(), 0);
}

TEST_F(BytecodeTest,
       RewriteBytecodeDoesNotRewriteFunctionsWithNoOptimizedNorNewLocalsFlag) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      NOP,          99,        EXTENDED_ARG, 0xca, LOAD_ATTR,    0xfe,
      NOP,          LOAD_ATTR, EXTENDED_ARG, 1,    EXTENDED_ARG, 2,
      EXTENDED_ARG, 3,         LOAD_ATTR,    4,    LOAD_ATTR,    77,
  };
  code.setCode(runtime_.newBytesWithAll(bytecode));
  code.setFlags(code.flags() & ~Code::Flags::kOptimized &
                ~Code::Flags::kNewlocals);
  Module module(&scope, runtime_.findOrCreateMainModule());
  Function function(&scope,
                    runtime_.newFunctionWithCode(thread_, name, code, module));

  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, bytecode));
}

}  // namespace py
