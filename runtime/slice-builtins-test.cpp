#include "gtest/gtest.h"

#include "runtime.h"
#include "slice-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(SliceBuiltinsTest, UnpackWithAllNoneSetsDefaults) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Slice slice(&scope, runtime.newSlice());
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(start, 0);
  EXPECT_EQ(stop, SmallInt::kMaxValue);
  EXPECT_EQ(step, 1);
}

TEST(SliceBuiltinsTest, UnpackWithNegativeStepSetsReverseDefaults) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(-1));
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(start, SmallInt::kMaxValue);
  EXPECT_EQ(stop, SmallInt::kMinValue);
  EXPECT_EQ(step, -1);
}

TEST(SliceBuiltinsTest, UnpackWithNonIndexStartRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Slice slice(&scope, runtime.newSlice());
  slice.setStart(runtime.newSet());
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  EXPECT_TRUE(raisedWithStr(
      *result, LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST(SliceBuiltinsTest, UnpackWithNonIndexStopRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Slice slice(&scope, runtime.newSlice());
  slice.setStop(runtime.newSet());
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  EXPECT_TRUE(raisedWithStr(
      *result, LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST(SliceBuiltinsTest, UnpackWithNonIndexStepRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(runtime.newSet());
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  EXPECT_TRUE(raisedWithStr(
      *result, LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST(SliceBuiltinsTest, UnpackWithMistypedDunderIndexRaisesTypeError) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __index__(self): return ""
foo = Foo()
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(moduleAt(&runtime, "__main__", "foo"));
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "__index__ returned non-int (type str)"));
}

TEST(SliceBuiltinsTest, UnpackWithNonIntIndicesCallsDunderIndex) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  def __init__(self):
    self.count = 0
  def __index__(self):
    self.count += 1
    return self.count
foo = Foo()
)")
                   .isError());
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  Slice slice(&scope, runtime.newSlice());
  slice.setStart(*foo);
  slice.setStop(*foo);
  slice.setStep(*foo);
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(start, 2);
  EXPECT_EQ(stop, 3);
  EXPECT_EQ(step, 1);
}

TEST(SliceBuiltinsTest, UnpackWithZeroStepRaisesValueError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Slice slice(&scope, runtime.newSlice());
  slice.setStep(SmallInt::fromWord(0));
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kValueError,
                            "slice step cannot be zero"));
}

TEST(SliceBuiltinsTest, UnpackWithOverflowSilentlyReducesValues) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Slice slice(&scope, runtime.newSlice());
  Object large(&scope, runtime.newInt(SmallInt::kMaxValue + 1));
  slice.setStart(*large);
  slice.setStop(*large);
  slice.setStep(*large);
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(start, SmallInt::kMaxValue);
  EXPECT_EQ(stop, SmallInt::kMaxValue);
  EXPECT_EQ(step, SmallInt::kMaxValue);
}

TEST(SliceBuiltinsTest, UnpackWithUnderflowSilentlyBoostsValues) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Slice slice(&scope, runtime.newSlice());
  Object large(&scope, runtime.newInt(SmallInt::kMinValue - 1));
  slice.setStart(*large);
  slice.setStop(*large);
  slice.setStep(*large);
  word start, stop, step;
  Object result(&scope, sliceUnpack(thread, slice, &start, &stop, &step));
  ASSERT_FALSE(result.isError());
  EXPECT_EQ(start, SmallInt::kMinValue);
  EXPECT_EQ(stop, SmallInt::kMinValue);
  EXPECT_EQ(step, -SmallInt::kMaxValue);
}

TEST(SliceBuiltinsTest, SliceHasStartAttribute) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime.newStrFromCStr("start"));
  AttributeInfo info;
  ASSERT_TRUE(
      runtime.layoutFindAttribute(Thread::current(), layout, name, &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST(SliceBuiltinsTest, SliceHasStopAttribute) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime.newStrFromCStr("stop"));
  AttributeInfo info;
  ASSERT_TRUE(
      runtime.layoutFindAttribute(Thread::current(), layout, name, &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST(SliceBuiltinsTest, SliceHasStepAttribute) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime.newStrFromCStr("step"));
  AttributeInfo info;
  ASSERT_TRUE(
      runtime.layoutFindAttribute(Thread::current(), layout, name, &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST(SliceBuiltinsTest, DunderNewWithNonTypeRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object num(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(SliceBuiltins::dunderNew, num));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(SliceBuiltinsTest, DunderNewWithNonSliceTypeRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object type(&scope, runtime.typeAt(LayoutId::kInt));
  Object result(&scope, runBuiltin(SliceBuiltins::dunderNew, type));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(SliceBuiltinsTest, DunderNewWithOneArgSetsStop) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = slice(0)").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice.start(), NoneType::object());
  EXPECT_EQ(slice.stop(), SmallInt::fromWord(0));
  EXPECT_EQ(slice.step(), NoneType::object());
}

TEST(SliceBuiltinsTest, DunderNewWithTwoArgsSetsStartAndStop) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = slice(0, 1)").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice.start(), SmallInt::fromWord(0));
  EXPECT_EQ(slice.stop(), SmallInt::fromWord(1));
  EXPECT_EQ(slice.step(), NoneType::object());
}

TEST(SliceBuiltinsTest, DunderNewWithThreeArgsSetsAllIndices) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, "result = slice(0, 1, 2)").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice.start(), SmallInt::fromWord(0));
  EXPECT_EQ(slice.stop(), SmallInt::fromWord(1));
  EXPECT_EQ(slice.step(), SmallInt::fromWord(2));
}

TEST(SliceBuiltinsTest, IndicesWithNonSliceRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "slice.indices([], 1)"), LayoutId::kTypeError,
      "'indices' requires a 'slice' object but received a 'list'"));
}

TEST(SliceBuiltinsTest, IndicesWithNonIntRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "slice(1).indices([])"), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST(SliceBuiltinsTest, IndicesWithNegativeLengthRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "slice(1).indices(-1)"),
                            LayoutId::kValueError,
                            "length should not be negative"));
}

TEST(SliceBuiltinsTest, IndicesWithZeroStepRaisesValueError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "slice(1, 1, 0).indices(10)"),
                            LayoutId::kValueError,
                            "slice step cannot be zero"));
}

TEST(SliceBuiltinsTest, IndicesWithNonIntStartRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "slice('').indices(10)"), LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST(SliceBuiltinsTest, IndicesWithNonIntStopRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "slice(1, '').indices(10)"), LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST(SliceBuiltinsTest, IndicesWithNonIntStepRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, "slice(1, 6, '').indices(10)"),
      LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST(SliceBuiltinsTest, IndicesWithNoneReturnsDefaults) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = slice(None).indices(10)").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(0));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(10));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(1));
}

TEST(SliceBuiltinsTest, IndicesWithNoneAndNegativeReturnsDefaults) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = slice(None, None, -1).indices(10)")
          .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(9));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(-1));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(-1));
}

TEST(SliceBuiltinsTest, IndicesCallsDunderIndex) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Idx:
  def __init__(self):
    self.count = 0
  def __index__(self):
    self.count += 1
    return self.count
idx = Idx()
result = slice(idx, idx, idx).indices(10)
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(2));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(3));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(1));
}

TEST(SliceBuiltinsTest, IndicesTruncatesToLength) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = slice(-4, 10, 2).indices(5)").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(1));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(5));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(2));
}

}  // namespace python
