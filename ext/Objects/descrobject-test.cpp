#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using DescrExtensionApiTest = ExtensionApi;

// Create a new type with PyType_FromSpec with no methods, members, or getters
static void createEmptyBarType() {
  static PyType_Slot slots[1];
  slots[0] = {0, nullptr};
  static PyType_Spec spec;
  spec = {
      "__main__.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_EQ(PyType_CheckExact(type), 1);
  ASSERT_EQ(moduleSet("__main__", "Bar", type), 0);
}

TEST_F(DescrExtensionApiTest, ClassMethodAsDescriptorReturnsFunction) {
  ASSERT_NO_FATAL_FAILURE(createEmptyBarType());
  binaryfunc meth = [](PyObject* self, PyObject* args) {
    return PyTuple_Pack(2, self, args);
  };
  static PyMethodDef method_def;
  method_def = {"foo", meth, METH_VARARGS};
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  PyObjectPtr descriptor(PyDescr_NewClassMethod(
      reinterpret_cast<PyTypeObject*>(type.get()), &method_def));
  ASSERT_NE(descriptor, nullptr);
  PyObject_SetAttrString(type, "foo", descriptor);
  PyObjectPtr func(PyObject_GetAttrString(type, "foo"));
  ASSERT_NE(func, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr args(PyTuple_New(0));
  PyObjectPtr result(PyObject_CallObject(func, args));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_Check(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);

  // self
  PyObject* arg0 = PyTuple_GetItem(result, 0);
  ASSERT_NE(arg0, nullptr);
  EXPECT_EQ(arg0, type);

  // args
  PyObject* arg1 = PyTuple_GetItem(result, 1);
  ASSERT_NE(arg1, nullptr);
  EXPECT_EQ(args, arg1);
}

TEST_F(DescrExtensionApiTest, ClassMethodAsCallableReturnsTypeAsFirstArg) {
  ASSERT_NO_FATAL_FAILURE(createEmptyBarType());
  binaryfunc meth = [](PyObject* self, PyObject* args) {
    return PyTuple_Pack(2, self, args);
  };
  static PyMethodDef method_def;
  method_def = {"foo", meth, METH_VARARGS};
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  PyObjectPtr callable(PyDescr_NewClassMethod(
      reinterpret_cast<PyTypeObject*>(type.get()), &method_def));
  ASSERT_NE(callable, nullptr);

  PyObjectPtr args(PyTuple_New(1));
  Py_INCREF(type);  // SetItem steals a reference
  PyTuple_SetItem(args, 0, type);
  PyObjectPtr result(PyObject_CallObject(callable, args));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyTuple_Check(result), 1);
  ASSERT_EQ(PyTuple_Size(result), 2);

  // self
  PyObject* arg0 = PyTuple_GetItem(result, 0);
  ASSERT_NE(arg0, nullptr);
  EXPECT_EQ(arg0, type);

  // args
  PyObject* arg1 = PyTuple_GetItem(result, 1);
  ASSERT_NE(arg1, nullptr);
  ASSERT_EQ(PyTuple_Check(arg1), 1);
  EXPECT_EQ(PyTuple_Size(arg1), 0);
}

TEST_F(DescrExtensionApiTest, ClassMethodCallWithNoArgsRaisesTypeError) {
  ASSERT_NO_FATAL_FAILURE(createEmptyBarType());
  binaryfunc meth = [](PyObject*, PyObject*) {
    ADD_FAILURE();  // unreachable
    return Py_None;
  };
  static PyMethodDef method_def;
  method_def = {"foo", meth, METH_VARARGS};
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  PyObjectPtr callable(PyDescr_NewClassMethod(
      reinterpret_cast<PyTypeObject*>(type.get()), &method_def));
  ASSERT_NE(callable, nullptr);

  PyObjectPtr args(PyTuple_New(0));
  PyObject* result = PyObject_CallObject(callable, args);
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(DescrExtensionApiTest, ClassMethodCallWithNonBoundClassRaisesTypeError) {
  ASSERT_NO_FATAL_FAILURE(createEmptyBarType());
  binaryfunc meth = [](PyObject*, PyObject*) {
    ADD_FAILURE();  // unreachable
    return Py_None;
  };
  static PyMethodDef method_def;
  method_def = {"foo", meth, METH_VARARGS};
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  PyObjectPtr callable(PyDescr_NewClassMethod(
      reinterpret_cast<PyTypeObject*>(type.get()), &method_def));
  ASSERT_NE(callable, nullptr);

  PyObjectPtr args(PyTuple_New(1));
  PyTuple_SetItem(args, 0, PyLong_FromLong(123));
  PyObject* result = PyObject_CallObject(callable, args);
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(DescrExtensionApiTest, DictProxyNewWithMappingReturnsMappingProxy) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(54321));
  // Insert the value into the dictionary
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObjectPtr result(PyDictProxy_New(dict));
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  // Verify that __getitem__ returns the result from the embedded mapping.
  moduleSet("__main__", "foo", result);
  PyRun_SimpleString("value_from_proxy = foo[10]");
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr value_from_proxy(moduleGet("__main__", "value_from_proxy"));
  EXPECT_TRUE(PyLong_CheckExact(value_from_proxy));
  EXPECT_EQ(PyLong_AsDouble(value_from_proxy), 54321.0);

  // Verify that __setitem__ fails by raising TypeError.
  PyRun_SimpleString(R"(
type_error_caught = False
try:
  foo["random"] = 124134
except TypeError:
  type_error_caught = True
)");
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr type_error_caught(moduleGet("__main__", "type_error_caught"));
  EXPECT_EQ(type_error_caught, Py_True);
}

TEST_F(DescrExtensionApiTest, DictProxyNewWithNonMappingReturnsMappingProxy) {
  PyObjectPtr non_mapping(PyTuple_New(1));
  EXPECT_EQ(PyDictProxy_New(non_mapping), nullptr);
  EXPECT_NE(PyErr_Occurred(), nullptr);
}

TEST_F(DescrExtensionApiTest, MethodAsDescriptorReturnsFunction) {
  ASSERT_NO_FATAL_FAILURE(createEmptyBarType());
  binaryfunc meth = [](PyObject* self, PyObject* args) {
    return PyTuple_Pack(2, self, args);
  };
  static PyMethodDef method_def;
  method_def = {"foo", meth, METH_VARARGS};
  PyObjectPtr type(moduleGet("__main__", "Bar"));
  PyObjectPtr descriptor(PyDescr_NewMethod(
      reinterpret_cast<PyTypeObject*>(type.get()), &method_def));
  ASSERT_NE(descriptor, nullptr);
  PyObject_SetAttrString(type, "foo", descriptor);

  PyRun_SimpleString(R"(
bar = Bar()
r1 = bar.foo()
)");
  PyObjectPtr bar(moduleGet("__main__", "bar"));
  PyObjectPtr r1(moduleGet("__main__", "r1"));
  ASSERT_EQ(PyTuple_Check(r1), 1);
  ASSERT_EQ(PyTuple_Size(r1), 2);

  // self
  PyObject* arg0 = PyTuple_GetItem(r1, 0);
  ASSERT_NE(arg0, nullptr);
  EXPECT_EQ(arg0, bar);

  // args
  PyObject* arg1 = PyTuple_GetItem(r1, 1);
  ASSERT_NE(arg1, nullptr);
  ASSERT_EQ(PyTuple_Check(arg1), 1);
  EXPECT_EQ(PyTuple_Size(arg1), 0);
}

}  // namespace testing
}  // namespace py
