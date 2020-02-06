#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

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
  const long val = 5;
  {
    PyObjectPtr m(PyModule_Create(&def));
    PyObject* de = PyDict_New();
    PyModule_AddObject(m, "constants", de);

    const char* c = "CONST";
    PyObjectPtr u(PyUnicode_FromString(c));
    PyObjectPtr v(PyLong_FromLong(val));
    PyModule_AddIntConstant(m, c, val);
    PyDict_SetItem(de, v, u);
    ASSERT_EQ(testing::moduleSet("__main__", "spam", m), 0);
  }

  PyRun_SimpleString("x = spam.CONST");

  PyObjectPtr x(testing::moduleGet("__main__", "x"));
  long result = PyLong_AsLong(x);
  ASSERT_EQ(result, val);
}

TEST_F(ModuleExtensionApiTest, GetDictReturnsMapping) {
  PyRun_SimpleString(R"(
foo = 42
)");
  PyObjectPtr name(PyUnicode_FromString("__main__"));
  PyObjectPtr main(importGetModule(name));
  ASSERT_TRUE(PyModule_Check(main));
  PyObject* module_dict = PyModule_GetDict(main);
  PyObjectPtr value(PyMapping_GetItemString(module_dict, "foo"));
  EXPECT_TRUE(isLongEqualsLong(value, 42));
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

  PyObject* mods = PyImport_GetModuleDict();
  PyObject* item = PyDict_GetItem(mods, name);
  EXPECT_EQ(item, nullptr);

  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, NewWithEmptyStringReturnsModule) {
  testing::PyObjectPtr module(PyModule_New(""));
  ASSERT_TRUE(PyModule_CheckExact(module));

  testing::PyObjectPtr mod_name(PyObject_GetAttrString(module, "__name__"));
  EXPECT_TRUE(isUnicodeEqualsCStr(mod_name, ""));
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

  PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  PyObjectPtr doc(PyObject_GetAttrString(module, "__doc__"));
  EXPECT_TRUE(isUnicodeEqualsCStr(doc, mod_doc));
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
  testing::PyObjectPtr unique_obj(PyTuple_New(0));
  mod_state->object = unique_obj;

  ASSERT_EQ(PyModule_GetState(module), state);
  EXPECT_EQ(mod_state->letter, 'a');
  EXPECT_EQ(mod_state->number, 2);
  EXPECT_EQ(mod_state->big_number, 2.1);
  EXPECT_EQ(mod_state->object, unique_obj);

  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, GetStateFailsOnNonModule) {
  testing::PyObjectPtr not_a_module(PyLong_FromLong(0));

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

  PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);

  PyModuleDef* result = PyModule_GetDef(module);
  EXPECT_EQ(result, &def);
}

TEST_F(ModuleExtensionApiTest, GetDefWithNonModuleRetunsNull) {
  PyObject* integer = PyBool_FromLong(0);
  PyModuleDef* result = PyModule_GetDef(integer);
  EXPECT_EQ(result, nullptr);
}

TEST_F(ModuleExtensionApiTest, GetDefWithNonExtensionModuleReturnsNull) {
  PyRun_SimpleString("");
  PyObjectPtr module_name(PyUnicode_FromString("__main__"));
  PyObjectPtr main_module(testing::importGetModule(module_name));
  PyModuleDef* result = PyModule_GetDef(main_module);
  EXPECT_EQ(result, nullptr);
}

TEST_F(ModuleExtensionApiTest, CheckTypeOnNonModuleReturnsZero) {
  PyObjectPtr pylong(PyLong_FromLong(10));
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
  PyObjectPtr module(PyModule_Create(&def));
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

  PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  PyObjectPtr orig_doc(PyObject_GetAttrString(module, "__doc__"));
  EXPECT_TRUE(isUnicodeEqualsCStr(orig_doc, mod_doc));
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  const char* edit_mod_doc = "edited doc";
  int result = PyModule_SetDocString(module, edit_mod_doc);
  ASSERT_EQ(result, 0);

  PyObjectPtr edit_doc(PyObject_GetAttrString(module, "__doc__"));
  EXPECT_TRUE(isUnicodeEqualsCStr(edit_doc, edit_mod_doc));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, SetDocStringCreatesDoc) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "mymodule",
  };

  PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  const char* edit_mod_doc = "edited doc";
  ASSERT_EQ(PyModule_SetDocString(module, edit_mod_doc), 0);

  PyObjectPtr doc(PyObject_GetAttrString(module, "__doc__"));
  EXPECT_TRUE(isUnicodeEqualsCStr(doc, edit_mod_doc));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, SetDocStringSetsObjectAttribute) {
  PyRun_SimpleString(R"(
class C: pass
not_a_module = C()
)");
  PyObjectPtr not_a_module(moduleGet("__main__", "not_a_module"));
  EXPECT_EQ(PyModule_SetDocString(not_a_module, "baz"), 0);
  PyObjectPtr value(PyObject_GetAttrString(not_a_module, "__doc__"));
  EXPECT_TRUE(isUnicodeEqualsCStr(value, "baz"));
}

TEST_F(ModuleExtensionApiTest, ModuleCreateDoesNotAddToModuleDict) {
  const char* name = "mymodule";
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      name,
  };
  PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  PyObject* mods = PyImport_GetModuleDict();
  PyObjectPtr name_obj(PyUnicode_FromString(name));
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
  EXPECT_TRUE(isUnicodeEqualsCStr(result, mod_name));
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

TEST_F(ModuleExtensionApiTest, GetNameObjectWithModuleSubclassReturnsString) {
  PyRun_SimpleString(R"(
import builtins
ModuleType = type(builtins)
class C(ModuleType):
  pass
module = C("foo")
)");
  PyObjectPtr module(moduleGet("__main__", "module"));
  PyObjectPtr result(PyModule_GetNameObject(module));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "foo"));
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
  EXPECT_TRUE(isUnicodeEqualsCStr(result, filename));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, GetFilenameObjectWithSubclassReturnsFilename) {
  PyRun_SimpleString(R"(
import builtins
ModuleType = type(builtins)
class C(ModuleType):
  __file__ = "bar"
module = C("foo")
module.__file__ = "baz"
)");
  PyObjectPtr module(moduleGet("__main__", "module"));
  PyObjectPtr result(PyModule_GetFilenameObject(module));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "baz"));
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

  PyObjectPtr module(PyModule_Create(&def));
  ASSERT_NE(module, nullptr);
  EXPECT_TRUE(PyModule_CheckExact(module));

  PyObject* not_a_string = PyLong_FromLong(1);

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
  EXPECT_TRUE(isUnicodeEqualsCStr(doc, "testing"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

// TODO(T37048769): Replace _Create with _FromDefAndSpec and run with CPython
TEST_F(ModuleExtensionApiTest, ExecDefRunsMultipleSlotsInOrderPyro) {
  slot_func mod_exec = [](PyObject* module) {
    PyModule_SetDocString(module, "doc test");
    return 0;
  };

  slot_func mod_exec_second = [](PyObject* module) {
    PyObjectPtr doc(PyObject_GetAttrString(module, "__doc__"));
    if (doc != nullptr) {
      PyObjectPtr attr(PyUnicode_FromString("testing1"));
      PyObject_SetAttrString(module, "test1", attr);
    }
    return 0;
  };

  slot_func mod_exec_third = [](PyObject* module) {
    PyObjectPtr doc(PyObject_GetAttrString(module, "__doc__"));
    if (doc != nullptr) {
      PyObjectPtr attr(PyUnicode_FromString("testing2"));
      PyObject_SetAttrString(module, "test2", attr);
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
  EXPECT_TRUE(isUnicodeEqualsCStr(doc, "doc test"));
  testing::PyObjectPtr test_attr_one(PyObject_GetAttrString(module, "test1"));
  EXPECT_TRUE(isUnicodeEqualsCStr(test_attr_one, "testing1"));
  testing::PyObjectPtr test_attr_two(PyObject_GetAttrString(module, "test2"));
  EXPECT_TRUE(isUnicodeEqualsCStr(test_attr_two, "testing2"));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

// TODO(T37048769): Replace _Create with _FromDefAndSpec and run with CPython
TEST_F(ModuleExtensionApiTest, ExecDefFailsIfSlotHasErrorButReturnsZeroPyro) {
  slot_func mod_exec_fail_silently = [](PyObject* module) {
    testing::PyObjectPtr attr(PyObject_GetAttrString(module, "non-existent"));
    static_cast<void>(attr);
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
    testing::PyObjectPtr attr(PyObject_GetAttrString(module, "non-existent"));
    static_cast<void>(attr);
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
    testing::PyObjectPtr attr(PyObject_GetAttrString(module, "non-existent"));
    static_cast<void>(attr);
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

  PyObjectPtr name(PyModule_GetNameObject(module));
  EXPECT_TRUE(isUnicodeEqualsCStr(name, mod_name));

  Py_ssize_t name_count = Py_REFCNT(name);
  EXPECT_STREQ(PyModule_GetName(module), mod_name);
  EXPECT_EQ(Py_REFCNT(name), name_count);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, MethodNoArgsReturnsPyLong) {
  binaryfunc func = [](PyObject*, PyObject*) { return PyLong_FromLong(10); };
  PyMethodDef foo_methods[] = {{"noargs", func, METH_NOARGS}, {nullptr}};
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "foo", nullptr, 0, foo_methods,
  };
  PyObjectPtr module(PyModule_Create(&def));
  moduleSet("__main__", "foo", module);
  ASSERT_TRUE(PyModule_CheckExact(module));
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyRun_SimpleString(R"(
x = foo.noargs()
)");

  PyObjectPtr x(moduleGet("__main__", "x"));
  long result = PyLong_AsLong(x);
  ASSERT_EQ(result, 10);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, MethodWithClassFlagRaisesException) {
  binaryfunc foo_func = [](PyObject*, PyObject*) {
    return PyLong_FromLong(10);
  };
  PyMethodDef foo_methods[] = {
      {"longValue", foo_func, METH_NOARGS | METH_CLASS}, {nullptr}};
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "foo", nullptr, 0, foo_methods,
  };
  PyObjectPtr module(PyModule_Create(&def));
  EXPECT_EQ(module, nullptr);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(ModuleExtensionApiTest, MethodWithFastCallReturnsArg) {
  _PyCFunctionFast foo_func = [](PyObject* module, PyObject** args,
                                 Py_ssize_t num_args,
                                 PyObject* kwnames) -> PyObject* {
    EXPECT_TRUE(PyModule_Check(module));
    int value;
    static const char* const keywords[] = {"input", nullptr};
    static _PyArg_Parser parser = {"i:fastcall", keywords};
    EXPECT_EQ(_PyArg_ParseStack(args, num_args, kwnames, &parser, &value), 1);
    return PyLong_FromLong(value);
  };
  PyMethodDef foo_methods[] = {
      {"fastcall",
       reinterpret_cast<binaryfunc>(reinterpret_cast<void*>(foo_func)),
       METH_FASTCALL},
      {nullptr}};
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "foo", nullptr, 0, foo_methods,
  };
  PyObjectPtr module(PyModule_Create(&def));
  moduleSet("__main__", "foo", module);
  ASSERT_TRUE(PyModule_CheckExact(module));
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyRun_SimpleString(R"(
x = foo.fastcall(10)
)");
  PyObjectPtr result(moduleGet("__main__", "x"));
  ASSERT_EQ(isLongEqualsLong(result, 10), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, MethodWithFastCallKwReturnsArg) {
  _PyCFunctionFast foo_func = [](PyObject* module, PyObject** args,
                                 Py_ssize_t num_args,
                                 PyObject* kwnames) -> PyObject* {
    EXPECT_TRUE(PyModule_Check(module));
    int value;
    static const char* const keywords[] = {"input", nullptr};
    static _PyArg_Parser parser = {"i:fastcall", keywords};
    EXPECT_EQ(_PyArg_ParseStack(args, num_args, kwnames, &parser, &value), 1);
    return PyLong_FromLong(value);
  };
  PyMethodDef foo_methods[] = {
      {"fastcall",
       reinterpret_cast<binaryfunc>(reinterpret_cast<void*>(foo_func)),
       METH_FASTCALL},
      {nullptr}};
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "foo", nullptr, 0, foo_methods,
  };
  PyObjectPtr module(PyModule_Create(&def));
  moduleSet("__main__", "foo", module);
  ASSERT_TRUE(PyModule_CheckExact(module));
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyRun_SimpleString(R"(
z = foo.fastcall(input=30)
)");
  PyObjectPtr result(moduleGet("__main__", "z"));
  ASSERT_EQ(isLongEqualsLong(result, 30), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, MethodWithFastCallExTupleReturnsArg) {
  _PyCFunctionFast foo_func = [](PyObject* module, PyObject** args,
                                 Py_ssize_t num_args,
                                 PyObject* kwnames) -> PyObject* {
    EXPECT_TRUE(PyModule_Check(module));
    int value;
    static const char* const keywords[] = {"input", nullptr};
    static _PyArg_Parser parser = {"i:fastcall", keywords};
    EXPECT_EQ(_PyArg_ParseStack(args, num_args, kwnames, &parser, &value), 1);
    return PyLong_FromLong(value);
  };
  PyMethodDef foo_methods[] = {
      {"fastcall",
       reinterpret_cast<binaryfunc>(reinterpret_cast<void*>(foo_func)),
       METH_FASTCALL},
      {nullptr}};
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "foo", nullptr, 0, foo_methods,
  };
  PyObjectPtr module(PyModule_Create(&def));
  moduleSet("__main__", "foo", module);
  ASSERT_TRUE(PyModule_CheckExact(module));
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyRun_SimpleString(R"(
args = (20,)
y = foo.fastcall(*args)
)");
  PyObjectPtr result(moduleGet("__main__", "y"));
  ASSERT_EQ(isLongEqualsLong(result, 20), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, MethodWithFastCallExDictReturnsArg) {
  _PyCFunctionFast foo_func = [](PyObject* module, PyObject** args,
                                 Py_ssize_t num_args,
                                 PyObject* kwnames) -> PyObject* {
    EXPECT_TRUE(PyModule_Check(module));
    int value;
    static const char* const keywords[] = {"input", nullptr};
    static _PyArg_Parser parser = {"i:fastcall", keywords};
    EXPECT_EQ(_PyArg_ParseStack(args, num_args, kwnames, &parser, &value), 1);
    return PyLong_FromLong(value);
  };
  PyMethodDef foo_methods[] = {
      {"fastcall",
       reinterpret_cast<binaryfunc>(reinterpret_cast<void*>(foo_func)),
       METH_FASTCALL},
      {nullptr}};
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "foo", nullptr, 0, foo_methods,
  };
  PyObjectPtr module(PyModule_Create(&def));
  moduleSet("__main__", "foo", module);
  ASSERT_TRUE(PyModule_CheckExact(module));
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyRun_SimpleString(R"(
r = foo.fastcall(**{'input': 40})
)");
  PyObjectPtr result(moduleGet("__main__", "r"));
  ASSERT_EQ(isLongEqualsLong(result, 40), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, MethodWithVariableArgsReturnsArg) {
  binaryfunc foo_func = [](PyObject*, PyObject* args) -> PyObject* {
    int value;
    EXPECT_EQ(PyArg_ParseTuple(args, "i", &value), 1);
    return PyLong_FromLong(value);
  };
  PyMethodDef foo_methods[] = {{"varargs", foo_func, METH_VARARGS}, {nullptr}};
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "foo", nullptr, 0, foo_methods,
  };
  PyObjectPtr module(PyModule_Create(&def));
  moduleSet("__main__", "foo", module);
  ASSERT_TRUE(PyModule_CheckExact(module));
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyRun_SimpleString(R"(
x = foo.varargs(10)
)");

  PyObjectPtr x(moduleGet("__main__", "x"));
  long result = PyLong_AsLong(x);
  ASSERT_EQ(result, 10);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest,
       MethodTupleAndKeywordsFastWithKeywordArgReturnsArg) {
  ternaryfunc foo_func = [](PyObject*, PyObject* args, PyObject* kwargs) {
    int value;
    static const char* const keywords[] = {"value", nullptr};
    static _PyArg_Parser parser = {"i:fastcall", keywords};
    EXPECT_EQ(_PyArg_ParseTupleAndKeywordsFast(args, kwargs, &parser, &value),
              1);
    return PyLong_FromLong(value);
  };
  PyMethodDef foo_methods[] = {
      {"kwArgs",
       reinterpret_cast<binaryfunc>(reinterpret_cast<void*>(foo_func)),
       METH_VARARGS | METH_KEYWORDS},
      {nullptr}};
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "foo", nullptr, 0, foo_methods,
  };
  PyObjectPtr module(PyModule_Create(&def));
  moduleSet("__main__", "foo", module);
  ASSERT_TRUE(PyModule_CheckExact(module));
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyRun_SimpleString(R"(
x = foo.kwArgs(value=40)
)");

  PyObjectPtr x(moduleGet("__main__", "x"));
  long result = PyLong_AsLong(x);
  ASSERT_EQ(result, 40);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ModuleExtensionApiTest, MethodWithKeywordArgReturnsArg) {
  ternaryfunc foo_func = [](PyObject*, PyObject* args, PyObject* kwargs) {
    int value;
    const char* kwnames[] = {"value", nullptr};
    PyArg_ParseTupleAndKeywords(args, kwargs, "i", const_cast<char**>(kwnames),
                                &value);
    return PyLong_FromLong(value);
  };
  PyMethodDef foo_methods[] = {
      {"kwArgs",
       reinterpret_cast<binaryfunc>(reinterpret_cast<void*>(foo_func)),
       METH_VARARGS | METH_KEYWORDS},
      {nullptr}};
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT, "foo", nullptr, 0, foo_methods,
  };
  PyObjectPtr module(PyModule_Create(&def));
  moduleSet("__main__", "foo", module);
  ASSERT_TRUE(PyModule_CheckExact(module));
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyRun_SimpleString(R"(
x = foo.kwArgs(value=40)
)");

  PyObjectPtr x(moduleGet("__main__", "x"));
  long result = PyLong_AsLong(x);
  ASSERT_EQ(result, 40);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

}  // namespace testing
}  // namespace py
