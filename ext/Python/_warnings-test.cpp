#include "Python.h"
#include "gmock/gmock-matchers.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using UnderWarningsExtensionApiTest = ExtensionApi;

TEST_F(UnderWarningsExtensionApiTest,
       WarnFormatWithNullCategoryPrintsRuntimeWarning) {
  CaptureStdStreams streams;
  EXPECT_EQ(PyErr_WarnFormat(nullptr, 0, "%d", 0), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(), ::testing::EndsWith("RuntimeWarning: 0\n"));
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnFormatWithNonTypeCategoryRaisesTypeError) {
  EXPECT_EQ(PyErr_WarnFormat(Py_True, 0, "blah"), -1);
  EXPECT_EQ(PyErr_Occurred(), PyExc_TypeError);
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnExWithNullCategoryPrintsRuntimeWarning) {
  CaptureStdStreams streams;
  EXPECT_EQ(PyErr_WarnEx(nullptr, "bar", 0), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(), ::testing::EndsWith("RuntimeWarning: bar\n"));
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnExWithNegativeStackLevelDefaultsToCurrentModule) {
  CaptureStdStreams streams;
  EXPECT_EQ(PyErr_WarnEx(PyExc_RuntimeWarning, "bar", -10), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(),
              ::testing::EndsWith("sys:1: RuntimeWarning: bar\n"));
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnExWithStackLevelGreaterThanDepthDefaultsToSys) {
  CaptureStdStreams streams;
  // TODO(T43609717): Determine the current stack depth with C-API and ensure
  // that this is a bigger number.
  EXPECT_EQ(PyErr_WarnEx(PyExc_RuntimeWarning, "bar", PY_SSIZE_T_MAX - 1), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(),
              ::testing::EndsWith("sys:1: RuntimeWarning: bar\n"));
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnExWithNonTypeCategoryRaisesTypeError) {
  EXPECT_EQ(PyErr_WarnEx(Py_True, "blah", 0), -1);
  EXPECT_EQ(PyErr_Occurred(), PyExc_TypeError);
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnExplicitObjectWithNoneModuleDoesNothing) {
  CaptureStdStreams streams;
  PyObjectPtr message(PyUnicode_FromString("foo"));
  PyObjectPtr filename(PyUnicode_FromString("bar"));
  EXPECT_EQ(PyErr_WarnExplicitObject(PyExc_RuntimeWarning, message, filename,
                                     /*lineno=*/1, /*module=*/Py_None,
                                     /*registry=*/Py_None),
            0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(streams.err(), "");
  EXPECT_EQ(streams.out(), "");
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnExplicitObjectWithNullCategoryPrintsRuntimeWarning) {
  CaptureStdStreams streams;
  PyObjectPtr message(PyUnicode_FromString("foo"));
  PyObjectPtr filename(PyUnicode_FromString("bar"));
  PyObjectPtr module(PyUnicode_FromString("baz"));
  EXPECT_EQ(PyErr_WarnExplicitObject(/*category=*/nullptr, message, filename,
                                     /*lineno=*/1, module,
                                     /*registry=*/Py_None),
            0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(),
              ::testing::EndsWith("bar:1: RuntimeWarning: foo\n"));
}

TEST_F(UnderWarningsExtensionApiTest,
       WarnExplicitObjectWithNullRegistryPassesNoneRegistry) {
  CaptureStdStreams streams;
  PyObjectPtr message(PyUnicode_FromString("foo"));
  PyObjectPtr filename(PyUnicode_FromString("bar"));
  PyObjectPtr module(PyUnicode_FromString("baz"));
  EXPECT_EQ(PyErr_WarnExplicitObject(/*category=*/PyExc_FutureWarning, message,
                                     filename,
                                     /*lineno=*/1, module,
                                     /*registry=*/nullptr),
            0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyErr_WarnExplicitObject(/*category=*/PyExc_FutureWarning, message,
                                     filename,
                                     /*lineno=*/1, module,
                                     /*registry=*/nullptr),
            0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_THAT(streams.err(),
              ::testing::EndsWith(
                  "bar:1: FutureWarning: foo\nbar:1: FutureWarning: foo\n"));
}

}  // namespace testing
}  // namespace py
