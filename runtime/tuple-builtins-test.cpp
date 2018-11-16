#include "gtest/gtest.h"

#include "objects.h"
#include "runtime.h"
#include "test-utils.h"
#include "tuple-builtins.h"

namespace python {

using namespace testing;

TEST(TupleBuiltinsTest, SubscriptTuple) {
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, R"(
a = 1
b = (a, 2)
print(b[0])
)");
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

  Object slice(&scope, moduleAt(&runtime, "__main__", "slice"));
  ASSERT_TRUE(slice->isObjectArray());

  ObjectArray tuple(&scope, *slice);
  ASSERT_EQ(tuple->length(), 2);
  EXPECT_EQ(RawSmallInt::cast(tuple->at(0))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(tuple->at(1))->value(), 3);
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

  Int a_len(&scope, moduleAt(&runtime, "__main__", "a_len"));
  Int a_len_implicit(&scope, moduleAt(&runtime, "__main__", "a_len_implicit"));
  Int b_len(&scope, moduleAt(&runtime, "__main__", "b_len"));
  Int b_len_implicit(&scope, moduleAt(&runtime, "__main__", "b_len_implicit"));

  EXPECT_EQ(a_len->asWord(), 3);
  EXPECT_EQ(a_len->asWord(), a_len_implicit->asWord());
  EXPECT_EQ(b_len->asWord(), 0);
  EXPECT_EQ(b_len->asWord(), b_len_implicit->asWord());
}

// Equivalent to evaluating "tuple(range(start, stop))" in Python
static RawObject tupleFromRange(word start, word stop) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray result(&scope, thread->runtime()->newObjectArray(stop - start));
  for (word i = 0, j = start; j < stop; i++, j++) {
    result->atPut(i, SmallInt::fromWord(j));
  }
  return *result;
}

TEST(TupleBuiltinsTest, SlicePositiveStartIndex) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray tuple1(&scope, tupleFromRange(1, 6));

  // Test [2:]
  Object start(&scope, SmallInt::fromWord(2));
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  ObjectArray test(&scope, TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 4);
  EXPECT_EQ(RawSmallInt::cast(test->at(2))->value(), 5);
}

TEST(TupleBuiltinsTest, SliceNegativeStartIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray tuple1(&scope, tupleFromRange(1, 6));

  // Test [-2:]
  Object start(&scope, SmallInt::fromWord(-2));
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  ObjectArray test(&scope, TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 2);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 4);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 5);
}

TEST(TupleBuiltinsTest, SlicePositiveStopIndex) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray tuple1(&scope, tupleFromRange(1, 6));

  // Test [:2]
  Object start(&scope, NoneType::object());
  Object stop(&scope, SmallInt::fromWord(2));
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  ObjectArray test(&scope, TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 2);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 2);
}

TEST(TupleBuiltinsTest, SliceNegativeStopIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray tuple1(&scope, tupleFromRange(1, 6));

  // Test [:-2]
  Object start(&scope, NoneType::object());
  Object stop(&scope, SmallInt::fromWord(-2));
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  ObjectArray test(&scope, TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(test->at(2))->value(), 3);
}

TEST(TupleBuiltinsTest, SlicePositiveStep) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray tuple1(&scope, tupleFromRange(1, 6));

  // Test [::-2]
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, SmallInt::fromWord(2));
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  ObjectArray test(&scope, TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(2))->value(), 5);
}

TEST(TupleBuiltinsTest, SliceNegativeStepReversesOrder) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray tuple1(&scope, tupleFromRange(1, 6));

  // Test [::-2]
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, SmallInt::fromWord(-2));
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  ObjectArray test(&scope, TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 5);
  EXPECT_EQ(RawSmallInt::cast(test->at(1))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(test->at(2))->value(), 1);
}

TEST(TupleBuiltinsTest, SliceStartIndexOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray tuple1(&scope, tupleFromRange(1, 6));

  // Test [10:]
  Object start(&scope, SmallInt::fromWord(10));
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  ObjectArray test(&scope, TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 0);
}

TEST(TupleBuiltinsTest, SliceStopIndexOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray tuple1(&scope, tupleFromRange(1, 6));

  // Test [:10]
  Object start(&scope, NoneType::object());
  Object stop(&scope, SmallInt::fromWord(10));
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  ObjectArray test(&scope, TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 5);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(4))->value(), 5);
}

TEST(TupleBuiltinsTest, SliceStepOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray tuple1(&scope, tupleFromRange(1, 6));

  // Test [::10]
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, SmallInt::fromWord(10));
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  ObjectArray test(&scope, TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(test->length(), 1);
  EXPECT_EQ(RawSmallInt::cast(test->at(0))->value(), 1);
}

TEST(TupleBuiltinsTest, IdenticalSliceIsNotCopy) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  ObjectArray tuple1(&scope, tupleFromRange(1, 6));

  // Test: t[::] is t
  Object start(&scope, NoneType::object());
  Object stop(&scope, NoneType::object());
  Object step(&scope, NoneType::object());
  Slice slice(&scope, runtime.newSlice(start, stop, step));
  ObjectArray test1(&scope, TupleBuiltins::slice(thread, *tuple1, *slice));
  ASSERT_EQ(*test1, *tuple1);
}

TEST(TupleBuiltinsTest, DunderNewWithNoIterableArgReturnsEmptyTuple) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kObjectArray));
  ObjectArray ret(&scope, runBuiltin(TupleBuiltins::dunderNew, type));
  EXPECT_EQ(ret->length(), 0);
}

TEST(TupleBuiltinsTest, DunderNewWithIterableReturnsTuple) {
  Runtime runtime;
  HandleScope scope;
  runtime.runFromCStr(R"(
a = tuple.__new__(tuple, [1, 2, 3])
)");
  ObjectArray a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a->length(), 3);
  EXPECT_EQ(RawSmallInt::cast(a->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(a->at(1))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(a->at(2))->value(), 3);
}

TEST(TupleBuiltinsTest, DunderReprWithManyPrimitives) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = (1, 2, 3).__repr__()
b = ("hello", 2, "world", 4).__repr__()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  Str b(&scope, moduleAt(&runtime, "__main__", "b"));

  EXPECT_PYSTRING_EQ(*a, "(1, 2, 3)");
  EXPECT_PYSTRING_EQ(*b, "('hello', 2, 'world', 4)");
}

TEST(TupleBuiltinsTest, DunderReprWithOnePrimitive) {
  Runtime runtime;
  runtime.runFromCStr(R"(
a = (1,).__repr__()
b = ("foo",).__repr__()
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  Str b(&scope, moduleAt(&runtime, "__main__", "b"));

  EXPECT_PYSTRING_EQ(*a, "(1,)");
  EXPECT_PYSTRING_EQ(*b, "('foo',)");
}

TEST(TupleBuiltinsTest, DunderReprWithNoElements) {
  Runtime runtime;
  runtime.runFromCStr("a = ().__repr__()");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_PYSTRING_EQ(*a, "()");
}

TEST(TupleBuiltinsTest, DunderMulWithOneElement) {
  Runtime runtime;
  runtime.runFromCStr("a = (1,) * 4");
  HandleScope scope;
  ObjectArray a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a->length(), 4);
  EXPECT_EQ(RawSmallInt::cast(a->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(a->at(1))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(a->at(2))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(a->at(3))->value(), 1);
}

TEST(TupleBuiltinsTest, DunderMulWithManyElements) {
  Runtime runtime;
  runtime.runFromCStr("a = (1,2,3) * 2");
  HandleScope scope;
  ObjectArray a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a->length(), 6);
  EXPECT_EQ(RawSmallInt::cast(a->at(0))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(a->at(1))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(a->at(2))->value(), 3);
  EXPECT_EQ(RawSmallInt::cast(a->at(3))->value(), 1);
  EXPECT_EQ(RawSmallInt::cast(a->at(4))->value(), 2);
  EXPECT_EQ(RawSmallInt::cast(a->at(5))->value(), 3);
}

TEST(TupleBuiltinsTest, DunderMulWithEmptyTuple) {
  Runtime runtime;
  runtime.runFromCStr("a = () * 5");
  HandleScope scope;
  ObjectArray a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_EQ(a->length(), 0);
}

TEST(TupleBuiltinsTest, DunderMulWithNegativeTimes) {
  Runtime runtime;
  runtime.runFromCStr("a = (1,2,3) * -2");
  HandleScope scope;
  ObjectArray a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_EQ(a->length(), 0);
}

TEST(TupleBuiltinsTest, DunderIterReturnsTupleIter) {
  Runtime runtime;
  HandleScope scope;
  ObjectArray empty_tuple(&scope, tupleFromRange(0, 0));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, empty_tuple));
  ASSERT_TRUE(iter->isTupleIterator());
}

TEST(TupleIteratorBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  HandleScope scope;
  ObjectArray tuple(&scope, tupleFromRange(0, 2));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, tuple));
  ASSERT_TRUE(iter->isTupleIterator());

  Object item1(&scope, runBuiltin(TupleIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object item2(&scope, runBuiltin(TupleIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item2->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item2)->value(), 1);
}

TEST(TupleIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  ObjectArray empty_tuple(&scope, tupleFromRange(0, 0));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, empty_tuple));
  ASSERT_TRUE(iter->isTupleIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(TupleIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(TupleIteratorBuiltinsTest,
     DunderLengthHintOnEmptyTupleIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  ObjectArray empty_tuple(&scope, tupleFromRange(0, 0));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, empty_tuple));

  Object length_hint(&scope,
                     runBuiltin(TupleIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint)->value(), 0);
}

TEST(TupleIteratorBuiltinsTest, DunderLengthHintOnConsumedTupleIterator) {
  Runtime runtime;
  HandleScope scope;
  ObjectArray tuple(&scope, tupleFromRange(0, 1));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, tuple));

  Object length_hint1(
      &scope, runBuiltin(TupleIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint1)->value(), 1);

  // Consume the iterator
  Object item1(&scope, runBuiltin(TupleIteratorBuiltins::dunderNext, iter));
  ASSERT_TRUE(item1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*item1)->value(), 0);

  Object length_hint2(
      &scope, runBuiltin(TupleIteratorBuiltins::dunderLengthHint, iter));
  ASSERT_TRUE(length_hint1->isSmallInt());
  ASSERT_EQ(RawSmallInt::cast(*length_hint2)->value(), 0);
}

}  // namespace python
