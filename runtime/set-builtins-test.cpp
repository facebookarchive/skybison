#include "gtest/gtest.h"

#include "runtime.h"
#include "set-builtins.h"
#include "test-utils.h"

namespace py {

using namespace testing;

using FrozenSetBuiltinsTest = RuntimeFixture;
using SetBuiltinsTest = RuntimeFixture;
using SetIteratorBuiltinsTest = RuntimeFixture;

TEST_F(SetBuiltinsTest, SetPopException) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
s = {1}
s.pop()
s.pop()
)"),
                            LayoutId::kKeyError, "pop from an empty set"));
}

TEST_F(SetBuiltinsTest, SetPop) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = {1}
a = s.pop()
b = len(s)
)")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 0));
}

TEST_F(SetBuiltinsTest, InitializeByTypeCall) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = set()
)")
                   .isError());
  Object s(&scope, mainModuleAt(&runtime_, "s"));
  EXPECT_TRUE(s.isSet());
  EXPECT_EQ(Set::cast(*s).numItems(), 0);
}

TEST_F(SetBuiltinsTest, SetAdd) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
s = set()
s.add(1)
s.add("Hello, World")
)")
                   .isError());
  Set s(&scope, mainModuleAt(&runtime_, "s"));
  Object one(&scope, runtime_.newInt(1));
  Object hello_world(&scope, runtime_.newStrFromCStr("Hello, World"));
  EXPECT_EQ(s.numItems(), 2);
  EXPECT_TRUE(setIncludes(thread, s, one));
  EXPECT_TRUE(setIncludes(thread, s, hello_world));
}

TEST_F(SetBuiltinsTest, DunderIterReturnsSetIterator) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set empty_set(&scope, runtime_.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, empty_set));
  ASSERT_TRUE(iter.isSetIterator());
}

TEST_F(SetBuiltinsTest, DunderAnd) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Set set1(&scope, runtime_.newSet());
  Set set2(&scope, runtime_.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, set1, set2));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(Set::cast(*result).numItems(), 0);

  Object key(&scope, SmallInt::fromWord(1));
  setHashAndAdd(thread, set1, key);
  key = SmallInt::fromWord(2);
  setHashAndAdd(thread, set1, key);
  Object result1(&scope, runBuiltin(SetBuiltins::dunderAnd, set1, set2));
  ASSERT_TRUE(result1.isSet());
  EXPECT_EQ(Set::cast(*result1).numItems(), 0);

  key = SmallInt::fromWord(1);
  setHashAndAdd(thread, set2, key);
  Object result2(&scope, runBuiltin(SetBuiltins::dunderAnd, set1, set2));
  ASSERT_TRUE(result2.isSet());
  Set set(&scope, *result2);
  EXPECT_EQ(set.numItems(), 1);
  EXPECT_TRUE(setIncludes(thread, set, key));
}

TEST_F(SetBuiltinsTest, DunderAndWithNonSet) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Object empty_set(&scope, runtime_.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, empty_set, none));
  ASSERT_TRUE(result.isNotImplementedType());
}

TEST_F(SetBuiltinsTest, DunderIand) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Set set1(&scope, runtime_.newSet());
  Set set2(&scope, runtime_.newSet());
  Object key(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderIand, set1, set2));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(*result, *set1);
  EXPECT_EQ(Set::cast(*result).numItems(), 0);

  key = SmallInt::fromWord(1);
  setHashAndAdd(thread, set1, key);
  key = SmallInt::fromWord(2);
  setHashAndAdd(thread, set1, key);
  Object result1(&scope, runBuiltin(SetBuiltins::dunderIand, set1, set2));
  ASSERT_TRUE(result1.isSet());
  EXPECT_EQ(*result1, *set1);
  EXPECT_EQ(Set::cast(*result1).numItems(), 0);

  set1 = runtime_.newSet();
  key = SmallInt::fromWord(1);
  setHashAndAdd(thread, set1, key);
  key = SmallInt::fromWord(2);
  setHashAndAdd(thread, set1, key);
  setHashAndAdd(thread, set2, key);
  Object result2(&scope, runBuiltin(SetBuiltins::dunderIand, set1, set2));
  ASSERT_TRUE(result2.isSet());
  EXPECT_EQ(*result2, *set1);
  Set set(&scope, *result2);
  EXPECT_EQ(set.numItems(), 1);
  EXPECT_TRUE(setIncludes(thread, set, key));
}

TEST_F(SetBuiltinsTest, DunderIandWithNonSet) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Object empty_set(&scope, runtime_.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderIand, empty_set, none));
  ASSERT_TRUE(result.isNotImplementedType());
}

TEST_F(SetBuiltinsTest, SetIntersectionWithNoArgsReturnsCopy) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  // set.intersect() with no arguments
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set));
  ASSERT_TRUE(result.isSet());
  EXPECT_NE(*result, *set);
  set = *result;
  EXPECT_EQ(set.numItems(), 3);

  Object key(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(setIncludes(thread, set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(setIncludes(thread, set, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(setIncludes(thread, set, key));
}

TEST_F(SetBuiltinsTest, SetIntersectionWithOneArgumentReturnsIntersection) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 2));

  // set.intersect() with 1 argument
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, set1));
  ASSERT_TRUE(result.isSet());
  EXPECT_NE(*result, *set);
  set = *result;
  EXPECT_EQ(set.numItems(), 2);
  Object key(&scope, SmallInt::fromWord(0));
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(setIncludes(thread, set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(setIncludes(thread, set, key));
}

TEST_F(SetBuiltinsTest, SetIntersectionWithTwoArgumentsReturnsIntersection) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 2));
  Set set2(&scope, setFromRange(0, 1));

  // set.intersect() with 2 arguments
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, set1, set2));
  ASSERT_TRUE(result.isSet());
  EXPECT_NE(*result, *set);
  set = *result;
  EXPECT_EQ(set.numItems(), 1);
  Object key(&scope, SmallInt::fromWord(0));
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(setIncludes(thread, set, key));
}

TEST_F(SetBuiltinsTest, SetIntersectionWithEmptySetReturnsEmptySet) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 2));
  Set set2(&scope, runtime_.newSet());

  // set.intersect() with 2 arguments
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, set1, set2));
  ASSERT_TRUE(result.isSet());
  EXPECT_NE(*result, *set);
  EXPECT_EQ(Set::cast(*result).numItems(), 0);
}

TEST_F(SetBuiltinsTest, SetIntersectionWithEmptyIterableReturnsEmptySet) {
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  List list(&scope, runtime_.newList());
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, list));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(Set::cast(*result).numItems(), 0);
}

TEST_F(SetBuiltinsTest, SetIntersectionWithIterableReturnsIntersection) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  List list(&scope, runtime_.newList());
  Object key(&scope, SmallInt::fromWord(4));
  runtime_.listAdd(thread, list, key);
  key = SmallInt::fromWord(0);
  runtime_.listAdd(thread, list, key);
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, list));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(Set::cast(*result).numItems(), 1);
  set = *result;
  EXPECT_TRUE(setIncludes(thread, set, key));
}

TEST_F(SetBuiltinsTest, SetIntersectionWithFrozenSetReturnsSet) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  FrozenSet frozen_set(&scope, runtime_.emptyFrozenSet());
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, frozen_set));
  ASSERT_TRUE(result.isSet());
}

TEST_F(SetBuiltinsTest, FrozenSetIntersectionWithSetReturnsFrozenSet) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  FrozenSet frozen_set(&scope, runtime_.emptyFrozenSet());
  Set set(&scope, runtime_.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::intersection, frozen_set, set));
  ASSERT_TRUE(result.isFrozenSet());
}

TEST_F(SetBuiltinsTest, SetAndWithFrozenSetReturnsSet) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  FrozenSet frozen_set(&scope, runtime_.emptyFrozenSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, set, frozen_set));
  ASSERT_TRUE(result.isSet());
}

TEST_F(SetBuiltinsTest, FrozenSetAndWithSetReturnsFrozenSet) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  FrozenSet frozen_set(&scope, runtime_.emptyFrozenSet());
  Set set(&scope, runtime_.newSet());
  Object result(&scope,
                runBuiltin(FrozenSetBuiltins::dunderAnd, frozen_set, set));
  ASSERT_TRUE(result.isFrozenSet());
}

TEST_F(SetIteratorBuiltinsTest, CallDunderNext) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object value(&scope, SmallInt::fromWord(0));
  setHashAndAdd(thread, set, value);
  value = SmallInt::fromWord(1);
  setHashAndAdd(thread, set, value);

  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, set));
  ASSERT_TRUE(iter.isSetIterator());

  Object item1(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item1, 0));

  Object item2(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item2, 1));

  Object item3(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3.isError());
}

TEST_F(SetIteratorBuiltinsTest, CallDunderNextWithEmptySet) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, set));
  ASSERT_TRUE(iter.isSetIterator());

  Object result(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(result.isError());
}

TEST_F(SetIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set empty_set(&scope, runtime_.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, empty_set));
  ASSERT_TRUE(iter.isSetIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(SetIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST_F(SetIteratorBuiltinsTest, DunderLengthHintOnEmptySetReturnsZero) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set empty_set(&scope, runtime_.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, empty_set));
  ASSERT_TRUE(iter.isSetIterator());

  Object length_hint(&scope,
                     runBuiltin(SetIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST_F(SetIteratorBuiltinsTest, DunderLengthHintOnConsumedSetReturnsZero) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set one_element_set(&scope, runtime_.newSet());
  Object zero(&scope, SmallInt::fromWord(0));
  setHashAndAdd(thread, one_element_set, zero);

  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, one_element_set));
  ASSERT_TRUE(iter.isSetIterator());

  Object length_hint1(&scope,
                      runBuiltin(SetIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint1, 1));

  // Consume the iterator
  Object item1(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item1, 0));

  Object length_hint2(&scope,
                      runBuiltin(SetIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint2, 0));
}

TEST_F(SetBuiltinsTest, IsdisjointWithNonIterableArg) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
s = {1}
s.isdisjoint(None)
)"),
                            LayoutId::kTypeError, "object is not iterable"));
}

TEST_F(SetBuiltinsTest, IsdisjointWithSetArg) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Set set(&scope, runtime_.newSet());
  Set other(&scope, runtime_.newSet());
  Object value(&scope, NoneType::object());

  // set().isdisjoint(set())
  Object result(&scope, runBuiltin(SetBuiltins::isdisjoint, set, other));
  ASSERT_TRUE(result.isBool());
  EXPECT_EQ(*result, Bool::trueObj());

  // set().isdisjoint({None})
  setHashAndAdd(thread, other, value);
  Object result1(&scope, runBuiltin(SetBuiltins::isdisjoint, set, other));
  ASSERT_TRUE(result1.isBool());
  EXPECT_EQ(*result1, Bool::trueObj());

  // {None}.isdisjoint({None})
  setHashAndAdd(thread, set, value);
  Object result2(&scope, runBuiltin(SetBuiltins::isdisjoint, set, other));
  ASSERT_TRUE(result2.isBool());
  EXPECT_EQ(*result2, Bool::falseObj());

  // {None}.isdisjoint({1})
  other = runtime_.newSet();
  value = SmallInt::fromWord(1);
  setHashAndAdd(thread, other, value);
  Object result3(&scope, runBuiltin(SetBuiltins::isdisjoint, set, other));
  ASSERT_TRUE(result3.isBool());
  EXPECT_EQ(*result3, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, IsdisjointWithIterableArg) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);

  Set set(&scope, runtime_.newSet());
  List other(&scope, runtime_.newList());
  Object value(&scope, NoneType::object());

  // set().isdisjoint([])
  Object result(&scope, runBuiltin(SetBuiltins::isdisjoint, set, other));
  ASSERT_TRUE(result.isBool());
  EXPECT_EQ(*result, Bool::trueObj());

  // set().isdisjoint([None])
  runtime_.listAdd(thread, other, value);
  Object result1(&scope, runBuiltin(SetBuiltins::isdisjoint, set, other));
  ASSERT_TRUE(result1.isBool());
  EXPECT_EQ(*result1, Bool::trueObj());

  // {None}.isdisjoint([None])
  setHashAndAdd(thread, set, value);
  Object result2(&scope, runBuiltin(SetBuiltins::isdisjoint, set, other));
  ASSERT_TRUE(result2.isBool());
  EXPECT_EQ(*result2, Bool::falseObj());

  // {None}.isdisjoint([1])
  other = runtime_.newList();
  value = SmallInt::fromWord(1);
  runtime_.listAdd(thread, other, value);
  Object result3(&scope, runBuiltin(SetBuiltins::isdisjoint, set, other));
  ASSERT_TRUE(result3.isBool());
  EXPECT_EQ(*result3, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderEqWithSetSubclass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a == b)
cmp1 = (a1 == b)
cmp2 = (b == a)
cmp3 = (b == a1)
cmp4 = (b == b)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp1"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp2"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp3"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp4"), Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderNeWithSetSubclass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a != b)
cmp1 = (a1 != b)
cmp2 = (b != a)
cmp3 = (b != a1)
cmp4 = (b != b)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp1"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp2"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp3"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp4"), Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderGeWithSetSubclass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a >= b)
cmp1 = (a1 >= b)
cmp2 = (b >= a)
cmp3 = (b >= a1)
cmp4 = (b >= b)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp1"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp2"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp3"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp4"), Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderGtWithSetSubclass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a > b)
cmp1 = (a1 > b)
cmp2 = (b > a)
cmp3 = (b > a1)
cmp4 = (b > b)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp1"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp2"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp3"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp4"), Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderLeWithSetSubclass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a <= b)
cmp1 = (a1 <= b)
cmp2 = (b <= a)
cmp3 = (b <= a1)
cmp4 = (b <= b)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp1"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp2"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp3"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp4"), Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderLtWithSetSubclass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a < b)
cmp1 = (a1 < b)
cmp2 = (b < a)
cmp3 = (b < a1)
cmp4 = (b < b)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp1"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp2"), Bool::falseObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp3"), Bool::trueObj());
  EXPECT_EQ(mainModuleAt(&runtime_, "cmp4"), Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderEqWithEmptySetsReturnsTrue) {
  // (set() == set()) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Set set1(&scope, runtime_.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderEqWithSameSetReturnsTrue) {
  // s = {0, 1, 2}; (s == s) is True
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, set));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderEqWithEqualSetsReturnsTrue) {
  // ({0, 1, 2) == {0, 1, 2}) is True
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderEqWithUnequalSetsReturnsFalse) {
  // ({0, 1, 2} == {1, 2, 3}) is False
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(1, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderNeWithEmptySetsReturnsFalse) {
  // (set() != set()) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Set set1(&scope, runtime_.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderNeWithSameSetReturnsFalse) {
  // s = {0, 1, 2}; (s != s) is False
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, set));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderNeWithEqualSetsReturnsFalse) {
  // ({0, 1, 2} != {0, 1, 2}) is False
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderNeWithUnequalSetsReturnsTrue) {
  // ({0, 1, 2} != {1, 2, 3}) is True
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(1, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderGeWithSameSetReturnsTrue) {
  // s = {0, 1, 2}; (s >= s) is True
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, set));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderGeWithEqualSetsReturnsTrue) {
  // ({0, 1, 2} >= {0, 1, 2}) is True
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderGeWithSupersetReturnsFalse) {
  // ({0, 1, 2} >= {0, 1, 2, 3}) is False
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderGeWithEmptySetReturnsTrue) {
  // ({0, 1, 2} >= set()) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, runtime_.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderLeWithEmptySetReturnsTrue) {
  // s = {0, 1, 2}; (s <= s) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Set set1(&scope, runtime_.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderLeWithEqualSetsReturnsTrue) {
  // ({0, 1, 2} <= {0, 1, 2}) is True
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderLeWithSubsetReturnsFalse) {
  // ({0, 1, 2, 3} <= {0, 1, 2}) is False
  HandleScope scope;
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderLeWithEmptySetReturnsFalse) {
  // ({0, 1, 2} <= set()) is False
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, runtime_.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderGtWithEqualSetsReturnsFalse) {
  // ({0, 1, 2} > {0, 1, 2}) is False
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGt, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderGtWithSubsetReturnsTrue) {
  // ({0, 1, 2, 3} > {0, 1, 2}) is True
  HandleScope scope;
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGt, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderGtWithSupersetReturnsFalse) {
  // ({0, 1, 2} > {0, 1, 2, 3}) is False
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGt, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderLtWithEqualSetsReturnsFalse) {
  // ({0, 1, 2} < {0, 1, 2}) is False
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderLt, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderLtWithSupersetReturnsTrue) {
  // ({0, 1, 2} < {0, 1, 2, 3})  is True
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderLt, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST_F(SetBuiltinsTest, DunderLtWithSubsetReturnsFalse) {
  // ({0, 1, 2, 3} < {0, 1, 2}) is False
  HandleScope scope;
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderLt, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST_F(SetBuiltinsTest, DunderEqWithNonSetSecondArgReturnsNotImplemented) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST_F(SetBuiltinsTest, DunderNeWithNonSetSecondArgReturnsNotImplemented) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST_F(SetBuiltinsTest, DunderGeWithNonSetSecondArgReturnsNotImplemented) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST_F(SetBuiltinsTest, DunderGtWithNonSetSecondArgReturnsNotImplemented) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderGt, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST_F(SetBuiltinsTest, DunderLeWithNonSetSecondArgReturnsNotImplemented) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST_F(SetBuiltinsTest, DunderLtWithNonSetSecondArgReturnsNotImplemented) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderLt, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST_F(SetBuiltinsTest, DunderEqWithNonSetFirstArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
set.__eq__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__eq__() requires a 'set' or 'frozenset' object"));
}

TEST_F(SetBuiltinsTest, DunderNeWithNonSetFirstArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
set.__ne__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__ne__() requires a 'set' or 'frozenset' object"));
}

TEST_F(SetBuiltinsTest, DunderGeWithNonSetFirstArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
set.__ge__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__ge__() requires a 'set' or 'frozenset' object"));
}

TEST_F(SetBuiltinsTest, DunderGtWithNonSetFirstArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
set.__gt__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__gt__() requires a 'set' or 'frozenset' object"));
}

TEST_F(SetBuiltinsTest, DunderLeWithNonSetFirstArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
set.__le__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__le__() requires a 'set' or 'frozenset' object"));
}

TEST_F(SetBuiltinsTest, DunderLtWithNonSetFirstArgRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
set.__lt__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__lt__() requires a 'set' or 'frozenset' object"));
}

TEST_F(SetBuiltinsTest, DunderInitWithNonSetFirstArgRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, R"(
set.__init__([])
)"),
                    LayoutId::kTypeError,
                    "'__init__' requires a 'set' object but got 'list'"));
}

TEST_F(SetBuiltinsTest, DunderInitWithNonIterableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
set.__init__(set(), None)
)"),
                            LayoutId::kTypeError, "object is not iterable"));
}

TEST_F(SetBuiltinsTest, DunderInitWithIteratorUpdatesSet) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderInit, set, set1));
  ASSERT_TRUE(result.isNoneType());
  EXPECT_EQ(set.numItems(), set1.numItems());
  Object key(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(setIncludes(thread, set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(setIncludes(thread, set, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(setIncludes(thread, set, key));
}

TEST_F(SetBuiltinsTest, DunderInitWithSetSubclassUpdatesSet) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Set(set): pass

s = Set([0, 1, 2])
)")
                   .isError());
  Object s(&scope, mainModuleAt(&runtime_, "s"));
  ASSERT_TRUE(runtime_.isInstanceOfSet(*s));
  Object key(&scope, SmallInt::fromWord(0));
  Set set(&scope, *s);
  EXPECT_TRUE(setIncludes(thread, set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(setIncludes(thread, set, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(setIncludes(thread, set, key));
}

TEST_F(SetBuiltinsTest, DunderLenWithSetSubclassReturnsLen) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Set(set): pass

s = Set([0, 1, 2])
)")
                   .isError());
  Object s(&scope, mainModuleAt(&runtime_, "s"));
  ASSERT_TRUE(runtime_.isInstanceOfSet(*s));

  Object result(&scope, runBuiltin(SetBuiltins::dunderLen, s));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST_F(SetBuiltinsTest, FrozenSetDunderNewReturnsSingleton) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = frozenset.__new__(frozenset)")
                   .isError());
  HandleScope scope;
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(result.isFrozenSet());
  EXPECT_EQ(*result, runtime_.emptyFrozenSet());
}

TEST_F(SetBuiltinsTest, SubclassOfFrozenSetDunderNewDoesNotReturnSingleton) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(frozenset):
    pass
o = C()
)")
                   .isError());
  HandleScope scope;
  Object o(&scope, mainModuleAt(&runtime_, "o"));
  EXPECT_NE(*o, runtime_.emptyFrozenSet());
}

TEST_F(SetBuiltinsTest, FrozenSetDunderNewFromEmptyIterableReturnsSingleton) {
  HandleScope scope;
  Type type(&scope, runtime_.typeAt(LayoutId::kFrozenSet));
  List empty_iterable(&scope, runtime_.newList());
  Object result(&scope,
                runBuiltin(FrozenSetBuiltins::dunderNew, type, empty_iterable));
  EXPECT_EQ(*result, runtime_.emptyFrozenSet());
}

TEST_F(SetBuiltinsTest, FrozenSetDunderNewFromFrozenSetIsIdempotent) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, runtime_.typeAt(LayoutId::kFrozenSet));
  List nonempty_list(&scope, listFromRange(1, 5));
  FrozenSet frozenset(&scope, runtime_.newFrozenSet());
  frozenset = setUpdate(Thread::current(), frozenset, nonempty_list);
  Object result(&scope,
                runBuiltin(FrozenSetBuiltins::dunderNew, type, frozenset));
  EXPECT_EQ(*result, *frozenset);
}

TEST_F(SetBuiltinsTest,
       FrozenSetDunderNewFromIterableContainsIterableElements) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, runtime_.typeAt(LayoutId::kFrozenSet));
  List nonempty_list(&scope, listFromRange(1, 5));
  Object result_obj(
      &scope, runBuiltin(FrozenSetBuiltins::dunderNew, type, nonempty_list));
  ASSERT_TRUE(result_obj.isFrozenSet());
  FrozenSet result(&scope, *result_obj);
  EXPECT_EQ(result.numItems(), 4);
  Int one(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(setIncludes(thread, result, one));
  Int two(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(setIncludes(thread, result, two));
  Int three(&scope, SmallInt::fromWord(3));
  EXPECT_TRUE(setIncludes(thread, result, three));
  Int four(&scope, SmallInt::fromWord(4));
  EXPECT_TRUE(setIncludes(thread, result, four));
}

TEST_F(SetBuiltinsTest, FrozenSetFromIterableIsNotSingleton) {
  HandleScope scope;
  Type type(&scope, runtime_.typeAt(LayoutId::kFrozenSet));
  List nonempty_list(&scope, listFromRange(1, 5));
  Object result1(&scope,
                 runBuiltin(FrozenSetBuiltins::dunderNew, type, nonempty_list));
  ASSERT_TRUE(result1.isFrozenSet());
  Object result2(&scope,
                 runBuiltin(FrozenSetBuiltins::dunderNew, type, nonempty_list));
  ASSERT_TRUE(result2.isFrozenSet());
  ASSERT_NE(*result1, *result2);
}

TEST_F(SetBuiltinsTest, FrozenSetDunderNewWithNonIterableRaisesTypeError) {
  HandleScope scope;
  Type type(&scope, runtime_.typeAt(LayoutId::kFrozenSet));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(FrozenSetBuiltins::dunderNew, type, none));
  ASSERT_TRUE(result.isError());
}

TEST_F(SetBuiltinsTest, SetCopy) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object set_copy(&scope, setCopy(thread, set));
  ASSERT_TRUE(set_copy.isSet());
  EXPECT_EQ(Set::cast(*set_copy).numItems(), 0);

  Object key(&scope, SmallInt::fromWord(0));
  setHashAndAdd(thread, set, key);
  key = SmallInt::fromWord(1);
  setHashAndAdd(thread, set, key);
  key = SmallInt::fromWord(2);
  setHashAndAdd(thread, set, key);

  Object set_copy1(&scope, setCopy(thread, set));
  ASSERT_TRUE(set_copy1.isSet());
  EXPECT_EQ(Set::cast(*set_copy1).numItems(), 3);
  set = *set_copy1;
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(setIncludes(thread, set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(setIncludes(thread, set, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(setIncludes(thread, set, key));
}

TEST_F(SetBuiltinsTest, SetEqualsWithSameSetReturnsTrue) {
  // s = {0, 1, 2}; (s == s) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  ASSERT_TRUE(setEquals(thread, set, set));
}

TEST_F(SetBuiltinsTest, SetIsSubsetWithEmptySetsReturnsTrue) {
  // (set() <= set()) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Set set1(&scope, runtime_.newSet());
  ASSERT_TRUE(setIsSubset(thread, set, set1));
}

TEST_F(SetBuiltinsTest, SetIsSubsetWithEmptySetAndNonEmptySetReturnsTrue) {
  // (set() <= {0, 1, 2}) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Set set1(&scope, setFromRange(0, 3));
  ASSERT_TRUE(setIsSubset(thread, set, set1));
}

TEST_F(SetBuiltinsTest, SetIsSubsetWithEqualsetReturnsTrue) {
  // ({0, 1, 2} <= {0, 1, 2}) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  ASSERT_TRUE(setIsSubset(thread, set, set1));
}

TEST_F(SetBuiltinsTest, SetIsSubsetWithSubsetReturnsTrue) {
  // ({1, 2, 3} <= {1, 2, 3, 4}) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(1, 4));
  Set set1(&scope, setFromRange(1, 5));
  ASSERT_TRUE(setIsSubset(thread, set, set1));
}

TEST_F(SetBuiltinsTest, SetIsSubsetWithSupersetReturnsFalse) {
  // ({1, 2, 3, 4} <= {1, 2, 3}) is False
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(1, 5));
  Set set1(&scope, setFromRange(1, 4));
  ASSERT_FALSE(setIsSubset(thread, set, set1));
}

TEST_F(SetBuiltinsTest, SetIsSubsetWithSameSetReturnsTrue) {
  // s = {0, 1, 2}; (s <= s) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 4));
  ASSERT_TRUE(setIsSubset(thread, set, set));
}

TEST_F(SetBuiltinsTest, SetIsProperSubsetWithSupersetReturnsTrue) {
  // ({0, 1, 2, 3} < {0, 1, 2, 3, 4}) is True
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 5));
  ASSERT_TRUE(setIsProperSubset(thread, set, set1));
}

TEST_F(SetBuiltinsTest, SetIsProperSubsetWithUnequalSetsReturnsFalse) {
  // ({1, 2, 3} < {0, 1, 2}) is False
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(1, 4));
  Set set1(&scope, setFromRange(0, 3));
  ASSERT_FALSE(setIsProperSubset(thread, set, set1));
}

TEST_F(SetBuiltinsTest, SetIsProperSubsetWithSameSetReturnsFalse) {
  // s = {0, 1, 2}; (s < s) is False
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  ASSERT_FALSE(setIsProperSubset(thread, set, set));
}

TEST_F(SetBuiltinsTest, SetIsProperSubsetWithSubsetReturnsFalse) {
  // ({1, 2, 3, 4} < {1, 2, 3}) is False
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(1, 5));
  Set set1(&scope, setFromRange(1, 4));
  ASSERT_FALSE(setIsProperSubset(thread, set, set1));
}

TEST_F(SetBuiltinsTest, RecursiveSetPrintsEllipsis) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self, obj):
    self.val = obj
  def __repr__(self):
    return self.val.__repr__()
  def __hash__(self):
    return 5

s = set()
c = C(s)
s.add(c)
result = s.__repr__()
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "{set(...)}"));
}

TEST_F(SetBuiltinsTest, CopyWithNonSetRaisesTypeError) {
  HandleScope scope;
  Object not_a_set(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SetBuiltins::copy, not_a_set), LayoutId::kTypeError,
      "'<anonymous>' requires a 'set' object but got 'NoneType'"));
}

TEST_F(SetBuiltinsTest, CopyReturnsNewObject) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::copy, set));
  EXPECT_NE(*set, *result);
  EXPECT_TRUE(result.isSet());
}

TEST_F(SetBuiltinsTest, CopyFrozenSetRaisesTypeError) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  FrozenSet set(&scope, runtime_.newFrozenSet());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SetBuiltins::copy, set), LayoutId::kTypeError,
      "'<anonymous>' requires a 'set' object but got 'frozenset'"));
}

TEST_F(SetBuiltinsTest, CopyReturnsShallowCopy) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Object obj(&scope, runtime_.newTuple(5));
  setHashAndAdd(thread, set, obj);
  Set set2(&scope, runBuiltin(SetBuiltins::copy, set));
  Tuple data(&scope, set2.data());
  bool has_object = false;
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    if (SetBase::Bucket::value(*data, i) == *obj) {
      has_object = true;
      break;
    }
  }
  EXPECT_TRUE(has_object);
}

TEST_F(FrozenSetBuiltinsTest, CopyWithNonFrozenSetRaisesTypeError) {
  HandleScope scope;
  Object not_a_set(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FrozenSetBuiltins::copy, not_a_set), LayoutId::kTypeError,
      "'<anonymous>' requires a 'frozenset' object but got 'NoneType'"));
}

TEST_F(FrozenSetBuiltinsTest, CopySetRaisesTypeError) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FrozenSetBuiltins::copy, set), LayoutId::kTypeError,
      "'<anonymous>' requires a 'frozenset' object but got 'set'"));
}

TEST_F(FrozenSetBuiltinsTest, CopyFrozenSetReturnsSameObject) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  FrozenSet set(&scope, runtime_.newFrozenSet());
  Object result(&scope, runBuiltin(FrozenSetBuiltins::copy, set));
  EXPECT_EQ(*set, *result);
  EXPECT_TRUE(result.isFrozenSet());
}

TEST_F(FrozenSetBuiltinsTest, CopyFrozenSetSubsetReturnsNewObject) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C(frozenset):
  pass
sub = C()
result = frozenset.copy(sub)
)")
                   .isError());
  Object sub(&scope, mainModuleAt(&runtime_, "sub"));
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(runtime_.isInstanceOfFrozenSet(*sub));
  EXPECT_TRUE(runtime_.isInstanceOfFrozenSet(*result));
  EXPECT_FALSE(sub.isFrozenSet());
  EXPECT_TRUE(result.isFrozenSet());
  EXPECT_NE(sub, result);
}

TEST_F(FrozenSetBuiltinsTest, CopyMakesShallowCopy) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  FrozenSet set(&scope, runtime_.newFrozenSet());
  Object obj(&scope, runtime_.newTuple(5));
  setHashAndAdd(thread, set, obj);
  FrozenSet set2(&scope, runBuiltin(FrozenSetBuiltins::copy, set));
  Tuple data(&scope, set2.data());
  bool has_object = false;
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    if (SetBase::Bucket::value(*data, i) == *obj) {
      has_object = true;
      break;
    }
  }
  EXPECT_TRUE(has_object);
}

TEST_F(SetBuiltinsTest, UpdateWithNoArgsDoesNothing) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime_.newSet());
  Tuple starargs(&scope, runtime_.emptyTuple());
  Object result(&scope, runBuiltin(SetBuiltins::update, set, starargs));
  EXPECT_TRUE(result.isNoneType());
  EXPECT_EQ(set.numItems(), 0);
}

TEST_F(SetBuiltinsTest, UpdateWithNonSetRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, R"(
set.update(None)
)"),
                    LayoutId::kTypeError,
                    "'update' requires a 'set' object but got 'NoneType'"));
}

TEST_F(SetBuiltinsTest, UpdateWithNonIterableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
result = set()
result.update({5}, {6}, None)
)"),
                            LayoutId::kTypeError, "object is not iterable"));
  HandleScope scope;
  Set result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_EQ(result.numItems(), 2);
}

TEST_F(SetBuiltinsTest, UpdateWithSetAddsElements) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = set()
result.update({5})
)")
                   .isError());
  HandleScope scope;
  Set result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_EQ(result.numItems(), 1);
}

TEST_F(SetBuiltinsTest, UpdateWithMultipleSetsAddsAllElements) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = set()
result.update({5}, {6})
)")
                   .isError());
  HandleScope scope;
  Set result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_EQ(result.numItems(), 2);
}

TEST_F(SetBuiltinsTest, UpdateWithIterableAddsElements) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = set([1, 2])
result.update([5, 6])
)")
                   .isError());
  HandleScope scope;
  Set result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_EQ(result.numItems(), 4);
}

TEST_F(SetBuiltinsTest, DunderOrWithNonSetBaseOtherReturnsNotImplemented) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = set.__or__(set(), None)").isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "result"), NotImplementedType::object());
}

TEST_F(SetBuiltinsTest, DunderOrReturnsSetContainingUnionOfElements) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = set.__or__({1, 2}, {2, 3})").isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(result_obj.isSet());
  Set result(&scope, *result_obj);
  EXPECT_EQ(result.numItems(), 3);
  Object one(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(setIncludes(thread, result, one));
  Object two(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(setIncludes(thread, result, two));
  Object three(&scope, SmallInt::fromWord(3));
  EXPECT_TRUE(setIncludes(thread, result, three));
}

}  // namespace py
