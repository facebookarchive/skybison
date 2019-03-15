#include "gtest/gtest.h"

#include <string>

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using SysExtensionApiTest = ExtensionApi;

TEST_F(SysExtensionApiTest, WriteStdout) {
  CaptureStdStreams streams;
  PySys_WriteStdout("Hello, %s!", "World");
  EXPECT_EQ(streams.out(), "Hello, World!");
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

}  // namespace python
