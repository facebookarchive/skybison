#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using TracebackExtensionApiTest = ExtensionApi;

TEST_F(TracebackExtensionApiTest, TraceBackPrintWithNullptrIsNoop) {
  CaptureStdStreams streams;
  ASSERT_EQ(PyTraceBack_Print(nullptr, nullptr), 0);

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(streams.out(), "");
  EXPECT_EQ(streams.err(), "");
}

TEST_F(TracebackExtensionApiTest,
       TraceBackPrintWithNonTracebackRaisesSystemError) {
  PyObjectPtr tb(PyLong_FromLong(42));
  ASSERT_EQ(PyTraceBack_Print(tb, nullptr), -1);

  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyErr_ExceptionMatches(PyExc_SystemError), 1);
}

TEST_F(TracebackExtensionApiTest, TraceBackPrintWithTracebackWritesToFile) {
  CaptureStdStreams streams;
  EXPECT_EQ(0, PyRun_SimpleString(R"(
try:
    raise RuntimeError("inner")
except Exception as e:
    tb = e.__traceback__
)"));

  PyObjectPtr tb(mainModuleGet("tb"));
  PyObject* file = PySys_GetObject("stderr");
  ASSERT_EQ(PyTraceBack_Print(tb, file), 0);

  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(streams.out(), "");
  EXPECT_EQ(streams.err(), R"(Traceback (most recent call last):
  File "<string>", line 3, in <module>
)");
}

TEST_F(TracebackExtensionApiTest, TracebackAddSetsTraceback) {
  CaptureStdStreams streams;
  _PyTraceback_Add("foo", "bar", 42);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObject *exc, *val, *tb;
  PyErr_Fetch(&exc, &val, &tb);
  PyObject* file = PySys_GetObject("stderr");
  ASSERT_EQ(PyTraceBack_Print(tb, file), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  EXPECT_EQ(streams.out(), "");
  EXPECT_EQ(streams.err(), R"(Traceback (most recent call last):
  File "bar", line 42, in foo
)");
}

}  // namespace testing
}  // namespace py
