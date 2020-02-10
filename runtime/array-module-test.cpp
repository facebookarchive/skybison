#include "array-module.h"

#include "gtest/gtest.h"

#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

namespace testing {

using ArrayModuleTest = testing::RuntimeFixture;

TEST_F(ArrayModuleTest, NewCreatesEmptyArrayObject) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import array
result = array.array('b')
)")
                   .isError());
  HandleScope scope(thread_);
  Object result_obj(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result_obj.isArray());

  Array result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 0);
  ASSERT_TRUE(isStrEqualsCStr(result.typecode(), "b"));
  ASSERT_EQ(result.buffer(), runtime_->emptyMutableBytes());
}

}  // namespace testing

}  // namespace py
