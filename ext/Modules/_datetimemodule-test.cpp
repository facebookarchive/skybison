#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"
#include "datetime.h"

namespace py {
namespace testing {

using DateTimeExtensionApiTest = ExtensionApi;

TEST_F(DateTimeExtensionApiTest, PyDateTimeAPIReturnsStructIfFound) {
  ASSERT_EQ(PyDateTimeAPI, nullptr);
  PyDateTime_IMPORT;
  ASSERT_NE(PyDateTimeAPI, nullptr);
}

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

TEST_F(DateTimeExtensionApiTest,
       PyDateTimeFromDateAndTimeReturnsDateTimeObject) {
  PyDateTime_IMPORT;
  PyObjectPtr result(PyDateTime_FromDateAndTime(1, 2, 3, 4, 5, 6, 10));
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyDateTime_Check(result));
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

TEST_F(DateTimeExtensionApiTest, PyDeltaCheckWithDeltaObjectReturnsTrue) {
  PyRun_SimpleString(R"(
import datetime
instance = datetime.date(1, 2, 3) - datetime.date(1, 2, 3)
)");
  PyObjectPtr instance(mainModuleGet("instance"));
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_TRUE(PyDelta_Check(instance));
}

TEST_F(DateTimeExtensionApiTest, PyDeltaCheckWithDeltaSubclassReturnsTrue) {
  PyRun_SimpleString(R"(
import datetime
class C(datetime.timedelta):
    pass
instance = C(1, 2, 3)
)");
  PyObjectPtr instance(mainModuleGet("instance"));
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_TRUE(PyDelta_Check(instance));
}

TEST_F(DateTimeExtensionApiTest, PyDeltaCheckWithNonDeltaReturnsFalse) {
  PyObjectPtr instance(PyLong_FromLong(100));
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_FALSE(PyDelta_Check(instance));
}

TEST_F(DateTimeExtensionApiTest, PyDeltaFromDSUReturnsObject) {
  PyDateTime_IMPORT;
  PyObjectPtr result(PyDelta_FromDSU(1, 2, 500));
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyDelta_Check(result));
}

TEST_F(DateTimeExtensionApiTest, PyDateTimeGETsEveryUnitOfTime) {
  PyRun_SimpleString(R"(
import datetime
instance = datetime.datetime(1990, 2, 3, 4, 5, 6, 10000)
)");
  PyObject* instance = mainModuleGet("instance");
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_EQ(PyDateTime_GET_YEAR(instance), 1990);
  EXPECT_EQ(PyDateTime_GET_MONTH(instance), 2);
  EXPECT_EQ(PyDateTime_GET_DAY(instance), 3);
  EXPECT_EQ(PyDateTime_DATE_GET_HOUR(instance), 4);
  EXPECT_EQ(PyDateTime_DATE_GET_MINUTE(instance), 5);
  EXPECT_EQ(PyDateTime_DATE_GET_SECOND(instance), 6);
  EXPECT_EQ(PyDateTime_DATE_GET_MICROSECOND(instance), 10000);
  Py_DECREF(instance);
}

TEST_F(DateTimeExtensionApiTest, PyDateTimeTimeGETsEveryUnitOfTime) {
  PyRun_SimpleString(R"(
import datetime
instance = datetime.time(1, 40, 50, 999)
)");
  PyObject* instance = mainModuleGet("instance");
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_EQ(PyDateTime_TIME_GET_HOUR(instance), 1);
  EXPECT_EQ(PyDateTime_TIME_GET_MINUTE(instance), 40);
  EXPECT_EQ(PyDateTime_TIME_GET_SECOND(instance), 50);
  EXPECT_EQ(PyDateTime_TIME_GET_MICROSECOND(instance), 999);
  Py_DECREF(instance);
}

TEST_F(DateTimeExtensionApiTest, PyDeltaGETsEveryUnitOfTime) {
  PyRun_SimpleString(R"(
import datetime
instance = datetime.timedelta(1, 2, 3)
)");
  PyObject* instance = mainModuleGet("instance");
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_EQ(PyDateTime_DELTA_GET_DAYS(instance), 1);
  EXPECT_EQ(PyDateTime_DELTA_GET_SECONDS(instance), 2);
  EXPECT_EQ(PyDateTime_DELTA_GET_MICROSECONDS(instance), 3);
  Py_DECREF(instance);
}

TEST_F(DateTimeExtensionApiTest, PyTimeCheckWithTimeObjectReturnsTrue) {
  PyRun_SimpleString(R"(
import datetime
instance = datetime.time(1, 2, 3)
)");
  PyObjectPtr instance(mainModuleGet("instance"));
  ASSERT_NE(instance, nullptr);
  PyDateTime_IMPORT;
  EXPECT_TRUE(PyTime_Check(instance));
}

}  // namespace testing
}  // namespace py
