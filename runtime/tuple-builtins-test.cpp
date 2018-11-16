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

  runtime.runFromCString(R"(
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

}  // namespace python
