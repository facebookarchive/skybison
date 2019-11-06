#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using MethodExtensionApiTest = ExtensionApi;

TEST_F(MethodExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyCFunction_ClearFreeList(), 0);
}

TEST_F(MethodExtensionApiTest, NeCFunctionWithModuleReturnsFunction) {
  PyObjectPtr module(PyModule_New("mod"));
  ASSERT_NE(module, nullptr);
  binaryfunc meth = [](PyObject*, PyObject*) { return PyLong_FromLong(1234); };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, nullptr, module));
  ASSERT_NE(func, nullptr);
  PyObjectPtr noargs(PyTuple_New(0));
  PyObjectPtr result(PyObject_Call(func, noargs, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isLongEqualsLong(result, 1234));
}

TEST_F(MethodExtensionApiTest, NewCFunctionWithTypeReturnsFunction) {
  PyRun_SimpleString(R"(
class Bar: pass
)");
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  ASSERT_NE(type, nullptr);
  binaryfunc meth = [](PyObject*, PyObject*) { return PyLong_FromLong(1234); };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, type, nullptr));
  ASSERT_NE(func, nullptr);
  PyObjectPtr noargs(PyTuple_New(0));
  PyObjectPtr result(PyObject_Call(func, noargs, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isLongEqualsLong(result, 1234));
}

TEST_F(MethodExtensionApiTest, NewCFunctionWithTypeIsFirstArgument) {
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "__main__.C", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  testing::moduleSet("__main__", "C", type);

  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    EXPECT_EQ(self, arg);
    return PyLong_FromLong(1234);
  };
  static PyMethodDef method = {"testself", meth, METH_O};
  PyObjectPtr func(PyCFunction_NewEx(&method, type, nullptr));

  testing::moduleSet("__main__", "f", func);
  PyRun_SimpleString(R"(
result = f(C)
)");
  PyObjectPtr result(testing::moduleGet("__main__", "result"));
  EXPECT_TRUE(isLongEqualsLong(result, 1234));
}

}  // namespace py
