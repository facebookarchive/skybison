#include "gtest/gtest.h"

#include "runtime.h"
#include "set-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(SetDeathTest, SetPopException) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
s = {1}
s.pop()
s.pop()
)"),
               "aborting due to pending exception: "
               "pop from an empty set");
}

TEST(SetBuiltinsTest, SetPop) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
s = {1}
a = s.pop()
b = len(s)
)");
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 0);
}

TEST(SetBuiltinsTest, InitializeByTypeCall) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
s = set()
)");
  Object s(&scope, moduleAt(&runtime, "__main__", "s"));
  EXPECT_TRUE(s->isSet());
  EXPECT_EQ(RawSet::cast(*s)->numItems(), 0);
}

TEST(SetBuiltinTest, SetAdd) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
s = set()
s.add(1)
s.add("Hello, World")
)");
  Set s(&scope, moduleAt(&runtime, "__main__", "s"));
  Object one(&scope, runtime.newInt(1));
  Object hello_world(&scope, runtime.newStrFromCStr("Hello, World"));
  EXPECT_EQ(s->numItems(), 2);
  EXPECT_TRUE(runtime.setIncludes(s, one));
  EXPECT_TRUE(runtime.setIncludes(s, hello_world));
}

TEST(SetDeathTest, SetAddException) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
s = set()
s.add(1, 2)
)"),
               "aborting due to pending exception: "
               "add\\(\\) takes exactly one argument");
}

TEST(SetBuiltinsTest, DunderIterReturnsSetIterator) {
  Runtime runtime;
  HandleScope scope;
  Set empty_set(&scope, runtime.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, empty_set));
  ASSERT_TRUE(iter->isSetIterator());
}

TEST(SetBuiltinsTest, DunderAnd) {
  Runtime runtime;
  HandleScope scope;

  Set set1(&scope, runtime.newSet());
  Set set2(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, set1, set2));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 0);

  Object key(&scope, SmallInt::fromWord(1));
  runtime.setAdd(set1, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set1, key);
  Object result1(&scope, runBuiltin(SetBuiltins::dunderAnd, set1, set2));
  ASSERT_TRUE(result1->isSet());
  EXPECT_EQ(RawSet::cast(*result1)->numItems(), 0);

  key = SmallInt::fromWord(1);
  runtime.setAdd(set2, key);
  Object result2(&scope, runBuiltin(SetBuiltins::dunderAnd, set1, set2));
  ASSERT_TRUE(result2->isSet());
  Set set(&scope, *result2);
  EXPECT_EQ(set->numItems(), 1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, DunderAndWithNonSet) {
  Runtime runtime;
  HandleScope scope;

  Object empty_set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, empty_set, none));
  ASSERT_TRUE(result->isNotImplemented());
}

TEST(SetBuiltinsTest, DunderIand) {
  Runtime runtime;
  HandleScope scope;

  Set set1(&scope, runtime.newSet());
  Set set2(&scope, runtime.newSet());
  Object key(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderIand, set1, set2));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(*result, *set1);
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 0);

  key = SmallInt::fromWord(1);
  runtime.setAdd(set1, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set1, key);
  Object result1(&scope, runBuiltin(SetBuiltins::dunderIand, set1, set2));
  ASSERT_TRUE(result1->isSet());
  EXPECT_EQ(*result1, *set1);
  EXPECT_EQ(RawSet::cast(*result1)->numItems(), 0);

  set1 = runtime.newSet();
  key = SmallInt::fromWord(1);
  runtime.setAdd(set1, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set1, key);
  runtime.setAdd(set2, key);
  Object result2(&scope, runBuiltin(SetBuiltins::dunderIand, set1, set2));
  ASSERT_TRUE(result2->isSet());
  EXPECT_EQ(*result2, *set1);
  Set set(&scope, *result2);
  EXPECT_EQ(set->numItems(), 1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, DunderIandWithNonSet) {
  Runtime runtime;
  HandleScope scope;

  Object empty_set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderIand, empty_set, none));
  ASSERT_TRUE(result->isNotImplemented());
}

TEST(SetBuiltinsTest, SetIntersectionWithNoArgsReturnsCopy) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  // set.intersect() with no arguments
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set));
  ASSERT_TRUE(result->isSet());
  EXPECT_NE(*result, *set);
  set = *result;
  EXPECT_EQ(set->numItems(), 3);

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
  ASSERT_TRUE(result->isSet());
  EXPECT_NE(*result, *set);
  set = *result;
  EXPECT_EQ(set->numItems(), 2);
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
  ASSERT_TRUE(result->isSet());
  EXPECT_NE(*result, *set);
  set = *result;
  EXPECT_EQ(set->numItems(), 1);
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
  ASSERT_TRUE(result->isSet());
  EXPECT_NE(*result, *set);
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 0);
}

TEST(SetBuiltinsTest, SetIntersectionWithEmptyIterableReturnsEmptySet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  List list(&scope, runtime.newList());
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, list));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 0);
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
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 1);
  set = *result;
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, SetIntersectionWithFrozenSetReturnsSet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  FrozenSet frozen_set(&scope, runtime.newFrozenSet());
  Object result(&scope, runBuiltin(SetBuiltins::intersection, set, frozen_set));
  ASSERT_TRUE(result->isSet());
}

TEST(SetBuiltinsTest, FrozenSetIntersectionWithSetReturnsFrozenSet) {
  Runtime runtime;
  HandleScope scope;
  FrozenSet frozen_set(&scope, runtime.newFrozenSet());
  Set set(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::intersection, frozen_set, set));
  ASSERT_TRUE(result->isFrozenSet());
}

TEST(SetBuiltinsTest, SetAndWithFrozenSetReturnsSet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  FrozenSet frozen_set(&scope, runtime.newFrozenSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, set, frozen_set));
  ASSERT_TRUE(result->isSet());
}

TEST(SetBuiltinsTest, FrozenSetAndWithSetReturnsFrozenSet) {
  Runtime runtime;
  HandleScope scope;
  FrozenSet frozen_set(&scope, runtime.newFrozenSet());
  Set set(&scope, runtime.newSet());
  Object result(&scope, runBuiltin(SetBuiltins::dunderAnd, frozen_set, set));
  ASSERT_TRUE(result->isFrozenSet());
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
  ASSERT_TRUE(iter->isSetIterator());

  Object item1(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object item2(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*item2)->value(), 1);

  Object item3(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item3->isError());
}

TEST(SetIteratorBuiltinsTest, CallDunderNextWithEmptySet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, set));
  ASSERT_TRUE(iter->isSetIterator());

  Object result(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(result->isError());
}

TEST(SetIteratorBuiltinsTes, DunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  Set empty_set(&scope, runtime.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, empty_set));
  ASSERT_TRUE(iter->isSetIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(SetIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(SetIteratorBuiltinsTest, DunderLengthHintOnEmptySetReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Set empty_set(&scope, runtime.newSet());
  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, empty_set));
  ASSERT_TRUE(iter->isSetIterator());

  Object length_hint(&scope,
                     runBuiltin(SetIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(SetIteratorBuiltinsTest, DunderLengthHintOnConsumedSetReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Set one_element_set(&scope, runtime.newSet());
  Object zero(&scope, SmallInt::fromWord(0));
  runtime.setAdd(one_element_set, zero);

  Object iter(&scope, runBuiltin(SetBuiltins::dunderIter, one_element_set));
  ASSERT_TRUE(iter->isSetIterator());

  Object length_hint1(&scope,
                      runBuiltin(SetIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint1)->value(), 1);

  // Consume the iterator
  Object item1(&scope, runBuiltin(SetIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object length_hint2(&scope,
                      runBuiltin(SetIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint2)->value(), 0);
}

TEST(SetBuiltinsDeathTest, SetIsDisjointWithNonIterableArg) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
s = {1}
s.isdisjoint(None)
)"),
               "object is not iterable");
}

TEST(SetBuiltinsTest, SetIsDisjointWithSetArg) {
  Runtime runtime;
  HandleScope scope;

  Set set(&scope, runtime.newSet());
  Set other(&scope, runtime.newSet());
  Object value(&scope, NoneType::object());

  // set().isdisjoint(set())
  Object result(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result->isBool());
  EXPECT_EQ(*result, Bool::trueObj());

  // set().isdisjoint({None})
  runtime.setAdd(other, value);
  Object result1(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result1->isBool());
  EXPECT_EQ(*result1, Bool::trueObj());

  // {None}.isdisjoint({None})
  runtime.setAdd(set, value);
  Object result2(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result2->isBool());
  EXPECT_EQ(*result2, Bool::falseObj());

  // {None}.isdisjoint({1})
  other = runtime.newSet();
  value = SmallInt::fromWord(1);
  runtime.setAdd(other, value);
  Object result3(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result3->isBool());
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
  ASSERT_TRUE(result->isBool());
  EXPECT_EQ(*result, Bool::trueObj());

  // set().isdisjoint([None])
  runtime.listAdd(other, value);
  Object result1(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result1->isBool());
  EXPECT_EQ(*result1, Bool::trueObj());

  // {None}.isdisjoint([None])
  runtime.setAdd(set, value);
  Object result2(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result2->isBool());
  EXPECT_EQ(*result2, Bool::falseObj());

  // {None}.isdisjoint([1])
  other = runtime.newList();
  value = SmallInt::fromWord(1);
  runtime.listAdd(other, value);
  Object result3(&scope, runBuiltin(SetBuiltins::isDisjoint, set, other));
  ASSERT_TRUE(result3->isBool());
  EXPECT_EQ(*result3, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderEqWithSetSubclass) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a == b)
cmp1 = (a1 == b)
cmp2 = (b == a)
cmp3 = (b == a1)
cmp4 = (b == b)
)");
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderNeWithSetSubclass) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a != b)
cmp1 = (a1 != b)
cmp2 = (b != a)
cmp3 = (b != a1)
cmp4 = (b != b)
)");
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderGeWithSetSubclass) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a >= b)
cmp1 = (a1 >= b)
cmp2 = (b >= a)
cmp3 = (b >= a1)
cmp4 = (b >= b)
)");
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGtWithSetSubclass) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a > b)
cmp1 = (a1 > b)
cmp2 = (b > a)
cmp3 = (b > a1)
cmp4 = (b > b)
)");
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderLeWithSetSubclass) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a <= b)
cmp1 = (a1 <= b)
cmp2 = (b <= a)
cmp3 = (b <= a1)
cmp4 = (b <= b)
)");
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp1"), Bool::falseObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp2"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp3"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "cmp4"), Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLtWithSetSubclass) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a < b)
cmp1 = (a1 < b)
cmp2 = (b < a)
cmp3 = (b < a1)
cmp4 = (b < b)
)");
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
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, set));
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderEqWithUnequalSetsReturnsFalse) {
  // ({0, 1, 2} == {1, 2, 3}) is False
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(1, 4));
  Object result(&scope, runBuiltin(SetBuiltins::dunderEq, set, set));
  ASSERT_EQ(*result, Bool::trueObj());
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
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsTest, DunderNeWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderNe, set, none));
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsTest, DunderGeWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderGe, set, none));
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsTest, DunderGtWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderGt, set, none));
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsTest, DunderLeWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderLe, set, none));
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsTest, DunderLtWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Object none(&scope, NoneType::object());
  Object result(&scope, runBuiltin(SetBuiltins::dunderLt, set, none));
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsDeathTest, DunderEqWithTooFewArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__eq__()
)"),
               "__eq__\\(\\) of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderNeWithTooFewArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__ne__()
)"),
               "__ne__\\(\\) of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderGeWithTooFewArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__ge__()
)"),
               "__ge__\\(\\) of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderGtWithTooFewArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__gt__()
)"),
               "__gt__\\(\\) of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderLeWithTooFewArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__le__()
)"),
               "__le__\\(\\) of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderLtWithTooFewArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__lt__()
)"),
               "__lt__\\(\\) of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderEqWithNonSetFirstArgThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__eq__(None, set())
)"),
               "__eq__\\(\\) requires a 'set' or 'frozenset' object");
}

TEST(SetBuiltinsDeathTest, DunderNeWithNonSetFirstArgThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__ne__(None, set())
)"),
               "__ne__\\(\\) requires a 'set' or 'frozenset' object");
}

TEST(SetBuiltinsDeathTest, DunderGeWithNonSetFirstArgThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__ge__(None, set())
)"),
               "__ge__\\(\\) requires a 'set' or 'frozenset' object");
}

TEST(SetBuiltinsDeathTest, DunderGtWithNonSetFirstArgThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__gt__(None, set())
)"),
               "__gt__\\(\\) requires a 'set' or 'frozenset' object");
}

TEST(SetBuiltinsDeathTest, DunderLeWithNonSetFirstArgThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__le__(None, set())
)"),
               "__le__\\(\\) requires a 'set' or 'frozenset' object");
}

TEST(SetBuiltinsDeathTest, DunderLtWithNonSetFirstArgThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__lt__(None, set())
)"),
               "__lt__\\(\\) requires a 'set' or 'frozenset' object");
}

TEST(SetBuiltinsDeathTest, DunderEqWithTooManyArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__eq__(set(), set(), set())
)"),
               "expected 1 arguments, got 2");
}

TEST(SetBuiltinsDeathTest, DunderNeWithTooManyArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__ne__(set(), set(), set())
)"),
               "expected 1 argument, got 2");
}

TEST(SetBuiltinsDeathTest, DunderGeWithTooManyArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__ge__(set(), set(), set())
)"),
               "expected 1 argument, got 2");
}

TEST(SetBuiltinsDeathTest, DunderGtWithTooManyArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__gt__(set(), set(), set())
)"),
               "expected 1 argument, got 2");
}

TEST(SetBuiltinsDeathTest, DunderLeWithTooManyArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__le__(set(), set(), set())
)"),
               "expected 1 argument, got 2");
}

TEST(SetBuiltinsDeathTest, DunderLtWithTooManyArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__lt__(set(), set(), set())
)"),
               "expected 1 argument, got 2");
}

TEST(SetBuiltinsDeathTest, DunderInitWithNonSetFirstArgThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__init__([])
)"),
               "__init__\\(\\) requires a 'set' object");
}

TEST(SetBuiltinsDeathTest, DunderInitWithTooFewArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__init__()
)"),
               "__init__\\(\\) of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderInitWithTooManyArgsThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__init__(set(), None, None, None)
)"),
               "set expected at most 1 argument, got 3");
}

TEST(SetBuiltinsDeathTest, DunderInitWithNonIterableThrowsTypeError) {
  Runtime runtime;
  ASSERT_DEATH(runFromCStr(&runtime, R"(
set.__init__(set(), None)
)"),
               "object is not iterable");
}

TEST(SetBuiltinsTest, DunderInitWithIteratorUpdatesSet) {
  Runtime runtime;
  HandleScope scope;
  Set set(&scope, runtime.newSet());
  Set set1(&scope, setFromRange(0, 3));
  Object result(&scope, runBuiltin(SetBuiltins::dunderInit, set, set1));
  ASSERT_TRUE(result->isNoneType());
  EXPECT_EQ(set->numItems(), set1->numItems());
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
  runFromCStr(&runtime, R"(
class Set(set): pass

s = Set([0, 1, 2])
)");
  Object s(&scope, moduleAt(&runtime, "__main__", "s"));
  ASSERT_TRUE(runtime.isInstanceOfSet(s));
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
  runFromCStr(&runtime, R"(
class Set(set): pass

s = Set([0, 1, 2])
)");
  Object s(&scope, moduleAt(&runtime, "__main__", "s"));
  ASSERT_TRUE(runtime.isInstanceOfSet(s));

  Object result(&scope, runBuiltin(SetBuiltins::dunderLen, s));
  ASSERT_TRUE(result.isInt());
  EXPECT_EQ(RawSmallInt::cast(*result).value(), 3);
}

}  // namespace python
