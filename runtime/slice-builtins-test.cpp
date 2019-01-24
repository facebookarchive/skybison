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

}  // namespace python
