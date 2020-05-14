#include "Python.h"
#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {
using CevalExtensionApiTest = ExtensionApi;

TEST_F(CevalExtensionApiTest, EvalCodeWithNullGlobalsRaisesSystemError) {
  PyObjectPtr empty_tuple(PyTuple_New(0));
  PyObjectPtr empty_bytes(PyBytes_FromString(""));
  PyObjectPtr empty_str(PyUnicode_FromString(""));
  PyCodeObject* code = PyCode_New(
      0, 0, 0, 0, 0, empty_bytes, empty_tuple, empty_tuple, empty_tuple,
      empty_tuple, empty_tuple, empty_str, empty_str, 0, empty_bytes);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(code, nullptr);
  PyObjectPtr locals(PyDict_New());
  EXPECT_EQ(PyEval_EvalCode(reinterpret_cast<PyObject*>(code),
                            /*globals=*/nullptr, locals),
            nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
  Py_DECREF(code);
}

TEST_F(CevalExtensionApiTest, EvalCodeReturnsNonNull) {
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  PyArena* arena = PyArena_New();
  const char* filename = "<string>";
  struct _mod* node = PyParser_ASTFromString("a = 1 + 2", filename,
                                             Py_file_input, &flags, arena);
  ASSERT_NE(node, nullptr);
  PyObjectPtr code(reinterpret_cast<PyObject*>(
      PyAST_CompileEx(node, filename, &flags, 0, arena)));
  ASSERT_NE(code, nullptr);
  PyArena_Free(arena);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  PyRun_SimpleString(R"(
module_dict = locals()
)");
  PyObjectPtr module_dict(mainModuleGet("module_dict"));
  PyObjectPtr locals(PyDict_New());
  EXPECT_NE(PyEval_EvalCode(code, /*globals=*/module_dict, locals), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(CevalExtensionApiTest, EvalCodeWithDictGlobalsUpdatesDict) {
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  PyArena* arena = PyArena_New();
  const char* filename = "<string>";
  struct _mod* node = PyParser_ASTFromString("global a; a = 1 + 2", filename,
                                             Py_file_input, &flags, arena);
  ASSERT_NE(node, nullptr);
  PyObjectPtr code(reinterpret_cast<PyObject*>(
      PyAST_CompileEx(node, filename, &flags, 0, arena)));
  ASSERT_NE(code, nullptr);
  PyArena_Free(arena);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr globals(PyDict_New());
  PyObjectPtr locals(PyDict_New());
  EXPECT_NE(PyEval_EvalCode(code, globals, locals), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(PyDict_GetItemString(globals, "a"));
  EXPECT_TRUE(isLongEqualsLong(result, 3));
  EXPECT_EQ(PyDict_Size(locals), 0);
}

TEST_F(CevalExtensionApiTest, EvalCodeWithModuleDictAsGlobalsReturnsNonNull) {
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  PyArena* arena = PyArena_New();
  const char* filename = "<string>";
  struct _mod* node =
      PyParser_ASTFromString(R"(
global a
a = 1 + 2
)",
                             filename, Py_file_input, &flags, arena);
  ASSERT_NE(node, nullptr);
  PyObjectPtr code(reinterpret_cast<PyObject*>(
      PyAST_CompileEx(node, filename, &flags, 0, arena)));
  ASSERT_NE(code, nullptr);
  PyArena_Free(arena);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  PyRun_SimpleString(R"(
module_dict = locals()
)");
  PyObjectPtr module_dict(mainModuleGet("module_dict"));
  PyObjectPtr locals(PyDict_New());
  EXPECT_NE(PyEval_EvalCode(code, /*globals=*/module_dict, locals), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(mainModuleGet("a"));
  EXPECT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsDouble(result), 3.0);
}

TEST_F(CevalExtensionApiTest,
       EvalCodeWithModuleDictAsGlobalsAndLocalsReturnsNonNull) {
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  PyArena* arena = PyArena_New();
  const char* filename = "<string>";
  struct _mod* node = PyParser_ASTFromString("a = 1 + 2", filename,
                                             Py_file_input, &flags, arena);
  ASSERT_NE(node, nullptr);
  PyObjectPtr code(reinterpret_cast<PyObject*>(
      PyAST_CompileEx(node, filename, &flags, 0, arena)));
  ASSERT_NE(code, nullptr);
  PyArena_Free(arena);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  PyRun_SimpleString(R"(
module_dict = locals()
)");
  PyObjectPtr module_dict(mainModuleGet("module_dict"));
  EXPECT_NE(
      PyEval_EvalCode(code, /*globals=*/module_dict, /*locals=*/module_dict),
      nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(mainModuleGet("a"));
  EXPECT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsDouble(result), 3.0);
}

TEST_F(CevalExtensionApiTest, GetBuiltinsReturnsMapping) {
  PyObjectPtr builtins(PyEval_GetBuiltins());  // returns a borrowed reference
  ASSERT_NE(builtins, nullptr);
  Py_INCREF(builtins);
  EXPECT_EQ(1, PyMapping_Check(builtins));
  // Check some sample builtins
  EXPECT_EQ(1, PyMapping_HasKeyString(builtins, "int"));
  EXPECT_EQ(1, PyMapping_HasKeyString(builtins, "compile"));
}

TEST_F(CevalExtensionApiTest, MergeCompilerFlagsReturnsTrue) {
  PyCompilerFlags flags;
  flags.cf_flags = CO_FUTURE_BARRY_AS_BDFL;
  EXPECT_NE(PyEval_MergeCompilerFlags(&flags), 0);
  EXPECT_EQ(flags.cf_flags, CO_FUTURE_BARRY_AS_BDFL);
}

TEST_F(CevalExtensionApiTest, MergeCompilerFlagsReturnsFalse) {
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  EXPECT_EQ(PyEval_MergeCompilerFlags(&flags), 0);
  EXPECT_EQ(flags.cf_flags, 0);
}

static PyObject* testMergeCompilerFlags(PyObject*, PyObject*) {
  PyCompilerFlags flags;
  flags.cf_flags = 0xfba0000;
  EXPECT_NE(PyEval_MergeCompilerFlags(&flags), 0);
  return PyLong_FromLong(flags.cf_flags);
}

TEST_F(CevalExtensionApiTest, MergeCompilerFlagsMergesCodeFlags) {
  static PyMethodDef methods[] = {
      {"test_merge_compiler_flags", testMergeCompilerFlags, METH_NOARGS, ""},
      {nullptr, nullptr}};
  static PyModuleDef def = {PyModuleDef_HEAD_INIT, "test_module", nullptr, 0,
                            methods};
  PyObjectPtr module(PyModule_Create(&def));
  moduleSet("__main__", "test_module", module);
  PyCompilerFlags flags;
  flags.cf_flags = CO_FUTURE_BARRY_AS_BDFL;
  ASSERT_EQ(PyRun_SimpleStringFlags(
                "result = test_module.test_merge_compiler_flags()", &flags),
            0);
  PyObjectPtr result(mainModuleGet("result"));
  ASSERT_FALSE(0xfba0000 & CO_FUTURE_BARRY_AS_BDFL);
  EXPECT_TRUE(isLongEqualsLong(result, 0xfba0000 | CO_FUTURE_BARRY_AS_BDFL));
}

TEST_F(CevalExtensionApiTest, CallObjectWithNonTupleArgsRaisesTypeError) {
  PyRun_SimpleString(R"(
def fn():
  pass
)");
  PyObjectPtr fn(mainModuleGet("fn"));
  PyObjectPtr args(PyList_New(0));
  PyEval_CallObject(fn, args);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(CevalExtensionApiTest, CallObjectWithNullArgsReturnsResult) {
  PyRun_SimpleString(R"(
def fn():
  return 19
)");
  PyObjectPtr fn(mainModuleGet("fn"));
  PyObjectPtr result(PyEval_CallObject(fn, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isLongEqualsLong(result, 19));
}

TEST_F(CevalExtensionApiTest, CallObjectWithTupleArgsReturnsResult) {
  PyRun_SimpleString(R"(
def fn(*args):
  return args[0]
)");
  PyObjectPtr fn(mainModuleGet("fn"));
  PyObjectPtr args(PyTuple_New(1));
  PyTuple_SetItem(args, 0, PyLong_FromLong(3));
  PyObjectPtr result(PyEval_CallObject(fn, args));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isLongEqualsLong(result, 3));
}

TEST_F(CevalExtensionApiTest,
       CallObjectWithKeywordsWithNonTupleArgsRaisesTypeError) {
  PyRun_SimpleString(R"(
def fn():
  pass
)");
  PyObjectPtr fn(mainModuleGet("fn"));
  PyObjectPtr args(PyList_New(0));
  PyEval_CallObjectWithKeywords(fn, args, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(CevalExtensionApiTest,
       CallObjectWithKeywordsWithNonDictKwargsRaisesTypeError) {
  PyRun_SimpleString(R"(
def fn():
  pass
)");
  PyObjectPtr fn(mainModuleGet("fn"));
  PyObjectPtr kwargs(PyList_New(0));
  PyEval_CallObjectWithKeywords(fn, nullptr, kwargs);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(CevalExtensionApiTest, CallObjectWithKeywordsWithNullArgsReturnsResult) {
  PyRun_SimpleString(R"(
def fn(*args, **kwargs):
  return kwargs["kwarg"]
)");
  PyObjectPtr fn(mainModuleGet("fn"));
  PyObjectPtr kwargs(PyDict_New());
  PyObjectPtr kwarg_name(PyUnicode_FromString("kwarg"));
  PyObjectPtr kwarg_value(PyLong_FromLong(2));
  PyDict_SetItem(kwargs, kwarg_name, kwarg_value);
  PyObjectPtr result(PyEval_CallObjectWithKeywords(fn, nullptr, kwargs));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isLongEqualsLong(result, 2));
}

TEST_F(CevalExtensionApiTest,
       CallObjectWithKeywordsWithArgsAndKeywordsReturnsResult) {
  PyRun_SimpleString(R"(
def fn(*args, **kwargs):
  return kwargs["kwarg"] + args[0]
)");
  PyObjectPtr fn(mainModuleGet("fn"));
  PyObjectPtr args(PyTuple_New(1));
  PyTuple_SetItem(args, 0, PyLong_FromLong(2));
  PyObjectPtr kwargs(PyDict_New());
  PyObjectPtr kwarg_name(PyUnicode_FromString("kwarg"));
  PyObjectPtr kwarg_value(PyLong_FromLong(2));
  PyDict_SetItem(kwargs, kwarg_name, kwarg_value);
  PyObjectPtr result(PyEval_CallObjectWithKeywords(fn, args, kwargs));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isLongEqualsLong(result, 4));
}

}  // namespace testing
}  // namespace py
