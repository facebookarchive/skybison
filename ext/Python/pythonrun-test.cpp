#include <cstdlib>

#include "Python.h"

#include "Python-ast.h"
#include "node.h"
// Unset Python-ast.h macros that conflict with gtest/gtest.h
#undef Compare
#undef Set
#undef Assert

#include "capi-fixture.h"
#include "capi-testing.h"
#include "gtest/gtest.h"

#include "gmock/gmock-matchers.h"

namespace py {

using namespace testing;

using PythonrunExtensionApiTest = ExtensionApi;

TEST_F(PythonrunExtensionApiTest, RunSimpleStringReturnsZero) {
  EXPECT_EQ(PyRun_SimpleString("a = 42"), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(PythonrunExtensionApiTest,
       RunSimpleStringWithSyntaxErrorRaisesSyntaxError) {
  EXPECT_EQ(PyRun_SimpleString(",,,"), -1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(PythonrunExtensionApiTest, RunSimpleStringPrintsUncaughtException) {
  CaptureStdStreams streams;
  ASSERT_EQ(PyRun_SimpleString("raise RuntimeError('boom')"), -1);
  // TODO(T39919701): Check the whole string once we have tracebacks.
  ASSERT_THAT(streams.err(), ::testing::EndsWith("RuntimeError: boom\n"));
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest, PyErrDisplayPrintsException) {
  PyErr_SetString(PyExc_RuntimeError, "oopsie");
  PyObject *exc, *value, *tb;
  PyErr_Fetch(&exc, &value, &tb);
  // PyErr_Display() expects a real exception in `value`.
  PyErr_NormalizeException(&exc, &value, &tb);

  CaptureStdStreams streams;
  PyErr_Display(exc, value, tb);
  ASSERT_EQ(streams.err(), "RuntimeError: oopsie\n");
  EXPECT_EQ(streams.out(), "");

  Py_DECREF(exc);
  Py_DECREF(value);
  Py_XDECREF(tb);
}

TEST_F(PythonrunExtensionApiTest, PyErrDisplayPrintsExceptionChain) {
  // TODO(T39919701): Don't clear __traceback__ below.
  ASSERT_EQ(PyRun_SimpleString(R"(
try:
  try:
    raise RuntimeError("inner")
  except Exception as e:
      e.__traceback__ = None
      e.__context__ = ValueError("non-raised inner")
      raise RuntimeError("outer") from e
except Exception as e:
  e.__traceback__ = None
  exc = e
)"),
            0);
  PyObjectPtr exc(moduleGet("__main__", "exc"));

  CaptureStdStreams streams;
  PyErr_Display(nullptr, exc, nullptr);
  ASSERT_EQ(streams.err(),
            "ValueError: non-raised inner\n\nDuring handling of the above "
            "exception, another exception occurred:\n\nRuntimeError: "
            "inner\n\nThe above exception was the direct cause "
            "of the following exception:\n\nRuntimeError: outer\n");
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest, PyErrDisplayAvoidsCauseCycle) {
  ASSERT_EQ(PyRun_SimpleString(R"(
exc = RuntimeError("outer")
exc.__cause__ = RuntimeError("inner")
exc.__cause__.__cause__ = exc
)"),
            0);
  PyObjectPtr exc(moduleGet("__main__", "exc"));

  CaptureStdStreams streams;
  PyErr_Display(nullptr, exc, nullptr);
  ASSERT_EQ(streams.err(),
            "RuntimeError: inner\n\nThe above exception was the direct cause "
            "of the following exception:\n\nRuntimeError: outer\n");
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest, PyErrDisplayAvoidsContextCycle) {
  ASSERT_EQ(PyRun_SimpleString(R"(
exc = RuntimeError("outer")
exc.__context__ = RuntimeError("inner")
exc.__context__.__context__ = exc
)"),
            0);
  PyObjectPtr exc(moduleGet("__main__", "exc"));

  CaptureStdStreams streams;
  PyErr_Display(nullptr, exc, nullptr);
  ASSERT_EQ(streams.err(),
            "RuntimeError: inner\n\nDuring handling of the above exception, "
            "another exception occurred:\n\nRuntimeError: outer\n");
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest,
       PyErrDisplayWithSuppressContextDoesntPrintContext) {
  ASSERT_EQ(PyRun_SimpleString(R"(
exc = RuntimeError("error")
exc.__context__ = RuntimeError("inner error")
exc.__suppress_context__ = True
)"),
            0);
  PyObjectPtr exc(moduleGet("__main__", "exc"));

  CaptureStdStreams streams;
  PyErr_Display(nullptr, exc, nullptr);
  ASSERT_EQ(streams.err(), "RuntimeError: error\n");
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest, PyErrDisplayWithRaisingStr) {
  ASSERT_EQ(PyRun_SimpleString(R"(
class MyExc(Exception):
  def __str__(self):
    raise RuntimeError()
exc = MyExc()
)"),
            0);
  PyObjectPtr exc(moduleGet("__main__", "exc"));

  CaptureStdStreams streams;
  PyErr_Display(nullptr, exc, nullptr);
  ASSERT_EQ(streams.err(), "__main__.MyExc: <exception str() failed>\n");
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest, PyErrDisplayWithNoModule) {
  ASSERT_EQ(PyRun_SimpleString(R"(
class MyExc(Exception):
  __module__ = None
exc = MyExc("hi")
)"),
            0);
  PyObjectPtr exc(moduleGet("__main__", "exc"));

  CaptureStdStreams streams;
  PyErr_Display(nullptr, exc, nullptr);
  ASSERT_EQ(streams.err(), "<unknown>MyExc: hi\n");
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest, PyErrDisplayWithNonException) {
  PyObjectPtr value(PyFloat_FromDouble(123.0));

  CaptureStdStreams streams;
  PyErr_Display(nullptr, value, nullptr);
  ASSERT_EQ(streams.err(),
            "TypeError: print_exception(): Exception expected for value, float "
            "found\n");
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest, PyErrDisplayWithSyntaxError) {
  ASSERT_EQ(PyRun_SimpleString(R"(
se = SyntaxError()
se.print_file_and_line = None
se.msg = "bad syntax"
se.filename = "some_file.py"
se.lineno = 0
se.offset = 31
se.text = "this is fake source code\nthat is multiple lines long"
)"),
            0);
  PyObjectPtr se(moduleGet("__main__", "se"));

  CaptureStdStreams streams;
  PyErr_Display(nullptr, se, nullptr);
  ASSERT_EQ(streams.err(),
            R"(  File "some_file.py", line 0
    that is multiple lines long
         ^
SyntaxError: bad syntax
)");
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest, PyErrPrintExPrintsExceptionDoesntSetVars) {
  PyErr_SetString(PyExc_RuntimeError, "abcd");

  CaptureStdStreams streams;
  PyErr_PrintEx(0);
  ASSERT_EQ(streams.err(), "RuntimeError: abcd\n");
  EXPECT_EQ(streams.out(), "");

  ASSERT_EQ(moduleGet("sys", "last_type"), nullptr);
  PyErr_Clear();
  ASSERT_EQ(moduleGet("sys", "last_value"), nullptr);
  PyErr_Clear();
  ASSERT_EQ(moduleGet("sys", "last_traceback"), nullptr);
  PyErr_Clear();
}

static void checkSysVars() {
  PyObjectPtr type(moduleGet("sys", "last_type"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(type, nullptr);
  EXPECT_EQ(type, PyExc_RuntimeError);

  PyObjectPtr value(moduleGet("sys", "last_value"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(value, nullptr);
  EXPECT_EQ(PyErr_GivenExceptionMatches(value, PyExc_RuntimeError), 1);

  PyObjectPtr tb(moduleGet("sys", "last_traceback"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(tb, nullptr);
  // TODO(T39919701): Check for a real traceback once we have tracebacks.
}

TEST_F(PythonrunExtensionApiTest, PyErrPrintExWithArgSetsSysVars) {
  PyErr_SetString(PyExc_RuntimeError, "critical error");

  CaptureStdStreams streams;
  PyErr_PrintEx(1);
  ASSERT_EQ(streams.err(), "RuntimeError: critical error\n");
  EXPECT_EQ(streams.out(), "");

  EXPECT_NO_FATAL_FAILURE(checkSysVars());
}

TEST_F(PythonrunExtensionApiTest, PyErrPrintSetsSysVars) {
  PyErr_SetString(PyExc_RuntimeError, "I don't hate you");

  CaptureStdStreams streams;
  PyErr_Print();
  ASSERT_EQ(streams.err(), "RuntimeError: I don't hate you\n");
  EXPECT_EQ(streams.out(), "");

  EXPECT_NO_FATAL_FAILURE(checkSysVars());
}

TEST_F(PythonrunExtensionApiTest, PyErrPrintExCallsCustomExcepthook) {
  ASSERT_EQ(PyRun_SimpleString(R"(
import sys
def my_hook(type, value, tb):
  print("What exception?", file=sys.stderr)
  print("Everything is fine. Nothing is ruined.")
sys.excepthook = my_hook
)"),
            0);
  PyErr_SetString(PyExc_RuntimeError, "boop");

  CaptureStdStreams streams;
  PyErr_PrintEx(0);
  ASSERT_EQ(streams.err(), "What exception?\n");
  EXPECT_EQ(streams.out(), "Everything is fine. Nothing is ruined.\n");
}

TEST_F(PythonrunExtensionApiTest, PyErrPrintExWithRaisingExcepthook) {
  ASSERT_EQ(PyRun_SimpleString(R"(
import sys
def my_hook(type, value, tb):
  raise RuntimeError("I'd rather not")
sys.excepthook = my_hook
)"),
            0);
  PyErr_SetString(PyExc_TypeError, "bad type");

  CaptureStdStreams streams;
  PyErr_PrintEx(0);
  // TODO(T39919701): Check the whole string once we have tracebacks.
  std::string err = streams.err();
  ASSERT_THAT(err, ::testing::StartsWith("Error in sys.excepthook:\n"));
  ASSERT_THAT(err,
              ::testing::EndsWith("RuntimeError: I'd rather not\n\nOriginal "
                                  "exception was:\nTypeError: bad type\n"));
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest, PyErrPrintExWithNoExceptHookPrintsException) {
  ASSERT_EQ(PyRun_SimpleString("import sys; del sys.excepthook"), 0);
  PyErr_SetString(PyExc_RuntimeError, "something broke");

  CaptureStdStreams streams;
  PyErr_PrintEx(0);
  ASSERT_EQ(streams.err(),
            "sys.excepthook is missing\nRuntimeError: something broke\n");
  EXPECT_EQ(streams.out(), "");
}

TEST_F(PythonrunExtensionApiTest, PyErrPrintWithSystemExitExits) {
  PyObject* zero = PyLong_FromLong(0);
  PyErr_SetObject(PyExc_SystemExit, zero);
  Py_DECREF(zero);
  EXPECT_EXIT(PyErr_Print(), ::testing::ExitedWithCode(0), "^$");

  PyErr_Clear();
  PyObject* three = PyLong_FromLong(3);
  PyErr_SetObject(PyExc_SystemExit, three);
  Py_DECREF(three);
  EXPECT_EXIT(PyErr_Print(), ::testing::ExitedWithCode(3), "^$");
}

TEST_F(PythonrunExtensionApiTest, PyErrPrintWithSystemExitFromExcepthookExits) {
  ASSERT_EQ(PyRun_SimpleString(R"(
import sys
def my_hook(type, value, tb):
  raise SystemExit(123)
sys.excepthook = my_hook
)"),
            0);
  PyErr_SetObject(PyExc_RuntimeError, Py_None);
  EXPECT_EXIT(PyErr_Print(), ::testing::ExitedWithCode(123), "^$");
}

TEST_F(PythonrunExtensionApiTest, PyParserFromStringReturnsModuleNode) {
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  PyArena* arena = PyArena_New();
  ASSERT_NE(arena, nullptr);
  mod_ty module =
      PyParser_ASTFromString("a = 123", "test", Py_file_input, &flags, arena);
  EXPECT_NE(module, nullptr);
  EXPECT_EQ(module->kind, Module_kind);
  PyArena_Free(arena);
}

TEST_F(PythonrunExtensionApiTest, PyParserFromFileReturnsModuleNode) {
  const char* buffer = "a = 123";
  FILE* fp = fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                      sizeof(buffer), "r");
  const char* enc = nullptr;
  const char* ps1 = nullptr;
  const char* ps2 = nullptr;
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  PyArena* arena = PyArena_New();
  ASSERT_NE(arena, nullptr);
  mod_ty module = PyParser_ASTFromFile(fp, "test", enc, Py_file_input, ps1, ps2,
                                       &flags, nullptr, arena);
  EXPECT_NE(module, nullptr);
  EXPECT_EQ(module->kind, Module_kind);
  PyArena_Free(arena);
}

TEST_F(PythonrunExtensionApiTest,
       PyParserSimpleParseStringFlagsFilenameReturnsNonNull) {
  struct _node* node = PyParser_SimpleParseStringFlagsFilename(
      "a = 123", "test", Py_file_input, /*flags=*/0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_NE(node, nullptr);
  PyNode_Free(node);
}

}  // namespace py
