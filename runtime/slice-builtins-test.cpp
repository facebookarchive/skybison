#include "gtest/gtest.h"

#include "runtime.h"
#include "slice-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

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
