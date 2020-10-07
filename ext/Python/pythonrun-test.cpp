#include <cstdlib>
#include <cstring>

#include "Python.h"
#include "gmock/gmock-matchers.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using PythonrunExtensionApiTest = ExtensionApi;

TEST_F(PythonrunExtensionApiTest, CompileStringWithEmptyStrReturnsCode) {
  PyObjectPtr result(Py_CompileString("", "<string>", Py_file_input));
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyCode_Check(result));
}

TEST_F(PythonrunExtensionApiTest, CompileStringCompilesCode) {
  PyObjectPtr result(Py_CompileString("a = 3", "<string>", Py_file_input));
  EXPECT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyCode_Check(result));
  PyObjectPtr locals(PyDict_New());
  PyObjectPtr globals(PyDict_New());
  EXPECT_NE(PyEval_EvalCode(result, globals, locals), nullptr);

  EXPECT_EQ(PyDict_Size(locals), 1);
  PyObjectPtr local(PyDict_GetItemString(locals, "a"));
  EXPECT_TRUE(isLongEqualsLong(local, 3));
}

TEST_F(PythonrunExtensionApiTest,
       CompileStringWithInvalidCodeRaisesSyntaxError) {
  PyObjectPtr result(Py_CompileString(";", "<string>", Py_file_input));
  EXPECT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyErr_ExceptionMatches(PyExc_SyntaxError), 1);
}

TEST_F(PythonrunExtensionApiTest, CompileWithSourceIsUTF8RaisesValueError) {
  int flags = PyCF_SOURCE_IS_UTF8;
  EXPECT_EQ(0, moduleSet("__main__", "flags", PyLong_FromLong(flags)));
  EXPECT_EQ(-1, PyRun_SimpleString(R"(
try:
  compile("1", "filename", "exec", flags=flags)
  failed = False
except ValueError:
  failed = True
  raise
)"));
  PyObjectPtr failed(mainModuleGet("failed"));
  EXPECT_EQ(Py_True, failed);
}

TEST_F(PythonrunExtensionApiTest, PyRunAnyFileReturnsZero) {
  CaptureStdStreams streams;
  const char* buffer = R"(print(f"good morning by {__file__}"))";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  int returncode = PyRun_AnyFile(fp, "test string");
  fclose(fp);
  EXPECT_EQ(returncode, 0);
  EXPECT_EQ(streams.out(), "good morning by test string\n");
}

TEST_F(PythonrunExtensionApiTest, PyRunAnyFileExReturnsZero) {
  CaptureStdStreams streams;
  const char* buffer = R"(print(f"I am {__file__}"))";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  int returncode = PyRun_AnyFileEx(fp, "test string", /*closeit=*/1);
  EXPECT_EQ(returncode, 0);
  EXPECT_EQ(streams.out(), "I am test string\n");
}

TEST_F(PythonrunExtensionApiTest, PyRunAnyFileExFlagsReturnsZero) {
  CaptureStdStreams streams;
  const char* buffer = R"(x = 2 <> 3; print(f"{x} by {__file__}"))";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  PyCompilerFlags flags;
  flags.cf_flags = CO_FUTURE_BARRY_AS_BDFL;
  int returncode = PyRun_AnyFileExFlags(fp, nullptr, /*closeit=*/1, &flags);
  EXPECT_EQ(returncode, 0);
  EXPECT_EQ(streams.out(), "True by ???\n");
}

TEST_F(PythonrunExtensionApiTest, PyRunAnyFileFlagsReturnsZero) {
  CaptureStdStreams streams;
  const char* buffer = R"(x = 4 <> 4; print(f"{x} by {__file__}"))";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  PyCompilerFlags flags;
  flags.cf_flags = CO_FUTURE_BARRY_AS_BDFL;
  int returncode = PyRun_AnyFileFlags(fp, "a test", &flags);
  fclose(fp);
  EXPECT_EQ(returncode, 0);
  EXPECT_EQ(streams.out(), "False by a test\n");
}

TEST_F(PythonrunExtensionApiTest, PyRunFileReturnsStr) {
  PyObjectPtr module(PyModule_New("testmodule"));
  PyModule_AddStringConstant(module, "shout", "ya!");
  PyObject* module_dict = PyModule_GetDict(module);
  const char* buffer = R"("hey " + shout)";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  PyObjectPtr result(
      PyRun_File(fp, "a test", Py_eval_input, module_dict, module_dict));
  fclose(fp);
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "hey ya!"));
}

TEST_F(PythonrunExtensionApiTest, PyRunFileExReturnsStr) {
  PyObjectPtr module(PyModule_New("testmodule"));
  PyModule_AddStringConstant(module, "shout", "ya!");
  PyObject* module_dict = PyModule_GetDict(module);
  const char* buffer = R"("hey " + shout)";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  PyObjectPtr result(PyRun_FileEx(fp, "a test", Py_eval_input, module_dict,
                                  module_dict, /*closeit=*/1));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "hey ya!"));
}

TEST_F(PythonrunExtensionApiTest, PyRunFileExFlagsReturnsTrue) {
  PyObjectPtr module(PyModule_New("testmodule"));
  PyModule_AddIntConstant(module, "number", 42);
  PyObjectPtr module_dict(PyModule_GetDict(module));
  Py_INCREF(module_dict);
  char buffer[] = R"(7 != number)";
  FILE* fp =
      ::fmemopen(reinterpret_cast<void*>(buffer), std::strlen(buffer), "r");
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  PyObjectPtr result(PyRun_FileExFlags(fp, "a test", Py_eval_input,
                                       module_dict.get(), module_dict.get(),
                                       /*closeit=*/1, &flags));
  EXPECT_EQ(result, Py_True);
}

TEST_F(PythonrunExtensionApiTest, PyRunFileExFlagsWithUserLocalsReturnsTrue) {
  PyObjectPtr module(PyModule_New("testmodule"));
  PyModule_AddIntConstant(module, "number", 42);
  PyObjectPtr module_dict(PyModule_GetDict(module));
  Py_INCREF(module_dict);
  char buffer[] = R"(7 != number)";
  FILE* fp =
      ::fmemopen(reinterpret_cast<void*>(buffer), std::strlen(buffer), "r");
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  PyObjectPtr locals(PyDict_New());
  PyObjectPtr result(PyRun_FileExFlags(fp, "a test", Py_eval_input,
                                       module_dict.get(), locals.get(),
                                       /*closeit=*/1, &flags));
  EXPECT_EQ(result, Py_True);
}

TEST_F(PythonrunExtensionApiTest, PyRunFileFlagsReturnsFalse) {
  PyObjectPtr module(PyModule_New("testmodule"));
  PyModule_AddIntConstant(module, "number", 9);
  PyObject* module_dict = PyModule_GetDict(module);
  const char* buffer = R"(number <> 9)";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  PyCompilerFlags flags;
  flags.cf_flags = CO_FUTURE_BARRY_AS_BDFL;
  PyObjectPtr result(PyRun_FileFlags(fp, "a test", Py_eval_input, module_dict,
                                     module_dict, &flags));
  fclose(fp);
  EXPECT_EQ(result, Py_False);
}

TEST_F(PythonrunExtensionApiTest, PyRunSimpleStringReturnsInt) {
  EXPECT_EQ(PyRun_SimpleString("a = 42"), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr value(mainModuleGet("a"));
  EXPECT_TRUE(isLongEqualsLong(value, 42));
}

TEST_F(PythonrunExtensionApiTest, PyRunSimpleStringPrintsSyntaxError) {
  CaptureStdStreams streams;
  EXPECT_EQ(PyRun_SimpleString(",,,"), -1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(streams.err().find("SyntaxError: invalid syntax\n"),
            std::string::npos);
}

TEST_F(PythonrunExtensionApiTest, PyRunSimpleStringPrintsUncaughtException) {
  CaptureStdStreams streams;
  ASSERT_EQ(PyRun_SimpleString("raise RuntimeError('boom')"), -1);
  EXPECT_EQ(streams.out(), "");
  EXPECT_EQ(streams.err(), R"(Traceback (most recent call last):
  File "<string>", line 1, in <module>
RuntimeError: boom
)");
}

TEST_F(PythonrunExtensionApiTest, PyRunSimpleStringFlagsReturnsTrue) {
  PyCompilerFlags flags;
  flags.cf_flags = CO_FUTURE_BARRY_AS_BDFL;
  EXPECT_EQ(PyRun_SimpleStringFlags("foo = 13 <> 42", &flags), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr value(mainModuleGet("foo"));
  EXPECT_EQ(value, Py_True);
}

TEST_F(PythonrunExtensionApiTest, PyRunStringReturnsString) {
  PyObjectPtr module(PyModule_New("testmodule"));
  PyModule_AddStringConstant(module, "name", "tester");
  PyObject* module_dict = PyModule_GetDict(module);
  PyObjectPtr result(PyRun_String(R"(f"hello {name}")", Py_eval_input,
                                  module_dict, module_dict));
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "hello tester"));
}

TEST_F(PythonrunExtensionApiTest, PyRunStringFlagsReturnsResult) {
  PyObject* module = PyImport_AddModule("__main__");
  ASSERT_NE(module, nullptr);
  PyObject* module_proxy = PyModule_GetDict(module);
  PyCompilerFlags flags;
  flags.cf_flags = CO_FUTURE_BARRY_AS_BDFL;
  EXPECT_TRUE(
      isLongEqualsLong(PyRun_StringFlags("(7 <> 7) + 3", Py_eval_input,
                                         module_proxy, module_proxy, &flags),
                       3));
}

TEST_F(PythonrunExtensionApiTest,
       PyRunStringFlagsWithSourceIsUtf8FlagReturnsResult) {
  PyObject* module = PyImport_AddModule("__main__");
  ASSERT_NE(module, nullptr);
  PyObject* module_proxy = PyModule_GetDict(module);
  PyCompilerFlags flags;
  flags.cf_flags = PyCF_SOURCE_IS_UTF8;
  EXPECT_TRUE(
      isLongEqualsLong(PyRun_StringFlags("1 + 2", Py_eval_input, module_proxy,
                                         module_proxy, &flags),
                       3));
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
  PyObjectPtr exc(mainModuleGet("exc"));

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
  PyObjectPtr exc(mainModuleGet("exc"));

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
  PyObjectPtr exc(mainModuleGet("exc"));

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
  PyObjectPtr exc(mainModuleGet("exc"));

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
  PyObjectPtr exc(mainModuleGet("exc"));

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
  PyObjectPtr exc(mainModuleGet("exc"));

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
  PyObjectPtr se(mainModuleGet("se"));

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

TEST_F(PythonrunExtensionApiTest, PyRunSimpleFileReturnsZero) {
  CaptureStdStreams streams;
  const char* buffer = R"(print(f"Greetings from {__file__}"))";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  int returncode = PyRun_SimpleFile(fp, "test string");
  fclose(fp);
  EXPECT_EQ(returncode, 0);
  EXPECT_EQ(streams.out(), "Greetings from test string\n");
}

TEST_F(PythonrunExtensionApiTest, PyRunSimpleFileExReturnsZero) {
  CaptureStdStreams streams;
  const char* buffer = R"(print(f"This is {__file__}"))";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  int returncode = PyRun_SimpleFileEx(fp, "zombocom", /*closeit=*/1);
  EXPECT_EQ(returncode, 0);
  EXPECT_EQ(streams.out(), "This is zombocom\n");
}

TEST_F(PythonrunExtensionApiTest, PyRunSimpleFileExFlagsWithPyFileReturnsZero) {
  CaptureStdStreams streams;
  const char* buffer = R"(print("pyhello"))";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  int returncode = PyRun_SimpleFileExFlags(fp, "test.py", 1, &flags);
  EXPECT_EQ(returncode, 0);
  EXPECT_EQ(streams.out(), "pyhello\n");
}

TEST_F(PythonrunExtensionApiTest,
       PyRunSimpleFileExFlagsSetsAndUnsetsDunderFile) {
  CaptureStdStreams streams;
  const char* buffer = R"(print(__file__))";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  int returncode = PyRun_SimpleFileExFlags(fp, "test.py", 1, &flags);
  EXPECT_EQ(returncode, 0);
  EXPECT_EQ(streams.out(), "test.py\n");
  PyObject* mods = PyImport_GetModuleDict();
  PyObject* dunder_main = PyUnicode_FromString("__main__");
  PyObject* main_mod = PyDict_GetItem(mods, dunder_main);
  PyObject* dunder_file = PyUnicode_FromString("__file__");
  EXPECT_FALSE(PyObject_HasAttr(main_mod, dunder_file));
}

TEST_F(PythonrunExtensionApiTest, PyRunSimpleFileExFlagsPrintsSyntaxError) {
  CaptureStdStreams streams;
  const char* buffer = ",,,";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  int returncode = PyRun_SimpleFileExFlags(fp, "test.py", 1, &flags);
  EXPECT_EQ(returncode, -1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_NE(streams.err().find(R"(  File "test.py", line 1
    ,,,
    ^
SyntaxError: invalid syntax
)"),
            std::string::npos);
}

TEST_F(PythonrunExtensionApiTest,
       PyRunSimpleFileExFlagsPrintsUncaughtException) {
  CaptureStdStreams streams;
  const char* buffer = "raise RuntimeError('boom')";
  FILE* fp = ::fmemopen(reinterpret_cast<void*>(const_cast<char*>(buffer)),
                        std::strlen(buffer), "r");
  PyCompilerFlags flags;
  flags.cf_flags = 0;
  int returncode = PyRun_SimpleFileExFlags(fp, "test.py", 1, &flags);
  EXPECT_EQ(returncode, -1);
  EXPECT_EQ(streams.out(), "");
  // TODO(T39919701): Check the whole string once we have tracebacks.
  EXPECT_NE(streams.err().find("RuntimeError: boom\n"), std::string::npos);
}

}  // namespace testing
}  // namespace py
