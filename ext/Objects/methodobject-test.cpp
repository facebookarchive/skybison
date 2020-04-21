#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using PyCFunctionExtensionApiTest = ExtensionApi;

PyObject* getPyCFunctionDunderModule(PyObject* function) {
  PyObject* real_function = function;
  // Work around PyRo behavior.
  if (PyMethod_Check(function)) {
    real_function = PyMethod_Function(function);
  }
  return PyObject_GetAttrString(real_function, "__module__");
}

TEST_F(PyCFunctionExtensionApiTest, NewReturnsCallable) {
  PyObjectPtr self_value(PyUnicode_FromString("baz"));
  binaryfunc meth = [](PyObject* self, PyObject* arg) {
    EXPECT_EQ(arg, nullptr);
    Py_INCREF(self);
    return self;
  };
  static PyMethodDef func_def = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_New(&func_def, self_value));
  ASSERT_NE(func, nullptr);
  PyObjectPtr result(_PyObject_CallNoArg(func));
  EXPECT_EQ(result, self_value);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr dunder_module(getPyCFunctionDunderModule(func));
  EXPECT_EQ(dunder_module, Py_None);
}

TEST_F(PyCFunctionExtensionApiTest, NewExWithModuleReturnsCallable) {
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

TEST_F(PyCFunctionExtensionApiTest, NewExWithNullSelfReturnsCallable) {
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

TEST_F(PyCFunctionExtensionApiTest, NewExResultDoesNotBindSelfInClass) {
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
  PyObjectPtr c(mainModuleGet("C"));
  PyObjectPtr instance(mainModuleGet("instance"));
  PyObject_SetAttrString(c, "foo", func);
  PyObjectPtr result(PyObject_CallMethod(instance, "foo", ""));
  EXPECT_NE(result, c);
  EXPECT_EQ(result, self_value);
}

TEST_F(PyCFunctionExtensionApiTest, NewExWithMethNoArgsCallsFunction) {
  PyCFunction foo_func = [](PyObject* self, PyObject* args) {
    EXPECT_TRUE(isUnicodeEqualsCStr(self, "self"));
    EXPECT_EQ(args, nullptr);
    return PyLong_FromLong(-7);
  };
  PyMethodDef def = {
      "foo", reinterpret_cast<PyCFunction>(reinterpret_cast<void*>(foo_func)),
      METH_NOARGS};
  PyObjectPtr self(PyUnicode_FromString("self"));
  PyObjectPtr func(PyCFunction_NewEx(&def, self, nullptr));

  PyObjectPtr result(_PyObject_CallNoArg(func));
  ASSERT_TRUE(isLongEqualsLong(result, -7));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(PyCFunctionExtensionApiTest, NewExWithMethOCallsFunction) {
  PyCFunction foo_func = [](PyObject* self, PyObject* arg) {
    EXPECT_TRUE(isUnicodeEqualsCStr(self, "self"));
    EXPECT_TRUE(isLongEqualsLong(arg, 42));
    return PyLong_FromLong(1);
  };
  PyMethodDef def = {"foo", foo_func, METH_O};
  PyObjectPtr self(PyUnicode_FromString("self"));
  PyObjectPtr func(PyCFunction_NewEx(&def, self, nullptr));
  PyObjectPtr arg(PyLong_FromLong(42));

  PyObject* argo = arg.get();
  PyObjectPtr result(_PyObject_CallArg1(func, argo));
  ASSERT_TRUE(isLongEqualsLong(result, 1));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(PyCFunctionExtensionApiTest, NewExWithMethVarArgsCallsFunction) {
  PyCFunction foo_func = [](PyObject* self, PyObject* args) {
    EXPECT_TRUE(isUnicodeEqualsCStr(self, "self"));
    EXPECT_TRUE(PyTuple_Check(args));
    EXPECT_EQ(PyTuple_Size(args), 2);
    EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(args, 0), -14));
    EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(args, 1), 15));
    return PyLong_FromLong(22);
  };
  PyMethodDef def = {"foo", foo_func, METH_VARARGS};
  PyObjectPtr self(PyUnicode_FromString("self"));
  PyObjectPtr func(PyCFunction_NewEx(&def, self, nullptr));
  PyObjectPtr arg0(PyLong_FromLong(-14));
  PyObjectPtr arg1(PyLong_FromLong(15));
  PyObjectPtr args(PyTuple_Pack(2, arg0.get(), arg1.get()));

  PyObjectPtr result(PyObject_Call(func, args, nullptr));
  ASSERT_TRUE(isLongEqualsLong(result, 22));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(PyCFunctionExtensionApiTest, NewExWithVarArgsAndKeywordsCallsFunction) {
  PyCFunctionWithKeywords foo_func = [](PyObject* self, PyObject* args,
                                        PyObject* kwargs) -> PyObject* {
    EXPECT_TRUE(isUnicodeEqualsCStr(self, "self"));
    EXPECT_TRUE(PyTuple_Check(args));
    EXPECT_EQ(PyTuple_Size(args), 2);
    EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(args, 0), -111));
    EXPECT_TRUE(isLongEqualsLong(PyTuple_GetItem(args, 1), 222));
    EXPECT_TRUE(PyDict_Check(kwargs));
    EXPECT_EQ(PyDict_Size(kwargs), 1);
    EXPECT_TRUE(isLongEqualsLong(PyDict_GetItemString(kwargs, "keyword"), 333));
    return PyLong_FromLong(876);
  };
  PyMethodDef def = {
      "foo", reinterpret_cast<PyCFunction>(reinterpret_cast<void*>(foo_func)),
      METH_VARARGS | METH_KEYWORDS};
  PyObjectPtr self(PyUnicode_FromString("self"));
  PyObjectPtr func(PyCFunction_NewEx(&def, self, nullptr));

  PyObjectPtr arg0(PyLong_FromLong(-111));
  PyObjectPtr arg1(PyLong_FromLong(222));
  PyObjectPtr args(PyTuple_Pack(2, arg0.get(), arg1.get()));
  PyObjectPtr kwargs(PyDict_New());
  PyObjectPtr value(PyLong_FromLong(333));
  PyDict_SetItemString(kwargs, "keyword", value);
  PyObjectPtr result(PyObject_Call(func, args, kwargs));
  ASSERT_TRUE(isLongEqualsLong(result, 876));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(PyCFunctionExtensionApiTest, NewExWithMethFastCallCallsFunction) {
  _PyCFunctionFast foo_func = [](PyObject* self, PyObject* const* args,
                                 Py_ssize_t num_args) -> PyObject* {
    EXPECT_TRUE(isUnicodeEqualsCStr(self, "self"));
    EXPECT_EQ(num_args, 3);
    EXPECT_TRUE(isLongEqualsLong(args[0], 17));
    EXPECT_TRUE(isLongEqualsLong(args[1], -8));
    EXPECT_TRUE(isLongEqualsLong(args[2], 99));
    return PyLong_FromLong(4444);
  };
  PyMethodDef def = {
      "foo", reinterpret_cast<PyCFunction>(reinterpret_cast<void*>(foo_func)),
      METH_FASTCALL};
  PyObjectPtr self(PyUnicode_FromString("self"));
  PyObjectPtr func(PyCFunction_NewEx(&def, self, nullptr));

  PyObjectPtr arg0(PyLong_FromLong(17));
  PyObjectPtr arg1(PyLong_FromLong(-8));
  PyObjectPtr arg2(PyLong_FromLong(99));
  PyObjectPtr args(PyTuple_Pack(3, arg0.get(), arg1.get(), arg2.get()));
  PyObjectPtr result(PyObject_Call(func, args, nullptr));
  ASSERT_TRUE(isLongEqualsLong(result, 4444));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(PyCFunctionExtensionApiTest,
       NewExWithMethFastCallAndKeywordsCallsFunction) {
  _PyCFunctionFastWithKeywords foo_func =
      [](PyObject* self, PyObject* const* args, Py_ssize_t num_args,
         PyObject* kwnames) -> PyObject* {
    EXPECT_TRUE(isUnicodeEqualsCStr(self, "self"));
    EXPECT_EQ(num_args, 1);
    EXPECT_TRUE(isLongEqualsLong(args[0], 42));
    EXPECT_TRUE(isLongEqualsLong(args[1], 30));
    EXPECT_TRUE(PyTuple_Check(kwnames));
    EXPECT_EQ(PyTuple_Size(kwnames), 1);
    EXPECT_TRUE(isUnicodeEqualsCStr(PyTuple_GetItem(kwnames, 0), "keyword"));
    return PyLong_FromLong(333);
  };
  PyMethodDef def = {
      "foo", reinterpret_cast<PyCFunction>(reinterpret_cast<void*>(foo_func)),
      METH_FASTCALL | METH_KEYWORDS};
  PyObjectPtr self(PyUnicode_FromString("self"));
  PyObjectPtr func(PyCFunction_NewEx(&def, self, nullptr));

  PyObjectPtr arg(PyLong_FromLong(42));
  PyObjectPtr args(PyTuple_Pack(1, arg.get()));
  PyObjectPtr kwargs(PyDict_New());
  PyObjectPtr value(PyLong_FromLong(30));
  PyDict_SetItemString(kwargs, "keyword", value);
  PyObjectPtr result(PyObject_Call(func, args, kwargs));
  ASSERT_TRUE(isLongEqualsLong(result, 333));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace testing
}  // namespace py
