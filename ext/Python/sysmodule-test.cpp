#include "gtest/gtest.h"

#include <string>

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {

using namespace testing;

using SysModuleExtensionApiTest = ExtensionApi;

TEST_F(SysModuleExtensionApiTest, GetObjectWithNonExistentNameReturnsNull) {
  EXPECT_EQ(PySys_GetObject("foo_bar_not_a_real_name"), nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(SysModuleExtensionApiTest, GetObjectReturnsValueFromSysModule) {
  PyRun_SimpleString(R"(
import sys
sys.foo = 'bar'
)");
  PyObject* result = PySys_GetObject("foo");  // borrowed reference
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(result, "bar"));
}

TEST_F(SysModuleExtensionApiTest, GetSizeOfPropagatesException) {
  PyRun_SimpleString(R"(
class C:
  def __sizeof__(self): raise Exception()
o = C()
)");
  PyObjectPtr object(moduleGet("__main__", "o"));
  EXPECT_EQ(_PySys_GetSizeOf(object), static_cast<size_t>(-1));
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_Exception));
}

TEST_F(SysModuleExtensionApiTest, GetSizeOfReturnsDunderSizeOfPyro) {
  PyRun_SimpleString(R"(
class C:
  def __sizeof__(self): return 10
o = C()
)");
  PyObjectPtr object(moduleGet("__main__", "o"));
  EXPECT_EQ(_PySys_GetSizeOf(object), 10);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(SysModuleExtensionApiTest, GetSizeOfWithIntSubclassReturnsIntPyro) {
  PyRun_SimpleString(R"(
class N(int): pass
class C:
  def __sizeof__(self): return N(10)
o = C()
)");
  PyObjectPtr object(moduleGet("__main__", "o"));
  EXPECT_EQ(_PySys_GetSizeOf(object), 10);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(SysModuleExtensionApiTest, WriteStdout) {
  CaptureStdStreams streams;
  PySys_WriteStdout("Hello, %s!", "World");
  EXPECT_EQ(streams.out(), "Hello, World!");
  EXPECT_EQ(streams.err(), "");
}

TEST_F(
    SysModuleExtensionApiTest,
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

TEST_F(SysModuleExtensionApiTest, WriteStdoutWithSysStdoutNoneWritesToStdout) {
  PyRun_SimpleString(R"(
import sys
sys.stdout = None
)");
  CaptureStdStreams streams;
  PySys_WriteStdout("Hello");
  EXPECT_EQ(streams.out(), "Hello");
  EXPECT_EQ(streams.err(), "");
}

TEST_F(SysModuleExtensionApiTest, WriteStdoutWithoutSysStdoutWritesToStdout) {
  PyRun_SimpleString(R"(
import sys
del sys.stdout
)");
  CaptureStdStreams streams;
  PySys_WriteStdout("Konnichiwa");
  EXPECT_EQ(streams.out(), "Konnichiwa");
  EXPECT_EQ(streams.err(), "");
}

TEST_F(SysModuleExtensionApiTest, WriteStdoutTruncatesLongOutput) {
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

TEST_F(SysModuleExtensionApiTest, WriteStderr) {
  CaptureStdStreams streams;
  PySys_WriteStderr("2 + 2 = %d", 4);
  EXPECT_EQ(streams.out(), "");
  EXPECT_EQ(streams.err(), "2 + 2 = 4");
}

}  // namespace py
