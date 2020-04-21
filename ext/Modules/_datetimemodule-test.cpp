#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"
#include "datetime.h"

namespace py {
namespace testing {

using DateTimeExtensionApiTest = ExtensionApi;

TEST_F(DateTimeExtensionApiTest, PyDateTimeCheckWithDateTimeObjectReturnsTrue) {
  PyRun_SimpleString(R"(
import datetime
instance = datetime.datetime(1, 2, 3)
)");
  PyObjectPtr instance(mainModuleGet("instance"));
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_TRUE(PyDateTime_Check(instance));
}

TEST_F(DateTimeExtensionApiTest,
       PyDateTimeCheckWithDateTimeSubclassReturnsTrue) {
  PyRun_SimpleString(R"(
import datetime
class C(datetime.datetime):
    pass
instance = C(1, 2, 3)
)");
  PyObjectPtr instance(mainModuleGet("instance"));
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_TRUE(PyDateTime_Check(instance));
}

TEST_F(DateTimeExtensionApiTest, PyDateTimeCheckWithNonDateTimeReturnsFalse) {
  PyObjectPtr instance(PyLong_FromLong(100));
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_FALSE(PyDateTime_Check(instance));
}

TEST_F(DateTimeExtensionApiTest, PyDateCheckWithDateObjectReturnsTrue) {
  PyRun_SimpleString(R"(
import datetime
instance = datetime.date(1, 2, 3)
)");
  PyObjectPtr instance(mainModuleGet("instance"));
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_TRUE(PyDate_Check(instance));
}

TEST_F(DateTimeExtensionApiTest, PyDateCheckWithDateSubclassReturnsTrue) {
  PyRun_SimpleString(R"(
import datetime
class C(datetime.date):
    pass
instance = C(1, 2, 3)
)");
  PyObjectPtr instance(mainModuleGet("instance"));
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_TRUE(PyDate_Check(instance));
}

TEST_F(DateTimeExtensionApiTest, PyDateCheckWithNonDateReturnsFalse) {
  PyObjectPtr instance(PyLong_FromLong(100));
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_FALSE(PyDate_Check(instance));
}

TEST_F(DateTimeExtensionApiTest, PyDateFromDateReturnsDateObject) {
  PyDateTime_IMPORT;
  PyObjectPtr result(PyDate_FromDate(1, 2, 3));
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyDate_Check(result));
}

}  // namespace testing
}  // namespace py
