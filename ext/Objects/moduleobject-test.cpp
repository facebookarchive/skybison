#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using ModuleExtensionApiTest = ExtensionApi;
// Used to convert non-capturing closures into function pointers.
using slot_func = int (*)(PyObject*);

TEST_F(ModuleExtensionApiTest, SpamModule) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "spam",
  };

  // PyInit_spam
  const int val = 5;
  {
    PyObject* m = PyModule_Create(&def);
    PyObject* de = PyDict_New();
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

TEST_F(ModuleExtensionApiTest, NewObjectWithNonStringNameReturnsModule) {
  testing::PyObjectPtr long_name(PyLong_FromLong(2));
  testing::PyObjectPtr module(PyModule_NewObject(long_name));
  ASSERT_TRUE(PyModule_CheckExact(module));

  testing::PyObjectPtr mod_name(PyObject_GetAttrString(module, "__name__"));
  EXPECT_EQ(mod_name, long_name);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, NewObjectDoesNotAddModuleToModuleDict) {
  testing::PyObjectPtr name(PyUnicode_FromString("mymodule"));
  testing::PyObjectPtr module(PyModule_NewObject(name));
  ASSERT_TRUE(PyModule_CheckExact(module));

  testing::PyObjectPtr mods(PyImport_GetModuleDict());
  testing::PyObjectPtr item(PyDict_GetItem(mods, name));
  EXPECT_EQ(item, nullptr);

  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, NewWithEmptyStringReturnsModule) {
  testing::PyObjectPtr module(PyModule_New(""));
  ASSERT_TRUE(PyModule_CheckExact(module));

  testing::PyObjectPtr mod_name(PyObject_GetAttrString(module, "__name__"));
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(mod_name, ""));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, NewDoesNotAddModuleToModuleDict) {
  testing::PyObjectPtr module(PyModule_New("mymodule"));
  ASSERT_TRUE(PyModule_CheckExact(module));

  PyObject* mods = PyImport_GetModuleDict();
  testing::PyObjectPtr name(PyUnicode_FromString("mymodule"));
  PyObject* item = PyDict_GetItem(mods, name);
  EXPECT_EQ(item, nullptr);

  EXPECT_EQ(PyErr_Occurred(), nullptr);
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

  PyObject* doc = PyObject_GetAttrString(module, "__doc__");
  ASSERT_STREQ(PyUnicode_AsUTF8(doc), mod_doc);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, CreateSetsStateNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  ASSERT_EQ(PyModule_GetState(module), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, GetStateAllocatesAndAllowsMutation) {
  struct MyState {
    char letter;
    int number;
    double big_number;
    PyObject* object;
  };

  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
      "doc",
      sizeof(MyState),
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  void* state = PyModule_GetState(module);
  ASSERT_NE(state, nullptr);
  MyState* mod_state = static_cast<MyState*>(state);
  mod_state->letter = 'a';
  mod_state->number = 2;
  mod_state->big_number = 2.1;
  testing::PyObjectPtr unique_obj(testing::createUniqueObject());
  mod_state->object = unique_obj;

  ASSERT_EQ(PyModule_GetState(module), state);
  EXPECT_EQ(mod_state->letter, 'a');
  EXPECT_EQ(mod_state->number, 2);
  EXPECT_EQ(mod_state->big_number, 2.1);
  EXPECT_EQ(mod_state->object, unique_obj);

  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, GetStateFailsOnNonModule) {
  testing::PyObjectPtr not_a_module(testing::createUniqueObject());

  EXPECT_EQ(PyModule_GetState(not_a_module), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
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

  PyObject* orig_doc = PyObject_GetAttrString(module, "__doc__");
  ASSERT_NE(orig_doc, nullptr);
  EXPECT_TRUE(PyUnicode_CheckExact(orig_doc));
  ASSERT_STREQ(PyUnicode_AsUTF8(orig_doc), mod_doc);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  const char* edit_mod_doc = "edited doc";
  int result = PyModule_SetDocString(module, edit_mod_doc);
  ASSERT_EQ(result, 0);

  PyObject* edit_doc = PyObject_GetAttrString(module, "__doc__");
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

  PyObject* doc = PyObject_GetAttrString(module, "__doc__");
  ASSERT_STREQ(PyUnicode_AsUTF8(doc), edit_mod_doc);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, ModuleCreateDoesNotAddToModuleDict) {
  const char* name = "mymodule";
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      name,
  };
  ASSERT_NE(PyModule_Create(&def), nullptr);
  PyObject* mods = PyImport_GetModuleDict();
  PyObject* name_obj = PyUnicode_FromString(name);
  EXPECT_EQ(PyDict_GetItem(mods, name_obj), nullptr);
}

TEST_F(ModuleExtensionApiTest, GetNameObjectGetsName) {
  const char* mod_name = "mymodule";
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      mod_name,
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_Check(module));

  PyObject* result = PyModule_GetNameObject(module);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyUnicode_Check(result));

  EXPECT_STREQ(PyUnicode_AsUTF8(result), mod_name);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  Py_DECREF(result);

  Py_DECREF(module);
}

TEST_F(ModuleExtensionApiTest, GetNameObjectFailsIfNotModule) {
  PyObject* not_a_module = PyTuple_New(10);
  PyObject* result = PyModule_GetNameObject(not_a_module);
  EXPECT_EQ(result, nullptr);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));

  Py_DECREF(not_a_module);
}

TEST_F(ModuleExtensionApiTest, GetNameObjectFailsIfNotString) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  PyObject* not_a_module = PyTuple_New(10);
  PyObject_SetAttrString(module, "__name__", not_a_module);
  PyObject* result = PyModule_GetNameObject(module);
  EXPECT_EQ(result, nullptr);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));

  Py_DECREF(module);
  Py_DECREF(not_a_module);
}

TEST_F(ModuleExtensionApiTest, GetFilenameObjectReturnsFilename) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_Check(module));

  const char* filename = "file";
  PyModule_AddObject(module, "__file__", PyUnicode_FromString(filename));
  testing::PyObjectPtr result(PyModule_GetFilenameObject(module));

  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyUnicode_Check(result));
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(result, filename));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, GetFilenameObjectFailsIfNotModule) {
  testing::PyObjectPtr not_a_module(PyLong_FromLong(1));
  testing::PyObjectPtr result(PyModule_GetFilenameObject(not_a_module));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ModuleExtensionApiTest, GetFilenameObjectFailsIfFilenameNotString) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  PyObject* module = PyModule_Create(&def);
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  testing::PyObjectPtr not_a_string(PyLong_FromLong(1));

  PyModule_AddObject(module, "__file__", not_a_string);
  testing::PyObjectPtr result(PyModule_GetFilenameObject(module));
  EXPECT_EQ(result, nullptr);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ModuleExtensionApiTest, ExecDefReturnsZeroWithNoSlots) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  EXPECT_EQ(PyModule_ExecDef(module, &def), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, ExecDefFailsIfPassedNamelessModule) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  testing::PyObjectPtr module(PyModule_NewObject(Py_None));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  EXPECT_EQ(PyModule_ExecDef(module, &def), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);
}

// TODO(T37048769): Replace _Create with _FromDefAndSpec and run with CPython
TEST_F(ModuleExtensionApiTest, ExecDefFailsIfDefHasUnknownSlotPyro) {
  slot_func mod_exec = [](PyObject* module) {
    PyModule_SetDocString(module, "testing");
    return 0;
  };

  static PyModuleDef_Slot slots[] = {
      {-1, reinterpret_cast<void*>(mod_exec)},
      {0, nullptr},
  };
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "mymodule", nullptr, 0, nullptr, slots,
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  EXPECT_EQ(PyModule_ExecDef(module, &def), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

// TODO(T37048769): Replace _Create with _FromDefAndSpec and run with CPython
TEST_F(ModuleExtensionApiTest, ExecDefRunsCorrectSingleSlotPyro) {
  slot_func mod_exec = [](PyObject* module) {
    PyModule_SetDocString(module, "testing");
    return 0;
  };

  static PyModuleDef_Slot slots[] = {
      {Py_mod_exec, reinterpret_cast<void*>(mod_exec)},
      {0, nullptr},
  };
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "mymodule", nullptr, 0, nullptr, slots,
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  EXPECT_EQ(PyModule_ExecDef(module, &def), 0);

  testing::PyObjectPtr doc(PyObject_GetAttrString(module, "__doc__"));
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(doc, "testing"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

// TODO(T37048769): Replace _Create with _FromDefAndSpec and run with CPython
TEST_F(ModuleExtensionApiTest, ExecDefRunsMultipleSlotsInOrderPyro) {
  slot_func mod_exec = [](PyObject* module) {
    PyModule_SetDocString(module, "doc test");
    return 0;
  };

  slot_func mod_exec_second = [](PyObject* module) {
    if (PyObject_GetAttrString(module, "__doc__") != nullptr) {
      PyObject_SetAttrString(module, "test1", PyUnicode_FromString("testing1"));
    }
    return 0;
  };

  slot_func mod_exec_third = [](PyObject* module) {
    if (PyObject_GetAttrString(module, "__doc__") != nullptr) {
      PyObject_SetAttrString(module, "test2", PyUnicode_FromString("testing2"));
    }
    return 0;
  };

  static PyModuleDef_Slot slots[] = {
      {Py_mod_exec, reinterpret_cast<void*>(mod_exec)},
      {Py_mod_exec, reinterpret_cast<void*>(mod_exec_second)},
      {Py_mod_exec, reinterpret_cast<void*>(mod_exec_third)},
      {0, nullptr},
  };
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "mymodule", nullptr, 0, nullptr, slots,
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  EXPECT_EQ(PyModule_ExecDef(module, &def), 0);

  testing::PyObjectPtr doc(PyObject_GetAttrString(module, "__doc__"));
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(doc, "doc test"));
  testing::PyObjectPtr test_attr_one(PyObject_GetAttrString(module, "test1"));
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(test_attr_one, "testing1"));
  testing::PyObjectPtr test_attr_two(PyObject_GetAttrString(module, "test2"));
  EXPECT_TRUE(_PyUnicode_EqualToASCIIString(test_attr_two, "testing2"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

// TODO(T37048769): Replace _Create with _FromDefAndSpec and run with CPython
TEST_F(ModuleExtensionApiTest, ExecDefFailsIfSlotHasErrorButReturnsZeroPyro) {
  slot_func mod_exec_fail_silently = [](PyObject* module) {
    testing::PyObjectPtr attr(PyObject_GetAttrString(module, "non-existant"));
    return 0;
  };

  static PyModuleDef_Slot slots[] = {
      {Py_mod_exec, reinterpret_cast<void*>(mod_exec_fail_silently)},
      {0, nullptr},
  };
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "mymodule", nullptr, 0, nullptr, slots,
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  EXPECT_EQ(PyModule_ExecDef(module, &def), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

// TODO(T37048769): Replace _Create with _FromDefAndSpec and run with CPython
TEST_F(ModuleExtensionApiTest, ExecDefFailsIfSlotFailsButDoesntSetErrorPyro) {
  slot_func mod_exec_fail_no_error = [](PyObject* module) {
    testing::PyObjectPtr attr(PyObject_GetAttrString(module, "non-existant"));
    PyErr_Clear();
    return -1;
  };

  static PyModuleDef_Slot slots[] = {
      {Py_mod_exec, reinterpret_cast<void*>(mod_exec_fail_no_error)},
      {0, nullptr},
  };
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "mymodule", nullptr, 0, nullptr, slots,
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  EXPECT_EQ(PyModule_ExecDef(module, &def), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

// TODO(T37048769): Replace _Create with _FromDefAndSpec and run with CPython
TEST_F(ModuleExtensionApiTest, ExecDefFailsIfSlotFailsAndPropogatesErrorPyro) {
  slot_func mod_exec_fail = [](PyObject* module) {
    testing::PyObjectPtr attr(PyObject_GetAttrString(module, "non-existant"));
    return -1;
  };

  static PyModuleDef_Slot slots[] = {
      {Py_mod_exec, reinterpret_cast<void*>(mod_exec_fail)},
      {0, nullptr},
  };
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "mymodule", nullptr, 0, nullptr, slots,
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  EXPECT_EQ(PyModule_ExecDef(module, &def), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(ModuleExtensionApiTest, GetNameGetsName) {
  const char* mod_name = "mymodule";
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      mod_name,
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_Check(module));

  EXPECT_STREQ(PyModule_GetName(module), mod_name);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, GetNameReturnsNullIfNoName) {
  testing::PyObjectPtr not_a_module(PyLong_FromLong(1));
  EXPECT_EQ(PyModule_GetName(not_a_module), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ModuleExtensionApiTest, GetNameDoesNotIncrementModuleNameRefcount) {
  const char* mod_name = "mymodule";
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      mod_name,
  };

  testing::PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_Check(module));

  PyObject* name = PyModule_GetNameObject(module);
  ASSERT_NE(name, nullptr);
  EXPECT_TRUE(PyUnicode_Check(name));

  Py_ssize_t name_count = Py_REFCNT(name);
  EXPECT_STREQ(PyModule_GetName(module), mod_name);
  EXPECT_EQ(Py_REFCNT(name), name_count);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace python
