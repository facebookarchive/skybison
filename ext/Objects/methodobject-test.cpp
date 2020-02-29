#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using MethodExtensionApiTest = ExtensionApi;

PyObject* getPyCFunctionDunderModule(PyObject* function) {
  PyObject* real_function = function;
  // Work around PyRo behavior.
  if (PyMethod_Check(function)) {
    real_function = PyMethod_Function(function);
  }
  return PyObject_GetAttrString(real_function, "__module__");
}

TEST_F(MethodExtensionApiTest, NewCFunctionWithModuleReturnsCallable) {
  PyObjectPtr self_value(PyUnicode_FromString("foo"));
  PyObjectPtr module_name(PyUnicode_FromString("bar"));
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    EXPECT_EQ(arg, nullptr);
    Py_INCREF(self);
    return self;
  };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, self_value, module_name));
  ASSERT_NE(func, nullptr);
  PyObjectPtr noargs(PyTuple_New(0));
  PyObjectPtr result(PyObject_Call(func, noargs, nullptr));
  EXPECT_EQ(result, self_value);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr dunder_module(getPyCFunctionDunderModule(func));
  EXPECT_EQ(dunder_module, module_name);
}

TEST_F(MethodExtensionApiTest, NewCFunctionWithNullSelfReturnsCallable) {
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    EXPECT_EQ(self, nullptr);
    EXPECT_EQ(arg, nullptr);
    Py_INCREF(Py_None);
    return Py_None;
  };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(
      PyCFunction_NewEx(&foo_func, /*self=*/nullptr, /*module=*/nullptr));
  ASSERT_NE(func, nullptr);
  PyObjectPtr noargs(PyTuple_New(0));
  PyObjectPtr result(PyObject_Call(func, noargs, nullptr));
  EXPECT_EQ(result, Py_None);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr dunder_module(getPyCFunctionDunderModule(func));
  EXPECT_EQ(dunder_module, Py_None);
}

TEST_F(MethodExtensionApiTest, NewCFunctionResultDoesNotBindSelfInClass) {
  PyRun_SimpleString(R"(
class C:
  pass
instance = C()
)");
  PyObjectPtr self_value(PyUnicode_FromString("foo"));
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    EXPECT_EQ(arg, nullptr);
    Py_INCREF(self);
    return self;
  };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(
      PyCFunction_NewEx(&foo_func, self_value, /*module=*/nullptr));
  ASSERT_NE(func, nullptr);
  PyObjectPtr c(moduleGet("__main__", "C"));
  PyObjectPtr instance(moduleGet("__main__", "instance"));
  PyObject_SetAttrString(c, "foo", func);
  PyObjectPtr result(PyObject_CallMethod(instance, "foo", ""));
  EXPECT_NE(result, c);
  EXPECT_EQ(result, self_value);
}

}  // namespace testing
}  // namespace py
