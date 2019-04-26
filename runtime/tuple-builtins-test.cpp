#include "gtest/gtest.h"

#include "objects.h"
#include "runtime.h"
#include "test-utils.h"
#include "tuple-builtins.h"

namespace python {

using namespace testing;

TEST(TupleBuiltinsTest, TupleSubclassIsInstanceOfTuple) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(tuple): pass
a = Foo()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(runtime.isInstanceOfTuple(*a));
}

TEST(TupleBuiltinsTest, TupleSubclassHasTupleAttribute) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(tuple): pass
a = Foo()
)")
                   .isError());
  HandleScope scope;
  UserTupleBase a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object obj(&scope, a.tupleValue());
  EXPECT_TRUE(obj.isTuple());
}

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

  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = 1
t = (a, 2, 3, 4, 5)
slice = t[1:3]
)")
                   .isError());

  Object slice(&scope, moduleAt(&runtime, "__main__", "slice"));
  ASSERT_TRUE(slice.isTuple());

  Tuple tuple(&scope, *slice);
  ASSERT_EQ(tuple.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 2));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 3));
}

TEST(TupleBuiltinsTest, SubscriptWithTupleSubclassReturnsValue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(tuple): pass
obj = Foo((0, 1))
item = obj[0]
)")
                   .isError());
  HandleScope scope;
  Object item(&scope, moduleAt(&runtime, "__main__", "item"));
  EXPECT_TRUE(isIntEqualsWord(*item, 0));
}

TEST(TupleBuiltinsTest, SubscriptWithTupleSubclassReturnsSliceValue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(tuple): pass
obj = Foo((0, 1, 2, 3, 4))
slice = obj[1:3]
)")
                   .isError());
  HandleScope scope;
  Object slice(&scope, moduleAt(&runtime, "__main__", "slice"));
  ASSERT_TRUE(slice.isTuple());

  Tuple tuple(&scope, *slice);
  ASSERT_EQ(tuple.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(tuple.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(tuple.at(1), 2));
}

TEST(TupleBuiltinsTest, DunderGetItemWithIndexMinusOneReturnsLastValue) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, runtime.newTuple(2));
  tuple.atPut(0, runtime.newInt(42));
  tuple.atPut(1, runtime.newInt(7));
  Object index(&scope, runtime.newInt(-1));
  Object result(&scope, runBuiltin(TupleBuiltins::dunderGetItem, tuple, index));
  EXPECT_TRUE(isIntEqualsWord(*result, 7));
}

TEST(TupleBuiltinsTest, DunderLen) {
  Runtime runtime;
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = (1, 2, 3)
a_len = tuple.__len__(a)
a_len_implicit = a.__len__()
b = ()
b_len = tuple.__len__(b)
b_len_implicit = b.__len__()
)")
                   .isError());

  Object a_len(&scope, moduleAt(&runtime, "__main__", "a_len"));
  Object a_len_implicit(&scope,
                        moduleAt(&runtime, "__main__", "a_len_implicit"));
  Object b_len(&scope, moduleAt(&runtime, "__main__", "b_len"));
  Object b_len_implicit(&scope,
                        moduleAt(&runtime, "__main__", "b_len_implicit"));

  EXPECT_TRUE(isIntEqualsWord(*a_len, 3));
  EXPECT_TRUE(isIntEqualsWord(*a_len_implicit, 3));
  EXPECT_TRUE(isIntEqualsWord(*b_len, 0));
  EXPECT_TRUE(isIntEqualsWord(*b_len_implicit, 0));
}

TEST(TupleBuiltinsTest, DunderLenWithTupleSubclassReturnsLen) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(tuple): pass
a = Foo((1, 2, 3))
a_len = tuple.__len__(a)
a_len_implicit = a.__len__()
b = Foo(())
b_len = tuple.__len__(b)
b_len_implicit = b.__len__()
)")
                   .isError());

  HandleScope scope;
  Object a_len(&scope, moduleAt(&runtime, "__main__", "a_len"));
  Object a_len_implicit(&scope,
                        moduleAt(&runtime, "__main__", "a_len_implicit"));
  Object b_len(&scope, moduleAt(&runtime, "__main__", "b_len"));
  Object b_len_implicit(&scope,
                        moduleAt(&runtime, "__main__", "b_len_implicit"));

  EXPECT_TRUE(isIntEqualsWord(*a_len, 3));
  EXPECT_TRUE(isIntEqualsWord(*a_len_implicit, 3));
  EXPECT_TRUE(isIntEqualsWord(*b_len, 0));
  EXPECT_TRUE(isIntEqualsWord(*b_len_implicit, 0));
}

// Equivalent to evaluating "tuple(range(start, stop))" in Python
static RawObject tupleFromRange(word start, word stop) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple result(&scope, thread->runtime()->newTuple(stop - start));
  for (word i = 0, j = start; j < stop; i++, j++) {
    result.atPut(i, SmallInt::fromWord(j));
  }
  return *result;
}

TEST(TupleBuiltinsTest, SlicePositiveStartIndex) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple1(&scope, tupleFromRange(1, 6));

  // Test [2:]
  Slice slice(&scope, runtime.newSlice());
  slice.setStart(SmallInt::fromWord(2));
  Tuple test(&scope, TupleBuiltins::slice(thread, tuple1, slice));
  ASSERT_EQ(test.length(), 3);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 3));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 4));
  EXPECT_TRUE(isIntEqualsWord(test.at(2), 5));
}

TEST(TupleBuiltinsTest, SliceNegativeStartIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple1(&scope, tupleFromRange(1, 6));

  // Test [-2:]
  Slice slice(&scope, runtime.newSlice());
  slice.setStart(SmallInt::fromWord(-2));
  Tuple test(&scope, TupleBuiltins::slice(thread, tuple1, slice));
  ASSERT_EQ(test.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 4));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 5));
}

TEST(TupleBuiltinsTest, SlicePositiveStopIndex) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple1(&scope, tupleFromRange(1, 6));

  // Test [:2]
  Slice slice(&scope, runtime.newSlice());
  slice.setStop(SmallInt::fromWord(2));
  Tuple test(&scope, TupleBuiltins::slice(thread, tuple1, slice));
  ASSERT_EQ(test.length(), 2);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 2));
}

TEST(TupleBuiltinsTest, SliceNegativeStopIndexIsRelativeToEnd) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple1(&scope, tupleFromRange(1, 6));

  // Test [:-2]
  Slice slice(&scope, runtime.newSlice());
  slice.setStop(SmallInt::fromWord(-2));
  Tuple test(&scope, TupleBuiltins::slice(thread, tuple1, slice));
  ASSERT_EQ(test.length(), 3);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(test.at(2), 3));
}

TEST(TupleBuiltinsTest, SlicePositiveStep) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple1(&scope, tupleFromRange(1, 6));

  // Test [::2]
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(2));
  Tuple test(&scope, TupleBuiltins::slice(thread, tuple1, slice));
  ASSERT_EQ(test.length(), 3);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 3));
  EXPECT_TRUE(isIntEqualsWord(test.at(2), 5));
}

TEST(TupleBuiltinsTest, SliceNegativeStepReversesOrder) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple1(&scope, tupleFromRange(1, 6));

  // Test [::-2]
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(-2));
  Tuple test(&scope, TupleBuiltins::slice(thread, tuple1, slice));
  ASSERT_EQ(test.length(), 3);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 5));
  EXPECT_TRUE(isIntEqualsWord(test.at(1), 3));
  EXPECT_TRUE(isIntEqualsWord(test.at(2), 1));
}

TEST(TupleBuiltinsTest, SliceStartIndexOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple1(&scope, tupleFromRange(1, 6));

  // Test [10:]
  Slice slice(&scope, runtime.newSlice());
  slice.setStart(SmallInt::fromWord(10));
  Tuple test(&scope, TupleBuiltins::slice(thread, tuple1, slice));
  ASSERT_EQ(test.length(), 0);
}

TEST(TupleBuiltinsTest, SliceStopIndexOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple1(&scope, tupleFromRange(1, 6));

  // Test [:10]
  Slice slice(&scope, runtime.newSlice());
  slice.setStop(SmallInt::fromWord(10));
  Tuple test(&scope, TupleBuiltins::slice(thread, tuple1, slice));
  ASSERT_EQ(test.length(), 5);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(test.at(4), 5));
}

TEST(TupleBuiltinsTest, SliceStepOutOfBounds) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple1(&scope, tupleFromRange(1, 6));

  // Test [::10]
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(10));
  Tuple test(&scope, TupleBuiltins::slice(thread, tuple1, slice));
  ASSERT_EQ(test.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(test.at(0), 1));
}

TEST(TupleBuiltinsTest, IdenticalSliceIsNotCopy) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Tuple tuple1(&scope, tupleFromRange(1, 6));

  // Test: t[::] is t
  Slice slice(&scope, runtime.newSlice());
  Tuple test1(&scope, TupleBuiltins::slice(thread, tuple1, slice));
  ASSERT_EQ(*test1, *tuple1);
}

TEST(TupleBuiltinsTest, DunderNewWithNoIterableArgReturnsEmptyTuple) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = tuple.__new__(tuple)");
  Tuple ret(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(ret.length(), 0);
}

TEST(TupleBuiltinsTest, DunderNewWithIterableReturnsTuple) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = tuple.__new__(tuple, [1, 2, 3])
)")
                   .isError());
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a.length(), 3);
  EXPECT_TRUE(isIntEqualsWord(a.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(a.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(a.at(2), 3));
}

TEST(TupleBuiltinsTest, DunderReprWithManyPrimitives) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = (1, 2, 3).__repr__()
b = ("hello", 2, "world", 4).__repr__()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "(1, 2, 3)"));
  EXPECT_TRUE(isStrEqualsCStr(*b, "('hello', 2, 'world', 4)"));
}

TEST(TupleBuiltinsTest, DunderReprWithOnePrimitive) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = (1,).__repr__()
b = ("foo",).__repr__()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "(1,)"));
  EXPECT_TRUE(isStrEqualsCStr(*b, "('foo',)"));
}

TEST(TupleBuiltinsTest, DunderReprWithNoElements) {
  Runtime runtime;
  runFromCStr(&runtime, "a = ().__repr__()");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_TRUE(isStrEqualsCStr(*a, "()"));
}

TEST(TupleBuiltinsTest, DunderReprWithTupleSubclassReturnsTupleRepr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(tuple): pass
repr = Foo((1, 2, 3)).__repr__()
)")
                   .isError());
  HandleScope scope;
  Object repr(&scope, moduleAt(&runtime, "__main__", "repr"));
  EXPECT_TRUE(isStrEqualsCStr(*repr, "(1, 2, 3)"));
}

TEST(TupleBuiltinsTest, DunderMulWithOneElement) {
  Runtime runtime;
  runFromCStr(&runtime, "a = (1,) * 4");
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(a.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(a.at(1), 1));
  EXPECT_TRUE(isIntEqualsWord(a.at(2), 1));
  EXPECT_TRUE(isIntEqualsWord(a.at(3), 1));
}

TEST(TupleBuiltinsTest, DunderMulWithManyElements) {
  Runtime runtime;
  runFromCStr(&runtime, "a = (1,2,3) * 2");
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  ASSERT_EQ(a.length(), 6);
  EXPECT_TRUE(isIntEqualsWord(a.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(a.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(a.at(2), 3));
  EXPECT_TRUE(isIntEqualsWord(a.at(3), 1));
  EXPECT_TRUE(isIntEqualsWord(a.at(4), 2));
  EXPECT_TRUE(isIntEqualsWord(a.at(5), 3));
}

TEST(TupleBuiltinsTest, DunderMulWithEmptyTuple) {
  Runtime runtime;
  runFromCStr(&runtime, "a = () * 5");
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_EQ(a.length(), 0);
}

TEST(TupleBuiltinsTest, DunderMulWithNegativeTimes) {
  Runtime runtime;
  runFromCStr(&runtime, "a = (1,2,3) * -2");
  HandleScope scope;
  Tuple a(&scope, moduleAt(&runtime, "__main__", "a"));

  EXPECT_EQ(a.length(), 0);
}

TEST(TupleBuiltinsTest, DunderMulWithTupleSubclassReturnsTuple) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(tuple): pass
a = Foo((1, 2, 3)) * 2
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a.isTuple());
}

TEST(TupleBuiltinsTest, DunderAddWithNonTupleLeftHandSideReturnsError) {
  Runtime runtime;
  HandleScope scope;
  Tuple empty_tuple(&scope, tupleFromRange(0, 0));
  Int zero(&scope, runtime.newInt(0));
  Object error(&scope, runBuiltin(TupleBuiltins::dunderAdd, empty_tuple, zero));
  ASSERT_TRUE(error.isError());
  EXPECT_EQ(Thread::current()->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
}

TEST(TupleBuiltinsTest, DunderAddWithNonTupleRightHandSideReturnsError) {
  Runtime runtime;
  HandleScope scope;
  Tuple empty_tuple(&scope, tupleFromRange(0, 0));
  Int zero(&scope, runtime.newInt(0));
  Object error(&scope, runBuiltin(TupleBuiltins::dunderAdd, zero, empty_tuple));
  ASSERT_TRUE(error.isError());
  EXPECT_EQ(Thread::current()->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
}

TEST(TupleBuiltinsTest, DunderAddWithEmptyTupleReturnsTuple) {
  Runtime runtime;
  HandleScope scope;
  Tuple empty_tuple(&scope, tupleFromRange(0, 0));
  Tuple one_tuple(&scope, tupleFromRange(1, 2));
  Object lhs_result(
      &scope, runBuiltin(TupleBuiltins::dunderAdd, empty_tuple, one_tuple));
  Object rhs_result(
      &scope, runBuiltin(TupleBuiltins::dunderAdd, empty_tuple, one_tuple));

  ASSERT_TRUE(lhs_result.isTuple());
  Tuple lhs_tuple(&scope, *lhs_result);
  EXPECT_EQ(lhs_tuple.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(lhs_tuple.at(0), 1));

  ASSERT_TRUE(rhs_result.isTuple());
  Tuple rhs_tuple(&scope, *rhs_result);
  EXPECT_EQ(rhs_tuple.length(), 1);
  EXPECT_TRUE(isIntEqualsWord(rhs_tuple.at(0), 1));
}

TEST(TupleBuiltinsTest, DunderAddWithManyElementsReturnsTuple) {
  Runtime runtime;
  HandleScope scope;
  Tuple lhs(&scope, tupleFromRange(1, 4));
  Tuple rhs(&scope, tupleFromRange(4, 8));
  Object result(&scope, runBuiltin(TupleBuiltins::dunderAdd, lhs, rhs));
  ASSERT_TRUE(result.isTuple());
  Tuple a(&scope, *result);

  EXPECT_EQ(a.length(), 7);
  EXPECT_TRUE(isIntEqualsWord(a.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(a.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(a.at(2), 3));
  EXPECT_TRUE(isIntEqualsWord(a.at(3), 4));
  EXPECT_TRUE(isIntEqualsWord(a.at(4), 5));
  EXPECT_TRUE(isIntEqualsWord(a.at(5), 6));
  EXPECT_TRUE(isIntEqualsWord(a.at(6), 7));
}

TEST(TupleBuiltinsTest, DunderAddWithTupleSubclassReturnsTuple) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(tuple): pass
a = Foo((1, 2)) + (3, 4)
)")
                   .isError());
  HandleScope scope;
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a_obj.isTuple());
  Tuple a(&scope, *a_obj);
  EXPECT_EQ(a.length(), 4);
  EXPECT_TRUE(isIntEqualsWord(a.at(0), 1));
  EXPECT_TRUE(isIntEqualsWord(a.at(1), 2));
  EXPECT_TRUE(isIntEqualsWord(a.at(2), 3));
  EXPECT_TRUE(isIntEqualsWord(a.at(3), 4));
}

TEST(TupleBuiltinsTest, DunderEqWithDifferentSizeTuplesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newTuple(0));
  Object right(&scope, runtime.newTuple(3));
  Object a(&scope, runBuiltin(TupleBuiltins::dunderEq, left, right));
  ASSERT_TRUE(a.isBool());
  EXPECT_FALSE(Bool::cast(*a).value());
}

TEST(TupleBuiltinsTest, DunderEqWithDifferentValueTuplesReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Tuple left(&scope, runtime.newTuple(2));
  left.atPut(0, runtime.newInt(1));
  left.atPut(1, runtime.newInt(2));
  Tuple right(&scope, runtime.newTuple(2));
  right.atPut(0, runtime.newInt(1));
  right.atPut(1, runtime.newInt(3));
  Object a(&scope, runBuiltin(TupleBuiltins::dunderEq, left, right));
  ASSERT_TRUE(a.isBool());
  EXPECT_FALSE(Bool::cast(*a).value());
}

TEST(TupleBuiltinsTest, DunderEqWithTupleSubclassReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Tuple left(&scope, runtime.newTuple(2));
  left.atPut(0, runtime.newInt(1));
  left.atPut(1, runtime.newInt(2));
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(tuple): pass
right = Foo((1, 2))
)")
                   .isError());
  Object right(&scope, moduleAt(&runtime, "__main__", "right"));
  ASSERT_FALSE(right.isTuple());
  ASSERT_TRUE(runtime.isInstanceOfTuple(*right));
  Object a(&scope, runBuiltin(TupleBuiltins::dunderEq, left, right));
  ASSERT_TRUE(a.isBool());
  EXPECT_TRUE(Bool::cast(*a).value());
}

TEST(TupleBuiltinsTest, DunderEqWithNonTupleSecondArgReturnsNotImplemented) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newTuple(0));
  Object right(&scope, runtime.newInt(1));
  Object a(&scope, runBuiltin(TupleBuiltins::dunderEq, left, right));
  EXPECT_TRUE(a.isNotImplementedType());
}

TEST(TupleBuiltinsTest, DunderEqWithNonTupleFirstArgRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object left(&scope, runtime.newInt(1));
  Object right(&scope, runtime.newTuple(0));
  Object a(&scope, runBuiltin(TupleBuiltins::dunderEq, left, right));
  ASSERT_TRUE(a.isError());
  Thread* thread = Thread::current();
  EXPECT_EQ(thread->pendingExceptionType(),
            runtime.typeAt(LayoutId::kTypeError));
}

TEST(TupleBuiltinsTest, DunderIterReturnsTupleIter) {
  Runtime runtime;
  HandleScope scope;
  Tuple empty_tuple(&scope, tupleFromRange(0, 0));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, empty_tuple));
  ASSERT_TRUE(iter.isTupleIterator());
}

TEST(TupleBuiltinsTest, DunderIterWithTupleSubclassReturnsTupleIter) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(tuple): pass
a = Foo()
)")
                   .isError());
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, a));
  ASSERT_TRUE(iter.isTupleIterator());
}

TEST(TupleIteratorBuiltinsTest, CallDunderNext) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, tupleFromRange(0, 2));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, tuple));
  ASSERT_TRUE(iter.isTupleIterator());

  Object item1(&scope, runBuiltin(TupleIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item1, 0));

  Object item2(&scope, runBuiltin(TupleIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item2, 1));
}

TEST(TupleIteratorBuiltinsTest, DunderIterReturnsSelf) {
  Runtime runtime;
  HandleScope scope;
  Tuple empty_tuple(&scope, tupleFromRange(0, 0));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, empty_tuple));
  ASSERT_TRUE(iter.isTupleIterator());

  // Now call __iter__ on the iterator object
  Object result(&scope, runBuiltin(TupleIteratorBuiltins::dunderIter, iter));
  ASSERT_EQ(*result, *iter);
}

TEST(TupleIteratorBuiltinsTest,
     DunderLengthHintOnEmptyTupleIteratorReturnsZero) {
  Runtime runtime;
  HandleScope scope;
  Tuple empty_tuple(&scope, tupleFromRange(0, 0));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, empty_tuple));

  Object length_hint(&scope,
                     runBuiltin(TupleIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint, 0));
}

TEST(TupleIteratorBuiltinsTest, DunderLengthHintOnConsumedTupleIterator) {
  Runtime runtime;
  HandleScope scope;
  Tuple tuple(&scope, tupleFromRange(0, 1));
  Object iter(&scope, runBuiltin(TupleBuiltins::dunderIter, tuple));

  Object length_hint1(
      &scope, runBuiltin(TupleIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint1, 1));

  // Consume the iterator
  Object item1(&scope, runBuiltin(TupleIteratorBuiltins::dunderNext, iter));
  EXPECT_TRUE(isIntEqualsWord(*item1, 0));

  Object length_hint2(
      &scope, runBuiltin(TupleIteratorBuiltins::dunderLengthHint, iter));
  EXPECT_TRUE(isIntEqualsWord(*length_hint2, 0));
}

TEST(TupleBuiltinsTest, RecursiveTuplePrintsNicely) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    self.val = (self,)
  def __repr__(self):
    return self.val.__repr__()

result = C().__repr__()
)")
                   .isError());
  EXPECT_TRUE(
      isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"), "((...),)"));
}

TEST(TupleBuiltinsTest, DunderContainsWithContainedElementReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Int value0(&scope, runtime.newInt(1));
  Bool value1(&scope, RawBool::falseObj());
  Str value2(&scope, runtime.newStrFromCStr("hello"));
  Tuple tuple(&scope, runtime.newTuple(3));
  tuple.atPut(0, *value0);
  tuple.atPut(1, *value1);
  tuple.atPut(2, *value2);
  EXPECT_EQ(runBuiltin(TupleBuiltins::dunderContains, tuple, value0),
            RawBool::trueObj());
  EXPECT_EQ(runBuiltin(TupleBuiltins::dunderContains, tuple, value1),
            RawBool::trueObj());
  EXPECT_EQ(runBuiltin(TupleBuiltins::dunderContains, tuple, value2),
            RawBool::trueObj());
}

TEST(TupleBuiltinsTest, DunderContainsWithUncontainedElementReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  Int value0(&scope, runtime.newInt(7));
  NoneType value1(&scope, RawNoneType::object());
  Tuple tuple(&scope, runtime.newTuple(2));
  tuple.atPut(0, *value0);
  tuple.atPut(1, *value1);
  Int value2(&scope, runtime.newInt(42));
  Bool value3(&scope, RawBool::trueObj());
  EXPECT_EQ(runBuiltin(TupleBuiltins::dunderContains, tuple, value2),
            RawBool::falseObj());
  EXPECT_EQ(runBuiltin(TupleBuiltins::dunderContains, tuple, value3),
            RawBool::falseObj());
}

TEST(TupleBuiltinsTest, DunderContainsWithIdenticalObjectReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __eq__(self, other):
    return False
value = Foo()
t = (value,)
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  Tuple tuple(&scope, moduleAt(&runtime, "__main__", "t"));
  EXPECT_EQ(runBuiltin(TupleBuiltins::dunderContains, tuple, value),
            RawBool::trueObj());
}

TEST(TupleBuiltinsTest,
     DunderContainsWithNonIdenticalEqualKeyObjectReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __eq__(self, other):
    return True
value = Foo()
t = (None,)
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  Tuple tuple(&scope, moduleAt(&runtime, "__main__", "t"));
  EXPECT_EQ(runBuiltin(TupleBuiltins::dunderContains, tuple, value),
            RawBool::trueObj());
}

TEST(TupleBuiltinsTest,
     DunderContainsWithNonIdenticalEqualTupleObjectReturnsFalse) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __eq__(self, other):
    return True
class Bar:
  def __eq__(self, other):
    return False
value0 = Foo()
value1 = Bar()
t = (value0,)
)")
                   .isError());
  Object value1(&scope, moduleAt(&runtime, "__main__", "value1"));
  Tuple tuple(&scope, moduleAt(&runtime, "__main__", "t"));
  EXPECT_EQ(runBuiltin(TupleBuiltins::dunderContains, tuple, value1),
            RawBool::falseObj());
}

TEST(TupleBuiltinsTest, DunderContainsWithRaisingEqPropagatesException) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __eq__(self, other):
    raise UserWarning("")
value = Foo()
t = (None,)
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  Tuple tuple(&scope, moduleAt(&runtime, "__main__", "t"));
  EXPECT_TRUE(raised(runBuiltin(TupleBuiltins::dunderContains, tuple, value),
                     LayoutId::kUserWarning));
}

TEST(TupleBuiltinsTest,
     DunderContainsWithRaisingDunderBoolPropagatesException) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __bool__(self):
    raise UserWarning("")
class Bar:
  def __eq__(self, other):
    return Foo()
value = Bar()
t = (None,)
)")
                   .isError());
  Object value(&scope, moduleAt(&runtime, "__main__", "value"));
  Tuple tuple(&scope, moduleAt(&runtime, "__main__", "t"));
  EXPECT_TRUE(raised(runBuiltin(TupleBuiltins::dunderContains, tuple, value),
                     LayoutId::kUserWarning));
}

TEST(TupleBuiltinsTest, DunderContainsWithNonTupleSelfRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Int i(&scope, SmallInt::fromWord(3));
  EXPECT_TRUE(raised(runBuiltin(TupleBuiltins::dunderContains, i, i),
                     LayoutId::kTypeError));
}

TEST(TupleBuiltinsTest, DunderHashReturnsSmallInt) {
  Runtime runtime;
  runFromCStr(&runtime, "result = (1, 2, 3).__hash__()");
  EXPECT_FALSE(Thread::current()->hasPendingException());
  EXPECT_TRUE(moduleAt(&runtime, "__main__", "result").isSmallInt());
}

TEST(TupleBuiltinsTest, DunderHashCallsDunderHashOnElements) {
  Runtime runtime;
  EXPECT_FALSE(runFromCStr(&runtime, R"(
sideeffect = 0
class C:
  def __hash__(self):
    global sideeffect
    sideeffect += 1
    return object.__hash__(self)
result = (C(), C(), C()).__hash__()
)")
                   .isError());
  EXPECT_TRUE(moduleAt(&runtime, "__main__", "result").isSmallInt());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime, "__main__", "sideeffect"), 3));
}

TEST(TupleBuiltinsTest, DunderHashWithEquivalentTuplesReturnsSameHash) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t1 = (1, 2, 3)
t2 = (1, 2, 3)
result1 = t1.__hash__()
result2 = t2.__hash__()
)")
                   .isError());
  Thread* thread = Thread::current();
  ASSERT_FALSE(thread->hasPendingException());
  HandleScope scope(thread);
  Object result1(&scope, moduleAt(&runtime, "__main__", "result1"));
  Object result2(&scope, moduleAt(&runtime, "__main__", "result2"));
  EXPECT_TRUE(result1.isSmallInt());
  EXPECT_TRUE(result2.isSmallInt());
  EXPECT_EQ(*result1, *result2);
}

TEST(TupleBuiltinsTest, DunderLtWithNonTupleSelfRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "tuple.__lt__(None, tuple())"),
      LayoutId::kTypeError, "__lt__ expected 'tuple' but got NoneType"));
}

TEST(TupleBuiltinsTest, DunderLtWithNonTupleOtherRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "tuple.__lt__(tuple(), None)"),
      LayoutId::kTypeError, "__lt__ expected 'tuple' but got NoneType"));
}

TEST(TupleBuiltinsTest, DunderLtComparesFirstNonEqualElement) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t1 = (1, 2, 3)
t2 = (1, 2, 4)
result = tuple.__lt__(t1, t2)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST(TupleBuiltinsTest, DunderLtWithTwoEqualTuplesReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t1 = (1, 2, 3)
t2 = (1, 2, 3)
result = tuple.__lt__(t1, t2)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, Bool::falseObj());
}

TEST(TupleBuiltinsTest, DunderLtWithLongerOtherReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
t1 = (1, 2, 3)
t2 = (1, 2, 3, 4, 5, 6)
result = tuple.__lt__(t1, t2)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST(TupleBuiltinsTest,
     DunderLtWithIdenticalElementsDoesNotCallCompareMethods) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __eq__(self, other):
    raise Exception("__eq__")
  def __ne__(self, other):
    raise Exception("__ne__")
  def __lt__(self, other):
    return True
c = C()
t1 = (c, 1)
t2 = (c, 2)
tuple.__lt__(t1, t2)
)")
                   .isError());
  EXPECT_FALSE(Thread::current()->hasPendingException());
}

}  // namespace python
