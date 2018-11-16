#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using ModuleExtensionApiTest = ExtensionApi;

TEST_F(ModuleExtensionApiTest, SpamModule) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "spam",
  };

  // PyInit_spam
  const int val = 5;
  {
    PyObject *m, *d, *de;
    m = PyModule_Create(&def);
    de = PyDict_New();
    PyModule_AddObject(m, "constants", de);

    const char* c = "CONST";
    PyObject* u = PyUnicode_FromString(c);
    PyObject* v = PyLong_FromLong(val);
    PyModule_AddIntConstant(m, c, val);
    PyDict_SetItem(de, v, u);
    ASSERT_EQ(testing::moduleSet("__main__", "spam", m), 0);
  }

  PyRun_SimpleString("x = spam.CONST");

  PyObject* x = testing::moduleGet("__main__", "x");
  int result = PyLong_AsLong(x);
  ASSERT_EQ(result, val);
}

TEST_F(ModuleExtensionApiTest, CreateAddsDocstring) {
  const char* mod_doc = "documentation for spam";
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
      mod_doc,
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  PyObject* doc = testing::moduleGet("mymodule", "__doc__");
  ASSERT_STREQ(PyUnicode_AsUTF8(doc), mod_doc);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, GetDefWithExtensionModuleRetunsNonNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
      "mydoc",
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);

  PyModuleDef* result = PyModule_GetDef(module);
  EXPECT_EQ(result, &def);
}

TEST_F(ModuleExtensionApiTest, GetDefWithNonModuleRetunsNull) {
  PyObject* integer = PyBool_FromLong(0);
  PyModuleDef* result = PyModule_GetDef(integer);
  EXPECT_EQ(result, nullptr);
}

TEST_F(ModuleExtensionApiTest, CheckTypeOnNonModuleReturnsZero) {
  PyObject* pylong = PyLong_FromLong(10);
  EXPECT_FALSE(PyModule_Check(pylong));
  EXPECT_FALSE(PyModule_CheckExact(pylong));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, CheckTypeOnModuleReturnsOne) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };
  PyObject* module = PyModule_Create(&def);
  EXPECT_TRUE(PyModule_Check(module));
  EXPECT_TRUE(PyModule_CheckExact(module));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, SetDocStringChangesDoc) {
  const char* mod_doc = "mymodule doc";
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
      mod_doc,
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  PyObject* orig_doc = testing::moduleGet("mymodule", "__doc__");
  ASSERT_NE(orig_doc, nullptr);
  EXPECT_TRUE(PyUnicode_CheckExact(orig_doc));
  ASSERT_STREQ(PyUnicode_AsUTF8(orig_doc), mod_doc);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  const char* edit_mod_doc = "edited doc";
  int result = PyModule_SetDocString(module, edit_mod_doc);
  ASSERT_EQ(result, 0);

  PyObject* edit_doc = testing::moduleGet("mymodule", "__doc__");
  ASSERT_NE(edit_doc, nullptr);
  EXPECT_TRUE(PyUnicode_CheckExact(edit_doc));
  ASSERT_STREQ(PyUnicode_AsUTF8(edit_doc), edit_mod_doc);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, SetDocStringCreatesDoc) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  const char* edit_mod_doc = "edited doc";
  ASSERT_EQ(PyModule_SetDocString(module, edit_mod_doc), 0);

  PyObject* doc = testing::moduleGet("mymodule", "__doc__");
  ASSERT_STREQ(PyUnicode_AsUTF8(doc), edit_mod_doc);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}
}  // namespace python
