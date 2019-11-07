#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"
#include "node.h"

namespace py {

using namespace testing;

using CompileExtensionApiTest = ExtensionApi;

TEST_F(CompileExtensionApiTest, PyMangleReturnsIdent) {
  PyObjectPtr s0(PyUnicode_FromString("Foo"));
  PyObjectPtr s1(PyUnicode_FromString("bar"));
  PyObjectPtr result(_Py_Mangle(s0, s1));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "bar"));
}

TEST_F(CompileExtensionApiTest, PyMangleWithDunderIdentReturnsIdent) {
  PyObjectPtr s0(PyUnicode_FromString("Foo"));
  PyObjectPtr s1(PyUnicode_FromString("__bar__"));
  PyObjectPtr result(_Py_Mangle(s0, s1));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "__bar__"));
}

TEST_F(CompileExtensionApiTest, PyMangleWithDotIdentReturnsIdent) {
  PyObjectPtr s0(PyUnicode_FromString("Foo"));
  PyObjectPtr s1(PyUnicode_FromString("__ba.r"));
  PyObjectPtr result(_Py_Mangle(s0, s1));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "__ba.r"));
}

TEST_F(CompileExtensionApiTest, PyMangleWithNullPrivateObjReturnsIdent) {
  PyObjectPtr s1(PyUnicode_FromString("baz"));
  PyObjectPtr result(_Py_Mangle(nullptr, s1));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "baz"));
}

TEST_F(CompileExtensionApiTest, PyMangleWithOnlyUnderscoreClassReturnsIdent) {
  PyObjectPtr s0(PyUnicode_FromString("___"));
  PyObjectPtr s1(PyUnicode_FromString("__baz"));
  PyObjectPtr result(_Py_Mangle(s0, s1));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "__baz"));
}

TEST_F(CompileExtensionApiTest, PyMangleReturnsIdentWithClassnamePrefix) {
  PyObjectPtr s0(PyUnicode_FromString("Foo"));
  PyObjectPtr s1(PyUnicode_FromString("__bar"));
  PyObjectPtr result(_Py_Mangle(s0, s1));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "_Foo__bar"));
}

TEST_F(CompileExtensionApiTest, PyMangleReturnsClassnameWithoutUnderscores) {
  PyObjectPtr s0(PyUnicode_FromString("___Foo"));
  PyObjectPtr s1(PyUnicode_FromString("__bar"));
  PyObjectPtr result(_Py_Mangle(s0, s1));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "_Foo__bar"));
}

TEST_F(CompileExtensionApiTest, PyNodeCompileReturnsCodeObject) {
  _node* astnode = PyParser_SimpleParseStringFlagsFilename(
      "4+5", "<test string>", Py_eval_input, 0);
  ASSERT_NE(astnode, nullptr);

  PyObjectPtr code(
      reinterpret_cast<PyObject*>(PyNode_Compile(astnode, "<test string>")));
  EXPECT_TRUE(PyCode_Check(code));
  PyNode_Free(astnode);
}

TEST_F(CompileExtensionApiTest, PyAstCompileExReturnsCodeObject) {
  PyArena* arena = PyArena_New();
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  _mod* mod = PyParser_ASTFromString("4+5", "<test string>", Py_eval_input,
                                     &flags, arena);
  ASSERT_NE(mod, nullptr);
  PyObjectPtr code(reinterpret_cast<PyObject*>(
      PyAST_CompileEx(mod, "<test string>", &flags, -1, arena)));
  EXPECT_TRUE(PyCode_Check(code));
  PyArena_Free(arena);
}

TEST_F(CompileExtensionApiTest, PyAstCompileReturnsCodeObject) {
  PyArena* arena = PyArena_New();
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  _mod* mod = PyParser_ASTFromString("4+5", "<test string>", Py_single_input,
                                     &flags, arena);
  ASSERT_NE(mod, nullptr);
  PyCodeObject* code = PyAST_Compile(mod, "<test string>", &flags, arena);
  EXPECT_TRUE(PyCode_Check(code));
  Py_XDECREF(code);
  PyArena_Free(arena);
}

TEST_F(CompileExtensionApiTest, PyAstCompileObjectReturnsCodeObject) {
  PyArena* arena = PyArena_New();
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  _mod* mod = PyParser_ASTFromString("def foo(): pass", "<test string>",
                                     Py_file_input, &flags, arena);
  ASSERT_NE(mod, nullptr);
  PyObjectPtr filename(PyUnicode_FromString("<test string>"));
  PyCodeObject* code = PyAST_CompileObject(mod, filename, &flags, -1, arena);
  EXPECT_TRUE(PyCode_Check(code));
  Py_XDECREF(code);
  PyArena_Free(arena);
}

}  // namespace py
