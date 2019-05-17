#include "gtest/gtest.h"

#include "ic.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(IcTest, IcPrepareBytecodeRewritesLoadAttrOperations) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, runtime.newEmptyCode(name));
  byte bytecode[] = {
      NOP,          99,        EXTENDED_ARG, 0xca, LOAD_ATTR,    0xfe,
      NOP,          LOAD_ATTR, EXTENDED_ARG, 1,    EXTENDED_ARG, 2,
      EXTENDED_ARG, 3,         STORE_ATTR,   4,    LOAD_ATTR,    77,
  };
  code.setCode(runtime.newBytesWithAll(bytecode));
  Object none(&scope, NoneType::object());
  Dict globals(&scope, runtime.newDict());
  Dict builtins(&scope, runtime.newDict());
  Function function(
      &scope, Interpreter::makeFunction(thread, name, code, none, none, none,
                                        none, globals, builtins));

  icRewriteBytecode(thread, function);

  byte expected[] = {
      NOP,          99,        EXTENDED_ARG,      0, LOAD_ATTR_CACHED, 0,
      NOP,          LOAD_ATTR, EXTENDED_ARG,      0, EXTENDED_ARG,     0,
      EXTENDED_ARG, 0,         STORE_ATTR_CACHED, 1, LOAD_ATTR_CACHED, 2,
  };
  Object rewritten_bytecode(&scope, function.rewrittenBytecode());
  EXPECT_TRUE(isBytesEqualsBytes(rewritten_bytecode, expected));

  ASSERT_TRUE(function.caches().isTuple());
  Tuple caches(&scope, function.caches());
  EXPECT_EQ(caches.length(), 3 * kIcPointersPerCache);
  for (word i = 0, length = caches.length(); i < length; i++) {
    EXPECT_TRUE(caches.at(i).isNoneType()) << "index " << i;
  }

  Tuple original_args(&scope, function.originalArguments());
  EXPECT_EQ(icOriginalArg(original_args, 0), 0xcafe);
  EXPECT_EQ(icOriginalArg(original_args, 1), 0x01020304);
  EXPECT_EQ(icOriginalArg(original_args, 2), 77);
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

}  // namespace python
