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
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Object> b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_EQ(SmallInt::cast(*a)->value(), 1);
  EXPECT_EQ(SmallInt::cast(*b)->value(), 0);
}

TEST(SetBuiltinsTest, InitializeByTypeCall) {
  const char* src = R"(
s = set()
)";
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(src);
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> s(&scope, moduleAt(&runtime, main, "s"));
  EXPECT_TRUE(s->isSet());
  EXPECT_EQ(Set::cast(*s)->numItems(), 0);
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
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Set> s(&scope, moduleAt(&runtime, main, "s"));
  Handle<Object> one(&scope, runtime.newInt(1));
  Handle<Object> hello_world(&scope, runtime.newStrFromCStr("Hello, World"));
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
  Handle<Set> empty_set(&scope, runtime.newSet());

  frame->setLocal(0, *empty_set);
  Handle<Object> iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());
}

TEST(SetIteratorBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<Set> set(&scope, runtime.newSet());
  Handle<Object> value(&scope, SmallInt::fromWord(0));
  runtime.setAdd(set, value);
  value = SmallInt::fromWord(1);
  runtime.setAdd(set, value);

  frame->setLocal(0, *set);
  Handle<Object> iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());

  Handle<Object> next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());

  Handle<Object> item1(
      &scope, Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*item1)->value(), 0);

  Handle<Object> item2(
      &scope, Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item2->isSmallInt());
  EXPECT_EQ(SmallInt::cast(*item2)->value(), 1);

  Handle<Object> item3(
      &scope, Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item3->isError());
}

TEST(SetIteratorBuiltinsTest, CallDunderNextWithEmptySet) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<Set> set(&scope, runtime.newSet());

  frame->setLocal(0, *set);
  Handle<Object> iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());

  Handle<Object> next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());

  Handle<Object> result(
      &scope, Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(result->isError());
}

TEST(SetIteratorBuiltinsTes, DunderIterReturnsSelf) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<Set> empty_set(&scope, runtime.newSet());

  frame->setLocal(0, *empty_set);
  Handle<Object> iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());

  // Now call __iter__ on the iterator object
  Handle<Object> iter_iter(
      &scope,
      Interpreter::lookupMethod(thread, frame, iter, SymbolId::kDunderIter));
  ASSERT_FALSE(iter_iter->isError());
  Handle<Object> result(
      &scope, Interpreter::callMethod1(thread, frame, iter_iter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(SetIteratorBuiltinsTest, DunderLengthHintOnEmptyTupleIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<Set> empty_set(&scope, runtime.newSet());

  frame->setLocal(0, *empty_set);
  Handle<Object> iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());

  Handle<Object> length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderLengthHint));
  ASSERT_FALSE(length_hint_method->isError());

  Handle<Object> length_hint(
      &scope,
      Interpreter::callMethod1(thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*length_hint)->value(), 0);
}

TEST(SetIteratorBuiltinsTest, DunderLengthHintOnConsumedTupleIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<Set> one_element_set(&scope, runtime.newSet());
  Handle<Object> zero(&scope, SmallInt::fromWord(0));
  runtime.setAdd(one_element_set, zero);

  frame->setLocal(0, *one_element_set);
  Handle<Object> iter(&scope, SetBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isSetIterator());

  Handle<Object> length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderLengthHint));
  ASSERT_FALSE(length_hint_method->isError());

  Handle<Object> length_hint1(
      &scope,
      Interpreter::callMethod1(thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*length_hint1)->value(), 1);

  // Consume the iterator
  Handle<Object> next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());
  Handle<Object> item1(
      &scope, Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*item1)->value(), 0);

  Handle<Object> length_hint2(
      &scope,
      Interpreter::callMethod1(thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*length_hint2)->value(), 0);
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

  Handle<Set> set(&scope, runtime.newSet());
  Handle<Set> other(&scope, runtime.newSet());
  Handle<Object> value(&scope, None::object());

  // set().isdisjoint(set())
  frame->setLocal(0, *set);
  frame->setLocal(1, *other);
  Handle<Object> result(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result->isBool());
  EXPECT_EQ(*result, Bool::trueObj());

  // set().isdisjoint({None})
  runtime.setAdd(other, value);
  Handle<Object> result1(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result1->isBool());
  EXPECT_EQ(*result1, Bool::trueObj());

  // {None}.isdisjoint({None})
  runtime.setAdd(set, value);
  Handle<Object> result2(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result2->isBool());
  EXPECT_EQ(*result2, Bool::falseObj());

  // {None}.isdisjoint({1})
  other = runtime.newSet();
  value = SmallInt::fromWord(1);
  runtime.setAdd(other, value);
  frame->setLocal(1, *other);
  Handle<Object> result3(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result3->isBool());
  EXPECT_EQ(*result3, Bool::trueObj());

  thread->popFrame();
}

TEST(SetBuiltinsTest, SetIsDisjointWithIterableArg) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);

  Handle<Set> set(&scope, runtime.newSet());
  Handle<List> other(&scope, runtime.newList());
  Handle<Object> value(&scope, None::object());

  // set().isdisjoint([])
  frame->setLocal(0, *set);
  frame->setLocal(1, *other);
  Handle<Object> result(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result->isBool());
  EXPECT_EQ(*result, Bool::trueObj());

  // set().isdisjoint([None])
  runtime.listAdd(other, value);
  Handle<Object> result1(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result1->isBool());
  EXPECT_EQ(*result1, Bool::trueObj());

  // {None}.isdisjoint([None])
  runtime.setAdd(set, value);
  Handle<Object> result2(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result2->isBool());
  EXPECT_EQ(*result2, Bool::falseObj());

  // {None}.isdisjoint([1])
  other = runtime.newList();
  value = SmallInt::fromWord(1);
  runtime.listAdd(other, value);
  frame->setLocal(1, *other);
  Handle<Object> result3(&scope, SetBuiltins::isDisjoint(thread, frame, 2));
  ASSERT_TRUE(result3->isBool());
  EXPECT_EQ(*result3, Bool::trueObj());

  thread->popFrame();
}

}  // namespace python
