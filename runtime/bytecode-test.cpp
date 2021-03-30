#include "bytecode.h"

#include "gtest/gtest.h"

#include "ic.h"
#include "test-utils.h"

namespace py {
namespace testing {

using BytecodeTest = RuntimeFixture;

TEST_F(BytecodeTest, NextBytecodeOpReturnsNextBytecodeOpPair) {
  HandleScope scope(thread_);
  byte bytecode_raw[] = {NOP,          99,   0, 0, EXTENDED_ARG, 0xca, 0, 0,
                         LOAD_ATTR,    0xfe, 0, 0, LOAD_GLOBAL,  10,   0, 0,
                         EXTENDED_ARG, 1,    0, 0, EXTENDED_ARG, 2,    0, 0,
                         EXTENDED_ARG, 3,    0, 0, LOAD_ATTR,    4,    0, 0};
  Bytes original_bytecode(&scope, runtime_->newBytesWithAll(bytecode_raw));
  MutableBytes bytecode(
      &scope, runtime_->mutableBytesFromBytes(thread_, original_bytecode));
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
  static const int cache_limit = 65536;
  byte bytecode[(cache_limit + 2) * kCompilerCodeUnitSize];
  std::memset(bytecode, 0, sizeof bytecode);
  for (int i = 0; i < cache_limit; i++) {
    bytecode[i * kCompilerCodeUnitSize] = LOAD_ATTR;
    bytecode[(i * kCompilerCodeUnitSize) + 1] = i * 3;
  }
  // LOAD_GLOBAL 527 == 4 * 128 + 15.
  bytecode[cache_limit * kCompilerCodeUnitSize] = EXTENDED_ARG;
  bytecode[cache_limit * kCompilerCodeUnitSize + 1] = 4;
  bytecode[(cache_limit + 1) * kCompilerCodeUnitSize] = LOAD_GLOBAL;
  bytecode[(cache_limit + 1) * kCompilerCodeUnitSize + 1] = 15;

  byte rewritten_bytecode[(cache_limit + 2) * kCodeUnitSize];
  std::memset(rewritten_bytecode, 0, sizeof rewritten_bytecode);
  for (int i = 0; i < cache_limit; i++) {
    rewritten_bytecode[i * kCodeUnitSize] = LOAD_ATTR;
    rewritten_bytecode[(i * kCodeUnitSize) + 1] = i * 3;
  }
  // LOAD_GLOBAL 527 == 4 * 128 + 15.
  rewritten_bytecode[cache_limit * kCodeUnitSize] = EXTENDED_ARG;
  rewritten_bytecode[cache_limit * kCodeUnitSize + 1] = 4;
  rewritten_bytecode[(cache_limit + 1) * kCodeUnitSize] = LOAD_GLOBAL;
  rewritten_bytecode[(cache_limit + 1) * kCodeUnitSize + 1] = 15;

  code.setCode(runtime_->newBytesWithAll(bytecode));
  word global_names_length = 600;
  Tuple names(&scope, newTupleWithNone(global_names_length));
  code.setNames(*names);

  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));

  // newFunctionWithCode() calls rewriteBytecode().
  Object rewritten_bytecode_obj(&scope, function.rewrittenBytecode());
  // The bytecode hasn't changed.
  EXPECT_TRUE(
      isMutableBytesEqualsBytes(rewritten_bytecode_obj, rewritten_bytecode));
  // The cache for LOAD_GLOBAL was populated.
  EXPECT_GT(Tuple::cast(function.caches()).length(), 527);
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesLoadAttrOperations) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      NOP,          99,  EXTENDED_ARG, 0xca, LOAD_ATTR,    0xfe,
      NOP,          106, EXTENDED_ARG, 1,    EXTENDED_ARG, 2,
      EXTENDED_ARG, 3,   LOAD_ATTR,    4,    LOAD_ATTR,    77,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      NOP,
      99,
      0,
      0,
      EXTENDED_ARG,
      0,
      0,
      0,
      LOAD_ATTR_ANAMORPHIC,
      0,
      0,
      0,
      NOP,
      106,
      0,
      0,
      EXTENDED_ARG,
      0,
      0,
      0,
      EXTENDED_ARG,
      0,
      0,
      0,
      EXTENDED_ARG,
      0,
      0,
      0,
      LOAD_ATTR_ANAMORPHIC,
      1,
      0,
      0,
      LOAD_ATTR_ANAMORPHIC,
      2,
      0,
      0,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  EXPECT_EQ(caches.length(), 3 * kIcPointersPerEntry);
  for (word i = 0, length = caches.length(); i < length; i++) {
    EXPECT_TRUE(caches.at(i).isNoneType()) << "index " << i;
  }

  EXPECT_EQ(originalArg(*function, 0), 0xcafe);
  EXPECT_EQ(originalArg(*function, 1), 0x01020304);
  EXPECT_EQ(originalArg(*function, 2), 77);
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesLoadConstOperations) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {LOAD_CONST, 0,          LOAD_CONST, 1,          LOAD_CONST,
                     2,          LOAD_CONST, 3,          LOAD_CONST, 4};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  // Immediate objects.
  Object obj0(&scope, NoneType::object());
  Object obj1(&scope, SmallInt::fromWord(0));
  Object obj2(&scope, Str::empty());
  // Not immediate since it doesn't fit in byte.
  Object obj3(&scope, SmallInt::fromWord(64));
  // Not immediate since it's a heap object.
  Object obj4(&scope, runtime_->newList());
  Tuple consts(&scope,
               runtime_->newTupleWithN(5, &obj0, &obj1, &obj2, &obj3, &obj4));
  code.setConsts(*consts);

  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));

  byte expected[] = {
      LOAD_IMMEDIATE,
      static_cast<byte>(opargFromObject(NoneType::object())),
      0,
      0,
      LOAD_IMMEDIATE,
      static_cast<byte>(opargFromObject(SmallInt::fromWord(0))),
      0,
      0,
      LOAD_IMMEDIATE,
      static_cast<byte>(opargFromObject(Str::empty())),
      0,
      0,
      LOAD_CONST,
      3,
      0,
      0,
      LOAD_CONST,
      4,
      0,
      0,
  };
  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesLoadConstToLoadBool) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {LOAD_CONST, 0, LOAD_CONST, 1};
  code.setCode(runtime_->newBytesWithAll(bytecode));

  // Immediate objects.
  Object obj0(&scope, Bool::trueObj());
  Object obj1(&scope, Bool::falseObj());
  Tuple consts(&scope, runtime_->newTupleWith2(obj0, obj1));
  code.setConsts(*consts);

  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));

  byte expected[] = {
      LOAD_BOOL, 0x80, 0, 0, LOAD_BOOL, 0, 0, 0,
  };
  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesLoadMethodOperations) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      NOP,          99,  EXTENDED_ARG, 0xca, LOAD_METHOD,  0xfe,
      NOP,          160, EXTENDED_ARG, 1,    EXTENDED_ARG, 2,
      EXTENDED_ARG, 3,   LOAD_METHOD,  4,    LOAD_METHOD,  77,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      NOP,
      99,
      0,
      0,
      EXTENDED_ARG,
      0,
      0,
      0,
      LOAD_METHOD_ANAMORPHIC,
      0,
      0,
      0,
      NOP,
      160,
      0,
      0,
      EXTENDED_ARG,
      0,
      0,
      0,
      EXTENDED_ARG,
      0,
      0,
      0,
      EXTENDED_ARG,
      0,
      0,
      0,
      LOAD_METHOD_ANAMORPHIC,
      1,
      0,
      0,
      LOAD_METHOD_ANAMORPHIC,
      2,
      0,
      0,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  EXPECT_EQ(caches.length(), 3 * kIcPointersPerEntry);
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
  code.setCode(runtime_->newBytesWithAll(bytecode));
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      STORE_ATTR_ANAMORPHIC,
      0,
      0,
      0,
  };
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
  code.setCode(runtime_->newBytesWithAll(bytecode));
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      BINARY_OP_ANAMORPHIC, 0,  0, 0, BINARY_OP_ANAMORPHIC, 1,  0, 0,
      BINARY_OP_ANAMORPHIC, 2,  0, 0, BINARY_OP_ANAMORPHIC, 3,  0, 0,
      BINARY_OP_ANAMORPHIC, 4,  0, 0, BINARY_OP_ANAMORPHIC, 5,  0, 0,
      BINARY_OP_ANAMORPHIC, 6,  0, 0, BINARY_OP_ANAMORPHIC, 7,  0, 0,
      BINARY_OP_ANAMORPHIC, 8,  0, 0, BINARY_OP_ANAMORPHIC, 9,  0, 0,
      BINARY_OP_ANAMORPHIC, 10, 0, 0, BINARY_OP_ANAMORPHIC, 11, 0, 0,
      BINARY_OP_ANAMORPHIC, 12, 0, 0,
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
  code.setCode(runtime_->newBytesWithAll(bytecode));
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      INPLACE_OP_ANAMORPHIC, 0,  0, 0, INPLACE_OP_ANAMORPHIC, 1,  0, 0,
      INPLACE_OP_ANAMORPHIC, 2,  0, 0, INPLACE_OP_ANAMORPHIC, 3,  0, 0,
      INPLACE_OP_ANAMORPHIC, 4,  0, 0, INPLACE_OP_ANAMORPHIC, 5,  0, 0,
      INPLACE_OP_ANAMORPHIC, 6,  0, 0, INPLACE_OP_ANAMORPHIC, 7,  0, 0,
      INPLACE_OP_ANAMORPHIC, 8,  0, 0, INPLACE_OP_ANAMORPHIC, 9,  0, 0,
      INPLACE_OP_ANAMORPHIC, 10, 0, 0, INPLACE_OP_ANAMORPHIC, 11, 0, 0,
      INPLACE_OP_ANAMORPHIC, 12, 0, 0,
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
  code.setCode(runtime_->newBytesWithAll(bytecode));
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      COMPARE_OP_ANAMORPHIC,
      0,
      0,
      0,
      COMPARE_OP_ANAMORPHIC,
      1,
      0,
      0,
      COMPARE_OP_ANAMORPHIC,
      2,
      0,
      0,
      COMPARE_OP_ANAMORPHIC,
      3,
      0,
      0,
      COMPARE_OP_ANAMORPHIC,
      4,
      0,
      0,
      COMPARE_OP_ANAMORPHIC,
      5,
      0,
      0,
      COMPARE_IN_ANAMORPHIC,
      6,
      0,
      0,
      COMPARE_OP,
      CompareOp::NOT_IN,
      0,
      0,
      COMPARE_IS,
      0,
      0,
      0,
      COMPARE_IS_NOT,
      0,
      0,
      0,
      COMPARE_OP,
      CompareOp::EXC_MATCH,
      0,
      0,
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
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setNames(newTupleWithNone(12));
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      LOAD_GLOBAL,
      0,
      0,
      0,
      STORE_GLOBAL,
      1,
      0,
      0,
      // Note that LOAD_ATTR's cache index starts at 6 to reserve the first 6
      // cache lines for 12 global variables.
      LOAD_ATTR_ANAMORPHIC,
      6,
      0,
      0,
      DELETE_GLOBAL,
      2,
      0,
      0,
      STORE_NAME,
      3,
      0,
      0,
      DELETE_NAME,
      4,
      0,
      0,
      LOAD_ATTR_ANAMORPHIC,
      7,
      0,
      0,
      LOAD_NAME,
      5,
      0,
      0,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  Tuple caches(&scope, function.caches());
  word num_global = 6;
  word num_attr = 2;
  EXPECT_EQ(caches.length(), (num_global + num_attr) * kIcPointersPerEntry);
}

TEST_F(BytecodeTest, RewriteBytecodeRewritesLoadFastAndStoreFastOpcodes) {
  HandleScope scope(thread_);
  Object arg0(&scope, Runtime::internStrFromCStr(thread_, "arg0"));
  Object var0(&scope, Runtime::internStrFromCStr(thread_, "var0"));
  Object var1(&scope, Runtime::internStrFromCStr(thread_, "var1"));
  Tuple varnames(&scope, runtime_->newTupleWith3(arg0, var0, var1));
  Object freevar0(&scope, Runtime::internStrFromCStr(thread_, "freevar0"));
  Tuple freevars(&scope, runtime_->newTupleWith1(freevar0));
  Object cellvar0(&scope, Runtime::internStrFromCStr(thread_, "cellvar0"));
  Tuple cellvars(&scope, runtime_->newTupleWith1(cellvar0));
  word argcount = 1;
  word nlocals = 3;
  byte bytecode[] = {
      LOAD_FAST,  2, LOAD_FAST,  1, LOAD_FAST,  1,
      STORE_FAST, 2, STORE_FAST, 1, STORE_FAST, 0,
  };
  Bytes code_code(&scope, runtime_->newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime_->emptyTuple());
  Object empty_string(&scope, Str::empty());
  Object lnotab(&scope, Bytes::empty());
  Code code(&scope,
            runtime_->newCode(argcount, /*posonlyargcount=*/0,
                              /*kwonlyargcount=*/0, nlocals,
                              /*stacksize=*/0, /*flags=*/0, code_code,
                              /*consts=*/empty_tuple, /*names=*/empty_tuple,
                              varnames, freevars, cellvars,
                              /*filename=*/empty_string, /*name=*/empty_string,
                              /*firstlineno=*/0, lnotab));
  code.setFlags(code.flags() | Code::Flags::kOptimized);

  Module module(&scope, findMainModule(runtime_));
  Function function(&scope, runtime_->newFunctionWithCode(thread_, empty_string,
                                                          code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      LOAD_FAST_REVERSE,  2, 0, 0, LOAD_FAST_REVERSE,  3, 0, 0,
      LOAD_FAST_REVERSE,  3, 0, 0, STORE_FAST_REVERSE, 2, 0, 0,
      STORE_FAST_REVERSE, 3, 0, 0, STORE_FAST_REVERSE, 4, 0, 0,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));
  EXPECT_EQ(Tuple::cast(function.originalArguments()).length(), 0);
  EXPECT_TRUE(function.caches().isNoneType());
}

TEST_F(BytecodeTest,
       RewriteBytecodeRewritesLoadFastToLoadFastReverseBoundForArguments) {
  HandleScope scope(thread_);
  Object arg0(&scope, Runtime::internStrFromCStr(thread_, "arg0"));
  Object var0(&scope, Runtime::internStrFromCStr(thread_, "var0"));
  Object var1(&scope, Runtime::internStrFromCStr(thread_, "var1"));
  Tuple varnames(&scope, runtime_->newTupleWith3(arg0, var0, var1));
  Object freevar0(&scope, Runtime::internStrFromCStr(thread_, "freevar0"));
  Tuple freevars(&scope, runtime_->newTupleWith1(freevar0));
  Object cellvar0(&scope, Runtime::internStrFromCStr(thread_, "cellvar0"));
  Tuple cellvars(&scope, runtime_->newTupleWith1(cellvar0));
  word argcount = 1;
  word nlocals = 3;
  byte bytecode[] = {
      LOAD_FAST,  2, LOAD_FAST,  1, LOAD_FAST,  0,
      STORE_FAST, 2, STORE_FAST, 1, STORE_FAST, 0,
  };
  Bytes code_code(&scope, runtime_->newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime_->emptyTuple());
  Object empty_string(&scope, Str::empty());
  Object lnotab(&scope, Bytes::empty());
  Code code(&scope,
            runtime_->newCode(argcount, /*posonlyargcount=*/0,
                              /*kwonlyargcount=*/0, nlocals,
                              /*stacksize=*/0, /*flags=*/0, code_code,
                              /*consts=*/empty_tuple, /*names=*/empty_tuple,
                              varnames, freevars, cellvars,
                              /*filename=*/empty_string, /*name=*/empty_string,
                              /*firstlineno=*/0, lnotab));
  code.setFlags(code.flags() | Code::Flags::kOptimized);

  Module module(&scope, findMainModule(runtime_));
  Function function(&scope, runtime_->newFunctionWithCode(thread_, empty_string,
                                                          code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      LOAD_FAST_REVERSE,           2, 0, 0, LOAD_FAST_REVERSE,  3, 0, 0,
      LOAD_FAST_REVERSE_UNCHECKED, 4, 0, 0, STORE_FAST_REVERSE, 2, 0, 0,
      STORE_FAST_REVERSE,          3, 0, 0, STORE_FAST_REVERSE, 4, 0, 0,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));
  EXPECT_EQ(Tuple::cast(function.originalArguments()).length(), 0);
  EXPECT_TRUE(function.caches().isNoneType());
}

TEST_F(
    BytecodeTest,
    RewriteBytecodeRewritesLoadFastToLoadFastReverseWhenDeleteFastIsPresent) {
  HandleScope scope(thread_);
  Object arg0(&scope, Runtime::internStrFromCStr(thread_, "arg0"));
  Object var0(&scope, Runtime::internStrFromCStr(thread_, "var0"));
  Object var1(&scope, Runtime::internStrFromCStr(thread_, "var1"));
  Tuple varnames(&scope, runtime_->newTupleWith3(arg0, var0, var1));
  Object freevar0(&scope, Runtime::internStrFromCStr(thread_, "freevar0"));
  Tuple freevars(&scope, runtime_->newTupleWith1(freevar0));
  Object cellvar0(&scope, Runtime::internStrFromCStr(thread_, "cellvar0"));
  Tuple cellvars(&scope, runtime_->newTupleWith1(cellvar0));
  word argcount = 1;
  word nlocals = 3;
  byte bytecode[] = {
      LOAD_FAST,  2, LOAD_FAST,  1, LOAD_FAST,   0, STORE_FAST, 2,
      STORE_FAST, 1, STORE_FAST, 0, DELETE_FAST, 0,
  };
  Bytes code_code(&scope, runtime_->newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime_->emptyTuple());
  Object empty_string(&scope, Str::empty());
  Object lnotab(&scope, Bytes::empty());
  Code code(&scope,
            runtime_->newCode(argcount, /*posonlyargcount=*/0,
                              /*kwonlyargcount=*/0, nlocals,
                              /*stacksize=*/0, /*flags=*/0, code_code,
                              /*consts=*/empty_tuple, /*names=*/empty_tuple,
                              varnames, freevars, cellvars,
                              /*filename=*/empty_string, /*name=*/empty_string,
                              /*firstlineno=*/0, lnotab));
  code.setFlags(code.flags() | Code::Flags::kOptimized);

  Module module(&scope, findMainModule(runtime_));
  Function function(&scope, runtime_->newFunctionWithCode(thread_, empty_string,
                                                          code, module));
  // newFunctionWithCode() calls rewriteBytecode().

  byte expected[] = {
      LOAD_FAST_REVERSE,  2, 0, 0, LOAD_FAST_REVERSE,  3, 0, 0,
      LOAD_FAST_REVERSE,  4, 0, 0, STORE_FAST_REVERSE, 2, 0, 0,
      STORE_FAST_REVERSE, 3, 0, 0, STORE_FAST_REVERSE, 4, 0, 0,
      DELETE_FAST,        0, 0, 0,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));
  EXPECT_EQ(Tuple::cast(function.originalArguments()).length(), 0);
  EXPECT_TRUE(function.caches().isNoneType());
}

TEST_F(BytecodeTest,
       RewriteBytecodeDoesNotRewriteFunctionsWithNoOptimizedNorNewLocalsFlag) {
  HandleScope scope(thread_);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      NOP,          99,  EXTENDED_ARG, 0xca, LOAD_ATTR,    0xfe,
      NOP,          106, EXTENDED_ARG, 1,    EXTENDED_ARG, 2,
      EXTENDED_ARG, 3,   LOAD_ATTR,    4,    LOAD_ATTR,    77,
  };
  code.setCode(runtime_->newBytesWithAll(bytecode));
  code.setFlags(code.flags() & ~Code::Flags::kOptimized &
                ~Code::Flags::kNewlocals);
  Module module(&scope, findMainModule(runtime_));
  Function function(&scope,
                    runtime_->newFunctionWithCode(thread_, name, code, module));

  byte expected[] = {
      NOP,          99,   0, 0, EXTENDED_ARG, 0xca, 0, 0,
      LOAD_ATTR,    0xfe, 0, 0, NOP,          106,  0, 0,
      EXTENDED_ARG, 1,    0, 0, EXTENDED_ARG, 2,    0, 0,
      EXTENDED_ARG, 3,    0, 0, LOAD_ATTR,    4,    0, 0,
      LOAD_ATTR,    77,   0, 0,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));
}

}  // namespace testing
}  // namespace py
