#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using SysModuleExtensionApiTest = ExtensionApi;

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

}  // namespace python
