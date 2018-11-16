#include "gtest/gtest.h"

#include "frame.h"
#include "objects.h"
#include "range-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(RangeBuiltinsTest, DunderIterReturnsRangeIter) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  HandleScope scope(thread);
  Object empty_range(&scope, runtime.newRange(0, 0, 1));

  frame->setLocal(0, *empty_range);
  Object iter(&scope, RangeBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isRangeIterator());
}

TEST(RangeBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Range non_empty_range(&scope, runtime.newRange(0, 2, 1));

  frame->setLocal(0, *non_empty_range);
  Object iter(&scope, RangeBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isRangeIterator());

  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());

  Object item1(&scope,
               Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object item2(&scope,
               Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item2->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item2)->value(), 1);
}

TEST(RangeIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  HandleScope scope(thread);
  Object empty_range(&scope, runtime.newRange(0, 0, 1));

  frame->setLocal(0, *empty_range);
  Object iter(&scope, RangeBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isRangeIterator());

  // Now call __iter__ on the iterator object
  Object iter_iter(&scope, Interpreter::lookupMethod(thread, frame, iter,
                                                     SymbolId::kDunderIter));
  ASSERT_FALSE(iter_iter->isError());
  Object result(&scope,
                Interpreter::callMethod1(thread, frame, iter_iter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(RangeIteratorBuiltinsTest, DunderLengthHintReturnsPendingLength) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  HandleScope scope(thread);
  Object empty_range(&scope, runtime.newRange(0, 0, 1));

  frame->setLocal(0, *empty_range);
  Object iter(&scope, RangeBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isRangeIterator());

  Object length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderLengthHint));
  ASSERT_FALSE(length_hint_method->isError());

  Object length_hint1(&scope, Interpreter::callMethod1(
                                  thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint1)->value(), 0);

  RawRangeIterator::cast(*iter)->setRange(runtime.newRange(0, 1, 1));
  Object length_hint2(&scope, Interpreter::callMethod1(
                                  thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint2->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint2)->value(), 1);

  // Consume the iterator
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());
  Object item1(&scope,
               Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object length_hint3(&scope, Interpreter::callMethod1(
                                  thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint3->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint3)->value(), 0);
}

TEST(RangeIteratorBuiltinsTest, DunderLengthHintWithNegativeStepRange) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  HandleScope scope(thread);
  Object neg_range(&scope, runtime.newRange(0, -2, -1));
  frame->setLocal(0, *neg_range);

  Object iter(&scope, RangeBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isRangeIterator());

  Object length_hint_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderLengthHint));
  ASSERT_FALSE(length_hint_method->isError());

  Object length_hint1(&scope, Interpreter::callMethod1(
                                  thread, frame, length_hint_method, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint1)->value(), 2);

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
  ASSERT_TRUE(length_hint2->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint2)->value(), 1);
}

}  // namespace python
