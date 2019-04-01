#include "gtest/gtest.h"

#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ItertoolsTest, CountWithIntReturnsIterator) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
import itertools
iterator = itertools.count(start=7, step=-2)
list = [next(iterator) for _ in range(5)]
)")
                   .isError());
  Object list(&scope, moduleAt(&runtime, "__main__", "list"));
  EXPECT_PYLIST_EQ(list, {7, 5, 3, 1, -1});
}

TEST(ItertoolsTest, CountWithNonNumberRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
import itertools
iterator = itertools.count(start="a", step=".")
)"),
                            LayoutId::kTypeError, "a number is required"));
}

}  // namespace python
