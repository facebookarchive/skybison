#include "gtest/gtest.h"

#include "ic.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using IcTest = RuntimeFixture;

TEST(IcTestNoFixture, IcRewriteBytecodeRewritesLoadAttrOperations) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      NOP,          99,        EXTENDED_ARG, 0xca, LOAD_ATTR,    0xfe,
      NOP,          LOAD_ATTR, EXTENDED_ARG, 1,    EXTENDED_ARG, 2,
      EXTENDED_ARG, 3,         LOAD_ATTR,    4,    LOAD_ATTR,    77,
  };
  code.setCode(runtime.newBytesWithAll(bytecode));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {
      NOP,          99,        EXTENDED_ARG,     0, LOAD_ATTR_CACHED, 0,
      NOP,          LOAD_ATTR, EXTENDED_ARG,     0, EXTENDED_ARG,     0,
      EXTENDED_ARG, 0,         LOAD_ATTR_CACHED, 1, LOAD_ATTR_CACHED, 2,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  EXPECT_EQ(caches.length(), 3 * kIcPointersPerCache);
  for (word i = 0, length = caches.length(); i < length; i++) {
    EXPECT_TRUE(caches.at(i).isNoneType()) << "index " << i;
  }

  EXPECT_EQ(icOriginalArg(*function, 0), 0xcafe);
  EXPECT_EQ(icOriginalArg(*function, 1), 0x01020304);
  EXPECT_EQ(icOriginalArg(*function, 2), 77);
}

TEST(IcTestNoFixture, IcRewriteBytecodeRewritesZeroArgMethodCalls) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      NOP,          99,        EXTENDED_ARG,  0xca, LOAD_ATTR,     0xfe,
      NOP,          LOAD_ATTR, EXTENDED_ARG,  1,    EXTENDED_ARG,  2,
      EXTENDED_ARG, 3,         LOAD_ATTR,     4,    CALL_FUNCTION, 1,
      LOAD_ATTR,    4,         CALL_FUNCTION, 0};
  code.setCode(runtime.newBytesWithAll(bytecode));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals));

  byte expected[] = {NOP,
                     99,
                     EXTENDED_ARG,
                     0,
                     LOAD_ATTR_CACHED,
                     0,
                     NOP,
                     LOAD_ATTR,
                     EXTENDED_ARG,
                     0,
                     EXTENDED_ARG,
                     0,
                     EXTENDED_ARG,
                     0,
                     LOAD_ATTR_CACHED,
                     1,
                     CALL_FUNCTION,
                     1,
                     LOAD_METHOD_CACHED,
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

  EXPECT_EQ(icOriginalArg(*function, 0), 0xcafe);
  EXPECT_EQ(icOriginalArg(*function, 1), 0x01020304);
  EXPECT_EQ(icOriginalArg(*function, 2), 4);
}

TEST(IcTestNoFixture, IcRewriteBytecodeRewritesLoadMethodOperations) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      NOP,          99,          EXTENDED_ARG, 0xca, LOAD_METHOD,  0xfe,
      NOP,          LOAD_METHOD, EXTENDED_ARG, 1,    EXTENDED_ARG, 2,
      EXTENDED_ARG, 3,           LOAD_METHOD,  4,    LOAD_METHOD,  77,
  };
  code.setCode(runtime.newBytesWithAll(bytecode));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {
      NOP,          99,          EXTENDED_ARG,       0, LOAD_METHOD_CACHED, 0,
      NOP,          LOAD_METHOD, EXTENDED_ARG,       0, EXTENDED_ARG,       0,
      EXTENDED_ARG, 0,           LOAD_METHOD_CACHED, 1, LOAD_METHOD_CACHED, 2,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  EXPECT_EQ(caches.length(), 3 * kIcPointersPerCache);
  for (word i = 0, length = caches.length(); i < length; i++) {
    EXPECT_TRUE(caches.at(i).isNoneType()) << "index " << i;
  }

  EXPECT_EQ(icOriginalArg(*function, 0), 0xcafe);
  EXPECT_EQ(icOriginalArg(*function, 1), 0x01020304);
  EXPECT_EQ(icOriginalArg(*function, 2), 77);
}

TEST(IcTestNoFixture, IcRewriteBytecodeRewritesStoreAttr) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {STORE_ATTR, 48};
  code.setCode(runtime.newBytesWithAll(bytecode));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {STORE_ATTR_CACHED, 0};
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  EXPECT_EQ(icOriginalArg(*function, 0), 48);
}

TEST(IcTestNoFixture, IcRewriteBytecodeRewritesBinaryOpcodes) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
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
  code.setCode(runtime.newBytesWithAll(bytecode));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {
      BINARY_OP_CACHED, 0,  BINARY_OP_CACHED, 1,  BINARY_OP_CACHED, 2,
      BINARY_OP_CACHED, 3,  BINARY_OP_CACHED, 4,  BINARY_OP_CACHED, 5,
      BINARY_OP_CACHED, 6,  BINARY_OP_CACHED, 7,  BINARY_OP_CACHED, 8,
      BINARY_OP_CACHED, 9,  BINARY_OP_CACHED, 10, BINARY_OP_CACHED, 11,
      BINARY_OP_CACHED, 12,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  EXPECT_EQ(icOriginalArg(*function, 0),
            static_cast<word>(Interpreter::BinaryOp::MATMUL));
  EXPECT_EQ(icOriginalArg(*function, 1),
            static_cast<word>(Interpreter::BinaryOp::POW));
  EXPECT_EQ(icOriginalArg(*function, 2),
            static_cast<word>(Interpreter::BinaryOp::MUL));
  EXPECT_EQ(icOriginalArg(*function, 3),
            static_cast<word>(Interpreter::BinaryOp::MOD));
  EXPECT_EQ(icOriginalArg(*function, 4),
            static_cast<word>(Interpreter::BinaryOp::ADD));
  EXPECT_EQ(icOriginalArg(*function, 5),
            static_cast<word>(Interpreter::BinaryOp::SUB));
  EXPECT_EQ(icOriginalArg(*function, 6),
            static_cast<word>(Interpreter::BinaryOp::FLOORDIV));
  EXPECT_EQ(icOriginalArg(*function, 7),
            static_cast<word>(Interpreter::BinaryOp::TRUEDIV));
  EXPECT_EQ(icOriginalArg(*function, 8),
            static_cast<word>(Interpreter::BinaryOp::LSHIFT));
  EXPECT_EQ(icOriginalArg(*function, 9),
            static_cast<word>(Interpreter::BinaryOp::RSHIFT));
  EXPECT_EQ(icOriginalArg(*function, 10),
            static_cast<word>(Interpreter::BinaryOp::AND));
  EXPECT_EQ(icOriginalArg(*function, 11),
            static_cast<word>(Interpreter::BinaryOp::XOR));
  EXPECT_EQ(icOriginalArg(*function, 12),
            static_cast<word>(Interpreter::BinaryOp::OR));
}

TEST(IcTestNoFixture, IcRewriteBytecodeRewritesInplaceOpcodes) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
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
  code.setCode(runtime.newBytesWithAll(bytecode));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {
      INPLACE_OP_CACHED, 0,  INPLACE_OP_CACHED, 1,  INPLACE_OP_CACHED, 2,
      INPLACE_OP_CACHED, 3,  INPLACE_OP_CACHED, 4,  INPLACE_OP_CACHED, 5,
      INPLACE_OP_CACHED, 6,  INPLACE_OP_CACHED, 7,  INPLACE_OP_CACHED, 8,
      INPLACE_OP_CACHED, 9,  INPLACE_OP_CACHED, 10, INPLACE_OP_CACHED, 11,
      INPLACE_OP_CACHED, 12,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  EXPECT_EQ(icOriginalArg(*function, 0),
            static_cast<word>(Interpreter::BinaryOp::MATMUL));
  EXPECT_EQ(icOriginalArg(*function, 1),
            static_cast<word>(Interpreter::BinaryOp::POW));
  EXPECT_EQ(icOriginalArg(*function, 2),
            static_cast<word>(Interpreter::BinaryOp::MUL));
  EXPECT_EQ(icOriginalArg(*function, 3),
            static_cast<word>(Interpreter::BinaryOp::MOD));
  EXPECT_EQ(icOriginalArg(*function, 4),
            static_cast<word>(Interpreter::BinaryOp::ADD));
  EXPECT_EQ(icOriginalArg(*function, 5),
            static_cast<word>(Interpreter::BinaryOp::SUB));
  EXPECT_EQ(icOriginalArg(*function, 6),
            static_cast<word>(Interpreter::BinaryOp::FLOORDIV));
  EXPECT_EQ(icOriginalArg(*function, 7),
            static_cast<word>(Interpreter::BinaryOp::TRUEDIV));
  EXPECT_EQ(icOriginalArg(*function, 8),
            static_cast<word>(Interpreter::BinaryOp::LSHIFT));
  EXPECT_EQ(icOriginalArg(*function, 9),
            static_cast<word>(Interpreter::BinaryOp::RSHIFT));
  EXPECT_EQ(icOriginalArg(*function, 10),
            static_cast<word>(Interpreter::BinaryOp::AND));
  EXPECT_EQ(icOriginalArg(*function, 11),
            static_cast<word>(Interpreter::BinaryOp::XOR));
  EXPECT_EQ(icOriginalArg(*function, 12),
            static_cast<word>(Interpreter::BinaryOp::OR));
}

TEST(IcTestNoFixture, IcRewriteBytecodeRewritesCompareOpOpcodes) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
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
  code.setCode(runtime.newBytesWithAll(bytecode));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {
      COMPARE_OP_CACHED, 0,
      COMPARE_OP_CACHED, 1,
      COMPARE_OP_CACHED, 2,
      COMPARE_OP_CACHED, 3,
      COMPARE_OP_CACHED, 4,
      COMPARE_OP_CACHED, 5,
      COMPARE_OP,        CompareOp::IN,
      COMPARE_OP,        CompareOp::NOT_IN,
      COMPARE_OP,        CompareOp::IS,
      COMPARE_OP,        CompareOp::IS_NOT,
      COMPARE_OP,        CompareOp::EXC_MATCH,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));

  EXPECT_EQ(icOriginalArg(*function, 0), static_cast<word>(CompareOp::LT));
  EXPECT_EQ(icOriginalArg(*function, 1), static_cast<word>(CompareOp::LE));
  EXPECT_EQ(icOriginalArg(*function, 2), static_cast<word>(CompareOp::EQ));
  EXPECT_EQ(icOriginalArg(*function, 3), static_cast<word>(CompareOp::NE));
  EXPECT_EQ(icOriginalArg(*function, 4), static_cast<word>(CompareOp::GT));
  EXPECT_EQ(icOriginalArg(*function, 5), static_cast<word>(CompareOp::GE));
}

TEST(IcTestNoFixture,
     IcRewriteBytecodeRewritesReservesCachesForGlobalVariables) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  byte bytecode[] = {
      LOAD_GLOBAL, 0, STORE_GLOBAL, 1, LOAD_ATTR, 9, DELETE_GLOBAL, 2,
      STORE_NAME,  3, DELETE_NAME,  4, LOAD_ATTR, 9, LOAD_NAME,     5,
  };
  code.setCode(runtime.newBytesWithAll(bytecode));
  code.setNames(runtime.newTuple(12));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {
      LOAD_GLOBAL,
      0,
      STORE_GLOBAL,
      1,
      // Note that LOAD_ATTR's cache index starts at 2 to reserve the first 2
      // cache lines for 12 global variables.
      LOAD_ATTR_CACHED,
      2,
      DELETE_GLOBAL,
      2,
      STORE_NAME,
      3,
      DELETE_NAME,
      4,
      LOAD_ATTR_CACHED,
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

TEST(IcTestNoFixture, IcRewriteBytecodeRewritesLoadFastAndStoreFastOpcodes) {
  Runtime runtime(/*cache_enabled=*/true);
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple varnames(&scope, runtime.newTuple(3));
  varnames.atPut(0, runtime.internStrFromCStr(thread, "arg0"));
  varnames.atPut(1, runtime.internStrFromCStr(thread, "var0"));
  varnames.atPut(2, runtime.internStrFromCStr(thread, "var1"));
  word argcount = 1;
  word nlocals = 3;
  byte bytecode[] = {
      LOAD_FAST,  2, LOAD_FAST,  1, LOAD_FAST,  0,
      STORE_FAST, 2, STORE_FAST, 1, STORE_FAST, 0,
  };
  Bytes code_code(&scope, runtime.newBytesWithAll(bytecode));
  Object empty_tuple(&scope, runtime.emptyTuple());
  Object empty_string(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  Code code(&scope,
            runtime.newCode(argcount, 0, nlocals, 0, 0, code_code, empty_tuple,
                            empty_tuple, varnames, empty_tuple, empty_tuple,
                            empty_string, empty_string, 0, empty_bytes));

  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, empty_string, code, none, none,
                                        none, none, globals));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {
      LOAD_FAST_REVERSE,  0, LOAD_FAST_REVERSE,  1, LOAD_FAST_REVERSE,  2,
      STORE_FAST_REVERSE, 0, STORE_FAST_REVERSE, 1, STORE_FAST_REVERSE, 2,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isMutableBytesEqualsBytes(rewritten_bytecode, expected));
  EXPECT_EQ(Tuple::cast(function.originalArguments()).length(), 0);
  EXPECT_EQ(Tuple::cast(function.caches()).length(), 0);
}

static RawObject layoutIdAsSmallInt(LayoutId id) {
  return SmallInt::fromWord(static_cast<word>(id));
}

TEST_F(IcTest, IcLookupReturnsFirstCachedValue) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(1 * kIcPointersPerCache));
  caches.atPut(kIcEntryKeyOffset, layoutIdAsSmallInt(LayoutId::kSmallInt));
  caches.atPut(kIcEntryValueOffset, runtime_.newInt(44));
  EXPECT_TRUE(isIntEqualsWord(icLookup(caches, 0, LayoutId::kSmallInt), 44));
}

TEST_F(IcTest, IcLookupReturnsFourthCachedValue) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(2 * kIcPointersPerCache));
  caches.atPut(kIcEntryKeyOffset, layoutIdAsSmallInt(LayoutId::kSmallInt));
  word cache_offset = kIcPointersPerCache;
  caches.atPut(cache_offset + 0 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kSmallStr));
  caches.atPut(cache_offset + 1 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kStopIteration));
  caches.atPut(cache_offset + 2 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kLargeStr));
  caches.atPut(cache_offset + 3 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kSmallInt));
  caches.atPut(cache_offset + 3 * kIcPointersPerEntry + kIcEntryValueOffset,
               runtime_.newInt(7));
  EXPECT_TRUE(isIntEqualsWord(icLookup(caches, 1, LayoutId::kSmallInt), 7));
}

TEST_F(IcTest, IcLookupWithoutMatchReturnsErrorNotFound) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(2 * kIcPointersPerCache));
  EXPECT_TRUE(icLookup(caches, 1, LayoutId::kSmallInt).isErrorNotFound());
}

static RawObject binopKey(LayoutId left, LayoutId right, IcBinopFlags flags) {
  return SmallInt::fromWord((static_cast<word>(left) << Header::kLayoutIdBits |
                             static_cast<word>(right))
                                << kBitsPerByte |
                            static_cast<word>(flags));
}

TEST_F(IcTest, IcLookupBinopReturnsCachedValue) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(2 * kIcPointersPerCache));
  word cache_offset = kIcPointersPerCache;
  caches.atPut(
      cache_offset + 0 * kIcPointersPerEntry + kIcEntryKeyOffset,
      binopKey(LayoutId::kSmallInt, LayoutId::kNoneType, IC_BINOP_NONE));
  caches.atPut(
      cache_offset + 1 * kIcPointersPerEntry + kIcEntryKeyOffset,
      binopKey(LayoutId::kNoneType, LayoutId::kBytes, IC_BINOP_REFLECTED));
  caches.atPut(
      cache_offset + 2 * kIcPointersPerEntry + kIcEntryKeyOffset,
      binopKey(LayoutId::kSmallInt, LayoutId::kBytes, IC_BINOP_REFLECTED));
  caches.atPut(cache_offset + 2 * kIcPointersPerEntry + kIcEntryValueOffset,
               runtime_.newStrFromCStr("xy"));

  IcBinopFlags flags;
  EXPECT_TRUE(isStrEqualsCStr(
      icLookupBinop(caches, 1, LayoutId::kSmallInt, LayoutId::kBytes, &flags),
      "xy"));
  EXPECT_EQ(flags, IC_BINOP_REFLECTED);
}

TEST_F(IcTest, IcLookupBinopReturnsErrorNotFound) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(kIcPointersPerCache));
  IcBinopFlags flags;
  EXPECT_TRUE(
      icLookupBinop(caches, 0, LayoutId::kSmallInt, LayoutId::kSmallInt, &flags)
          .isErrorNotFound());
}

TEST_F(IcTest, IcLookupGlobalVar) {
  HandleScope scope(thread_);
  Tuple caches(&scope, runtime_.newTuple(2));
  ValueCell cache(&scope, runtime_.newValueCell());
  cache.setValue(SmallInt::fromWord(99));
  caches.atPut(0, *cache);
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches, 0)), 99));
  EXPECT_TRUE(icLookupGlobalVar(caches, 1).isNoneType());
}

TEST_F(IcTest, IcUpdateSetsEmptyEntry) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(1 * kIcPointersPerCache));
  Object value(&scope, runtime_.newInt(88));
  icUpdate(thread_, caches, 0, LayoutId::kSmallStr, value);
  EXPECT_TRUE(isIntEqualsWord(caches.at(kIcEntryKeyOffset),
                              static_cast<word>(LayoutId::kSmallStr)));
  EXPECT_TRUE(isIntEqualsWord(caches.at(kIcEntryValueOffset), 88));
}

TEST_F(IcTest, IcUpdateUpdatesExistingEntry) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(2 * kIcPointersPerCache));
  word cache_offset = kIcPointersPerCache;
  caches.atPut(cache_offset + 0 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kSmallInt));
  caches.atPut(cache_offset + 1 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kSmallBytes));
  caches.atPut(cache_offset + 2 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kSmallStr));
  caches.atPut(cache_offset + 3 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kBytes));
  Object value(&scope, runtime_.newStrFromCStr("test"));
  icUpdate(thread_, caches, 1, LayoutId::kSmallStr, value);
  EXPECT_TRUE(isIntEqualsWord(
      caches.at(cache_offset + 2 * kIcPointersPerEntry + kIcEntryKeyOffset),
      static_cast<word>(LayoutId::kSmallStr)));
  EXPECT_TRUE(isStrEqualsCStr(
      caches.at(cache_offset + 2 * kIcPointersPerEntry + kIcEntryValueOffset),
      "test"));
}

TEST(IcTestNoFixture, BinarySubscrUpdateCacheWithFunctionUpdatesCache) {
  Runtime runtime(/*cache_enabled=*/true);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f(c, k):
  return c[k]

container = [1, 2, 3]
getitem = type(container).__getitem__
result = f(container, 0)
)")
                   .isError());

  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));

  Object container(&scope, moduleAt(&runtime, "__main__", "container"));
  Object getitem(&scope, moduleAt(&runtime, "__main__", "getitem"));
  Function f(&scope, moduleAt(&runtime, "__main__", "f"));
  Tuple caches(&scope, f.caches());
  // Expect that BINARY_SUBSCR is the only cached opcode in f().
  ASSERT_EQ(caches.length(), 1 * kIcPointersPerCache);
  EXPECT_EQ(icLookup(caches, 0, container.layoutId()), *getitem);

  ASSERT_FALSE(runFromCStr(&runtime, R"(
container2 = [4, 5, 6]
result2 = f(container2, 1)
)")
                   .isError());
  Object container2(&scope, moduleAt(&runtime, "__main__", "container2"));
  Object result2(&scope, moduleAt(&runtime, "__main__", "result2"));
  EXPECT_EQ(container2.layoutId(), container.layoutId());
  EXPECT_TRUE(isIntEqualsWord(*result2, 5));
}

TEST(IcTestNoFixture, BinarySubscrUpdateCacheWithNonFunctionDoesntUpdateCache) {
  Runtime runtime(/*cache_enabled=*/true);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f(c, k):
  return c[k]
class Container:
  def get(self):
    def getitem(key):
      return key
    return getitem

  __getitem__ = property(get)

container = Container()
result = f(container, "hi")
)")
                   .isError());

  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "hi"));

  Object container(&scope, moduleAt(&runtime, "__main__", "container"));
  Function f(&scope, moduleAt(&runtime, "__main__", "f"));
  Tuple caches(&scope, f.caches());
  // Expect that BINARY_SUBSCR is the only cached opcode in f().
  ASSERT_EQ(caches.length(), 1 * kIcPointersPerCache);
  EXPECT_TRUE(icLookup(caches, 0, container.layoutId()).isErrorNotFound());

  ASSERT_FALSE(runFromCStr(&runtime, R"(
container2 = Container()
result2 = f(container, "hello there!")
)")
                   .isError());
  Object container2(&scope, moduleAt(&runtime, "__main__", "container2"));
  Object result2(&scope, moduleAt(&runtime, "__main__", "result2"));
  ASSERT_EQ(container2.layoutId(), container.layoutId());
  EXPECT_TRUE(isStrEqualsCStr(*result2, "hello there!"));
}

TEST_F(IcTest, IcUpdateBinopSetsEmptyEntry) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(kIcPointersPerCache));
  Object value(&scope, runtime_.newInt(-44));
  icUpdateBinop(thread_, caches, 0, LayoutId::kSmallStr, LayoutId::kLargeBytes,
                value, IC_BINOP_REFLECTED);
  EXPECT_EQ(
      caches.at(kIcEntryKeyOffset),
      binopKey(LayoutId::kSmallStr, LayoutId::kLargeBytes, IC_BINOP_REFLECTED));
  EXPECT_TRUE(isIntEqualsWord(caches.at(kIcEntryValueOffset), -44));
}

TEST_F(IcTest, IcUpdateBinopSetsExistingEntry) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(2 * kIcPointersPerCache));
  word cache_offset = kIcPointersPerCache;
  caches.atPut(
      cache_offset + 0 * kIcPointersPerEntry + kIcEntryKeyOffset,
      binopKey(LayoutId::kSmallInt, LayoutId::kLargeInt, IC_BINOP_NONE));
  caches.atPut(
      cache_offset + 1 * kIcPointersPerEntry + kIcEntryKeyOffset,
      binopKey(LayoutId::kLargeInt, LayoutId::kSmallInt, IC_BINOP_REFLECTED));
  Object value(&scope, runtime_.newStrFromCStr("yyy"));
  icUpdateBinop(thread_, caches, 1, LayoutId::kLargeInt, LayoutId::kSmallInt,
                value, IC_BINOP_NONE);
  EXPECT_TRUE(
      caches.at(cache_offset + 0 * kIcPointersPerEntry + kIcEntryValueOffset)
          .isNoneType());
  EXPECT_EQ(
      caches.at(cache_offset + 1 * kIcPointersPerEntry + kIcEntryKeyOffset),
      binopKey(LayoutId::kLargeInt, LayoutId::kSmallInt, IC_BINOP_NONE));
  EXPECT_TRUE(isStrEqualsCStr(
      caches.at(cache_offset + 1 * kIcPointersPerEntry + kIcEntryValueOffset),
      "yyy"));
}

TEST(IcTestNoFixture, ForIterUpdateCacheWithFunctionUpdatesCache) {
  Runtime runtime(/*cache_enabled=*/true);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f(container):
  for i in container:
    return i

container = [1, 2, 3]
iterator = iter(container)
iter_next = type(iterator).__next__
result = f(container)
)")
                   .isError());

  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));

  Object iterator(&scope, moduleAt(&runtime, "__main__", "iterator"));
  Object iter_next(&scope, moduleAt(&runtime, "__main__", "iter_next"));
  Function f(&scope, moduleAt(&runtime, "__main__", "f"));
  Tuple caches(&scope, f.caches());
  // Expect that FOR_ITER is the only cached opcode in f().
  ASSERT_EQ(caches.length(), 1 * kIcPointersPerCache);
  EXPECT_EQ(icLookup(caches, 0, iterator.layoutId()), *iter_next);
}

TEST(IcTestNoFixture, ForIterUpdateCacheWithNonFunctionDoesntUpdateCache) {
  Runtime runtime(/*cache_enabled=*/true);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f(container):
  for i in container:
    return i

class Iter:
  def get(self):
    def next():
      return 123
    return next
  __next__ = property(get)

class Container:
  def __iter__(self):
    return Iter()

container = Container()
iterator = iter(container)
result = f(container)
)")
                   .isError());

  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));

  Object iterator(&scope, moduleAt(&runtime, "__main__", "iterator"));
  Function f(&scope, moduleAt(&runtime, "__main__", "f"));
  Tuple caches(&scope, f.caches());
  // Expect that FOR_ITER is the only cached opcode in f().
  ASSERT_EQ(caches.length(), 1 * kIcPointersPerCache);
  EXPECT_TRUE(icLookup(caches, 0, iterator.layoutId()).isErrorNotFound());
}

static RawObject testingFunction(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime->newDict());
  MutableBytes rewritten_bytecode(&scope,
                                  runtime->newMutableBytesUninitialized(8));
  rewritten_bytecode.byteAtPut(0, LOAD_GLOBAL);
  rewritten_bytecode.byteAtPut(1, 0);
  rewritten_bytecode.byteAtPut(2, STORE_GLOBAL);
  rewritten_bytecode.byteAtPut(3, 1);
  rewritten_bytecode.byteAtPut(4, LOAD_GLOBAL);
  rewritten_bytecode.byteAtPut(5, 0);
  rewritten_bytecode.byteAtPut(6, STORE_GLOBAL);
  rewritten_bytecode.byteAtPut(7, 1);

  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals));
  function.setRewrittenBytecode(*rewritten_bytecode);

  code.setNames(runtime->newTuple(2));
  Tuple caches(&scope, runtime->newTuple(2));
  function.setCaches(*caches);
  return *function;
}

TEST_F(IcTest, insertDependencyInsertsDependentAsHead) {
  HandleScope scope(thread_);
  Function function0(&scope, testingFunction(thread_));
  Function function1(&scope, testingFunction(thread_));

  ValueCell cache(&scope, runtime_.newValueCell());
  ASSERT_TRUE(cache.dependencyLink().isNoneType());

  insertDependency(thread_, function0, cache);
  WeakLink link0(&scope, cache.dependencyLink());
  EXPECT_EQ(link0.referent(), *function0);
  EXPECT_TRUE(link0.prev().isNoneType());
  EXPECT_TRUE(link0.next().isNoneType());

  insertDependency(thread_, function1, cache);
  WeakLink link1(&scope, cache.dependencyLink());
  EXPECT_EQ(link1.referent(), *function1);
  EXPECT_TRUE(link1.prev().isNoneType());
  EXPECT_EQ(link1.next(), *link0);
}

TEST_F(IcTest, IcUpdateGlobalVarFillsCacheLineAndReplaceOpcode) {
  HandleScope scope(thread_);
  Function function(&scope, testingFunction(thread_));
  Tuple caches(&scope, function.caches());
  MutableBytes rewritten_bytecode(&scope, function.rewrittenBytecode());

  ValueCell cache(&scope, runtime_.newValueCell());
  cache.setValue(SmallInt::fromWord(99));
  ValueCell another_cache(&scope, runtime_.newValueCell());
  another_cache.setValue(SmallInt::fromWord(123));

  icUpdateGlobalVar(thread_, function, 0, cache);

  EXPECT_EQ(caches.at(0), cache);
  EXPECT_EQ(rewritten_bytecode.byteAt(0), LOAD_GLOBAL_CACHED);
  EXPECT_EQ(rewritten_bytecode.byteAt(2), STORE_GLOBAL);

  icUpdateGlobalVar(thread_, function, 1, another_cache);

  EXPECT_EQ(caches.at(0), cache);
  EXPECT_EQ(rewritten_bytecode.byteAt(0), LOAD_GLOBAL_CACHED);
  EXPECT_EQ(rewritten_bytecode.byteAt(2), STORE_GLOBAL_CACHED);
}

TEST_F(IcTest, IcUpdateGlobalVarFillsCacheLineAndReplaceOpcodeWithExtendedArg) {
  HandleScope scope(thread_);
  Function function(&scope, testingFunction(thread_));
  Tuple caches(&scope, function.caches());

  MutableBytes rewritten_bytecode(&scope,
                                  runtime_.newMutableBytesUninitialized(8));
  // TODO(T45440363): Replace the argument of EXTENDED_ARG for a non-zero value.
  rewritten_bytecode.byteAtPut(0, EXTENDED_ARG);
  rewritten_bytecode.byteAtPut(1, 0);
  rewritten_bytecode.byteAtPut(2, LOAD_GLOBAL);
  rewritten_bytecode.byteAtPut(3, 0);
  rewritten_bytecode.byteAtPut(4, EXTENDED_ARG);
  rewritten_bytecode.byteAtPut(5, 0);
  rewritten_bytecode.byteAtPut(6, STORE_GLOBAL);
  rewritten_bytecode.byteAtPut(7, 1);
  function.setRewrittenBytecode(*rewritten_bytecode);

  ValueCell cache(&scope, runtime_.newValueCell());
  cache.setValue(SmallInt::fromWord(99));
  ValueCell another_cache(&scope, runtime_.newValueCell());
  another_cache.setValue(SmallInt::fromWord(123));

  icUpdateGlobalVar(thread_, function, 0, cache);

  EXPECT_EQ(caches.at(0), cache);
  EXPECT_EQ(rewritten_bytecode.byteAt(2), LOAD_GLOBAL_CACHED);
  EXPECT_EQ(rewritten_bytecode.byteAt(6), STORE_GLOBAL);

  icUpdateGlobalVar(thread_, function, 1, another_cache);

  EXPECT_EQ(caches.at(0), cache);
  EXPECT_EQ(rewritten_bytecode.byteAt(2), LOAD_GLOBAL_CACHED);
  EXPECT_EQ(rewritten_bytecode.byteAt(6), STORE_GLOBAL_CACHED);
}

TEST_F(IcTest, IcUpdateGlobalVarCreatesDependencyLink) {
  HandleScope scope(thread_);
  Function function(&scope, testingFunction(thread_));
  ValueCell cache(&scope, runtime_.newValueCell());
  cache.setValue(SmallInt::fromWord(99));
  icUpdateGlobalVar(thread_, function, 0, cache);

  ASSERT_TRUE(cache.dependencyLink().isWeakLink());
  WeakLink link(&scope, cache.dependencyLink());
  EXPECT_EQ(link.referent(), *function);
  EXPECT_EQ(link.prev(), NoneType::object());
  EXPECT_EQ(link.next(), NoneType::object());
}

TEST_F(IcTest, IcUpdateGlobalVarInsertsHeadOfDependencyLink) {
  HandleScope scope(thread_);
  Function function0(&scope, testingFunction(thread_));
  Function function1(&scope, testingFunction(thread_));

  // Adds cache into function0's caches first, then to function1's.
  ValueCell cache(&scope, runtime_.newValueCell());
  cache.setValue(SmallInt::fromWord(99));
  icUpdateGlobalVar(thread_, function0, 0, cache);
  icUpdateGlobalVar(thread_, function1, 0, cache);

  ASSERT_TRUE(cache.dependencyLink().isWeakLink());
  WeakLink link(&scope, cache.dependencyLink());
  EXPECT_EQ(link.referent(), *function1);
  EXPECT_TRUE(link.prev().isNoneType());

  WeakLink next_link(&scope, link.next());
  EXPECT_EQ(next_link.referent(), *function0);
  EXPECT_EQ(next_link.prev(), *link);
  EXPECT_TRUE(next_link.next().isNoneType());
}

TEST_F(IcTest,
       IcInvalidateGlobalVarRemovesInvalidatedCacheFromReferencedFunctions) {
  HandleScope scope(thread_);
  Function function0(&scope, testingFunction(thread_));
  Function function1(&scope, testingFunction(thread_));
  Tuple caches0(&scope, function0.caches());
  Tuple caches1(&scope, function1.caches());

  // Both caches of Function0 & 1 caches the same cache value.
  ValueCell cache(&scope, runtime_.newValueCell());
  cache.setValue(SmallInt::fromWord(99));
  ValueCell another_cache(&scope, runtime_.newValueCell());
  another_cache.setValue(SmallInt::fromWord(123));

  icUpdateGlobalVar(thread_, function0, 0, cache);
  icUpdateGlobalVar(thread_, function0, 1, another_cache);
  icUpdateGlobalVar(thread_, function1, 0, another_cache);
  icUpdateGlobalVar(thread_, function1, 1, cache);

  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches0, 0)), 99));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches0, 1)), 123));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches1, 0)), 123));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches1, 1)), 99));

  // Invalidating cache makes it removed from both caches, and nobody depends on
  // it anymore.
  icInvalidateGlobalVar(thread_, cache);

  EXPECT_TRUE(icLookupGlobalVar(caches0, 0).isNoneType());
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches0, 1)), 123));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches1, 0)), 123));
  EXPECT_TRUE(icLookupGlobalVar(caches1, 1).isNoneType());
  EXPECT_TRUE(cache.dependencyLink().isNoneType());
}

TEST_F(IcTest, IcInvalidateGlobalVarDoNotDeferenceDeallocatedReferent) {
  HandleScope scope(thread_);
  Function function0(&scope, testingFunction(thread_));
  Function function1(&scope, testingFunction(thread_));
  Tuple caches0(&scope, function0.caches());
  Tuple caches1(&scope, function1.caches());

  // Both caches of Function0 & 1 caches the same cache value.
  ValueCell cache(&scope, runtime_.newValueCell());
  cache.setValue(SmallInt::fromWord(99));
  ValueCell another_cache(&scope, runtime_.newValueCell());
  another_cache.setValue(SmallInt::fromWord(123));

  icUpdateGlobalVar(thread_, function0, 0, cache);
  icUpdateGlobalVar(thread_, function0, 1, another_cache);
  icUpdateGlobalVar(thread_, function1, 0, another_cache);
  icUpdateGlobalVar(thread_, function1, 1, cache);

  ASSERT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches0, 0)), 99));
  ASSERT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches0, 1)), 123));
  ASSERT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches1, 0)), 123));
  ASSERT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches1, 1)), 99));

  // Simulate GCing function1.
  WeakLink link(&scope, cache.dependencyLink());
  ASSERT_EQ(link.referent(), *function1);
  link.setReferent(NoneType::object());

  // Invalidation cannot touch function1 anymore.
  icInvalidateGlobalVar(thread_, cache);

  EXPECT_TRUE(icLookupGlobalVar(caches0, 0).isNoneType());
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches0, 1)), 123));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches1, 0)), 123));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(caches1, 1)), 99));
  EXPECT_TRUE(cache.dependencyLink().isNoneType());
}

TEST_F(IcTest, IcInvalidateGlobalVarRevertsOpCodeToOriginalOnes) {
  HandleScope scope(thread_);
  Function function(&scope, testingFunction(thread_));
  MutableBytes bytecode(&scope, function.rewrittenBytecode());
  ValueCell cache(&scope, runtime_.newValueCell());
  cache.setValue(SmallInt::fromWord(99));
  ValueCell another_cache(&scope, runtime_.newValueCell());
  another_cache.setValue(SmallInt::fromWord(123));

  byte original_expected[] = {LOAD_GLOBAL, 0, STORE_GLOBAL, 1,
                              LOAD_GLOBAL, 0, STORE_GLOBAL, 1};
  ASSERT_TRUE(isMutableBytesEqualsBytes(bytecode, original_expected));

  icUpdateGlobalVar(thread_, function, 0, cache);
  byte cached_expected0[] = {LOAD_GLOBAL_CACHED, 0, STORE_GLOBAL, 1,
                             LOAD_GLOBAL_CACHED, 0, STORE_GLOBAL, 1};
  EXPECT_TRUE(isMutableBytesEqualsBytes(bytecode, cached_expected0));

  icUpdateGlobalVar(thread_, function, 1, another_cache);
  byte cached_expected1[] = {LOAD_GLOBAL_CACHED, 0, STORE_GLOBAL_CACHED, 1,
                             LOAD_GLOBAL_CACHED, 0, STORE_GLOBAL_CACHED, 1};
  EXPECT_TRUE(isMutableBytesEqualsBytes(bytecode, cached_expected1));

  icInvalidateGlobalVar(thread_, cache);

  // Only invalidated cache's opcode gets reverted to the original one.
  byte invalidated_expected[] = {LOAD_GLOBAL, 0, STORE_GLOBAL_CACHED, 1,
                                 LOAD_GLOBAL, 0, STORE_GLOBAL_CACHED, 1};
  EXPECT_TRUE(isMutableBytesEqualsBytes(bytecode, invalidated_expected));
}

}  // namespace python
