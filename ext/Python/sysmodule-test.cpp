#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using SysModuleExtensionApiTest = ExtensionApi;

TEST_F(SysModuleExtensionApiTest, GetObjectWithNonExistentNameReturnsNull) {
  EXPECT_EQ(PySys_GetObject("foo_bar_not_a_real_name"), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(SysModuleExtensionApiTest, GetObjectReturnsValueFromSysModule) {
  PyRun_SimpleString(R"(
import sys
sys.foo = 'bar'
)");
  PyObject* result = PySys_GetObject("foo");  // borrowed reference
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "bar"));
}

TEST_F(SysModuleExtensionApiTest, GetSizeOfPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __sizeof__(self): raise Exception()
o = C()
)");
  PyObjectPtr object(moduleGet("__main__", "o"));
  EXPECT_EQ(_PySys_GetSizeOf(object), static_cast<size_t>(-1));
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_Exception));
}

TEST_F(SysModuleExtensionApiTest, GetSizeOfReturnsDunderSizeOfPyro) {
  PyRun_SimpleString(R"(
class C:
  def __sizeof__(self): return 10
o = C()
)");
  PyObjectPtr object(moduleGet("__main__", "o"));
  EXPECT_EQ(_PySys_GetSizeOf(object), 10);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(SysModuleExtensionApiTest, GetSizeOfWithIntSubclassReturnsIntPyro) {
  PyRun_SimpleString(R"(
class N(int): pass
class C:
  def __sizeof__(self): return N(10)
o = C()
)");
  PyObjectPtr object(moduleGet("__main__", "o"));
  EXPECT_EQ(_PySys_GetSizeOf(object), 10);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace py
