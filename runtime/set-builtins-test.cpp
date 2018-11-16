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

// Equivalent to evaluating "set(range(start, stop))" in Python
static RawObject setFromRange(word start, word stop) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set result(&scope, thread->runtime()->newSet());
  Object value(&scope, NoneType::object());
  for (word i = start; i < stop; i++) {
    value = SmallInt::fromWord(i);
    thread->runtime()->setAdd(result, value);
  }
  return *result;
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

}  // namespace python
