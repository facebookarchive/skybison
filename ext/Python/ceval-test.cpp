#include "gtest/gtest.h"

#include "gmock/gmock-matchers.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;
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
  PyObjectPtr module_dict(moduleGet("__main__", "module_dict"));
  PyObjectPtr locals(PyDict_New());
  EXPECT_NE(PyEval_EvalCode(code, /*globals=*/module_dict, locals), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
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
  PyObjectPtr module_dict(moduleGet("__main__", "module_dict"));
  PyObjectPtr locals(PyDict_New());
  EXPECT_NE(PyEval_EvalCode(code, /*globals=*/module_dict, locals), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(moduleGet("__main__", "a"));
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
  PyObjectPtr module_dict(moduleGet("__main__", "module_dict"));
  EXPECT_NE(
      PyEval_EvalCode(code, /*globals=*/module_dict, /*locals=*/module_dict),
      nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(moduleGet("__main__", "a"));
  EXPECT_TRUE(PyLong_CheckExact(result));
  EXPECT_EQ(PyLong_AsDouble(result), 3.0);
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
  PyObjectPtr result(moduleGet("__main__", "result"));
  ASSERT_FALSE(0xfba0000 & CO_FUTURE_BARRY_AS_BDFL);
  EXPECT_TRUE(isLongEqualsLong(result, 0xfba0000 | CO_FUTURE_BARRY_AS_BDFL));
}

}  // namespace py
