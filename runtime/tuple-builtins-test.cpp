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

TEST(TupleBuiltinsTest, DunderLen) {
  Runtime runtime;
  HandleScope scope;

  runtime.runFromCString(R"(
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
  runtime.runFromCString(R"(
a = (1, 2, 3).__repr__()
b = ("hello", 2, "world", 4).__repr__()
c = (1,).__repr__()
d = ("foo",).__repr__()
e = ().__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<String> b(&scope, moduleAt(&runtime, main, "b"));
  Handle<String> c(&scope, moduleAt(&runtime, main, "c"));
  Handle<String> d(&scope, moduleAt(&runtime, main, "d"));
  Handle<String> e(&scope, moduleAt(&runtime, main, "e"));

  // TODO(dulinr): After int.__repr__ and str.__repr__ are implemented, fix
  // these.
  EXPECT_PYSTRING_EQ(*a,
                     "(<smallint object at 0x2>, <smallint object at 0x4>, "
                     "<smallint object at 0x6>)");
  EXPECT_PYSTRING_EQ(
      *b,
      "(<smallstr object at 0x6f6c6c6568bf>, <smallint object at 0x4>, "
      "<smallstr object at 0x646c726f77bf>, <smallint object at 0x8>)");
  EXPECT_PYSTRING_EQ(*c, "(<smallint object at 0x2>,)");
  EXPECT_PYSTRING_EQ(*d, "(<smallstr object at 0x6f6f667f>,)");
  EXPECT_PYSTRING_EQ(*e, "()");
}

TEST(TupleBuiltinsTest, DunderReprWithOnePrimitive) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = (1,).__repr__()
b = ("foo",).__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<String> b(&scope, moduleAt(&runtime, main, "b"));

  // TODO(dulinr): After int.__repr__ and str.__repr__ are implemented, fix
  // these.
  EXPECT_PYSTRING_EQ(*a, "(<smallint object at 0x2>,)");
  EXPECT_PYSTRING_EQ(*b, "(<smallstr object at 0x6f6f667f>,)");
}

TEST(TupleBuiltinsTest, DunderReprWithNoElements) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = ().__repr__()
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));

  EXPECT_PYSTRING_EQ(*a, "()");
}

}  // namespace python
