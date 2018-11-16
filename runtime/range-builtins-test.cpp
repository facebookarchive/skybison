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
  Handle<Object> empty_range(&scope, runtime.newRange(0, 0, 1));

  frame->setLocal(0, *empty_range);
  Handle<Object> iter(&scope, RangeBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isRangeIterator());
}

TEST(RangeBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<Range> non_empty_range(&scope, runtime.newRange(0, 2, 1));

  frame->setLocal(0, *non_empty_range);
  Handle<Object> iter(&scope, RangeBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isRangeIterator());

  Handle<Object> next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), iter,
                                        SymbolId::kDunderNext));
  ASSERT_FALSE(next_method->isError());

  Handle<Object> item1(
      &scope, Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*item1)->value(), 0);

  Handle<Object> item2(
      &scope, Interpreter::callMethod1(thread, frame, next_method, iter));
  ASSERT_TRUE(item2->isSmallInt());
  ASSERT_EQ(SmallInt::cast(*item2)->value(), 1);
}

TEST(RangeIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 1, 0);

  HandleScope scope(thread);
  Handle<Object> empty_range(&scope, runtime.newRange(0, 0, 1));

  frame->setLocal(0, *empty_range);
  Handle<Object> iter(&scope, RangeBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isRangeIterator());

  // Now call __iter__ on the iterator object
  Handle<Object> iter_iter(
      &scope,
      Interpreter::lookupMethod(thread, frame, iter, SymbolId::kDunderIter));
  ASSERT_FALSE(iter_iter->isError());
  Handle<Object> result(
      &scope, Interpreter::callMethod1(thread, frame, iter_iter, iter));
  ASSERT_EQ(*result, *iter);
}

}  // namespace python
