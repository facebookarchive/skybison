#include "gtest/gtest.h"

#include "ic.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using IcTest = RuntimeFixture;

static RawObject layoutIdAsSmallInt(LayoutId id) {
  return SmallInt::fromWord(static_cast<word>(id));
}

TEST_F(IcTest, IcLookupReturnsFirstCachedValue) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(1 * kIcPointersPerCache));
  caches.atPut(kIcEntryKeyOffset, layoutIdAsSmallInt(LayoutId::kSmallInt));
  caches.atPut(kIcEntryValueOffset, runtime_.newInt(44));
  EXPECT_TRUE(
      isIntEqualsWord(icLookupAttr(*caches, 0, LayoutId::kSmallInt), 44));
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
  EXPECT_TRUE(
      isIntEqualsWord(icLookupAttr(*caches, 1, LayoutId::kSmallInt), 7));
}

TEST_F(IcTest, IcLookupWithoutMatchReturnsErrorNotFound) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(2 * kIcPointersPerCache));
  EXPECT_TRUE(icLookupAttr(*caches, 1, LayoutId::kSmallInt).isErrorNotFound());
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
      icLookupBinop(*caches, 1, LayoutId::kSmallInt, LayoutId::kBytes, &flags),
      "xy"));
  EXPECT_EQ(flags, IC_BINOP_REFLECTED);
}

TEST_F(IcTest, IcLookupBinopReturnsErrorNotFound) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(kIcPointersPerCache));
  IcBinopFlags flags;
  EXPECT_TRUE(icLookupBinop(*caches, 0, LayoutId::kSmallInt,
                            LayoutId::kSmallInt, &flags)
                  .isErrorNotFound());
}

TEST_F(IcTest, IcLookupGlobalVar) {
  HandleScope scope(thread_);
  Tuple caches(&scope, runtime_.newTuple(2));
  ValueCell cache(&scope, runtime_.newValueCell());
  cache.setValue(SmallInt::fromWord(99));
  caches.atPut(0, *cache);
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches, 0)), 99));
  EXPECT_TRUE(icLookupGlobalVar(*caches, 1).isNoneType());
}

TEST_F(IcTest, IcUpdateSetsEmptyEntry) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(1 * kIcPointersPerCache));
  Object value(&scope, runtime_.newInt(88));
  icUpdateAttr(*caches, 0, LayoutId::kSmallStr, *value);
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
  icUpdateAttr(*caches, 1, LayoutId::kSmallStr, *value);
  EXPECT_TRUE(isIntEqualsWord(
      caches.at(cache_offset + 2 * kIcPointersPerEntry + kIcEntryKeyOffset),
      static_cast<word>(LayoutId::kSmallStr)));
  EXPECT_TRUE(isStrEqualsCStr(
      caches.at(cache_offset + 2 * kIcPointersPerEntry + kIcEntryValueOffset),
      "test"));
}

TEST_F(IcTest, IcInsertDependencyForTypeLookupInMroAddsDependencyFollowingMro) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  pass

class B(A):
  foo = "class B"

class C(B):
  bar = "class C"
)")
                   .isError());
  Type a(&scope, mainModuleAt(&runtime_, "A"));
  Type b(&scope, mainModuleAt(&runtime_, "B"));
  Type c(&scope, mainModuleAt(&runtime_, "C"));
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  Object dependent(&scope, SmallInt::fromWord(1234));

  // Inserting dependent adds dependent to a new Placeholder in C for 'foo', and
  // to the existing ValueCell in B. A won't be affected since it's not visited
  // during MRO traversal.
  icInsertDependencyForTypeLookupInMro(thread_, c, foo, dependent);

  Tuple mro(&scope, c.mro());
  ASSERT_EQ(mro.length(), 4);
  ASSERT_EQ(mro.at(0), c);
  ASSERT_EQ(mro.at(1), b);
  ASSERT_EQ(mro.at(2), a);

  Dict a_dict(&scope, a.dict());
  EXPECT_TRUE(runtime_.dictAt(thread_, a_dict, foo).isErrorNotFound());

  Dict b_dict(&scope, b.dict());
  ValueCell b_entry(&scope, runtime_.dictAt(thread_, b_dict, foo));
  EXPECT_FALSE(b_entry.isPlaceholder());
  WeakLink b_link(&scope, b_entry.dependencyLink());
  EXPECT_EQ(b_link.referent(), dependent);
  EXPECT_TRUE(b_link.next().isNoneType());

  Dict c_dict(&scope, c.dict());
  ValueCell c_entry(&scope, runtime_.dictAt(thread_, c_dict, foo));
  EXPECT_TRUE(c_entry.isPlaceholder());
  WeakLink c_link(&scope, c_entry.dependencyLink());
  EXPECT_EQ(c_link.referent(), dependent);
  EXPECT_TRUE(c_link.next().isNoneType());
}

TEST_F(IcTest, IcDeleteDependentInValueCellDependencyLinkDeletesDependent) {
  HandleScope scope(thread_);
  ValueCell value_cell(&scope, runtime_.newValueCell());
  Object dependent0(&scope, runtime_.newTuple(4));
  Object dependent1(&scope, runtime_.newTuple(5));
  Object dependent2(&scope, runtime_.newTuple(6));
  Object dependent3(&scope, runtime_.newTuple(7));
  icInsertDependentToValueCellDependencyLink(thread_, dependent3, value_cell);
  icInsertDependentToValueCellDependencyLink(thread_, dependent2, value_cell);
  icInsertDependentToValueCellDependencyLink(thread_, dependent1, value_cell);
  icInsertDependentToValueCellDependencyLink(thread_, dependent0, value_cell);

  // Delete the head.
  icDeleteDependentInValueCell(thread_, value_cell, dependent0);

  WeakLink link(&scope, value_cell.dependencyLink());
  EXPECT_EQ(link.referent(), *dependent1);
  EXPECT_TRUE(link.prev().isNoneType());
  EXPECT_EQ(WeakLink::cast(link.next()).referent(), *dependent2);
  EXPECT_EQ(WeakLink::cast(link.next()).prev(), *link);

  // Delete the dependent in the middle.
  icDeleteDependentInValueCell(thread_, value_cell, dependent2);

  link = value_cell.dependencyLink();
  EXPECT_EQ(link.referent(), *dependent1);
  EXPECT_EQ(WeakLink::cast(link.next()).referent(), *dependent3);
  EXPECT_EQ(WeakLink::cast(link.next()).prev(), *link);

  // Delete the tail.
  icDeleteDependentInValueCell(thread_, value_cell, dependent3);

  link = value_cell.dependencyLink();
  EXPECT_EQ(link.referent(), *dependent1);
  EXPECT_TRUE(link.next().isNoneType());

  // Delete the last node.
  icDeleteDependentInValueCell(thread_, value_cell, dependent1);
  EXPECT_TRUE(value_cell.dependencyLink().isNoneType());
}

TEST_F(
    IcTest,
    IcDeleteDependentFromCachedAttributeDeletesDependentUnderAttributeNameInMro) {
  HandleScope scope(thread_);
  Dict type_dict_a(&scope, runtime_.newDict());
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  Str bar_name(&scope, runtime_.newStrFromCStr("bar"));
  Object dependent_x(&scope, runtime_.newTuple(1));
  Object dependent_y(&scope, runtime_.newTuple(1));

  // A.foo -> x
  ValueCell foo_in_a(&scope, runtime_.newValueCell());
  ASSERT_TRUE(icInsertDependentToValueCellDependencyLink(thread_, dependent_x,
                                                         foo_in_a));
  ASSERT_EQ(WeakLink::cast(foo_in_a.dependencyLink()).referent(), *dependent_x);
  runtime_.dictAtPut(thread_, type_dict_a, foo_name, foo_in_a);

  // A.bar -> y
  ValueCell bar_in_a(&scope, runtime_.newValueCell());
  ASSERT_TRUE(icInsertDependentToValueCellDependencyLink(thread_, dependent_y,
                                                         bar_in_a));
  ASSERT_EQ(WeakLink::cast(bar_in_a.dependencyLink()).referent(), *dependent_y);
  runtime_.dictAtPut(thread_, type_dict_a, bar_name, bar_in_a);

  Type type_a(&scope, runtime_.newType());
  type_a.setDict(*type_dict_a);
  Tuple mro(&scope, runtime_.newTuple(1));
  mro.atPut(0, *type_a);
  type_a.setMro(*mro);

  // Try to delete dependent_y under name "foo". Nothing happens.
  icDeleteDependentFromShadowedAttributes(thread_, type_a, foo_name, type_a,
                                          dependent_y);
  EXPECT_EQ(WeakLink::cast(foo_in_a.dependencyLink()).referent(), *dependent_x);
  EXPECT_EQ(WeakLink::cast(bar_in_a.dependencyLink()).referent(), *dependent_y);

  // Try to delete dependent_x under name "bar". Nothing happens.
  icDeleteDependentFromShadowedAttributes(thread_, type_a, bar_name, type_a,
                                          dependent_x);
  EXPECT_EQ(WeakLink::cast(foo_in_a.dependencyLink()).referent(), *dependent_x);
  EXPECT_EQ(WeakLink::cast(bar_in_a.dependencyLink()).referent(), *dependent_y);

  icDeleteDependentFromShadowedAttributes(thread_, type_a, foo_name, type_a,
                                          dependent_x);
  EXPECT_TRUE(foo_in_a.dependencyLink().isNoneType());
  EXPECT_EQ(WeakLink::cast(bar_in_a.dependencyLink()).referent(), *dependent_y);

  icDeleteDependentFromShadowedAttributes(thread_, type_a, bar_name, type_a,
                                          dependent_y);
  EXPECT_TRUE(foo_in_a.dependencyLink().isNoneType());
  EXPECT_TRUE(bar_in_a.dependencyLink().isNoneType());
}

TEST_F(IcTest,
       IcDeleteDependentFromCachedAttributeDeletesDependentUpToUpdatedType) {
  HandleScope scope(thread_);
  Dict type_dict_a(&scope, runtime_.newDict());
  Dict type_dict_b(&scope, runtime_.newDict());
  Dict type_dict_c(&scope, runtime_.newDict());
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  Object dependent_x(&scope, runtime_.newTuple(1));

  // Create a type hierarchy C -> B -> A depicted by the following Python code:
  // class A:
  //   foo = 1
  //
  // class B(A):
  //   foo = 2
  //
  // class C(B):
  //   pass

  // A.foo -> x
  ValueCell foo_in_a(&scope, runtime_.newValueCell());
  ASSERT_TRUE(!foo_in_a.isPlaceholder());
  ASSERT_TRUE(icInsertDependentToValueCellDependencyLink(thread_, dependent_x,
                                                         foo_in_a));
  runtime_.dictAtPut(thread_, type_dict_a, foo_name, foo_in_a);

  // B.foo -> x
  ValueCell foo_in_b(&scope, runtime_.newValueCell());
  ASSERT_TRUE(!foo_in_b.isPlaceholder());
  ASSERT_TRUE(icInsertDependentToValueCellDependencyLink(thread_, dependent_x,
                                                         foo_in_b));
  runtime_.dictAtPut(thread_, type_dict_b, foo_name, foo_in_b);

  // C.foo -> x
  // Note that this dependency is a placeholder.
  ValueCell foo_in_c(&scope, runtime_.newValueCell());
  foo_in_c.makePlaceholder();
  ASSERT_TRUE(foo_in_c.isPlaceholder());
  ASSERT_TRUE(icInsertDependentToValueCellDependencyLink(thread_, dependent_x,
                                                         foo_in_c));
  runtime_.dictAtPut(thread_, type_dict_c, foo_name, foo_in_c);

  // C -> B -> A.
  Type type_a(&scope, runtime_.newType());
  type_a.setDict(*type_dict_a);
  Type type_b(&scope, runtime_.newType());
  type_b.setDict(*type_dict_b);
  Type type_c(&scope, runtime_.newType());
  type_c.setDict(*type_dict_c);
  Tuple mro(&scope, runtime_.newTuple(3));
  mro.atPut(0, *type_c);
  mro.atPut(1, *type_b);
  mro.atPut(2, *type_a);
  type_c.setMro(*mro);

  // Delete dependent_x for an update to B.foo.
  icDeleteDependentFromShadowedAttributes(thread_, type_c, foo_name, type_b,
                                          dependent_x);

  // B.foo's update doesn't affect the cache for A.foo since the update does not
  // shadow a.foo where type(a) == A.
  EXPECT_TRUE(foo_in_c.dependencyLink().isNoneType());
  EXPECT_TRUE(foo_in_b.dependencyLink().isNoneType());
  // Didn't delete this since type lookup cannot reach A by successful attribute
  // lookup for "foo" in B.
  EXPECT_EQ(WeakLink::cast(foo_in_a.dependencyLink()).referent(), *dependent_x);
}

TEST_F(IcTest, IcIsCachedAttributeAffectedByUpdatedType) {
  // Create a type hierarchy C -> B -> A depicted by the following Python code:
  // class A:
  //   foo = 1
  //
  // class B(A):
  //   foo = 2
  //
  // class C(B):
  //   pass
  HandleScope scope(thread_);
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  Dict type_dict_a(&scope, runtime_.newDict());
  ValueCell foo_in_a(&scope, runtime_.newValueCell());
  runtime_.dictAtPut(thread_, type_dict_a, foo_name, foo_in_a);
  Dict type_dict_b(&scope, runtime_.newDict());
  ValueCell foo_in_b(&scope, runtime_.newValueCell());
  runtime_.dictAtPut(thread_, type_dict_b, foo_name, foo_in_b);
  Dict type_dict_c(&scope, runtime_.newDict());
  ValueCell foo_in_c(&scope, runtime_.newValueCell());
  foo_in_c.makePlaceholder();
  runtime_.dictAtPut(thread_, type_dict_c, foo_name, foo_in_c);

  // Create an mro with C -> B -> A.
  Type type_a(&scope, runtime_.newType());
  type_a.setDict(*type_dict_a);
  Type type_b(&scope, runtime_.newType());
  type_b.setDict(*type_dict_b);
  Type type_c(&scope, runtime_.newType());
  type_c.setDict(*type_dict_c);
  Tuple mro(&scope, runtime_.newTuple(3));
  mro.atPut(0, *type_c);
  mro.atPut(1, *type_b);
  mro.atPut(2, *type_a);
  type_c.setMro(*mro);

  // Check if A.foo is not retrived from C.foo.
  EXPECT_FALSE(icIsCachedAttributeAffectedByUpdatedType(thread_, type_c,
                                                        foo_name, type_a));
  // Check if B.foo is retrieved from C.foo.
  EXPECT_TRUE(icIsCachedAttributeAffectedByUpdatedType(thread_, type_c,
                                                       foo_name, type_b));

  // Assign C.foo to a real value.
  foo_in_c.setValue(NoneType::object());
  // Check if B.foo is not retrived from C.foo from now on.
  EXPECT_FALSE(icIsCachedAttributeAffectedByUpdatedType(thread_, type_c,
                                                        foo_name, type_b));
  // Instead, C.foo is retrieved.
  EXPECT_TRUE(icIsCachedAttributeAffectedByUpdatedType(thread_, type_c,
                                                       foo_name, type_c));
}

// Create a function that maps cache index 1 to the given attribute name.
static RawObject testingFunctionCachingAttributes(Thread* thread,
                                                  Str& attribute_name) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
  Dict globals(&scope, runtime->newDict());
  MutableBytes rewritten_bytecode(&scope,
                                  runtime->newMutableBytesUninitialized(8));
  rewritten_bytecode.byteAtPut(0, LOAD_ATTR_CACHED);
  rewritten_bytecode.byteAtPut(1, 1);

  Function function(&scope,
                    runtime->newFunctionWithCode(thread, name, code, globals));
  function.setRewrittenBytecode(*rewritten_bytecode);

  Tuple original_arguments(&scope, runtime->newTuple(2));
  original_arguments.atPut(1, SmallInt::fromWord(0));
  function.setOriginalArguments(*original_arguments);

  Tuple names(&scope, runtime->newTuple(2));
  names.atPut(0, *attribute_name);
  code.setNames(*names);

  Tuple caches(&scope, runtime->newTuple(2 * kIcPointersPerCache));
  function.setCaches(*caches);

  return *function;
}

TEST_F(
    IcTest,
    IcDeleteCacheForTypeAttrInDependentDeletesCachesForMatchingAttributeName) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass

c = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Type type(&scope, mainModuleAt(&runtime_, "C"));
  Dict type_dict(&scope, type.dict());
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  Str bar_name(&scope, runtime_.newStrFromCStr("bar"));
  Function dependent(&scope,
                     testingFunctionCachingAttributes(thread_, foo_name));

  // foo -> dependent.
  ValueCell foo(&scope, runtime_.newValueCell());
  ASSERT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, dependent, foo));
  runtime_.dictAtPut(thread_, type_dict, foo_name, foo);

  // Create an attribute cache for an instance of C, under name "foo".
  Object instance(&scope, mainModuleAt(&runtime_, "c"));
  Tuple caches(&scope, dependent.caches());
  icUpdateAttr(*caches, 1, instance.layoutId(), SmallInt::fromWord(1234));
  ASSERT_EQ(icLookupAttr(*caches, 1, instance.layoutId()),
            SmallInt::fromWord(1234));

  // Deleting caches for "bar" doesn't affect the cache for "foo".
  icDeleteCacheForTypeAttrInDependent(thread_, type, bar_name, true, dependent);
  EXPECT_EQ(icLookupAttr(*caches, 1, instance.layoutId()),
            SmallInt::fromWord(1234));

  // Deleting caches for "foo".
  icDeleteCacheForTypeAttrInDependent(thread_, type, foo_name, true, dependent);
  EXPECT_TRUE(icLookupAttr(*caches, 1, instance.layoutId()).isErrorNotFound());
}

TEST_F(
    IcTest,
    IcDeleteCacheForTypeAttrInDependentDeletesCachesForInstanceOffsetOnlyWhenDataDesciptorIsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass

c = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Type type(&scope, mainModuleAt(&runtime_, "C"));
  Dict type_dict(&scope, type.dict());
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  Function dependent(&scope,
                     testingFunctionCachingAttributes(thread_, foo_name));

  // foo -> dependent.
  ValueCell foo(&scope, runtime_.newValueCell());
  ASSERT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, dependent, foo));
  runtime_.dictAtPut(thread_, type_dict, foo_name, foo);

  // Create an instance offset cache for an instance of C, under name "foo".
  Object instance(&scope, mainModuleAt(&runtime_, "c"));
  Tuple caches(&scope, dependent.caches());
  icUpdateAttr(*caches, 1, instance.layoutId(), SmallInt::fromWord(1234));
  ASSERT_EQ(icLookupAttr(*caches, 1, instance.layoutId()),
            SmallInt::fromWord(1234));

  // An attempt to delete caches for "foo" with data_descriptor == false doesn't
  // affect it.
  icDeleteCacheForTypeAttrInDependent(thread_, type, foo_name, false,
                                      dependent);
  EXPECT_EQ(icLookupAttr(*caches, 1, instance.layoutId()),
            SmallInt::fromWord(1234));

  // Delete caches for "foo" with data_descriptor == true actually deletes it.
  icDeleteCacheForTypeAttrInDependent(thread_, type, foo_name, true, dependent);
  EXPECT_TRUE(icLookupAttr(*caches, 1, instance.layoutId()).isErrorNotFound());
}

TEST_F(IcTest, IcDeleteCacheForTypeAttrInDependentDeletesOnlyAffectedCaches) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  foo = 1

class B(A):
  foo = 2

class C(B): pass

a = A()
b = B()
c = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Type a_type(&scope, mainModuleAt(&runtime_, "A"));
  Dict a_type_dict(&scope, a_type.dict());
  Type b_type(&scope, mainModuleAt(&runtime_, "B"));
  Dict b_type_dict(&scope, b_type.dict());
  Type c_type(&scope, mainModuleAt(&runtime_, "C"));
  Dict c_type_dict(&scope, c_type.dict());
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  Function dependent(&scope,
                     testingFunctionCachingAttributes(thread_, foo_name));

  // The following lines simulate that dependent caches a.foo, b.foo, c.foo, and
  // x.foo. A.foo -> dependent.
  ValueCell a_foo(&scope, runtime_.dictAt(thread_, a_type_dict, foo_name));
  ASSERT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, dependent, a_foo));
  runtime_.dictAtPut(thread_, a_type_dict, foo_name, a_foo);
  // B.foo -> dependent.
  ValueCell b_foo(&scope, runtime_.dictAt(thread_, b_type_dict, foo_name));
  ASSERT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, dependent, b_foo));
  runtime_.dictAtPut(thread_, b_type_dict, foo_name, b_foo);
  // C.foo -> dependent.
  ValueCell c_foo(&scope, runtime_.newValueCell());
  // This is a placeholder since C.foo is resolved to B.foo.
  c_foo.makePlaceholder();
  ASSERT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, dependent, c_foo));
  runtime_.dictAtPut(thread_, c_type_dict, foo_name, c_foo);

  // Create a cache for a.foo in dependent.
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Tuple caches(&scope, dependent.caches());
  icUpdateAttr(*caches, 1, a.layoutId(), SmallInt::fromWord(100));
  ASSERT_EQ(icLookupAttr(*caches, 1, a.layoutId()), SmallInt::fromWord(100));
  // Create a cache for b.foo in dependent.
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  icUpdateAttr(*caches, 1, b.layoutId(), SmallInt::fromWord(200));
  ASSERT_EQ(icLookupAttr(*caches, 1, b.layoutId()), SmallInt::fromWord(200));
  // Create a cache for c.foo in dependent.
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  icUpdateAttr(*caches, 1, c.layoutId(), SmallInt::fromWord(300));
  ASSERT_EQ(icLookupAttr(*caches, 1, c.layoutId()), SmallInt::fromWord(300));

  // Trigger invalidation by updating B.foo.
  icDeleteCacheForTypeAttrInDependent(thread_, b_type, foo_name, true,
                                      dependent);
  // Note that only caches made for the type attribute are evincted, and
  // dependent is dropped from them.
  EXPECT_EQ(icLookupAttr(*caches, 1, a.layoutId()), SmallInt::fromWord(100));
  EXPECT_EQ(WeakLink::cast(a_foo.dependencyLink()).referent(), *dependent);
  EXPECT_TRUE(icLookupAttr(*caches, 1, b.layoutId()).isErrorNotFound());
  EXPECT_TRUE(b_foo.dependencyLink().isNoneType());
  EXPECT_TRUE(icLookupAttr(*caches, 1, c.layoutId()).isErrorNotFound());
  EXPECT_TRUE(c_foo.dependencyLink().isNoneType());

  // Trigger invalidation by updating A.foo.
  icDeleteCacheForTypeAttrInDependent(thread_, a_type, foo_name, true,
                                      dependent);
  EXPECT_TRUE(icLookupAttr(*caches, 1, a.layoutId()).isErrorNotFound());
  EXPECT_TRUE(a_foo.dependencyLink().isNoneType());
}

// Verify if IcInvalidateCachesForTypeAttr calls
// DeleteCachesForTypeAttrInDependent with all dependents.
TEST_F(IcTest, IcInvalidateCachesForTypeAttrProcessesAllDependents) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass

c = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Type type(&scope, mainModuleAt(&runtime_, "C"));
  Dict type_dict(&scope, type.dict());
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  Str bar_name(&scope, runtime_.newStrFromCStr("bar"));
  Function dependent0(&scope,
                      testingFunctionCachingAttributes(thread_, foo_name));
  Function dependent1(&scope,
                      testingFunctionCachingAttributes(thread_, bar_name));

  // foo -> dependent0.
  ValueCell foo(&scope, runtime_.newValueCell());
  ASSERT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, dependent0, foo));
  runtime_.dictAtPut(thread_, type_dict, foo_name, foo);

  // bar -> dependent1.
  ValueCell bar(&scope, runtime_.newValueCell());
  ASSERT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, dependent1, bar));
  runtime_.dictAtPut(thread_, type_dict, bar_name, bar);

  Object instance(&scope, mainModuleAt(&runtime_, "c"));
  Tuple dependent0_caches(&scope, dependent0.caches());
  {
    // Create an attribute cache for an instance of C, under name "foo" in
    // dependent0.
    icUpdateAttr(*dependent0_caches, 1, instance.layoutId(),
                 SmallInt::fromWord(1234));
    ASSERT_EQ(icLookupAttr(*dependent0_caches, 1, instance.layoutId()),
              SmallInt::fromWord(1234));
  }

  Tuple dependent1_caches(&scope, dependent1.caches());
  {
    // Create an attribute cache for an instance of C, under name "bar" in
    // dependent0.
    icUpdateAttr(*dependent1_caches, 1, instance.layoutId(),
                 SmallInt::fromWord(5678));
    ASSERT_EQ(icLookupAttr(*dependent1_caches, 1, instance.layoutId()),
              SmallInt::fromWord(5678));
  }

  icInvalidateCachesForTypeAttr(thread_, type, foo_name, true);
  EXPECT_TRUE(icLookupAttr(*dependent0_caches, 1, instance.layoutId())
                  .isErrorNotFound());
  EXPECT_EQ(icLookupAttr(*dependent1_caches, 1, instance.layoutId()),
            SmallInt::fromWord(5678));

  icInvalidateCachesForTypeAttr(thread_, type, bar_name, true);
  EXPECT_TRUE(icLookupAttr(*dependent0_caches, 1, instance.layoutId())
                  .isErrorNotFound());
  EXPECT_TRUE(icLookupAttr(*dependent1_caches, 1, instance.layoutId())
                  .isErrorNotFound());
}

TEST_F(IcTest, IcInvalidateCachesForTypeAttrDoesNothingForNotFoundTypeAttr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass
)")
                   .isError());
  HandleScope scope(thread_);
  Type type(&scope, mainModuleAt(&runtime_, "C"));
  Str foo_name(&scope, runtime_.newStrFromCStr("foo"));
  icInvalidateCachesForTypeAttr(thread_, type, foo_name, true);
}

TEST_F(IcTest,
       BinarySubscrUpdateCacheWithRaisingDescriptorPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Desc:
  def __get__(self, instance, type):
    raise UserWarning("foo")

class C:
  __getitem__ = Desc()

container = C()
result = container[0]
)"),
                            LayoutId::kUserWarning, "foo"));
}

TEST_F(IcTest,
       ForIterUpdateCacheWithRaisingDescriptorDunderNextPropagatesException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Desc:
  def __get__(self, instance, type):
    raise UserWarning("foo")

class C:
  def __iter__(self):
    return self
  __next__ = Desc()

container = C()
result = [x for x in container]
)"),
                            LayoutId::kUserWarning, "foo"));
}

TEST_F(IcTest, BinarySubscrUpdateCacheWithFunctionUpdatesCache) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def f(c, k):
  return c[k]

container = [1, 2, 3]
getitem = type(container).__getitem__
result = f(container, 0)
)")
                   .isError());

  HandleScope scope;
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));

  Object container(&scope, mainModuleAt(&runtime_, "container"));
  Object getitem(&scope, mainModuleAt(&runtime_, "getitem"));
  Function f(&scope, mainModuleAt(&runtime_, "f"));
  Tuple caches(&scope, f.caches());
  // Expect that BINARY_SUBSCR is the only cached opcode in f().
  ASSERT_EQ(caches.length(), 1 * kIcPointersPerCache);
  EXPECT_EQ(icLookupAttr(*caches, 0, container.layoutId()), *getitem);

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
container2 = [4, 5, 6]
result2 = f(container2, 1)
)")
                   .isError());
  Object container2(&scope, mainModuleAt(&runtime_, "container2"));
  Object result2(&scope, mainModuleAt(&runtime_, "result2"));
  EXPECT_EQ(container2.layoutId(), container.layoutId());
  EXPECT_TRUE(isIntEqualsWord(*result2, 5));
}

TEST_F(IcTest, BinarySubscrUpdateCacheWithNonFunctionDoesntUpdateCache) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "hi"));

  Object container(&scope, mainModuleAt(&runtime_, "container"));
  Function f(&scope, mainModuleAt(&runtime_, "f"));
  Tuple caches(&scope, f.caches());
  // Expect that BINARY_SUBSCR is the only cached opcode in f().
  ASSERT_EQ(caches.length(), 1 * kIcPointersPerCache);
  EXPECT_TRUE(icLookupAttr(*caches, 0, container.layoutId()).isErrorNotFound());

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
container2 = Container()
result2 = f(container, "hello there!")
)")
                   .isError());
  Object container2(&scope, mainModuleAt(&runtime_, "container2"));
  Object result2(&scope, mainModuleAt(&runtime_, "result2"));
  ASSERT_EQ(container2.layoutId(), container.layoutId());
  EXPECT_TRUE(isStrEqualsCStr(*result2, "hello there!"));
}

TEST_F(IcTest, IcUpdateBinopSetsEmptyEntry) {
  HandleScope scope(thread_);

  Tuple caches(&scope, runtime_.newTuple(kIcPointersPerCache));
  Object value(&scope, runtime_.newInt(-44));
  icUpdateBinop(*caches, 0, LayoutId::kSmallStr, LayoutId::kLargeBytes, *value,
                IC_BINOP_REFLECTED);
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
  icUpdateBinop(*caches, 1, LayoutId::kLargeInt, LayoutId::kSmallInt, *value,
                IC_BINOP_NONE);
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

TEST_F(IcTest, ForIterUpdateCacheWithFunctionUpdatesCache) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 1));

  Object iterator(&scope, mainModuleAt(&runtime_, "iterator"));
  Object iter_next(&scope, mainModuleAt(&runtime_, "iter_next"));
  Function f(&scope, mainModuleAt(&runtime_, "f"));
  Tuple caches(&scope, f.caches());
  // Expect that FOR_ITER is the only cached opcode in f().
  ASSERT_EQ(caches.length(), 1 * kIcPointersPerCache);
  EXPECT_EQ(icLookupAttr(*caches, 0, iterator.layoutId()), *iter_next);
}

TEST_F(IcTest, ForIterUpdateCacheWithNonFunctionDoesntUpdateCache) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 123));

  Object iterator(&scope, mainModuleAt(&runtime_, "iterator"));
  Function f(&scope, mainModuleAt(&runtime_, "f"));
  Tuple caches(&scope, f.caches());
  // Expect that FOR_ITER is the only cached opcode in f().
  ASSERT_EQ(caches.length(), 1 * kIcPointersPerCache);
  EXPECT_TRUE(icLookupAttr(*caches, 0, iterator.layoutId()).isErrorNotFound());
}

static RawObject testingFunction(Thread* thread) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object name(&scope, Str::empty());
  Code code(&scope, newEmptyCode());
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

  Function function(&scope,
                    runtime->newFunctionWithCode(thread, name, code, globals));
  function.setRewrittenBytecode(*rewritten_bytecode);

  code.setNames(runtime->newTuple(2));
  Tuple caches(&scope, runtime->newTuple(2));
  function.setCaches(*caches);
  return *function;
}

TEST_F(IcTest,
       IcInsertDependentToValueCellDependencyLinkInsertsDependentAsHead) {
  HandleScope scope(thread_);
  Function function0(&scope, testingFunction(thread_));
  Function function1(&scope, testingFunction(thread_));

  ValueCell cache(&scope, runtime_.newValueCell());
  ASSERT_TRUE(cache.dependencyLink().isNoneType());

  EXPECT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, function0, cache));
  WeakLink link0(&scope, cache.dependencyLink());
  EXPECT_EQ(link0.referent(), *function0);
  EXPECT_TRUE(link0.prev().isNoneType());
  EXPECT_TRUE(link0.next().isNoneType());

  EXPECT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, function1, cache));
  WeakLink link1(&scope, cache.dependencyLink());
  EXPECT_EQ(link1.referent(), *function1);
  EXPECT_TRUE(link1.prev().isNoneType());
  EXPECT_EQ(link1.next(), *link0);
}

TEST_F(
    IcTest,
    IcInsertDependentToValueCellDependencyLinkDoesNotInsertExistingDependent) {
  HandleScope scope(thread_);
  Function function0(&scope, testingFunction(thread_));
  Function function1(&scope, testingFunction(thread_));

  ValueCell cache(&scope, runtime_.newValueCell());
  EXPECT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, function0, cache));
  EXPECT_TRUE(
      icInsertDependentToValueCellDependencyLink(thread_, function1, cache));
  EXPECT_FALSE(
      icInsertDependentToValueCellDependencyLink(thread_, function0, cache));

  WeakLink link(&scope, cache.dependencyLink());
  EXPECT_EQ(link.referent(), *function1);
  EXPECT_TRUE(link.prev().isNoneType());
  EXPECT_EQ(WeakLink::cast(link.next()).referent(), *function0);
  EXPECT_TRUE(WeakLink::cast(link.next()).next().isNoneType());
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
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches0, 0)), 99));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches0, 1)), 123));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches1, 0)), 123));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches1, 1)), 99));

  // Invalidating cache makes it removed from both caches, and nobody depends on
  // it anymore.
  icInvalidateGlobalVar(thread_, cache);

  EXPECT_TRUE(icLookupGlobalVar(*caches0, 0).isNoneType());
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches0, 1)), 123));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches1, 0)), 123));
  EXPECT_TRUE(icLookupGlobalVar(*caches1, 1).isNoneType());
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
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches0, 0)), 99));
  ASSERT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches0, 1)), 123));
  ASSERT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches1, 0)), 123));
  ASSERT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches1, 1)), 99));

  // Simulate GCing function1.
  WeakLink link(&scope, cache.dependencyLink());
  ASSERT_EQ(link.referent(), *function1);
  link.setReferent(NoneType::object());

  // Invalidation cannot touch function1 anymore.
  icInvalidateGlobalVar(thread_, cache);

  EXPECT_TRUE(icLookupGlobalVar(*caches0, 0).isNoneType());
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches0, 1)), 123));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches1, 0)), 123));
  EXPECT_TRUE(
      isIntEqualsWord(valueCellValue(icLookupGlobalVar(*caches1, 1)), 99));
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

static RawObject layoutIdAsSmallInt(RawObject object) {
  return SmallInt::fromWord(static_cast<word>(object.layoutId()));
}

TEST_F(IcTest, IcIterator) {
  HandleScope scope(thread_);
  MutableBytes bytecode(&scope, runtime_.newMutableBytesUninitialized(16));
  bytecode.byteAtPut(0, LOAD_GLOBAL);
  bytecode.byteAtPut(1, 100);
  bytecode.byteAtPut(2, LOAD_ATTR_CACHED);
  bytecode.byteAtPut(3, 0);
  bytecode.byteAtPut(4, LOAD_GLOBAL);
  bytecode.byteAtPut(5, 100);
  bytecode.byteAtPut(6, LOAD_METHOD_CACHED);
  bytecode.byteAtPut(7, 1);
  bytecode.byteAtPut(8, LOAD_GLOBAL);
  bytecode.byteAtPut(9, 100);
  bytecode.byteAtPut(10, LOAD_ATTR_CACHED);
  bytecode.byteAtPut(11, 2);
  bytecode.byteAtPut(12, STORE_ATTR_CACHED);
  bytecode.byteAtPut(13, 3);
  bytecode.byteAtPut(14, LOAD_GLOBAL);
  bytecode.byteAtPut(15, 100);

  Tuple original_args(&scope, runtime_.newTuple(4));
  original_args.atPut(0, SmallInt::fromWord(0));
  original_args.atPut(1, SmallInt::fromWord(1));
  original_args.atPut(2, SmallInt::fromWord(2));
  original_args.atPut(3, SmallInt::fromWord(3));

  Tuple names(&scope, runtime_.newTuple(4));
  names.atPut(0, runtime_.newStrFromCStr("load_attr_cached_attr_name"));
  names.atPut(1, runtime_.newStrFromCStr("load_method_cached_attr_name"));
  names.atPut(2, runtime_.newStrFromCStr("load_attr_cached_attr_name2"));
  names.atPut(3, runtime_.newStrFromCStr("store_attr_cached_attr_name"));

  Tuple caches(&scope, runtime_.newTuple(4 * kIcPointersPerCache));
  // Caches for LOAD_ATTR_CACHED at 2.
  word load_attr_cached_cache_index0 =
      0 * kIcPointersPerCache + 1 * kIcPointersPerEntry;
  word load_attr_cached_cache_index1 =
      0 * kIcPointersPerCache + 3 * kIcPointersPerEntry;
  caches.atPut(load_attr_cached_cache_index0 + kIcEntryKeyOffset,
               layoutIdAsSmallInt(Bool::trueObj()));
  caches.atPut(load_attr_cached_cache_index0 + kIcEntryValueOffset,
               SmallInt::fromWord(10));
  caches.atPut(load_attr_cached_cache_index1 + kIcEntryKeyOffset,
               layoutIdAsSmallInt(Bool::falseObj()));
  caches.atPut(load_attr_cached_cache_index1 + kIcEntryValueOffset,
               SmallInt::fromWord(20));

  // Caches for LOAD_METHOD_CACHED at 6.
  word load_method_cached_index =
      1 * kIcPointersPerCache + 0 * kIcPointersPerEntry;
  caches.atPut(load_method_cached_index + kIcEntryKeyOffset,
               layoutIdAsSmallInt(SmallInt::fromWord(0)));
  caches.atPut(load_method_cached_index + kIcEntryValueOffset,
               SmallInt::fromWord(30));

  // Caches are empty for LOAD_ATTR_CACHED at 10.

  // Caches for STORE_ATTR_CACHED at 12.
  word store_attr_cached_index =
      3 * kIcPointersPerCache + 3 * kIcPointersPerEntry;
  caches.atPut(store_attr_cached_index + kIcEntryKeyOffset,
               layoutIdAsSmallInt(NoneType::object()));
  caches.atPut(store_attr_cached_index + kIcEntryValueOffset,
               SmallInt::fromWord(40));

  Function function(&scope, newEmptyFunction());
  function.setRewrittenBytecode(*bytecode);
  function.setCaches(*caches);
  Code::cast(function.code()).setNames(*names);
  function.setOriginalArguments(*original_args);

  IcIterator it(&scope, *function);
  ASSERT_TRUE(it.hasNext());
  Str load_attr_cached_attr_name(
      &scope, runtime_.newStrFromCStr("load_attr_cached_attr_name"));
  EXPECT_TRUE(it.isAttrNameEqualTo(load_attr_cached_attr_name));
  EXPECT_EQ(it.key(), Bool::trueObj().layoutId());
  EXPECT_TRUE(it.isInstanceAttr());

  it.next();
  ASSERT_TRUE(it.hasNext());
  EXPECT_TRUE(it.isAttrNameEqualTo(load_attr_cached_attr_name));
  EXPECT_EQ(it.key(), Bool::falseObj().layoutId());
  EXPECT_TRUE(it.isInstanceAttr());

  it.next();
  ASSERT_TRUE(it.hasNext());
  Str load_method_cached_attr_name(
      &scope, runtime_.newStrFromCStr("load_method_cached_attr_name"));
  EXPECT_TRUE(it.isAttrNameEqualTo(load_method_cached_attr_name));
  EXPECT_EQ(it.key(), SmallInt::fromWord(100).layoutId());
  EXPECT_TRUE(it.isInstanceAttr());

  it.next();
  Str store_attr_cached_attr_name(
      &scope, runtime_.newStrFromCStr("store_attr_cached_attr_name"));
  ASSERT_TRUE(it.hasNext());
  EXPECT_TRUE(it.isAttrNameEqualTo(store_attr_cached_attr_name));
  EXPECT_EQ(it.key(), NoneType::object().layoutId());
  EXPECT_TRUE(it.isInstanceAttr());

  it.evict();
  EXPECT_TRUE(
      caches.at(store_attr_cached_index + kIcEntryKeyOffset).isNoneType());
  EXPECT_TRUE(
      caches.at(store_attr_cached_index + kIcEntryValueOffset).isNoneType());

  it.next();
  EXPECT_FALSE(it.hasNext());
}

}  // namespace python
