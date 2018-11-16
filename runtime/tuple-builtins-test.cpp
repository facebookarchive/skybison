#include "gtest/gtest.h"

#include "objects.h"
#include "runtime.h"
#include "test-utils.h"
#include "tuple-builtins.h"

namespace python {

using namespace testing;

TEST(TupleBuiltinsTest, SubscriptTuple) {
  const char* src = R"(
a = 1
b = (a, 2)
print(b[0])
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n");
}

TEST(TupleBuiltinsTest, SubscriptTupleSlice) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = 1
t = (a, 2, 3, 4, 5)
slice = t[1:3]
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Object> slice(&scope, moduleAt(&runtime, main, "slice"));
  ASSERT_TRUE(slice->isObjectArray());

  Handle<ObjectArray> tuple(&scope, *slice);
  ASSERT_EQ(tuple->length(), 2);
  EXPECT_EQ(SmallInt::cast(tuple->at(0))->value(), 2);
  EXPECT_EQ(SmallInt::cast(tuple->at(1))->value(), 3);
}

TEST(TupleBuiltinsTest, DunderLen) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCStr(R"(
a = (1, 2, 3)
a_len = tuple.__len__(a)
a_len_implicit = a.__len__()
b = ()
b_len = tuple.__len__(b)
b_len_implicit = b.__len__()
)");

  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Int> a_len(&scope, moduleAt(&runtime, main, "a_len"));
  Handle<Int> a_len_implicit(&scope,
                             moduleAt(&runtime, main, "a_len_implicit"));
  Handle<Int> b_len(&scope, moduleAt(&runtime, main, "b_len"));
  Handle<Int> b_len_implicit(&scope,
                             moduleAt(&runtime, main, "b_len_implicit"));

  EXPECT_EQ(a_len->asWord(), 3);
  EXPECT_EQ(a_len->asWord(), a_len_implicit->asWord());
  EXPECT_EQ(b_len->asWord(), 0);
  EXPECT_EQ(b_len->asWord(), b_len_implicit->asWord());
}

// Equivalent to evaluating "tuple(range(start, stop))" in Python
static Object* tupleFromRange(word start, word stop) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> result(&scope,
                             thread->runtime()->newObjectArray(stop - start));
  for (word i = 0, j = start; j < stop; i++, j++) {
    result->atPut(i, SmallInt::fromWord(j));
  }
  return *result;
}

TEST(TupleBuiltinsTest, SlicePositiveStartIndex) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> tuple1(&scope, tupleFromRange(1, 6));

  // Test [2:]
  Handle<Object> start(&scope, SmallInt::fromWord(2));
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<ObjectArray> test(&scope,
                           TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 4);
  EXPECT_EQ(SmallInt::cast(test->at(2))->value(), 5);
}

TEST(TupleBuiltinsTest, SliceNegativeStartIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> tuple1(&scope, tupleFromRange(1, 6));

  // Test [-2:]
  Handle<Object> start(&scope, SmallInt::fromWord(-2));
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<ObjectArray> test(&scope,
                           TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 2);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 4);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 5);
}

TEST(TupleBuiltinsTest, SlicePositiveStopIndex) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> tuple1(&scope, tupleFromRange(1, 6));

  // Test [:2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, SmallInt::fromWord(2));
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<ObjectArray> test(&scope,
                           TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 2);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 2);
}

TEST(TupleBuiltinsTest, SliceNegativeStopIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> tuple1(&scope, tupleFromRange(1, 6));

  // Test [:-2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, SmallInt::fromWord(-2));
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<ObjectArray> test(&scope,
                           TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 2);
  EXPECT_EQ(SmallInt::cast(test->at(2))->value(), 3);
}

TEST(TupleBuiltinsTest, SlicePositiveStep) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> tuple1(&scope, tupleFromRange(1, 6));

  // Test [::-2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, SmallInt::fromWord(2));
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<ObjectArray> test(&scope,
                           TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(2))->value(), 5);
}

TEST(TupleBuiltinsTest, SliceNegativeStepReversesOrder) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> tuple1(&scope, tupleFromRange(1, 6));

  // Test [::-2]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, SmallInt::fromWord(-2));
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<ObjectArray> test(&scope,
                           TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 5);
  EXPECT_EQ(SmallInt::cast(test->at(1))->value(), 3);
  EXPECT_EQ(SmallInt::cast(test->at(2))->value(), 1);
}

TEST(TupleBuiltinsTest, SliceStartIndexOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> tuple1(&scope, tupleFromRange(1, 6));

  // Test [10:]
  Handle<Object> start(&scope, SmallInt::fromWord(10));
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<ObjectArray> test(&scope,
                           TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 0);
}

TEST(TupleBuiltinsTest, SliceStopIndexOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> tuple1(&scope, tupleFromRange(1, 6));

  // Test [:10]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, SmallInt::fromWord(10));
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<ObjectArray> test(&scope,
                           TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 5);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(4))->value(), 5);
}

TEST(TupleBuiltinsTest, SliceStepOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> tuple1(&scope, tupleFromRange(1, 6));

  // Test [::10]
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, SmallInt::fromWord(10));
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<ObjectArray> test(&scope,
                           TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 1);
  EXPECT_EQ(SmallInt::cast(test->at(0))->value(), 1);
}

TEST(TupleBuiltinsTest, IdenticalSliceIsNotCopy) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<ObjectArray> tuple1(&scope, tupleFromRange(1, 6));

  // Test: t[::] is t
  Handle<Object> start(&scope, None::object());
  Handle<Object> stop(&scope, None::object());
  Handle<Object> step(&scope, None::object());
  Handle<Slice> slice(&scope, runtime.newSlice(start, stop, step));
  Handle<ObjectArray> test1(&scope,
                            TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(*test1, *tuple1);
}

TEST(TupleBuiltinsTest, DunderNewWithNoIterableArgReturnsEmptyTuple) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  Frame* frame = thread->openAndLinkFrame(0, 1, 0);
  frame->setLocal(0, runtime.typeAt(LayoutId::kObjectArray));
  Handle<ObjectArray> ret(&scope, TupleBuiltins::dunderNew(thread, frame, 1));
  EXPECT_EQ(ret->length(), 0);
}

TEST(TupleBuiltinsTest, DunderReprWithManyPrimitives) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = (1, 2, 3).__repr__()
b = ("hello", 2, "world", 4).__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Str> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Str> b(&scope, moduleAt(&runtime, main, "b"));

  // TODO(dulinr): After int.__repr__ is implemented, fix this.
  EXPECT_PYSTRING_EQ(*a,
                     "(<smallint object at 0x2>, <smallint object at 0x4>, "
                     "<smallint object at 0x6>)");
  EXPECT_PYSTRING_EQ(
      *b,
      "('hello', <smallint object at 0x4>, 'world', <smallint object at 0x8>)");
}

TEST(TupleBuiltinsTest, DunderReprWithOnePrimitive) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = (1,).__repr__()
b = ("foo",).__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Str> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Str> b(&scope, moduleAt(&runtime, main, "b"));

  // TODO(dulinr): After int.__repr__ is implemented, fix this.
  EXPECT_PYSTRING_EQ(*a, "(<smallint object at 0x2>,)");
  EXPECT_PYSTRING_EQ(*b, "('foo',)");
}

TEST(TupleBuiltinsTest, DunderReprWithNoElements) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = ().__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Str> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "()");
}

TEST(TupleBuiltinsTest, DunderMulWithOneElement) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = (1,) * 4
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<ObjectArray> a(&scope, moduleAt(&runtime, main, "a"));

  ASSERT_EQ(a->length(), 4);
  EXPECT_EQ(SmallInt::cast(a->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(a->at(1))->value(), 1);
  EXPECT_EQ(SmallInt::cast(a->at(2))->value(), 1);
  EXPECT_EQ(SmallInt::cast(a->at(3))->value(), 1);
}

TEST(TupleBuiltinsTest, DunderMulWithManyElements) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = (1,2,3) * 2
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<ObjectArray> a(&scope, moduleAt(&runtime, main, "a"));

  ASSERT_EQ(a->length(), 6);
  EXPECT_EQ(SmallInt::cast(a->at(0))->value(), 1);
  EXPECT_EQ(SmallInt::cast(a->at(1))->value(), 2);
  EXPECT_EQ(SmallInt::cast(a->at(2))->value(), 3);
  EXPECT_EQ(SmallInt::cast(a->at(3))->value(), 1);
  EXPECT_EQ(SmallInt::cast(a->at(4))->value(), 2);
  EXPECT_EQ(SmallInt::cast(a->at(5))->value(), 3);
}

TEST(TupleBuiltinsTest, DunderMulWithEmptyTuple) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = () * 5
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<ObjectArray> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_EQ(a->length(), 0);
}

TEST(TupleBuiltinsTest, DunderMulWithNegativeTimes) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = (1,2,3) * -2
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<ObjectArray> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_EQ(a->length(), 0);
}

TEST(TupleBuiltinsTest, DunderIterReturnsTupleIter) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<ObjectArray> empty_tuple(&scope, tupleFromRange(0, 0));

  frame->setLocal(0, *empty_tuple);
  Handle<Object> iter(&scope, TupleBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isTupleIterator());
}

TEST(TupleIteratorBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<ObjectArray> tuple(&scope, tupleFromRange(0, 2));

  frame->setLocal(0, *tuple);
  Handle<Object> iter(&scope, TupleBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isTupleIterator());

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

TEST(TupleIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<ObjectArray> empty_tuple(&scope, tupleFromRange(0, 0));

  frame->setLocal(0, *empty_tuple);
  Handle<Object> iter(&scope, TupleBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isTupleIterator());

  // Now call __iter__ on the iterator object
  Handle<Object> iter_iter(
      &scope,
      Interpreter::lookupMethod(thread, frame, iter, SymbolId::kDunderIter));
  ASSERT_FALSE(iter_iter->isError());
  Handle<Object> result(
      &scope, Interpreter::callMethod1(thread, frame, iter_iter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(TupleIteratorBuiltinsTest, DunderLengthHintOnEmptyTupleIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<ObjectArray> empty_tuple(&scope, tupleFromRange(0, 0));

  frame->setLocal(0, *empty_tuple);
  Handle<Object> iter(&scope, TupleBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isTupleIterator());

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

TEST(TupleIteratorBuiltinsTest, DunderLengthHintOnConsumedTupleIterator) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(1, 0, 0);

  HandleScope scope(thread);
  Handle<ObjectArray> tuple(&scope, tupleFromRange(0, 1));

  frame->setLocal(0, *tuple);
  Handle<Object> iter(&scope, TupleBuiltins::dunderIter(thread, frame, 1));
  ASSERT_TRUE(iter->isTupleIterator());

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

}  // namespace python
