#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using ExceptionsExtensionApiTest = ExtensionApi;

TEST_F(ExceptionsExtensionApiTest,
       ExceptionInstanceCheckWithNonExceptionReturnsZero) {
  PyObjectPtr obj(PyLong_FromLong(5));
  EXPECT_EQ(PyExceptionInstance_Check(obj.get()), 0);
}

TEST_F(ExceptionsExtensionApiTest,
       ExceptionInstanceCheckWithExceptionReturnsOne) {
  PyRun_SimpleString("obj = TypeError()");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_EQ(PyExceptionInstance_Check(obj.get()), 1);
}

TEST_F(ExceptionsExtensionApiTest,
       ExceptionInstanceCheckWithExceptionSubclassReturnsOne) {
  PyRun_SimpleString(R"(
class C(TypeError):
  pass
obj = C()
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  EXPECT_EQ(PyExceptionInstance_Check(obj.get()), 1);
}

TEST_F(ExceptionsExtensionApiTest, GettingCauseWithoutSettingItReturnsNull) {
  PyRun_SimpleString("a = TypeError()");
  PyObjectPtr exc(moduleGet("__main__", "a"));
  PyObjectPtr cause(PyException_GetCause(exc));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(cause, nullptr);
}

TEST_F(ExceptionsExtensionApiTest, GettingCauseAfterSetReturnsSameObject) {
  PyRun_SimpleString("a = TypeError()");
  PyObjectPtr exc(moduleGet("__main__", "a"));
  PyObjectPtr str(PyUnicode_FromString(""));
  // Since SetCause steals a reference, but we want to keept the object around
  // we need to incref it before passing it to the function
  Py_INCREF(str);
  PyException_SetCause(exc, str);
  PyObjectPtr cause(PyException_GetCause(exc));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(cause, str);
}

TEST_F(ExceptionsExtensionApiTest, SettingCauseWithNullSetsCauseToNull) {
  PyRun_SimpleString("a = TypeError()");
  PyObjectPtr exc(moduleGet("__main__", "a"));
  PyException_SetCause(exc, PyUnicode_FromString(""));
  PyException_SetCause(exc, nullptr);
  PyObjectPtr cause(PyException_GetCause(exc));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(cause, nullptr);
}

TEST_F(ExceptionsExtensionApiTest, GettingContextWithoutSettingItReturnsNull) {
  PyRun_SimpleString("a = TypeError()");
  PyObjectPtr exc(moduleGet("__main__", "a"));
  PyObjectPtr context(PyException_GetContext(exc));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(context, nullptr);
}

TEST_F(ExceptionsExtensionApiTest, GettingContextAfterSetReturnsSameObject) {
  PyRun_SimpleString("a = TypeError()");
  PyObjectPtr exc(moduleGet("__main__", "a"));
  PyObjectPtr str(PyUnicode_FromString(""));
  // Since SetContext steals a reference, but we want to keept the object around
  // we need to incref it before passing it to the function
  Py_INCREF(str);
  PyException_SetContext(exc, str);
  PyObjectPtr context(PyException_GetContext(exc));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(context, str);
}

TEST_F(ExceptionsExtensionApiTest, SettingContextWithNullSetsContextToNull) {
  PyRun_SimpleString("a = TypeError()");
  PyObjectPtr exc(moduleGet("__main__", "a"));
  PyException_SetContext(exc, PyUnicode_FromString(""));
  PyException_SetContext(exc, nullptr);
  PyObjectPtr context(PyException_GetContext(exc));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(context, nullptr);
}

TEST_F(ExceptionsExtensionApiTest,
       GettingTracebackWithoutSettingItReturnsNull) {
  PyRun_SimpleString("a = TypeError()");
  PyObjectPtr exc(moduleGet("__main__", "a"));
  EXPECT_EQ(PyException_GetTraceback(exc), nullptr);
}

TEST_F(ExceptionsExtensionApiTest, SetTracebackWithNoneSetsNone) {
  // TODO(bsimmers): Once we have a way of creating a real traceback object,
  // test that as well.
  PyRun_SimpleString("a = TypeError()");
  PyObjectPtr exc(moduleGet("__main__", "a"));
  ASSERT_EQ(PyException_SetTraceback(exc, Py_None), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr tb(PyException_GetTraceback(exc));
  EXPECT_EQ(tb, Py_None);
}

TEST_F(ExceptionsExtensionApiTest, SetTracebackWithBadArgRaisesTypeError) {
  PyRun_SimpleString("a = TypeError()");
  PyObjectPtr exc(moduleGet("__main__", "a"));
  PyObjectPtr bad_tb(PyLong_FromLong(123));
  ASSERT_EQ(PyException_SetTraceback(exc, bad_tb), -1);
  ASSERT_EQ(PyErr_ExceptionMatches(PyExc_TypeError), 1);
}

TEST_F(ExceptionsExtensionApiTest, SetTracebackWithNullptrRaisesTypeError) {
  PyRun_SimpleString("a = TypeError()");
  PyObjectPtr exc(moduleGet("__main__", "a"));
  ASSERT_EQ(PyException_SetTraceback(exc, nullptr), -1);
  ASSERT_EQ(PyErr_ExceptionMatches(PyExc_TypeError), 1);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeDecodeErrorCreateReturnsNewInstance) {
  const char* encoding = "utf8";
  const char* object = "abcd";
  Py_ssize_t length = 5;
  Py_ssize_t start = 2;
  Py_ssize_t end = 4;
  const char* reason = u8"\U0001F37B";

  PyObjectPtr result(PyUnicodeDecodeError_Create(encoding, object, length,
                                                 start, end, reason));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(PyObject_HasAttrString(result, "encoding"));
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(
                PyObject_GetAttrString(result, "encoding"), encoding),
            0);
  EXPECT_TRUE(PyObject_HasAttrString(result, "object"));
  const char* object_attr =
      PyBytes_AsString(PyObject_GetAttrString(result, "object"));
  EXPECT_EQ(strcmp(object_attr, object), 0);
  EXPECT_TRUE(PyObject_HasAttrString(result, "start"));
  EXPECT_EQ(PyLong_AsLong(PyObject_GetAttrString(result, "start")), start);
  EXPECT_TRUE(PyObject_HasAttrString(result, "end"));
  EXPECT_EQ(PyLong_AsLong(PyObject_GetAttrString(result, "end")), end);
  EXPECT_TRUE(PyObject_HasAttrString(result, "reason"));
  PyObjectPtr reason_obj(PyUnicode_FromString(reason));
  EXPECT_EQ(
      PyUnicode_Compare(PyObject_GetAttrString(result, "reason"), reason_obj),
      0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetEncodingWithNonStrEncodingRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
exc.encoding = 5  # not a valid encoding
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  EXPECT_EQ(PyUnicodeDecodeError_GetEncoding(exc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetEncodingReturnsEncodingAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyObjectPtr result(PyUnicodeDecodeError_GetEncoding(exc));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "utf8"), 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetObjectWithNonBytesObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
exc.object = 5  # not a valid object
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  EXPECT_EQ(PyUnicodeDecodeError_GetObject(exc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetObjectReturnsObjectAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyObjectPtr result(PyUnicodeDecodeError_GetObject(exc));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  const char* object_string = PyBytes_AsString(result);
  EXPECT_EQ(strcmp(object_string, "object"), 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetReasonWithNonStrReasonRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
exc.reason = 5  # not a valid reason
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  EXPECT_EQ(PyUnicodeDecodeError_GetReason(exc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetReasonReturnsReasonAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyObjectPtr result(PyUnicodeDecodeError_GetReason(exc));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "reason"), 0);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeDecodeErrorSetReasonSetsReasonAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyUnicodeDecodeError_SetReason(exc, "foobar");
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr result(PyUnicodeDecodeError_GetReason(exc));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "foobar"), 0);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeDecodeErrorGetStartReturnsStartAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 2);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeDecodeErrorGetStartReturnsStartInt) {
  PyRun_SimpleString(R"(
class C(int): pass
exc = UnicodeDecodeError("utf8", b"object", C(2), 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 2);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetStartWithNonBytesObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
exc.object = 5  # not a valid object
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetStart(exc, &start), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetStartWithNegativeStartReturnsZero) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
exc.start = -5
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = -1;
  ASSERT_EQ(PyUnicodeDecodeError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetStartWithStartGreaterThanSizeReturnsSizeMinusOne) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
exc.start = 10
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 5);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeDecodeErrorGetEndReturnsEndAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 4);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeDecodeErrorGetEndReturnsEndInt) {
  PyRun_SimpleString(R"(
class C(int): pass
exc = UnicodeDecodeError("utf8", b"object", 2, C(4), "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 4);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetEndWithNonBytesObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
exc.object = 5  # not a valid object
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetEnd(exc, &end), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetEndWithEndLessThanOneReturnsOne) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
exc.end = -5
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 1);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorGetEndWithEndGreaterThanSizeReturnsSize) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
exc.end = 10
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 6);  // len("object")
}

TEST_F(ExceptionsExtensionApiTest, UnicodeDecodeErrorSetStartSetsStartAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeDecodeError_SetStart(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 5);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeDecodeErrorSetEndSetsEndAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeDecodeError("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeDecodeError_SetEnd(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 5);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorSubclassSetEndGetEndReturnsEnd) {
  PyRun_SimpleString(R"(
class ErrorSubclass(UnicodeDecodeError): pass
exc = ErrorSubclass("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeDecodeError_SetEnd(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 5);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorSubclassSetStartGetStartReturnsStart) {
  PyRun_SimpleString(R"(
class ErrorSubclass(UnicodeDecodeError): pass
exc = ErrorSubclass("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeDecodeError_SetStart(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeDecodeError_GetStart(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 5);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeDecodeErrorSubclassSetReasonGetReasonReturnsReason) {
  PyRun_SimpleString(R"(
class ErrorSubclass(UnicodeDecodeError): pass
exc = ErrorSubclass("utf8", b"object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyUnicodeDecodeError_SetReason(exc, "foobar");
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr result(PyUnicodeDecodeError_GetReason(exc));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "foobar"), 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetEncodingWithNonStrEncodingRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
exc.encoding = 5  # not a valid encoding
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  EXPECT_EQ(PyUnicodeEncodeError_GetEncoding(exc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetEncodingReturnsEncodingAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyObjectPtr result(PyUnicodeEncodeError_GetEncoding(exc));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "utf8"), 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetObjectWithNonStrObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
exc.object = 5  # not a valid object
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  EXPECT_EQ(PyUnicodeEncodeError_GetObject(exc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetObjectReturnsObjectAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyObjectPtr result(PyUnicodeEncodeError_GetObject(exc));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "object"), 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetReasonWithNonStrReasonRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
exc.reason = 5  # not a valid reason
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  EXPECT_EQ(PyUnicodeEncodeError_GetReason(exc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetReasonReturnsReasonAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyObjectPtr result(PyUnicodeEncodeError_GetReason(exc));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "reason"), 0);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeEncodeErrorSetReasonSetsReasonAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyUnicodeEncodeError_SetReason(exc, "foobar");
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr result(PyUnicodeEncodeError_GetReason(exc));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "foobar"), 0);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeEncodeErrorGetStartReturnsStartAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 2);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeEncodeErrorGetStartReturnsStartInt) {
  PyRun_SimpleString(R"(
class C(int): pass
exc = UnicodeEncodeError("utf8", "object", C(2), 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 2);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetStartWithNonStrObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
exc.object = 5  # not a valid object
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetStart(exc, &start), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetStartWithNegativeStartReturnsZero) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
exc.start = -5
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = -1;
  ASSERT_EQ(PyUnicodeEncodeError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetStartWithStartGreaterThanSizeReturnsSizeMinusOne) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
exc.start = 10
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 5);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeEncodeErrorGetEndReturnsEndAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 4);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeEncodeErrorGetEndReturnsEndInt) {
  PyRun_SimpleString(R"(
class C(int): pass
exc = UnicodeEncodeError("utf8", "object", 2, C(4), "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 4);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetEndWithNonStrObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
exc.object = 5  # not a valid object
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetEnd(exc, &end), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetEndWithEndLessThanOneReturnsOne) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
exc.end = -5
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 1);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorGetEndWithEndGreaterThanSizeReturnsSize) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
exc.end = 10
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 6);  // len("object")
}

TEST_F(ExceptionsExtensionApiTest, UnicodeEncodeErrorSetStartSetsStartAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeEncodeError_SetStart(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 5);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeEncodeErrorSetEndSetsEndAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeEncodeError("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeEncodeError_SetEnd(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 5);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorSubclassSetEndGetEndReturnsEnd) {
  PyRun_SimpleString(R"(
class ErrorSubclass(UnicodeEncodeError): pass
exc = ErrorSubclass("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeEncodeError_SetEnd(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 5);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorSubclassSetStartGetStartReturnsStart) {
  PyRun_SimpleString(R"(
class ErrorSubclass(UnicodeEncodeError): pass
exc = ErrorSubclass("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeEncodeError_SetStart(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeEncodeError_GetStart(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 5);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeEncodeErrorSubclassSetReasonGetReasonReturnsReason) {
  PyRun_SimpleString(R"(
class ErrorSubclass(UnicodeEncodeError): pass
exc = ErrorSubclass("utf8", "object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyUnicodeEncodeError_SetReason(exc, "foobar");
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr result(PyUnicodeEncodeError_GetReason(exc));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "foobar"), 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorGetObjectWithNonBytesObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
exc.object = 5  # not a valid object
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  EXPECT_EQ(PyUnicodeTranslateError_GetObject(exc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorGetObjectReturnsObjectAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyObjectPtr result(PyUnicodeTranslateError_GetObject(exc));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "object"), 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorGetReasonWithNonStrReasonRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
exc.reason = 5  # not a valid reason
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  EXPECT_EQ(PyUnicodeTranslateError_GetReason(exc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorGetReasonReturnsReasonAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyObjectPtr result(PyUnicodeTranslateError_GetReason(exc));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "reason"), 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorSetReasonSetsReasonAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyUnicodeTranslateError_SetReason(exc, "foobar");
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr result(PyUnicodeTranslateError_GetReason(exc));
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "foobar"), 0);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorGetStartReturnsStartAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 2);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorGetStartWithNonStrObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
exc.object = 5  # not a valid object
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetStart(exc, &start), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorGetStartWithNegativeStartReturnsZero) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
exc.start = -5
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = -1;
  ASSERT_EQ(PyUnicodeTranslateError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 0);
}

TEST_F(
    ExceptionsExtensionApiTest,
    UnicodeTranslateErrorGetStartWithStartGreaterThanSizeReturnsSizeMinusOne) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
exc.start = 10
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 5);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeTranslateErrorGetEndReturnsEndAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 4);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorGetEndWithNonStrObjectRaisesTypeError) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
exc.object = 5  # not a valid object
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetEnd(exc, &end), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorGetEndWithEndLessThanOneReturnsOne) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
exc.end = -5
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 1);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorGetEndWithEndGreaterThanSizeReturnsSize) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
exc.end = 10
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 6);  // len("object")
}

TEST_F(ExceptionsExtensionApiTest, UnicodeTranslateErrorSetStartSetsStartAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeTranslateError_SetStart(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t start = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetStart(exc, &start), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(start, 5);
}

TEST_F(ExceptionsExtensionApiTest, UnicodeTranslateErrorSetEndSetsEndAttr) {
  PyRun_SimpleString(R"(
exc = UnicodeTranslateError("object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeTranslateError_SetEnd(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 5);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorSubclassSetEndGetEndReturnsEnd) {
  PyRun_SimpleString(R"(
class ErrorSubclass(UnicodeTranslateError): pass
exc = ErrorSubclass("object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeTranslateError_SetEnd(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetEnd(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 5);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorSubclassSetStartGetStartReturnsStart) {
  PyRun_SimpleString(R"(
class ErrorSubclass(UnicodeTranslateError): pass
exc = ErrorSubclass("object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  ASSERT_EQ(PyUnicodeTranslateError_SetStart(exc, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ssize_t end = 0;
  ASSERT_EQ(PyUnicodeTranslateError_GetStart(exc, &end), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(end, 5);
}

TEST_F(ExceptionsExtensionApiTest,
       UnicodeTranslateErrorSubclassSetReasonGetReasonReturnsReason) {
  PyRun_SimpleString(R"(
class ErrorSubclass(UnicodeTranslateError): pass
exc = ErrorSubclass("object", 2, 4, "reason")
)");
  PyObjectPtr exc(moduleGet("__main__", "exc"));
  PyUnicodeTranslateError_SetReason(exc, "foobar");
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr result(PyUnicodeTranslateError_GetReason(exc));
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyUnicode_CompareWithASCIIString(result, "foobar"), 0);
}

}  // namespace testing
}  // namespace py
