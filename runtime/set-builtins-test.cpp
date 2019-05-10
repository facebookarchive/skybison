#include "gtest/gtest.h"

#include "runtime.h"
#include "set-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(SetBuiltinsTest, SetPopException) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
s = {1}
s.pop()
s.pop()
)"),
                            LayoutId::kKeyError, "pop from an empty set"));
}

TEST(SetBuiltinsTest, SetPop) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = {1}
a = s.pop()
b = len(s)
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1));
  EXPECT_TRUE(isIntEqualsWord(*b, 0));
}

TEST(SetBuiltinsTest, InitializeByTypeCall) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = set()
)")
                   .isError());
  Object s(&scope, moduleAt(&runtime, "__main__", "s"));
  EXPECT_TRUE(s.isSet());
  EXPECT_EQ(Set::cast(*s).numItems(), 0);
}

TEST(SetBuiltinsTest, SetAdd) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
s = set()
s.add(1)
s.add("Hello, World")
)")
                   .isError());
  Set s(&scope, moduleAt(&runtime, "__main__", "s"));
  Object one(&scope, runtime.newInt(1));
  Object hello_world(&scope, runtime.newStrFromCStr("Hello, World"));
  EXPECT_EQ(s.numItems(), 2);
  EXPECT_TRUE(runtime.setIncludes(s, one));
  EXPECT_TRUE(runtime.setIncludes(s, hello_world));
}

TEST(SetBuiltinsTest, DunderIterReturnsSetIterator) {
  Runtime runtime;
  HandleScope scope;
  Set empty_set(&scope, runtime.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, empty_set));
  ASSERT_TRUE(iter.isSetIterator());
}

TEST(SetBuiltinsTest, DunderAnd) {
  Runtime runtime;
  HandleScope scope;

  Set set1(&scope, runtime.newSet());
  Set set2(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, set1, set2));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(Set::cast(*result).numItems(), 0);

  Object key(&scope, SmallInt::fromWord(1));
  runtime.setAdd(set1, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set1, key);
  Object result1(&scope, runBuiltin(SetBuiltins::dunderAnd, set1, set2));
  ASSERT_TRUE(result1.isSet());
  EXPECT_EQ(Set::cast(*result1).numItems(), 0);

  key = SmallInt::fromWord(1);
  runtime.setAdd(set2, key);
  Object result2(&scope, runBuiltin(SetBuiltins::dunderAnd, set1, set2));
  ASSERT_TRUE(result2.isSet());
  Set set(&scope, *result2);
  EXPECT_EQ(set.numItems(), 1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, DunderAndWithNonSet) {
  Runtime runtime;
  HandleScope scope;

  Object empty_set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, empty_set, none));
  ASSERT_TRUE(result.isNotImplementedType());
}

TEST(SetBuiltinsTest, DunderIand) {
  Runtime runtime;
  HandleScope scope;

  Set set1(&scope, runtime.newSet());
  Set set2(&scope, runtime.newSet());
  Object key(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderIand, set1, set2));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(*result, *set1);
  EXPECT_EQ(Set::cast(*result).numItems(), 0);

  key = SmallInt::fromWord(1);
  runtime.setAdd(set1, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set1, key);
  Object result1(&scope, runBuiltin(SetBuiltins::dunderIand, set1, set2));
  ASSERT_TRUE(result1.isSet());
  EXPECT_EQ(*result1, *set1);
  EXPECT_EQ(Set::cast(*result1).numItems(), 0);

  set1 = runtime.newSet();
  key = SmallInt::fromWord(1);
  runtime.setAdd(set1, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set1, key);
  runtime.setAdd(set2, key);
  Object result2(&scope, runBuiltin(SetBuiltins::dunderIand, set1, set2));
  ASSERT_TRUE(result2.isSet());
  EXPECT_EQ(*result2, *set1);
  Set set(&scope, *result2);
  EXPECT_EQ(set.numItems(), 1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, DunderIandWithNonSet) {
  Runtime runtime;
  HandleScope scope;

  Object empty_set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderIand, empty_set, none));
  ASSERT_TRUE(result.isNotImplementedType());
}

TEST(SetBuiltinsTest, SetIntersectionWithNoArgsReturnsCopy) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  // set.intersect() with no arguments
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set));
  ASSERT_TRUE(result.isSet());
  EXPECT_NE(*result, *set);
  set = *result;
  EXPECT_EQ(set.numItems(), 3);

  Object key(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, SetIntersectionWithOneArgumentReturnsIntersection) {
  Runtime runtime;
  HandleScope scope;
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
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, SetIntersectionWithTwoArgumentsReturnsIntersection) {
  Runtime runtime;
  HandleScope scope;
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
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, SetIntersectionWithEmptySetReturnsEmptySet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 2));
  Set set2(&scope, runtime.newSet());

  // set.intersect() with 2 arguments
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, set1, set2));
  ASSERT_TRUE(result.isSet());
  EXPECT_NE(*result, *set);
  EXPECT_EQ(Set::cast(*result).numItems(), 0);
}

TEST(SetBuiltinsTest, SetIntersectionWithEmptyIterableReturnsEmptySet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  List list(&scope, runtime.newList());
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, list));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(Set::cast(*result).numItems(), 0);
}

TEST(SetBuiltinsTest, SetIntersectionWithIterableReturnsIntersection) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  List list(&scope, runtime.newList());
  Object key(&scope, SmallInt::fromWord(4));
  runtime.listAdd(list, key);
  key = SmallInt::fromWord(0);
  runtime.listAdd(list, key);
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, list));
  ASSERT_TRUE(result.isSet());
  EXPECT_EQ(Set::cast(*result).numItems(), 1);
  set = *result;
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, SetIntersectionWithFrozenSetReturnsSet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  FrozenSet frozen_set(&scope, runtime.emptyFrozenSet());
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, frozen_set));
  ASSERT_TRUE(result.isSet());
}

TEST(SetBuiltinsTest, FrozenSetIntersectionWithSetReturnsFrozenSet) {
  Runtime runtime;
  HandleScope scope;
  FrozenSet frozen_set(&scope, runtime.emptyFrozenSet());
  Set set(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::intersection, frozen_set, set));
  ASSERT_TRUE(result.isFrozenSet());
}

TEST(SetBuiltinsTest, SetAndWithFrozenSetReturnsSet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  FrozenSet frozen_set(&scope, runtime.emptyFrozenSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, set, frozen_set));
  ASSERT_TRUE(result.isSet());
}

TEST(SetBuiltinsTest, FrozenSetAndWithSetReturnsFrozenSet) {
  Runtime runtime;
  HandleScope scope;
  FrozenSet frozen_set(&scope, runtime.emptyFrozenSet());
  Set set(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, frozen_set, set));
  ASSERT_TRUE(result.isFrozenSet());
}

TEST(SetIteratorBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object value(&scope, SmallInt::fromWord(0));
  runtime.setAdd(set, value);
  value = SmallInt::fromWord(1);
  runtime.setAdd(set, value);

  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, set));
  ASSERT_TRUE(iter.isSetIterator());

  Object item1(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item1, 0));

  Object item2(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item2, 1));

  Object item3(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3.isError());
}

TEST(SetIteratorBuiltinsTest, CallDunderNextWithEmptySet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, set));
  ASSERT_TRUE(iter.isSetIterator());

  Object result(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(result.isError());
}

TEST(SetIteratorBuiltinsTes, DunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  Set empty_set(&scope, runtime.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, empty_set));
  ASSERT_TRUE(iter.isSetIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(SetIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(SetIteratorBuiltinsTest, DunderLengthHintOnEmptySetReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Set empty_set(&scope, runtime.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, empty_set));
  ASSERT_TRUE(iter.isSetIterator());

  Object length_hint(&scope,
                     runBuiltin(SetIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST(SetIteratorBuiltinsTest, DunderLengthHintOnConsumedSetReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Set one_element_set(&scope, runtime.newSet());
  Object zero(&scope, SmallInt::fromWord(0));
  runtime.setAdd(one_element_set, zero);

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

TEST(SetBuiltinsTest, SetIsDisjointWithNonIterableArg) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
s = {1}
s.isdisjoint(None)
)"),
                            LayoutId::kTypeError, "object is not iterable"));
}

TEST(SetBuiltinsTest, SetIsDisjointWithSetArg) {
  Runtime runtime;
  HandleScope scope;

  Set set(&scope, runtime.newSet());
  Set other(&scope, runtime.newSet());
  Object value(&scope, NoneType::object());

  // set().isdisjoint(set())
  Object result(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result.isBool());
  EXPECT_EQ(*result, Bool::trueObj());

  // set().isdisjoint({None})
  runtime.setAdd(other, value);
  Object result1(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result1.isBool());
  EXPECT_EQ(*result1, Bool::trueObj());

  // {None}.isdisjoint({None})
  runtime.setAdd(set, value);
  Object result2(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result2.isBool());
  EXPECT_EQ(*result2, Bool::falseObj());

  // {None}.isdisjoint({1})
  other = runtime.newSet();
  value = SmallInt::fromWord(1);
  runtime.setAdd(other, value);
  Object result3(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result3.isBool());
  EXPECT_EQ(*result3, Bool::trueObj());
}

TEST(SetBuiltinsTest, SetIsDisjointWithIterableArg) {
  Runtime runtime;
  HandleScope scope;

  Set set(&scope, runtime.newSet());
  List other(&scope, runtime.newList());
  Object value(&scope, NoneType::object());

  // set().isdisjoint([])
  Object result(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result.isBool());
  EXPECT_EQ(*result, Bool::trueObj());

  // set().isdisjoint([None])
  runtime.listAdd(other, value);
  Object result1(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result1.isBool());
  EXPECT_EQ(*result1, Bool::trueObj());

  // {None}.isdisjoint([None])
  runtime.setAdd(set, value);
  Object result2(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result2.isBool());
  EXPECT_EQ(*result2, Bool::falseObj());

  // {None}.isdisjoint([1])
  other = runtime.newList();
  value = SmallInt::fromWord(1);
  runtime.listAdd(other, value);
  Object result3(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result3.isBool());
  EXPECT_EQ(*result3, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderEqWithSetSubclass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderNeWithSetSubclass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderGeWithSetSubclass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGtWithSetSubclass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderLeWithSetSubclass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLtWithSetSubclass) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderEqWithEmptySetsReturnsTrue) {
  // (set() == set()) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderEqWithSameSetReturnsTrue) {
  // s = {0, 1, 2}; (s == s) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, set));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderEqWithEqualSetsReturnsTrue) {
  // ({0, 1, 2) == {0, 1, 2}) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderEqWithUnequalSetsReturnsFalse) {
  // ({0, 1, 2} == {1, 2, 3}) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(1, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderNeWithEmptySetsReturnsFalse) {
  // (set() != set()) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderNeWithSameSetReturnsFalse) {
  // s = {0, 1, 2}; (s != s) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, set));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderNeWithEqualSetsReturnsFalse) {
  // ({0, 1, 2} != {0, 1, 2}) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderNeWithUnequalSetsReturnsTrue) {
  // ({0, 1, 2} != {1, 2, 3}) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(1, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGeWithSameSetReturnsTrue) {
  // s = {0, 1, 2}; (s >= s) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, set));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGeWithEqualSetsReturnsTrue) {
  // ({0, 1, 2} >= {0, 1, 2}) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGeWithSupersetReturnsFalse) {
  // ({0, 1, 2} >= {0, 1, 2, 3}) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderGeWithEmptySetReturnsTrue) {
  // ({0, 1, 2} >= set()) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLeWithEmptySetReturnsTrue) {
  // s = {0, 1, 2}; (s <= s) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLeWithEqualSetsReturnsTrue) {
  // ({0, 1, 2} <= {0, 1, 2}) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLeWithSubsetReturnsFalse) {
  // ({0, 1, 2, 3} <= {0, 1, 2}) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderLeWithEmptySetReturnsFalse) {
  // ({0, 1, 2} <= set()) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderGtWithEqualSetsReturnsFalse) {
  // ({0, 1, 2} > {0, 1, 2}) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGt, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderGtWithSubsetReturnsTrue) {
  // ({0, 1, 2, 3} > {0, 1, 2}) is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGt, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGtWithSupersetReturnsFalse) {
  // ({0, 1, 2} > {0, 1, 2, 3}) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderGt, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderLtWithEqualSetsReturnsFalse) {
  // ({0, 1, 2} < {0, 1, 2}) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderLt, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderLtWithSupersetReturnsTrue) {
  // ({0, 1, 2} < {0, 1, 2, 3})  is True
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderLt, set, set1));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLtWithSubsetReturnsFalse) {
  // ({0, 1, 2, 3} < {0, 1, 2}) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderLt, set, set1));
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderEqWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST(SetBuiltinsTest, DunderNeWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST(SetBuiltinsTest, DunderGeWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST(SetBuiltinsTest, DunderGtWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderGt, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST(SetBuiltinsTest, DunderLeWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST(SetBuiltinsTest, DunderLtWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderLt, set, none));
  ASSERT_EQ(*result, NotImplementedType::object());
}

TEST(SetBuiltinsTest, DunderEqWithNonSetFirstArgRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
set.__eq__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__eq__() requires a 'set' or 'frozenset' object"));
}

TEST(SetBuiltinsTest, DunderNeWithNonSetFirstArgRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
set.__ne__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__ne__() requires a 'set' or 'frozenset' object"));
}

TEST(SetBuiltinsTest, DunderGeWithNonSetFirstArgRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
set.__ge__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__ge__() requires a 'set' or 'frozenset' object"));
}

TEST(SetBuiltinsTest, DunderGtWithNonSetFirstArgRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
set.__gt__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__gt__() requires a 'set' or 'frozenset' object"));
}

TEST(SetBuiltinsTest, DunderLeWithNonSetFirstArgRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
set.__le__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__le__() requires a 'set' or 'frozenset' object"));
}

TEST(SetBuiltinsTest, DunderLtWithNonSetFirstArgRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
set.__lt__(None, set())
)"),
                            LayoutId::kTypeError,
                            "__lt__() requires a 'set' or 'frozenset' object"));
}

TEST(SetBuiltinsTest, DunderInitWithNonSetFirstArgRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
set.__init__([])
)"),
                    LayoutId::kTypeError,
                    "'__init__' requires a 'set' object but got 'list'"));
}

TEST(SetBuiltinsTest, DunderInitWithNonIterableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
set.__init__(set(), None)
)"),
                            LayoutId::kTypeError, "object is not iterable"));
}

TEST(SetBuiltinsTest, DunderInitWithIteratorUpdatesSet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderInit, set, set1));
  ASSERT_TRUE(result.isNoneType());
  EXPECT_EQ(set.numItems(), set1.numItems());
  Object key(&scope, SmallInt::fromWord(0));
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, DunderInitWithSetSubclassUpdatesSet) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Set(set): pass

s = Set([0, 1, 2])
)")
                   .isError());
  Object s(&scope, moduleAt(&runtime, "__main__", "s"));
  ASSERT_TRUE(runtime.isInstanceOfSet(*s));
  Object key(&scope, SmallInt::fromWord(0));
  Set set(&scope, *s);
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, DunderLenWithSetSubclassReturnsLen) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Set(set): pass

s = Set([0, 1, 2])
)")
                   .isError());
  Object s(&scope, moduleAt(&runtime, "__main__", "s"));
  ASSERT_TRUE(runtime.isInstanceOfSet(*s));

  Object result(&scope, runBuiltin(SetBuiltins::dunderLen, s));
  EXPECT_TRUE(isIntEqualsWord(*result, 3));
}

TEST(SetBuiltinsTest, FrozenSetDunderNewReturnsSingleton) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = frozenset.__new__(frozenset)").isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isFrozenSet());
  EXPECT_EQ(*result, runtime.emptyFrozenSet());
}

TEST(SetBuiltinsTest, SubclassOfFrozenSetDunderNewDoesNotReturnSingleton) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C(frozenset):
    pass
o = C()
)")
                   .isError());
  HandleScope scope;
  Object o(&scope, moduleAt(&runtime, "__main__", "o"));
  EXPECT_NE(*o, runtime.emptyFrozenSet());
}

TEST(SetBuiltinsTest, FrozenSetDunderNewFromEmptyIterableReturnsSingleton) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kFrozenSet));
  List empty_iterable(&scope, runtime.newList());
  Object result(&scope,
                runBuiltin(FrozenSetBuiltins::dunderNew, type, empty_iterable));
  EXPECT_EQ(*result, runtime.emptyFrozenSet());
}

TEST(SetBuiltinsTest, FrozenSetDunderNewFromFrozenSetIsIdempotent) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kFrozenSet));
  List nonempty_list(&scope, listFromRange(1, 5));
  FrozenSet frozenset(&scope, runtime.newFrozenSet());
  frozenset = runtime.setUpdate(Thread::current(), frozenset, nonempty_list);
  Object result(&scope,
                runBuiltin(FrozenSetBuiltins::dunderNew, type, frozenset));
  EXPECT_EQ(*result, *frozenset);
}

TEST(SetBuiltinsTest, FrozenSetDunderNewFromIterableContainsIterableElements) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kFrozenSet));
  List nonempty_list(&scope, listFromRange(1, 5));
  Object result_obj(
      &scope, runBuiltin(FrozenSetBuiltins::dunderNew, type, nonempty_list));
  ASSERT_TRUE(result_obj.isFrozenSet());
  FrozenSet result(&scope, *result_obj);
  EXPECT_EQ(result.numItems(), 4);
  Int one(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(runtime.setIncludes(result, one));
  Int two(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(runtime.setIncludes(result, two));
  Int three(&scope, SmallInt::fromWord(3));
  EXPECT_TRUE(runtime.setIncludes(result, three));
  Int four(&scope, SmallInt::fromWord(4));
  EXPECT_TRUE(runtime.setIncludes(result, four));
}

TEST(SetBuiltinsTest, FrozenSetFromIterableIsNotSingleton) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kFrozenSet));
  List nonempty_list(&scope, listFromRange(1, 5));
  Object result1(&scope,
                 runBuiltin(FrozenSetBuiltins::dunderNew, type, nonempty_list));
  ASSERT_TRUE(result1.isFrozenSet());
  Object result2(&scope,
                 runBuiltin(FrozenSetBuiltins::dunderNew, type, nonempty_list));
  ASSERT_TRUE(result2.isFrozenSet());
  ASSERT_NE(*result1, *result2);
}

TEST(SetBuiltinsTest, FrozenSetDunderNewWithNonIterableRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kFrozenSet));
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(FrozenSetBuiltins::dunderNew, type, none));
  ASSERT_TRUE(result.isError());
}

TEST(SetBuiltinsTest, SetCopy) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Object set_copy(&scope, setCopy(thread, set));
  ASSERT_TRUE(set_copy.isSet());
  EXPECT_EQ(Set::cast(*set_copy).numItems(), 0);

  Object key(&scope, SmallInt::fromWord(0));
  runtime.setAdd(set, key);
  key = SmallInt::fromWord(1);
  runtime.setAdd(set, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set, key);

  Object set_copy1(&scope, setCopy(thread, set));
  ASSERT_TRUE(set_copy1.isSet());
  EXPECT_EQ(Set::cast(*set_copy1).numItems(), 3);
  set = *set_copy1;
  key = SmallInt::fromWord(0);
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, SetEqualsWithSameSetReturnsTrue) {
  // s = {0, 1, 2}; (s == s) is True
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  ASSERT_TRUE(setEquals(thread, set, set));
}

TEST(SetBuiltinsTest, SetIsSubsetWithEmptySetsReturnsTrue) {
  // (set() <= set()) is True
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  ASSERT_TRUE(setIsSubset(thread, set, set1));
}

TEST(SetBuiltinsTest, SetIsSubsetWithEmptySetAndNonEmptySetReturnsTrue) {
  // (set() <= {0, 1, 2}) is True
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, setFromRange(0, 3));
  ASSERT_TRUE(setIsSubset(thread, set, set1));
}

TEST(SetBuiltinsTest, SetIsSubsetWithEqualsetReturnsTrue) {
  // ({0, 1, 2} <= {0, 1, 2}) is True
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  ASSERT_TRUE(setIsSubset(thread, set, set1));
}

TEST(SetBuiltinsTest, SetIsSubsetWithSubsetReturnsTrue) {
  // ({1, 2, 3} <= {1, 2, 3, 4}) is True
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(1, 4));
  Set set1(&scope, setFromRange(1, 5));
  ASSERT_TRUE(setIsSubset(thread, set, set1));
}

TEST(SetBuiltinsTest, SetIsSubsetWithSupersetReturnsFalse) {
  // ({1, 2, 3, 4} <= {1, 2, 3}) is False
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(1, 5));
  Set set1(&scope, setFromRange(1, 4));
  ASSERT_FALSE(setIsSubset(thread, set, set1));
}

TEST(SetBuiltinsTest, SetIsSubsetWithSameSetReturnsTrue) {
  // s = {0, 1, 2}; (s <= s) is True
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 4));
  ASSERT_TRUE(setIsSubset(thread, set, set));
}

TEST(SetBuiltinsTest, SetIsProperSubsetWithSupersetReturnsTrue) {
  // ({0, 1, 2, 3} < {0, 1, 2, 3, 4}) is True
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 5));
  ASSERT_TRUE(setIsProperSubset(thread, set, set1));
}

TEST(SetBuiltinsTest, SetIsProperSubsetWithUnequalSetsReturnsFalse) {
  // ({1, 2, 3} < {0, 1, 2}) is False
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(1, 4));
  Set set1(&scope, setFromRange(0, 3));
  ASSERT_FALSE(setIsProperSubset(thread, set, set1));
}

TEST(SetBuiltinsTest, SetIsProperSubsetWithSameSetReturnsFalse) {
  // s = {0, 1, 2}; (s < s) is False
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  ASSERT_FALSE(setIsProperSubset(thread, set, set));
}

TEST(SetBuiltinsTest, SetIsProperSubsetWithSubsetReturnsFalse) {
  // ({1, 2, 3, 4} < {1, 2, 3}) is False
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(1, 5));
  Set set1(&scope, setFromRange(1, 4));
  ASSERT_FALSE(setIsProperSubset(thread, set, set1));
}

TEST(SetBuiltinsTest, ReprReturnsElements) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = set([3, 2, 1]).__repr__()").isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isStr());
  unique_c_ptr<char> result_str(Str::cast(*result).toCStr());
  word elts[3];
  ASSERT_EQ(
      sscanf(result_str.get(), "{%ld, %ld, %ld}", &elts[0], &elts[1], &elts[2]),
      3);
  std::sort(std::begin(elts), std::end(elts));
  EXPECT_EQ(elts[0], 1);
  EXPECT_EQ(elts[1], 2);
  EXPECT_EQ(elts[2], 3);
}

TEST(SetBuiltinsTest, RecursiveSetPrintsEllipsis) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
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
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "{set(...)}"));
}

TEST(SetBuiltinsTest, CopyWithNonSetRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object not_a_set(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SetBuiltins::copy, not_a_set), LayoutId::kTypeError,
      "'<anonymous>' requires a 'set' object but got 'NoneType'"));
}

TEST(SetBuiltinsTest, CopyReturnsNewObject) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::copy, set));
  EXPECT_NE(*set, *result);
  EXPECT_TRUE(result.isSet());
}

TEST(SetBuiltinsTest, CopyFrozenSetRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  FrozenSet set(&scope, runtime.newFrozenSet());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SetBuiltins::copy, set), LayoutId::kTypeError,
      "'<anonymous>' requires a 'set' object but got 'frozenset'"));
}

TEST(SetBuiltinsTest, CopyReturnsShallowCopy) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object obj(&scope, runtime.newTuple(5));
  EXPECT_FALSE(runtime.setAdd(set, obj).isError());
  Set set2(&scope, runBuiltin(SetBuiltins::copy, set));
  Tuple data(&scope, set2.data());
  bool has_object = false;
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    if (SetBase::Bucket::key(*data, i) == *obj) {
      has_object = true;
      break;
    }
  }
  EXPECT_TRUE(has_object);
}

TEST(FrozenSetBuiltinsTest, CopyWithNonFrozenSetRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object not_a_set(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FrozenSetBuiltins::copy, not_a_set), LayoutId::kTypeError,
      "'<anonymous>' requires a 'frozenset' object but got 'NoneType'"));
}

TEST(FrozenSetBuiltinsTest, CopySetRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(FrozenSetBuiltins::copy, set), LayoutId::kTypeError,
      "'<anonymous>' requires a 'frozenset' object but got 'set'"));
}

TEST(FrozenSetBuiltinsTest, CopyFrozenSetReturnsSameObject) {
  Runtime runtime;
  HandleScope scope;
  FrozenSet set(&scope, runtime.newFrozenSet());
  Object result(&scope, runBuiltin(FrozenSetBuiltins::copy, set));
  EXPECT_EQ(*set, *result);
  EXPECT_TRUE(result.isFrozenSet());
}

TEST(FrozenSetBuiltinsTest, CopyFrozenSetSubsetReturnsNewObject) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C(frozenset):
  pass
sub = C()
result = frozenset.copy(sub)
)")
                   .isError());
  Object sub(&scope, moduleAt(&runtime, "__main__", "sub"));
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(runtime.isInstanceOfFrozenSet(*sub));
  EXPECT_TRUE(runtime.isInstanceOfFrozenSet(*result));
  EXPECT_FALSE(sub.isFrozenSet());
  EXPECT_TRUE(result.isFrozenSet());
  EXPECT_NE(sub, result);
}

TEST(FrozenSetBuiltinsTest, CopyMakesShallowCopy) {
  Runtime runtime;
  HandleScope scope;
  FrozenSet set(&scope, runtime.newFrozenSet());
  Object obj(&scope, runtime.newTuple(5));
  EXPECT_FALSE(runtime.setAdd(set, obj).isError());
  FrozenSet set2(&scope, runBuiltin(FrozenSetBuiltins::copy, set));
  Tuple data(&scope, set2.data());
  bool has_object = false;
  for (word i = SetBase::Bucket::kFirst;
       SetBase::Bucket::nextItem(*data, &i);) {
    if (SetBase::Bucket::key(*data, i) == *obj) {
      has_object = true;
      break;
    }
  }
  EXPECT_TRUE(has_object);
}

TEST(SetBuiltinsTest, UpdateWithNoArgsDoesNothing) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Tuple starargs(&scope, runtime.newTuple(0));
  Object result(&scope, runBuiltin(SetBuiltins::update, set, starargs));
  EXPECT_TRUE(result.isNoneType());
  EXPECT_EQ(set.numItems(), 0);
}

TEST(SetBuiltinsTest, UpdateWithNonSetRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime, R"(
set.update(None)
)"),
                    LayoutId::kTypeError,
                    "'update' requires a 'set' object but got 'NoneType'"));
}

TEST(SetBuiltinsTest, UpdateWithNonIterableRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
result = set()
result.update({5}, {6}, None)
)"),
                            LayoutId::kTypeError, "object is not iterable"));
  HandleScope scope;
  Set result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.numItems(), 2);
}

TEST(SetBuiltinsTest, UpdateWithSetAddsElements) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = set()
result.update({5})
)")
                   .isError());
  HandleScope scope;
  Set result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.numItems(), 1);
}

TEST(SetBuiltinsTest, UpdateWithMultipleSetsAddsAllElements) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = set()
result.update({5}, {6})
)")
                   .isError());
  HandleScope scope;
  Set result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.numItems(), 2);
}

TEST(SetBuiltinsTest, UpdateWithIterableAddsElements) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = set([1, 2])
result.update([5, 6])
)")
                   .isError());
  HandleScope scope;
  Set result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(result.numItems(), 4);
}

TEST(SetBuiltinsTest, DunderOrWithNonSetBaseSelfReturnsNotImplemented) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = set.__or__(None, set())").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"),
            NotImplementedType::object());
}

TEST(SetBuiltinsTest, DunderOrWithNonSetBaseOtherReturnsNotImplemented) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = set.__or__(set(), None)").isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"),
            NotImplementedType::object());
}

TEST(SetBuiltinsTest, DunderOrReturnsSetContainingUnionOfElements) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = set.__or__({1, 2}, {2, 3})").isError());
  HandleScope scope;
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result_obj.isSet());
  Set result(&scope, *result_obj);
  EXPECT_EQ(result.numItems(), 3);
  Object one(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(runtime.setIncludes(result, one));
  Object two(&scope, SmallInt::fromWord(2));
  EXPECT_TRUE(runtime.setIncludes(result, two));
  Object three(&scope, SmallInt::fromWord(3));
  EXPECT_TRUE(runtime.setIncludes(result, three));
}

}  // namespace python
