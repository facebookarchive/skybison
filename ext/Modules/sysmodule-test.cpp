#include "gtest/gtest.h"

#include <string>

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using SysExtensionApiTest = ExtensionApi;

TEST_F(SysExtensionApiTest, WriteStdout) {
  CaptureStdStreams streams;
  PySys_WriteStdout("Hello, %s!", "World");
  EXPECT_EQ(streams.out(), "Hello, World!");
  EXPECT_EQ(streams.err(), "");
}

TEST_F(
    SysExtensionApiTest,
    WriteStdoutCallsSysStdoutWriteOnExceptionWritesToFallbackAndClearsError) {
  PyRun_SimpleString(R"(
import sys
x = 7
class C:
  def write(self, text):
    global x
    x = 42
    raise UserWarning()

sys.stdout = C()
)");
  CaptureStdStreams streams;
  PySys_WriteStdout("a");
  EXPECT_EQ(streams.out(), "a");
  EXPECT_EQ(streams.err(), "");
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr x(moduleGet("__main__", "x"));
  EXPECT_EQ(PyLong_AsLong(x), 42);
}

TEST_F(SysExtensionApiTest, WriteStdoutWithSysStdoutNoneWritesToStdout) {
  PyRun_SimpleString(R"(
import sys
sys.stdout = None
)");
  CaptureStdStreams streams;
  PySys_WriteStdout("Hello");
  EXPECT_EQ(streams.out(), "Hello");
  EXPECT_EQ(streams.err(), "");
}

TEST_F(SysExtensionApiTest, WriteStdoutWithoutSysStdoutWritesToStdout) {
  PyRun_SimpleString(R"(
import sys
del sys.stdout
)");
  CaptureStdStreams streams;
  PySys_WriteStdout("Konnichiwa");
  EXPECT_EQ(streams.out(), "Konnichiwa");
  EXPECT_EQ(streams.err(), "");
}

TEST_F(SysExtensionApiTest, WriteStdoutTruncatesLongOutput) {
  static const int max_out_len = 1000;
  std::string long_str;
  for (int i = 0; i < 100; i++) {
    long_str += "0123456789";
  }
  ASSERT_EQ(long_str.size(), max_out_len);

  CaptureStdStreams streams;
  PySys_WriteStdout("%s hello", long_str.c_str());
  EXPECT_EQ(streams.out(), long_str + "... truncated");
  EXPECT_EQ(streams.err(), "");
}

TEST_F(SysExtensionApiTest, WriteStderr) {
  CaptureStdStreams streams;
  PySys_WriteStderr("2 + 2 = %d", 4);
  EXPECT_EQ(streams.out(), "");
  EXPECT_EQ(streams.err(), "2 + 2 = 4");
}

}  // namespace py
