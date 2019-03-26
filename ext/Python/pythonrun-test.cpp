#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using PythonrunExtensionApiTest = ExtensionApi;

TEST_F(PythonrunExtensionApiTest, RunSimpleStringReturnsZero) {
  EXPECT_EQ(PyRun_SimpleString("a = 42"), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
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

}  // namespace python
