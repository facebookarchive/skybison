#include "gtest/gtest.h"

#include "runtime.h"
#include "set-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(SetDeathTest, SetPopException) {
  const char* src1 = R"(
s = {1}
s.pop()
s.pop()
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src1),
               "aborting due to pending exception: "
               "pop from an empty set");
}

TEST(SetBuiltinsTest, SetPop) {
  const char* src = R"(
s = {1}
a = s.pop()
b = len(s)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object a(&scope, moduleAt(&runtime, main, "a"));
  Object b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(RawSmallInt::cast(*a)->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(*b)->value(), 0);
}

TEST(SetBuiltinsTest, InitializeByTypeCall) {
  const char* src = R"(
s = set()
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object s(&scope, moduleAt(&runtime, main, "s"));
  EXPECT_TRUE(s->isSet());
  EXPECT_EQ(RawSet::cast(*s)->numItems(), 0);
}

TEST(SetBuiltinTest, SetAdd) {
  const char* src = R"(
s = set()
s.add(1)
s.add("Hello, World")
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Set s(&scope, moduleAt(&runtime, main, "s"));
  Object one(&scope, runtime.newInt(1));
  Object hello_world(&scope, runtime.newStrFromCStr("Hello, World"));
  EXPECT_EQ(s->numItems(), 2);
  EXPECT_TRUE(runtime.setIncludes(s, one));
  EXPECT_TRUE(runtime.setIncludes(s, hello_world));
}

TEST(SetDeathTest, SetAddException) {
  const char* src1 = R"(
s = set()
s.add(1, 2)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src1),
               "aborting due to pending exception: "
               "add\\(\\) takes exactly one argument");
}

TEST(SetBuiltinsTest, DunderIterReturnsSetIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Set empty_set(&scope, runtime.newSet());

  frame->setLocal(0, *empty_set);
  Object iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());
}

TEST(SetBuiltinsTest, DunderAnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(2, 0, 0);
  HandleScope scope;

  Set set1(&scope, runtime.newSet());
  Set set2(&scope, runtime.newSet());
  frame->setLocal(0, *set1);
  frame->setLocal(1, *set2);
  Object result(&scope, SetBuiltins::dunderAnd(thread, frame, 2));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 0);

  Object key(&scope, SmallInt::fromWord(1));
  runtime.setAdd(set1, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set1, key);
  frame->setLocal(0, *set1);
  frame->setLocal(1, *set2);
  Object result1(&scope, SetBuiltins::dunderAnd(thread, frame, 2));
  ASSERT_TRUE(result1->isSet());
  EXPECT_EQ(RawSet::cast(*result1)->numItems(), 0);

  key = SmallInt::fromWord(1);
  runtime.setAdd(set2, key);
  frame->setLocal(0, *set1);
  frame->setLocal(1, *set2);
  Object result2(&scope, SetBuiltins::dunderAnd(thread, frame, 2));
  ASSERT_TRUE(result2->isSet());
  Set set(&scope, *result2);
  EXPECT_EQ(set->numItems(), 1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, DunderAndWithNonSet) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(2, 0, 0);
  HandleScope scope;

  Object empty_set(&scope, runtime.newSet());
  frame->setLocal(0, *empty_set);
  frame->setLocal(1, NoneType::object());
  Object result(&scope, SetBuiltins::dunderAnd(thread, frame, 2));
  thread->popFrame();
  ASSERT_TRUE(result->isNotImplemented());
}

TEST(SetBuiltinsTest, DunderIand) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(2, 0, 0);
  HandleScope scope;

  Set set1(&scope, runtime.newSet());
  Set set2(&scope, runtime.newSet());
  Object key(&scope, NoneType::object());
  frame->setLocal(0, *set1);
  frame->setLocal(1, *set2);
  Object result(&scope, SetBuiltins::dunderIand(thread, frame, 2));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(*result, *set1);
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 0);

  key = SmallInt::fromWord(1);
  runtime.setAdd(set1, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set1, key);
  frame->setLocal(0, *set1);
  frame->setLocal(1, *set2);
  Object result1(&scope, SetBuiltins::dunderIand(thread, frame, 2));
  ASSERT_TRUE(result1->isSet());
  EXPECT_EQ(*result1, *set1);
  EXPECT_EQ(RawSet::cast(*result1)->numItems(), 0);

  set1 = runtime.newSet();
  key = SmallInt::fromWord(1);
  runtime.setAdd(set1, key);
  key = SmallInt::fromWord(2);
  runtime.setAdd(set1, key);
  runtime.setAdd(set2, key);
  frame->setLocal(0, *set1);
  frame->setLocal(1, *set2);
  Object result2(&scope, SetBuiltins::dunderIand(thread, frame, 2));
  ASSERT_TRUE(result2->isSet());
  EXPECT_EQ(*result2, *set1);
  Set set(&scope, *result2);
  EXPECT_EQ(set->numItems(), 1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetBuiltinsTest, DunderIandWithNonSet) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(2, 0, 0);
  HandleScope scope;

  Object empty_set(&scope, runtime.newSet());
  frame->setLocal(0, *empty_set);
  frame->setLocal(1, NoneType::object());
  Object result(&scope, SetBuiltins::dunderIand(thread, frame, 2));
  thread->popFrame();
  ASSERT_TRUE(result->isNotImplemented());
}

TEST(SetBuiltinsTest, SetIntersectionWithNoArgsReturnsCopy) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  // set.intersect() with no arguments
  frame->setLocal(0, *set);
  Object result(&scope, SetBuiltins::intersection(thread, frame, 1));
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
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(2, 0, 0);
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 2));

  // set.intersect() with 1 argument
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::intersection(thread, frame, 2));
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
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(3, 0, 0);
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 2));
  Set set2(&scope, setFromRange(0, 1));

  // set.intersect() with 2 arguments
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  frame->setLocal(2, *set2);
  Object result(&scope, SetBuiltins::intersection(thread, frame, 3));
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
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(3, 0, 0);
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 2));
  Set set2(&scope, runtime.newSet());

  // set.intersect() with 2 arguments
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  frame->setLocal(2, *set2);
  Object result(&scope, SetBuiltins::intersection(thread, frame, 3));
  ASSERT_TRUE(result->isSet());
  EXPECT_NE(*result, *set);
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 0);
}

TEST(SetBuiltinsTest, SetIntersectionWithEmptyIterableReturnsEmptySet) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(2, 0, 0);
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  List list(&scope, runtime.newList());
  frame->setLocal(0, *set);
  frame->setLocal(1, *list);
  Object result(&scope, SetBuiltins::intersection(thread, frame, 2));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 0);
}

TEST(SetBuiltinsTest, SetIntersectionWithIterableReturnsIntersection) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(2, 0, 0);
  HandleScope scope;
  Set set(&scope, setFromRange(0, 3));
  List list(&scope, runtime.newList());
  Object key(&scope, SmallInt::fromWord(4));
  runtime.listAdd(list, key);
  key = SmallInt::fromWord(0);
  runtime.listAdd(list, key);
  frame->setLocal(0, *set);
  frame->setLocal(1, *list);
  Object result(&scope, SetBuiltins::intersection(thread, frame, 2));
  ASSERT_TRUE(result->isSet());
  EXPECT_EQ(RawSet::cast(*result)->numItems(), 1);
  set = *result;
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

TEST(SetIteratorBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Object value(&scope, SmallInt::fromWord(0));
  runtime.setAdd(set, value);
  value = SmallInt::fromWord(1);
  runtime.setAdd(set, value);

  frame->setLocal(0, *set);
  Object iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());

  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());

  Object item1(&scope,
               Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object item2(&scope,
               Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item2->isSmallInt());
  EXPECT_EQ(RawSmallInt::cast(*item2)->value(), 1);

  Object item3(&scope,
               Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item3->isError());
}

TEST(SetIteratorBuiltinsTest, CallDunderNextWithEmptySet) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());

  frame->setLocal(0, *set);
  Object iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());

  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());

  Object result(&scope,
                Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(result->isError());
}

TEST(SetIteratorBuiltinsTes, DunderIterReturnsSelf) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Set empty_set(&scope, runtime.newSet());

  frame->setLocal(0, *empty_set);
  Object iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());

  // Now call __iter__ on the iterator object
  Object iter_iter(&scope, Interpreter::lookupMethod(thread, frame, iter,
                                                     SymbolId::kDunderIter));
  ASSERT_FALSE(iter_iter->isError());
  Object result(&scope,
                Interpreter::callMethod1(thread, frame, iter_iter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(SetIteratorBuiltinsTest, DunderLengthHintOnEmptySetReturnsZero) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Set empty_set(&scope, runtime.newSet());

  frame->setLocal(0, *empty_set);
  Object iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());

  Object length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderLengthHint));
  ASSERT_FALSE(length_hint_method->isError());

  Object length_hint(&scope, Interpreter::callMethod1(
                                 thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(SetIteratorBuiltinsTest, DunderLengthHintOnConsumedSetReturnsZero) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Set one_element_set(&scope, runtime.newSet());
  Object zero(&scope, SmallInt::fromWord(0));
  runtime.setAdd(one_element_set, zero);

  frame->setLocal(0, *one_element_set);
  Object iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());

  Object length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderLengthHint));
  ASSERT_FALSE(length_hint_method->isError());

  Object length_hint1(&scope, Interpreter::callMethod1(
                                  thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint1)->value(), 1);

  // Consume the iterator
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());
  Object item1(&scope,
               Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object length_hint2(&scope, Interpreter::callMethod1(
                                  thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint2)->value(), 0);
}

TEST(SetBuiltinsDeathTest, SetIsDisjointWithNonIterableArg) {
  const char* src = R"(
s = {1}
s.isdisjoint(None)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "object is not iterable");
}

TEST(SetBuiltinsTest, SetIsDisjointWithSetArg) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  Set set(&scope, runtime.newSet());
  Set other(&scope, runtime.newSet());
  Object value(&scope, NoneType::object());

  // set().isdisjoint(set())
  frame->setLocal(0, *set);
  frame->setLocal(1, *other);
  Object result(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result->isBool());
  EXPECT_EQ(*result, Bool::trueObj());

  // set().isdisjoint({None})
  runtime.setAdd(other, value);
  Object result1(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result1->isBool());
  EXPECT_EQ(*result1, Bool::trueObj());

  // {None}.isdisjoint({None})
  runtime.setAdd(set, value);
  Object result2(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result2->isBool());
  EXPECT_EQ(*result2, Bool::falseObj());

  // {None}.isdisjoint({1})
  other = runtime.newSet();
  value = SmallInt::fromWord(1);
  runtime.setAdd(other, value);
  frame->setLocal(1, *other);
  Object result3(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result3->isBool());
  EXPECT_EQ(*result3, Bool::trueObj());

  thread->popFrame();
}

TEST(SetBuiltinsTest, SetIsDisjointWithIterableArg) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  Set set(&scope, runtime.newSet());
  List other(&scope, runtime.newList());
  Object value(&scope, NoneType::object());

  // set().isdisjoint([])
  frame->setLocal(0, *set);
  frame->setLocal(1, *other);
  Object result(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result->isBool());
  EXPECT_EQ(*result, Bool::trueObj());

  // set().isdisjoint([None])
  runtime.listAdd(other, value);
  Object result1(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result1->isBool());
  EXPECT_EQ(*result1, Bool::trueObj());

  // {None}.isdisjoint([None])
  runtime.setAdd(set, value);
  Object result2(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result2->isBool());
  EXPECT_EQ(*result2, Bool::falseObj());

  // {None}.isdisjoint([1])
  other = runtime.newList();
  value = SmallInt::fromWord(1);
  runtime.listAdd(other, value);
  frame->setLocal(1, *other);
  Object result3(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result3->isBool());
  EXPECT_EQ(*result3, Bool::trueObj());

  thread->popFrame();
}

TEST(SetBuiltinsTest, DunderEqWithSetSubclass) {
  const char* src = R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a == b)
cmp1 = (a1 == b)
cmp2 = (b == a)
cmp3 = (b == a1)
cmp4 = (b == b)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object cmp(&scope, moduleAt(&runtime, main, "cmp"));
  Object cmp1(&scope, moduleAt(&runtime, main, "cmp1"));
  Object cmp2(&scope, moduleAt(&runtime, main, "cmp2"));
  Object cmp3(&scope, moduleAt(&runtime, main, "cmp3"));
  Object cmp4(&scope, moduleAt(&runtime, main, "cmp4"));
  EXPECT_EQ(*cmp, Bool::trueObj());
  EXPECT_EQ(*cmp1, Bool::falseObj());
  EXPECT_EQ(*cmp2, Bool::trueObj());
  EXPECT_EQ(*cmp3, Bool::falseObj());
  EXPECT_EQ(*cmp4, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderNeWithSetSubclass) {
  const char* src = R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a != b)
cmp1 = (a1 != b)
cmp2 = (b != a)
cmp3 = (b != a1)
cmp4 = (b != b)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object cmp(&scope, moduleAt(&runtime, main, "cmp"));
  Object cmp1(&scope, moduleAt(&runtime, main, "cmp1"));
  Object cmp2(&scope, moduleAt(&runtime, main, "cmp2"));
  Object cmp3(&scope, moduleAt(&runtime, main, "cmp3"));
  Object cmp4(&scope, moduleAt(&runtime, main, "cmp4"));
  EXPECT_EQ(*cmp, Bool::falseObj());
  EXPECT_EQ(*cmp1, Bool::trueObj());
  EXPECT_EQ(*cmp2, Bool::falseObj());
  EXPECT_EQ(*cmp3, Bool::trueObj());
  EXPECT_EQ(*cmp4, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderGeWithSetSubclass) {
  const char* src = R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a >= b)
cmp1 = (a1 >= b)
cmp2 = (b >= a)
cmp3 = (b >= a1)
cmp4 = (b >= b)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object cmp(&scope, moduleAt(&runtime, main, "cmp"));
  Object cmp1(&scope, moduleAt(&runtime, main, "cmp1"));
  Object cmp2(&scope, moduleAt(&runtime, main, "cmp2"));
  Object cmp3(&scope, moduleAt(&runtime, main, "cmp3"));
  Object cmp4(&scope, moduleAt(&runtime, main, "cmp4"));
  EXPECT_EQ(*cmp, Bool::trueObj());
  EXPECT_EQ(*cmp1, Bool::trueObj());
  EXPECT_EQ(*cmp2, Bool::trueObj());
  EXPECT_EQ(*cmp3, Bool::falseObj());
  EXPECT_EQ(*cmp4, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGtWithSetSubclass) {
  const char* src = R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a > b)
cmp1 = (a1 > b)
cmp2 = (b > a)
cmp3 = (b > a1)
cmp4 = (b > b)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object cmp(&scope, moduleAt(&runtime, main, "cmp"));
  Object cmp1(&scope, moduleAt(&runtime, main, "cmp1"));
  Object cmp2(&scope, moduleAt(&runtime, main, "cmp2"));
  Object cmp3(&scope, moduleAt(&runtime, main, "cmp3"));
  Object cmp4(&scope, moduleAt(&runtime, main, "cmp4"));
  EXPECT_EQ(*cmp, Bool::falseObj());
  EXPECT_EQ(*cmp1, Bool::trueObj());
  EXPECT_EQ(*cmp2, Bool::falseObj());
  EXPECT_EQ(*cmp3, Bool::falseObj());
  EXPECT_EQ(*cmp4, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderLeWithSetSubclass) {
  const char* src = R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a <= b)
cmp1 = (a1 <= b)
cmp2 = (b <= a)
cmp3 = (b <= a1)
cmp4 = (b <= b)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object cmp(&scope, moduleAt(&runtime, main, "cmp"));
  Object cmp1(&scope, moduleAt(&runtime, main, "cmp1"));
  Object cmp2(&scope, moduleAt(&runtime, main, "cmp2"));
  Object cmp3(&scope, moduleAt(&runtime, main, "cmp3"));
  Object cmp4(&scope, moduleAt(&runtime, main, "cmp4"));
  EXPECT_EQ(*cmp, Bool::trueObj());
  EXPECT_EQ(*cmp1, Bool::falseObj());
  EXPECT_EQ(*cmp2, Bool::trueObj());
  EXPECT_EQ(*cmp3, Bool::trueObj());
  EXPECT_EQ(*cmp4, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLtWithSetSubclass) {
  const char* src = R"(
class Bar(set): pass

a = set()
a1 = {1}
b = Bar()
cmp = (a < b)
cmp1 = (a1 < b)
cmp2 = (b < a)
cmp3 = (b < a1)
cmp4 = (b < b)
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object cmp(&scope, moduleAt(&runtime, main, "cmp"));
  Object cmp1(&scope, moduleAt(&runtime, main, "cmp1"));
  Object cmp2(&scope, moduleAt(&runtime, main, "cmp2"));
  Object cmp3(&scope, moduleAt(&runtime, main, "cmp3"));
  Object cmp4(&scope, moduleAt(&runtime, main, "cmp4"));
  EXPECT_EQ(*cmp, Bool::falseObj());
  EXPECT_EQ(*cmp1, Bool::falseObj());
  EXPECT_EQ(*cmp2, Bool::falseObj());
  EXPECT_EQ(*cmp3, Bool::trueObj());
  EXPECT_EQ(*cmp4, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderEqWithEmptySetsReturnsTrue) {
  // (set() == set()) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderEq(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderEqWithSameSetReturnsTrue) {
  // s = {0, 1, 2}; (s == s) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set);
  Object result(&scope, SetBuiltins::dunderEq(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderEqWithEqualSetsReturnsTrue) {
  // ({0, 1, 2) == {0, 1, 2}) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set);
  Object result(&scope, SetBuiltins::dunderEq(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderEqWithUnequalSetsReturnsFalse) {
  // ({0, 1, 2} == {1, 2, 3}) is False
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(1, 4));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set);
  Object result(&scope, SetBuiltins::dunderEq(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderNeWithEmptySetsReturnsFalse) {
  // (set() != set()) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderNe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderNeWithSameSetReturnsFalse) {
  // s = {0, 1, 2}; (s != s) is False
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set);
  Object result(&scope, SetBuiltins::dunderNe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderNeWithEqualSetsReturnsFalse) {
  // ({0, 1, 2} != {0, 1, 2}) is False
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderNe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderNeWithUnequalSetsReturnsTrue) {
  // ({0, 1, 2} != {1, 2, 3}) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(1, 4));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderNe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGeWithSameSetReturnsTrue) {
  // s = {0, 1, 2}; (s >= s) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set);
  Object result(&scope, SetBuiltins::dunderGe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGeWithEqualSetsReturnsTrue) {
  // ({0, 1, 2} >= {0, 1, 2}) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderGe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGeWithSupersetReturnsFalse) {
  // ({0, 1, 2} >= {0, 1, 2, 3}) is False
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 4));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderGe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderGeWithEmptySetReturnsTrue) {
  // ({0, 1, 2} >= set()) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, runtime.newSet());
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderGe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLeWithEmptySetReturnsTrue) {
  // s = {0, 1, 2}; (s <= s) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, runtime.newSet());
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderLe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLeWithEqualSetsReturnsTrue) {
  // ({0, 1, 2} <= {0, 1, 2}) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderLe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLeWithSubsetReturnsFalse) {
  // ({0, 1, 2, 3} <= {0, 1, 2}) is False
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderLe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderLeWithEmptySetReturnsFalse) {
  // ({0, 1, 2} <= set()) is False
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, runtime.newSet());
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderLe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderGtWithEqualSetsReturnsFalse) {
  // ({0, 1, 2} > {0, 1, 2}) is False
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderGt(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderGtWithSubsetReturnsTrue) {
  // ({0, 1, 2, 3} > {0, 1, 2}) is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderGt(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderGtWithSupersetReturnsFalse) {
  // ({0, 1, 2} > {0, 1, 2, 3}) is False
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 4));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderGt(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderLtWithEqualSetsReturnsFalse) {
  // ({0, 1, 2} < {0, 1, 2}) is False
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderLt(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderLtWithSupersetReturnsTrue) {
  // ({0, 1, 2} < {0, 1, 2, 3})  is True
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 3));
  Set set1(&scope, setFromRange(0, 4));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderLt(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::trueObj());
}

TEST(SetBuiltinsTest, DunderLtWithSubsetReturnsFalse) {
  // ({0, 1, 2, 3} < {0, 1, 2}) is False
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, setFromRange(0, 4));
  Set set1(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderLt(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, Bool::falseObj());
}

TEST(SetBuiltinsTest, DunderEqWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Set set(&scope, runtime.newSet());
  frame->setLocal(0, *set);
  frame->setLocal(1, NoneType::object());
  Object result(&scope, SetBuiltins::dunderEq(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsTest, DunderNeWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Set set(&scope, runtime.newSet());
  frame->setLocal(0, *set);
  frame->setLocal(1, NoneType::object());
  Object result(&scope, SetBuiltins::dunderNe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsTest, DunderGeWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Set set(&scope, runtime.newSet());
  frame->setLocal(0, *set);
  frame->setLocal(1, NoneType::object());
  Object result(&scope, SetBuiltins::dunderGe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsTest, DunderGtWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Set set(&scope, runtime.newSet());
  frame->setLocal(0, *set);
  frame->setLocal(1, NoneType::object());
  Object result(&scope, SetBuiltins::dunderGt(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsTest, DunderLeWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Set set(&scope, runtime.newSet());
  frame->setLocal(0, *set);
  frame->setLocal(1, NoneType::object());
  Object result(&scope, SetBuiltins::dunderLe(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsTest, DunderLtWithNonSetSecondArgReturnsNotImplemented) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Set set(&scope, runtime.newSet());
  frame->setLocal(0, *set);
  frame->setLocal(1, NoneType::object());
  Object result(&scope, SetBuiltins::dunderLt(thread, frame, 2));
  thread->popFrame();
  ASSERT_EQ(*result, runtime.notImplemented());
}

TEST(SetBuiltinsDeathTest, DunderEqWithTooFewArgsThrowsTypeError) {
  const char* src = R"(
set.__eq__()
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__eq__' of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderNeWithTooFewArgsThrowsTypeError) {
  const char* src = R"(
set.__ne__()
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__ne__' of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderGeWithTooFewArgsThrowsTypeError) {
  const char* src = R"(
set.__ge__()
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__ge__' of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderGtWithTooFewArgsThrowsTypeError) {
  const char* src = R"(
set.__gt__()
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__gt__' of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderLeWithTooFewArgsThrowsTypeError) {
  const char* src = R"(
set.__le__()
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__le__' of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderLtWithTooFewArgsThrowsTypeError) {
  const char* src = R"(
set.__lt__()
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__lt__' of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderEqWithNonSetFirstArgThrowsTypeError) {
  const char* src = R"(
set.__eq__(None, set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__eq__' requires a 'set' object");
}

TEST(SetBuiltinsDeathTest, DunderNeWithNonSetFirstArgThrowsTypeError) {
  const char* src = R"(
set.__ne__(None, set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__ne__' requires a 'set' object");
}

TEST(SetBuiltinsDeathTest, DunderGeWithNonSetFirstArgThrowsTypeError) {
  const char* src = R"(
set.__ge__(None, set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__ge__' requires a 'set' object");
}

TEST(SetBuiltinsDeathTest, DunderGtWithNonSetFirstArgThrowsTypeError) {
  const char* src = R"(
set.__gt__(None, set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__gt__' requires a 'set' object");
}

TEST(SetBuiltinsDeathTest, DunderLeWithNonSetFirstArgThrowsTypeError) {
  const char* src = R"(
set.__le__(None, set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__le__' requires a 'set' object");
}

TEST(SetBuiltinsDeathTest, DunderLtWithNonSetFirstArgThrowsTypeError) {
  const char* src = R"(
set.__lt__(None, set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__lt__' requires a 'set' object");
}

TEST(SetBuiltinsDeathTest, DunderEqWithTooManyArgsThrowsTypeError) {
  const char* src = R"(
set.__eq__(set(), set(), set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 1 arguments, got 2");
}

TEST(SetBuiltinsDeathTest, DunderNeWithTooManyArgsThrowsTypeError) {
  const char* src = R"(
set.__ne__(set(), set(), set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 1 arguments, got 2");
}

TEST(SetBuiltinsDeathTest, DunderGeWithTooManyArgsThrowsTypeError) {
  const char* src = R"(
set.__ge__(set(), set(), set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 1 arguments, got 2");
}

TEST(SetBuiltinsDeathTest, DunderGtWithTooManyArgsThrowsTypeError) {
  const char* src = R"(
set.__gt__(set(), set(), set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 1 arguments, got 2");
}

TEST(SetBuiltinsDeathTest, DunderLeWithTooManyArgsThrowsTypeError) {
  const char* src = R"(
set.__le__(set(), set(), set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 1 arguments, got 2");
}

TEST(SetBuiltinsDeathTest, DunderLtWithTooManyArgsThrowsTypeError) {
  const char* src = R"(
set.__lt__(set(), set(), set())
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "expected 1 arguments, got 2");
}

TEST(SetBuiltinsDeathTest, DunderInitWithNonSetFirstArgThrowsTypeError) {
  const char* src = R"(
set.__init__([])
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__init__' requires a 'set' object");
}

TEST(SetBuiltinsDeathTest, DunderInitWithTooFewArgsThrowsTypeError) {
  const char* src = R"(
set.__init__()
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "descriptor '__init__' of 'set' object needs an argument");
}

TEST(SetBuiltinsDeathTest, DunderInitWithTooManyArgsThrowsTypeError) {
  const char* src = R"(
set.__init__(set(), None, None, None)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src),
               "set expected at most 1 arguments, got 3");
}

TEST(SetBuiltinsDeathTest, DunderInitWithNonIterableThrowsTypeError) {
  const char* src = R"(
set.__init__(set(), None)
)";
  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCStr(src), "object is not iterable");
}

TEST(SetBuiltinsTest, DunderInitWithIteratorUpdatesSet) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set set(&scope, runtime.newSet());
  Set set1(&scope, setFromRange(0, 3));
  Frame* frame = thread->openAndLinkFrame(2, 0, 0);
  frame->setLocal(0, *set);
  frame->setLocal(1, *set1);
  Object result(&scope, SetBuiltins::dunderInit(thread, frame, 2));
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
  const char* src = R"(
class Set(set): pass

s = Set([0, 1, 2])
)";
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  runtime.runFromCStr(src);
  Module main(&scope, findModule(&runtime, "__main__"));
  Object s(&scope, moduleAt(&runtime, main, "s"));
  ASSERT_TRUE(runtime.isInstanceOfSet(s));
  Object key(&scope, SmallInt::fromWord(0));
  Set set(&scope, *s);
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(1);
  EXPECT_TRUE(runtime.setIncludes(set, key));
  key = SmallInt::fromWord(2);
  EXPECT_TRUE(runtime.setIncludes(set, key));
}

}  // namespace python
