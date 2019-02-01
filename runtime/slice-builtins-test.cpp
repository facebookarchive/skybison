#include "gtest/gtest.h"

#include "runtime.h"
#include "slice-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(SliceBuiltinsTest, UnpackWithAllNoneSetsDefaults) {
  Runtime runtime;
  HandleScope scope;
  Slice slice(&scope, runtime.newSlice());
  word start, stop, step;
  Object result(&scope, sliceUnpack(Thread::currentThread(), slice, &start,
                                    &stop, &step));
  ASSERT_FALSE(result->isError());
  EXPECT_EQ(start, 0);
  EXPECT_EQ(stop, SmallInt::kMaxValue);
  EXPECT_EQ(step, 1);
}

TEST(SliceBuiltinsTest, UnpackWithNegativeStepSetsReverseDefaults) {
  Runtime runtime;
  HandleScope scope;
  Slice slice(&scope, runtime.newSlice());
  slice->setStep(SmallInt::fromWord(-1));
  word start, stop, step;
  Object result(&scope, sliceUnpack(Thread::currentThread(), slice, &start,
                                    &stop, &step));
  ASSERT_FALSE(result->isError());
  EXPECT_EQ(start, SmallInt::kMaxValue);
  EXPECT_EQ(stop, SmallInt::kMinValue);
  EXPECT_EQ(step, -1);
}

TEST(SliceBuiltinsTest, UnpackWithNonIndexStartRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Slice slice(&scope, runtime.newSlice());
  slice->setStart(runtime.newSet());
  word start, stop, step;
  Object result(&scope, sliceUnpack(Thread::currentThread(), slice, &start,
                                    &stop, &step));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(SliceBuiltinsTest, UnpackWithNonIndexStopRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Slice slice(&scope, runtime.newSlice());
  slice->setStop(runtime.newSet());
  word start, stop, step;
  Object result(&scope, sliceUnpack(Thread::currentThread(), slice, &start,
                                    &stop, &step));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(SliceBuiltinsTest, UnpackWithNonIndexStepRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Slice slice(&scope, runtime.newSlice());
  slice->setStep(runtime.newSet());
  word start, stop, step;
  Object result(&scope, sliceUnpack(Thread::currentThread(), slice, &start,
                                    &stop, &step));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(SliceBuiltinsTest, UnpackWithMistypedDunderIndexRaisesTypeError) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  def __index__(self): return ""
foo = Foo()
)");
  HandleScope scope;
  Slice slice(&scope, runtime.newSlice());
  slice->setStep(moduleAt(&runtime, "__main__", "foo"));
  word start, stop, step;
  Object result(&scope, sliceUnpack(Thread::currentThread(), slice, &start,
                                    &stop, &step));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST(SliceBuiltinsTest, UnpackWithNonIntIndicesCallsDunderIndex) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  def __init__(self):
    self.count = 0
  def __index__(self):
    self.count += 1
    return self.count
foo = Foo()
)");
  HandleScope scope;
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  Slice slice(&scope, runtime.newSlice());
  slice->setStart(*foo);
  slice->setStop(*foo);
  slice->setStep(*foo);
  word start, stop, step;
  Object result(&scope, sliceUnpack(Thread::currentThread(), slice, &start,
                                    &stop, &step));
  ASSERT_FALSE(result->isError());
  EXPECT_EQ(start, 2);
  EXPECT_EQ(stop, 3);
  EXPECT_EQ(step, 1);
}

TEST(SliceBuiltinsTest, UnpackWithZeroStepRaisesValueError) {
  Runtime runtime;
  HandleScope scope;
  Slice slice(&scope, runtime.newSlice());
  slice->setStep(SmallInt::fromWord(0));
  word start, stop, step;
  Object result(&scope, sliceUnpack(Thread::currentThread(), slice, &start,
                                    &stop, &step));
  EXPECT_TRUE(raised(*result, LayoutId::kValueError));
}

TEST(SliceBuiltinsTest, UnpackWithOverflowSilentlyReducesValues) {
  Runtime runtime;
  HandleScope scope;
  Slice slice(&scope, runtime.newSlice());
  Object large(&scope, runtime.newInt(SmallInt::kMaxValue + 1));
  slice->setStart(*large);
  slice->setStop(*large);
  slice->setStep(*large);
  word start, stop, step;
  Object result(&scope, sliceUnpack(Thread::currentThread(), slice, &start,
                                    &stop, &step));
  ASSERT_FALSE(result->isError());
  EXPECT_EQ(start, SmallInt::kMaxValue);
  EXPECT_EQ(stop, SmallInt::kMaxValue);
  EXPECT_EQ(step, SmallInt::kMaxValue);
}

TEST(SliceBuiltinsTest, UnpackWithUnderflowSilentlyBoostsValues) {
  Runtime runtime;
  HandleScope scope;
  Slice slice(&scope, runtime.newSlice());
  Object large(&scope, runtime.newInt(SmallInt::kMinValue - 1));
  slice->setStart(*large);
  slice->setStop(*large);
  slice->setStep(*large);
  word start, stop, step;
  Object result(&scope, sliceUnpack(Thread::currentThread(), slice, &start,
                                    &stop, &step));
  ASSERT_FALSE(result->isError());
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
  ASSERT_TRUE(runtime.layoutFindAttribute(Thread::currentThread(), layout, name,
                                          &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST(SliceBuiltinsTest, SliceHasStopAttribute) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime.newStrFromCStr("stop"));
  AttributeInfo info;
  ASSERT_TRUE(runtime.layoutFindAttribute(Thread::currentThread(), layout, name,
                                          &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST(SliceBuiltinsTest, SliceHasStepAttribute) {
  Runtime runtime;
  HandleScope scope;
  Layout layout(&scope, runtime.layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime.newStrFromCStr("step"));
  AttributeInfo info;
  ASSERT_TRUE(runtime.layoutFindAttribute(Thread::currentThread(), layout, name,
                                          &info));
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
  runFromCStr(&runtime, "result = slice(0)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result->isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice->start(), NoneType::object());
  EXPECT_EQ(slice->stop(), SmallInt::fromWord(0));
  EXPECT_EQ(slice->step(), NoneType::object());
}

TEST(SliceBuiltinsTest, DunderNewWithTwoArgsSetsStartAndStop) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = slice(0, 1)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result->isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice->start(), SmallInt::fromWord(0));
  EXPECT_EQ(slice->stop(), SmallInt::fromWord(1));
  EXPECT_EQ(slice->step(), NoneType::object());
}

TEST(SliceBuiltinsTest, DunderNewWithThreeArgsSetsAllIndices) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = slice(0, 1, 2)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result->isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice->start(), SmallInt::fromWord(0));
  EXPECT_EQ(slice->stop(), SmallInt::fromWord(1));
  EXPECT_EQ(slice->step(), SmallInt::fromWord(2));
}

}  // namespace python
