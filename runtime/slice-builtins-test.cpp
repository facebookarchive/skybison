#include "slice-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {
namespace testing {

using SliceBuiltinsTest = RuntimeFixture;

TEST_F(SliceBuiltinsTest, SliceHasStartAttribute) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_->layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime_->newStrFromCStr("start"));
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST_F(SliceBuiltinsTest, SliceHasStopAttribute) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_->layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime_->newStrFromCStr("stop"));
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST_F(SliceBuiltinsTest, SliceHasStepAttribute) {
  HandleScope scope(thread_);
  Layout layout(&scope, runtime_->layoutAt(LayoutId::kSlice));
  Str name(&scope, runtime_->newStrFromCStr("step"));
  AttributeInfo info;
  ASSERT_TRUE(Runtime::layoutFindAttribute(*layout, name, &info));
  EXPECT_TRUE(info.isInObject());
  EXPECT_TRUE(info.isFixedOffset());
}

TEST_F(SliceBuiltinsTest, DunderNewWithNonTypeRaisesTypeError) {
  HandleScope scope(thread_);
  Object num(&scope, SmallInt::fromWord(0));
  Object result(&scope, runBuiltin(METH(slice, __new__), num));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(SliceBuiltinsTest, DunderNewWithNonSliceTypeRaisesTypeError) {
  HandleScope scope(thread_);
  Object type(&scope, runtime_->typeAt(LayoutId::kInt));
  Object result(&scope, runBuiltin(METH(slice, __new__), type));
  EXPECT_TRUE(raised(*result, LayoutId::kTypeError));
}

TEST_F(SliceBuiltinsTest, DunderNewWithOneArgSetsStop) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = slice(0)").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice.start(), NoneType::object());
  EXPECT_EQ(slice.stop(), SmallInt::fromWord(0));
  EXPECT_EQ(slice.step(), NoneType::object());
}

TEST_F(SliceBuiltinsTest, DunderNewWithTwoArgsSetsStartAndStop) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = slice(0, 1)").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice.start(), SmallInt::fromWord(0));
  EXPECT_EQ(slice.stop(), SmallInt::fromWord(1));
  EXPECT_EQ(slice.step(), NoneType::object());
}

TEST_F(SliceBuiltinsTest, DunderNewWithThreeArgsSetsAllIndices) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "result = slice(0, 1, 2)").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isSlice());
  Slice slice(&scope, *result);
  EXPECT_EQ(slice.start(), SmallInt::fromWord(0));
  EXPECT_EQ(slice.stop(), SmallInt::fromWord(1));
  EXPECT_EQ(slice.step(), SmallInt::fromWord(2));
}

TEST_F(SliceBuiltinsTest, IndicesWithNonIntRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "slice(1).indices([])"), LayoutId::kTypeError,
      "'list' object cannot be interpreted as an integer"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNegativeLengthRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "slice(1).indices(-1)"),
                            LayoutId::kValueError,
                            "length should not be negative"));
}

TEST_F(SliceBuiltinsTest, IndicesWithZeroStepRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "slice(1, 1, 0).indices(10)"),
                            LayoutId::kValueError,
                            "slice step cannot be zero"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNonIntStartRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "slice('').indices(10)"), LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNonIntStopRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "slice(1, '').indices(10)"), LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNonIntStepRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "slice(1, 6, '').indices(10)"),
      LayoutId::kTypeError,
      "slice indices must be integers or None or have an __index__ method"));
}

TEST_F(SliceBuiltinsTest, IndicesWithNoneReturnsDefaults) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = slice(None).indices(10)").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(0));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(10));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(1));
}

TEST_F(SliceBuiltinsTest, IndicesWithNoneAndNegativeReturnsDefaults) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = slice(None, None, -1).indices(10)")
          .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(9));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(-1));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(-1));
}

TEST_F(SliceBuiltinsTest, IndicesCallsDunderIndex) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(2));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(3));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(1));
}

TEST_F(SliceBuiltinsTest, IndicesTruncatesToLength) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = slice(-4, 10, 2).indices(5)").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isTuple());
  Tuple indices(&scope, *result);
  ASSERT_EQ(indices.length(), 3);
  EXPECT_EQ(indices.at(0), SmallInt::fromWord(1));
  EXPECT_EQ(indices.at(1), SmallInt::fromWord(5));
  EXPECT_EQ(indices.at(2), SmallInt::fromWord(2));
}

}  // namespace testing
}  // namespace py
