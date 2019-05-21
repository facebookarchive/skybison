#include "gtest/gtest.h"

#include "ic.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(IcTest, IcRewriteBytecodeRewritesLoadAttrOperations) {
  Runtime runtime;
  runtime.enableCache();
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  byte bytecode[] = {
      NOP,          99,        EXTENDED_ARG, 0xca, LOAD_ATTR,    0xfe,
      NOP,          LOAD_ATTR, EXTENDED_ARG, 1,    EXTENDED_ARG, 2,
      EXTENDED_ARG, 3,         LOAD_ATTR,    4,    LOAD_ATTR,    77,
  };
  code.setCode(runtime.newBytesWithAll(bytecode));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals, builtins));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {
      NOP,          99,        EXTENDED_ARG,     0, LOAD_ATTR_CACHED, 0,
      NOP,          LOAD_ATTR, EXTENDED_ARG,     0, EXTENDED_ARG,     0,
      EXTENDED_ARG, 0,         LOAD_ATTR_CACHED, 1, LOAD_ATTR_CACHED, 2,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  EXPECT_EQ(caches.length(), 3 * kIcPointersPerCache);
  for (word i = 0, length = caches.length(); i < length; i++) {
    EXPECT_TRUE(caches.at(i).isNoneType()) << "index " << i;
  }

  ASSERT_TRUE(function.originalArguments().isTuple());
  Tuple original_args(&scope, function.originalArguments());
  EXPECT_EQ(icOriginalArg(original_args, 0), 0xcafe);
  EXPECT_EQ(icOriginalArg(original_args, 1), 0x01020304);
  EXPECT_EQ(icOriginalArg(original_args, 2), 77);
}

TEST(IcTest, IcRewriteBytecodeRewritesStoreAttr) {
  Runtime runtime;
  runtime.enableCache();
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  byte bytecode[] = {STORE_ATTR, 48};
  code.setCode(runtime.newBytesWithAll(bytecode));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals, builtins));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {STORE_ATTR_CACHED, 0};
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.originalArguments().isTuple());
  Tuple original_args(&scope, function.originalArguments());
  EXPECT_EQ(icOriginalArg(original_args, 0), 48);
}

TEST(IcTest, IcRewriteBytecodeRewritesBinaryOpcodes) {
  Runtime runtime;
  runtime.enableCache();
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
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
  Dict builtins(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals, builtins));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {
      BINARY_OP_CACHED, 0,  BINARY_OP_CACHED, 1,  BINARY_OP_CACHED, 2,
      BINARY_OP_CACHED, 3,  BINARY_OP_CACHED, 4,  BINARY_OP_CACHED, 5,
      BINARY_OP_CACHED, 6,  BINARY_OP_CACHED, 7,  BINARY_OP_CACHED, 8,
      BINARY_OP_CACHED, 9,  BINARY_OP_CACHED, 10, BINARY_OP_CACHED, 11,
      BINARY_OP_CACHED, 12,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.originalArguments().isTuple());
  Tuple original_args(&scope, function.originalArguments());
  EXPECT_EQ(icOriginalArg(original_args, 0),
            static_cast<word>(Interpreter::BinaryOp::MATMUL));
  EXPECT_EQ(icOriginalArg(original_args, 1),
            static_cast<word>(Interpreter::BinaryOp::POW));
  EXPECT_EQ(icOriginalArg(original_args, 2),
            static_cast<word>(Interpreter::BinaryOp::MUL));
  EXPECT_EQ(icOriginalArg(original_args, 3),
            static_cast<word>(Interpreter::BinaryOp::MOD));
  EXPECT_EQ(icOriginalArg(original_args, 4),
            static_cast<word>(Interpreter::BinaryOp::ADD));
  EXPECT_EQ(icOriginalArg(original_args, 5),
            static_cast<word>(Interpreter::BinaryOp::SUB));
  EXPECT_EQ(icOriginalArg(original_args, 6),
            static_cast<word>(Interpreter::BinaryOp::FLOORDIV));
  EXPECT_EQ(icOriginalArg(original_args, 7),
            static_cast<word>(Interpreter::BinaryOp::TRUEDIV));
  EXPECT_EQ(icOriginalArg(original_args, 8),
            static_cast<word>(Interpreter::BinaryOp::LSHIFT));
  EXPECT_EQ(icOriginalArg(original_args, 9),
            static_cast<word>(Interpreter::BinaryOp::RSHIFT));
  EXPECT_EQ(icOriginalArg(original_args, 10),
            static_cast<word>(Interpreter::BinaryOp::AND));
  EXPECT_EQ(icOriginalArg(original_args, 11),
            static_cast<word>(Interpreter::BinaryOp::XOR));
  EXPECT_EQ(icOriginalArg(original_args, 12),
            static_cast<word>(Interpreter::BinaryOp::OR));
}

TEST(IcTest, IcRewriteBytecodeRewritesInplaceOpcodes) {
  Runtime runtime;
  runtime.enableCache();
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
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
  Dict builtins(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals, builtins));
  // makeFunction() calls icRewriteBytecode().

  byte expected[] = {
      INPLACE_OP_CACHED, 0,  INPLACE_OP_CACHED, 1,  INPLACE_OP_CACHED, 2,
      INPLACE_OP_CACHED, 3,  INPLACE_OP_CACHED, 4,  INPLACE_OP_CACHED, 5,
      INPLACE_OP_CACHED, 6,  INPLACE_OP_CACHED, 7,  INPLACE_OP_CACHED, 8,
      INPLACE_OP_CACHED, 9,  INPLACE_OP_CACHED, 10, INPLACE_OP_CACHED, 11,
      INPLACE_OP_CACHED, 12,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.originalArguments().isTuple());
  Tuple original_args(&scope, function.originalArguments());
  EXPECT_EQ(icOriginalArg(original_args, 0),
            static_cast<word>(Interpreter::BinaryOp::MATMUL));
  EXPECT_EQ(icOriginalArg(original_args, 1),
            static_cast<word>(Interpreter::BinaryOp::POW));
  EXPECT_EQ(icOriginalArg(original_args, 2),
            static_cast<word>(Interpreter::BinaryOp::MUL));
  EXPECT_EQ(icOriginalArg(original_args, 3),
            static_cast<word>(Interpreter::BinaryOp::MOD));
  EXPECT_EQ(icOriginalArg(original_args, 4),
            static_cast<word>(Interpreter::BinaryOp::ADD));
  EXPECT_EQ(icOriginalArg(original_args, 5),
            static_cast<word>(Interpreter::BinaryOp::SUB));
  EXPECT_EQ(icOriginalArg(original_args, 6),
            static_cast<word>(Interpreter::BinaryOp::FLOORDIV));
  EXPECT_EQ(icOriginalArg(original_args, 7),
            static_cast<word>(Interpreter::BinaryOp::TRUEDIV));
  EXPECT_EQ(icOriginalArg(original_args, 8),
            static_cast<word>(Interpreter::BinaryOp::LSHIFT));
  EXPECT_EQ(icOriginalArg(original_args, 9),
            static_cast<word>(Interpreter::BinaryOp::RSHIFT));
  EXPECT_EQ(icOriginalArg(original_args, 10),
            static_cast<word>(Interpreter::BinaryOp::AND));
  EXPECT_EQ(icOriginalArg(original_args, 11),
            static_cast<word>(Interpreter::BinaryOp::XOR));
  EXPECT_EQ(icOriginalArg(original_args, 12),
            static_cast<word>(Interpreter::BinaryOp::OR));
}

TEST(IcTest, IcRewriteBytecodeRewritesCompareOpOpcodes) {
  Runtime runtime;
  runtime.enableCache();
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
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
  Dict builtins(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals, builtins));
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
  EXPECT_TRUE(isBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.originalArguments().isTuple());
  Tuple original_args(&scope, function.originalArguments());
  EXPECT_EQ(icOriginalArg(original_args, 0), static_cast<word>(CompareOp::LT));
  EXPECT_EQ(icOriginalArg(original_args, 1), static_cast<word>(CompareOp::LE));
  EXPECT_EQ(icOriginalArg(original_args, 2), static_cast<word>(CompareOp::EQ));
  EXPECT_EQ(icOriginalArg(original_args, 3), static_cast<word>(CompareOp::NE));
  EXPECT_EQ(icOriginalArg(original_args, 4), static_cast<word>(CompareOp::GT));
  EXPECT_EQ(icOriginalArg(original_args, 5), static_cast<word>(CompareOp::GE));
}

static RawObject layoutIdAsSmallInt(LayoutId id) {
  return SmallInt::fromWord(static_cast<word>(id));
}

TEST(IcTest, IcLookupReturnsFirstCachedValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Tuple caches(&scope, runtime.newTuple(1 * kIcPointersPerCache));
  caches.atPut(kIcEntryKeyOffset, layoutIdAsSmallInt(LayoutId::kSmallInt));
  caches.atPut(kIcEntryValueOffset, runtime.newInt(44));
  EXPECT_TRUE(isIntEqualsWord(icLookup(caches, 0, LayoutId::kSmallInt), 44));
}

TEST(IcTest, IcLookupReturnsFourthCachedValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Tuple caches(&scope, runtime.newTuple(2 * kIcPointersPerCache));
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
               runtime.newInt(7));
  EXPECT_TRUE(isIntEqualsWord(icLookup(caches, 1, LayoutId::kSmallInt), 7));
}

TEST(IcTest, IcLookupWithoutMatchReturnsErrorNotFound) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Tuple caches(&scope, runtime.newTuple(2 * kIcPointersPerCache));
  EXPECT_TRUE(icLookup(caches, 1, LayoutId::kSmallInt).isErrorNotFound());
}

static RawObject binopKey(LayoutId left, LayoutId right, IcBinopFlags flags) {
  return SmallInt::fromWord((static_cast<word>(left) << Header::kLayoutIdSize |
                             static_cast<word>(right))
                                << kBitsPerByte |
                            static_cast<word>(flags));
}

TEST(IcTest, IcLookupBinopReturnsCachedValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Tuple caches(&scope, runtime.newTuple(2 * kIcPointersPerCache));
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
               runtime.newStrFromCStr("xy"));

  IcBinopFlags flags;
  EXPECT_TRUE(isStrEqualsCStr(
      icLookupBinop(caches, 1, LayoutId::kSmallInt, LayoutId::kBytes, &flags),
      "xy"));
  EXPECT_EQ(flags, IC_BINOP_REFLECTED);
}

TEST(IcTest, IcLookupBinopReturnsErrorNotFound) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Tuple caches(&scope, runtime.newTuple(kIcPointersPerCache));
  IcBinopFlags flags;
  EXPECT_TRUE(
      icLookupBinop(caches, 0, LayoutId::kSmallInt, LayoutId::kSmallInt, &flags)
          .isErrorNotFound());
}

TEST(IcTest, IcUpdateSetsEmptyEntry) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Tuple caches(&scope, runtime.newTuple(1 * kIcPointersPerCache));
  Object value(&scope, runtime.newInt(88));
  icUpdate(thread, caches, 0, LayoutId::kSmallStr, value);
  EXPECT_TRUE(isIntEqualsWord(caches.at(kIcEntryKeyOffset),
                              static_cast<word>(LayoutId::kSmallStr)));
  EXPECT_TRUE(isIntEqualsWord(caches.at(kIcEntryValueOffset), 88));
}

TEST(IcTest, IcUpdateUpdatesExistingEntry) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Tuple caches(&scope, runtime.newTuple(2 * kIcPointersPerCache));
  word cache_offset = kIcPointersPerCache;
  caches.atPut(cache_offset + 0 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kSmallInt));
  caches.atPut(cache_offset + 1 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kSmallBytes));
  caches.atPut(cache_offset + 2 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kSmallStr));
  caches.atPut(cache_offset + 3 * kIcPointersPerEntry + kIcEntryKeyOffset,
               layoutIdAsSmallInt(LayoutId::kBytes));
  Object value(&scope, runtime.newStrFromCStr("test"));
  icUpdate(thread, caches, 1, LayoutId::kSmallStr, value);
  EXPECT_TRUE(isIntEqualsWord(
      caches.at(cache_offset + 2 * kIcPointersPerEntry + kIcEntryKeyOffset),
      static_cast<word>(LayoutId::kSmallStr)));
  EXPECT_TRUE(isStrEqualsCStr(
      caches.at(cache_offset + 2 * kIcPointersPerEntry + kIcEntryValueOffset),
      "test"));
}

TEST(IcTest, BinarySubscrUpdateCacheWithFunctionUpdatesCache) {
  Runtime runtime;
  runtime.enableCache();
  ASSERT_FALSE(runFromCStr(&runtime, R"(
def f(c, k):
  return c[k]

container = [1, 2, 3]
getitem = type(container).__getitem__
result = f(container, 0)
)")
                   .isError());

  Thread* thread = Thread::current();
  HandleScope scope(thread);
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

TEST(IcTest, BinarySubscrUpdateCacheWithNonFunctionDoesntUpdateCache) {
  Runtime runtime;
  runtime.enableCache();
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

  Thread* thread = Thread::current();
  HandleScope scope(thread);
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

TEST(IcTest, IcUpdateBinopSetsEmptyEntry) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Tuple caches(&scope, runtime.newTuple(kIcPointersPerCache));
  Object value(&scope, runtime.newInt(-44));
  icUpdateBinop(thread, caches, 0, LayoutId::kSmallStr, LayoutId::kLargeBytes,
                value, IC_BINOP_REFLECTED);
  EXPECT_EQ(
      caches.at(kIcEntryKeyOffset),
      binopKey(LayoutId::kSmallStr, LayoutId::kLargeBytes, IC_BINOP_REFLECTED));
  EXPECT_TRUE(isIntEqualsWord(caches.at(kIcEntryValueOffset), -44));
}

TEST(IcTest, IcUpdateBinopSetsExistingEntry) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Tuple caches(&scope, runtime.newTuple(2 * kIcPointersPerCache));
  word cache_offset = kIcPointersPerCache;
  caches.atPut(
      cache_offset + 0 * kIcPointersPerEntry + kIcEntryKeyOffset,
      binopKey(LayoutId::kSmallInt, LayoutId::kLargeInt, IC_BINOP_NONE));
  caches.atPut(
      cache_offset + 1 * kIcPointersPerEntry + kIcEntryKeyOffset,
      binopKey(LayoutId::kLargeInt, LayoutId::kSmallInt, IC_BINOP_REFLECTED));
  Object value(&scope, runtime.newStrFromCStr("yyy"));
  icUpdateBinop(thread, caches, 1, LayoutId::kLargeInt, LayoutId::kSmallInt,
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

}  // namespace python
