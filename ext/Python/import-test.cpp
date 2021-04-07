#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using ImportExtensionApiTest = ExtensionApi;

TEST_F(ImportExtensionApiTest, AddNonExistingModuleReturnsNewModule) {
  const char* c_name = "foo";
  PyObject* pyname = PyUnicode_FromString(c_name);
  PyObject* new_module = PyImport_AddModuleObject(pyname);
  ASSERT_TRUE(PyModule_CheckExact(new_module));

  PyObject* module_name = PyModule_GetNameObject(new_module);
  EXPECT_TRUE(isUnicodeEqualsCStr(module_name, c_name));
  Py_DECREF(module_name);

  PyObject* module = testing::importGetModule(pyname);
  EXPECT_EQ(new_module, module);
  Py_DECREF(pyname);
  Py_DECREF(module);
}

TEST_F(ImportExtensionApiTest, AddExistingModuleReturnsModule) {
  PyObject* pyname = PyUnicode_FromString("sys");
  PyObject* module = PyImport_AddModuleObject(pyname);
  ASSERT_TRUE(PyModule_CheckExact(module));

  Py_ssize_t refcnt = Py_REFCNT(module);
  PyObject* module2 = PyImport_AddModuleObject(pyname);
  EXPECT_EQ(refcnt, Py_REFCNT(module2));
  EXPECT_EQ(module, module2);
  Py_DECREF(pyname);
}

TEST_F(ImportExtensionApiTest, ExecCodeModulePopulatesModuleFromCAPI) {
  PyObjectPtr code_object(
      Py_CompileString("a = 21 + 21", "test_module.py", Py_file_input));
  ASSERT_NE(code_object, nullptr);

  PyObjectPtr module(PyImport_ExecCodeModule("test_module", code_object));
  ASSERT_NE(module, nullptr);

  PyObjectPtr a(PyObject_GetAttrString(module, "a"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isLongEqualsLong(a, 42));
}

TEST_F(ImportExtensionApiTest, ExecCodeModuleAddsModuleToModules) {
  PyObjectPtr code(
      Py_CompileString("a = 21 + 21", "test_module.py", Py_file_input));
  ASSERT_NE(code, nullptr);

  PyObjectPtr module(PyImport_ExecCodeModule("test_module", code));
  ASSERT_NE(module, nullptr);

  PyImport_ImportModule("test_module");
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr a(moduleGet("test_module", "a"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isLongEqualsLong(a, 42));
}

TEST_F(ImportExtensionApiTest, ExecCodeModuleWithInvalidCodeDoesNotAddModule) {
  PyObjectPtr code(
      Py_CompileString("b = nonexistent.foo", "test_module.py", Py_file_input));
  ASSERT_NE(code, nullptr);

  PyObjectPtr module(PyImport_ExecCodeModule("test_module", code));
  ASSERT_EQ(module, nullptr);
  PyErr_Clear();

  PyImport_ImportModule("test_module");
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ModuleNotFoundError));
}

TEST_F(ImportExtensionApiTest,
       GetMagicNumberWithNonIntMagicNumberRaisesTypeError) {
  PyObjectPtr importlib(PyImport_ImportModule("_frozen_importlib_external"));
  ASSERT_EQ(PyObject_SetAttrString(importlib, "_RAW_MAGIC_NUMBER", Py_None), 0);
  EXPECT_EQ(PyImport_GetMagicNumber(), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ImportExtensionApiTest,
       GetMagicNumberReturnsMagicNumberFromImportlibExternal) {
  EXPECT_NE(PyImport_GetMagicNumber(), -1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ImportExtensionApiTest, GetModuleWithNotExistingModuleNameReturnsNull) {
  PyObjectPtr name(PyUnicode_FromString("not_existing"));
  EXPECT_EQ(PyImport_GetModule(name), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ImportExtensionApiTest,
       GetModuleWithNotNotYetLoadedModuleNameReturnsNull) {
  PyObjectPtr name(PyUnicode_FromString("test"));
  EXPECT_EQ(PyImport_GetModule(name), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ImportExtensionApiTest,
       GetModuleWithObjectRaisingNonKeyErrorPropagatesIt) {
  PyRun_SimpleString(R"(
class C:
  def __hash__(self):
    raise UserWarning("do not call me")

c = C()
  )");
  PyObjectPtr c(mainModuleGet("c"));
  EXPECT_EQ(PyImport_GetModule(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_UserWarning));
}

// TODO(T67033642): Add a test verifying that a non-dict modules supresses
// KeyError.
TEST_F(ImportExtensionApiTest, GetModuleWithObjectRaisingKeyErrorPropagatesIt) {
  PyRun_SimpleString(R"(
class C:
  def __hash__(self):
    raise KeyError("key_error")

c = C()
  )");
  PyObjectPtr c(mainModuleGet("c"));
  EXPECT_EQ(PyImport_GetModule(c), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_KeyError));
}

TEST_F(ImportExtensionApiTest, GetModuleWithExistingModuleNameReturnsModule) {
  PyObjectPtr imported_module(PyImport_ImportModule("test"));
  ASSERT_TRUE(PyModule_Check(imported_module));
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr name(PyUnicode_FromString("test"));
  PyObjectPtr found_module(PyImport_GetModule(name));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyModule_Check(found_module));
  EXPECT_EQ(imported_module, found_module);
}

TEST_F(ImportExtensionApiTest, PyImportAcquireLockAndReleaseLockDoesNothing) {
  _PyImport_AcquireLock();
  EXPECT_EQ(_PyImport_ReleaseLock(), 1);
}

TEST_F(ImportExtensionApiTest,
       PyImportReleaseLockWithoutAcquireLockReturnsMinusOne) {
  EXPECT_EQ(_PyImport_ReleaseLock(), -1);
}

TEST_F(ImportExtensionApiTest, ImportInvalidModuleReturnsNull) {
  PyObject* module = PyImport_ImportModule("this_file_should_not_exist");
  ASSERT_EQ(module, nullptr);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ModuleNotFoundError));
}

TEST_F(ImportExtensionApiTest, ImportModuleReturnsModule) {
  PyObject* module = PyImport_ImportModule("operator");
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
  Py_DECREF(module);
}

TEST_F(ImportExtensionApiTest,
       ImportModuleWithSubmoduleReturnsLowestLevelModule) {
  PyObjectPtr module(PyImport_ImportModule("collections.abc"));
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
  PyObjectPtr name(PyModule_GetNameObject(module));
  ASSERT_NE(name, nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(name, "collections.abc"));
}

TEST_F(ImportExtensionApiTest, ImportModuleNoBlockReturnsModule) {
  PyObject* module = PyImport_ImportModuleNoBlock("operator");
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
  Py_DECREF(module);
}

TEST_F(ImportExtensionApiTest, ImportFrozenModuleReturnsZeroPyro) {
  int result = PyImport_ImportFrozenModule("operator");
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, 0);
}

TEST_F(ImportExtensionApiTest, ImportReturnsModule) {
  PyObject* name = PyUnicode_FromString("operator");
  PyObject* module = PyImport_Import(name);
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
  Py_DECREF(name);
  Py_DECREF(module);
}

TEST_F(ImportExtensionApiTest, ImportFrozenModuleObjectReturnsZeroPyro) {
  PyObject* name = PyUnicode_FromString("operator");
  int result = PyImport_ImportFrozenModuleObject(name);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, 0);
  Py_DECREF(name);
}

TEST_F(ImportExtensionApiTest, ImportModuleLevelReturnsModule) {
  PyObject* globals = PyDict_New();
  PyObject* fromlist = PyList_New(0);
  PyObject* module =
      PyImport_ImportModuleLevel("operator", globals, nullptr, fromlist, 0);
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
  Py_DECREF(globals);
  Py_DECREF(fromlist);
  Py_DECREF(module);
}

TEST_F(ImportExtensionApiTest, ImportModuleLevelObjectReturnsModule) {
  PyObject* name = PyUnicode_FromString("operator");
  PyObject* globals = PyDict_New();
  PyObject* fromlist = PyList_New(0);
  PyObject* module =
      PyImport_ImportModuleLevelObject(name, globals, nullptr, fromlist, 0);
  ASSERT_NE(module, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyModule_Check(module));
  Py_DECREF(globals);
  Py_DECREF(fromlist);
  Py_DECREF(name);
  Py_DECREF(module);
}

TEST_F(ImportExtensionApiTest,
       ImportModuleLevelObjectWithNullNameRaisesValueError) {
  PyObject* globals = PyDict_New();
  PyObject* fromlist = PyList_New(0);
  PyObject* module =
      PyImport_ImportModuleLevelObject(nullptr, globals, nullptr, fromlist, 0);
  EXPECT_EQ(module, nullptr);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
  Py_DECREF(globals);
  Py_DECREF(fromlist);
}

TEST_F(ImportExtensionApiTest,
       ImportModuleLevelObjectWithNegativeLevelRaisesValueError) {
  PyObject* name = PyUnicode_FromString("operator");
  PyObject* globals = PyDict_New();
  PyObject* fromlist = PyList_New(0);
  PyObject* module =
      PyImport_ImportModuleLevelObject(name, globals, nullptr, fromlist, -1);
  EXPECT_EQ(module, nullptr);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
  Py_DECREF(globals);
  Py_DECREF(fromlist);
  Py_DECREF(name);
}

TEST_F(ImportExtensionApiTest,
       ImportModuleLevelObjectWithNullGlobalsRaisesKeyError) {
  PyObject* name = PyUnicode_FromString("operator");
  PyObject* fromlist = PyList_New(0);
  PyObject* module =
      PyImport_ImportModuleLevelObject(name, nullptr, nullptr, fromlist, 1);
  EXPECT_EQ(module, nullptr);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_KeyError));
  Py_DECREF(fromlist);
  Py_DECREF(name);
}

}  // namespace testing
}  // namespace py
