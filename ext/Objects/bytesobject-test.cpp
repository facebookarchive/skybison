#include "gtest/gtest.h"

#include <cstring>

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using BytesExtensionApiTest = ExtensionApi;

TEST_F(BytesExtensionApiTest,
       AsStringFromNonBytesReturnsNullAndRaisesTypeError) {
  PyObjectPtr num(PyLong_FromLong(0));
  EXPECT_EQ(PyBytes_AsString(num), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, AsStringWithBytesReturnsByteString) {
  const char* str = "foo";
  PyObjectPtr bytes(PyBytes_FromString(str));
  EXPECT_STREQ(PyBytes_AsString(bytes), str);
}

TEST_F(BytesExtensionApiTest, AsStringWithZeroByteReturnsByteString) {
  const char* str = "foo \0 bar";
  PyObjectPtr bytes(PyBytes_FromStringAndSize(str, 9));
  char* result = PyBytes_AsString(bytes);
  std::vector<char> expected(str, str + 9);
  std::vector<char> actual(result, result + 9);
  EXPECT_EQ(actual, expected);
}

TEST_F(BytesExtensionApiTest, AsStringReturnsSameBufferTwice) {
  const char* str = "foo";
  PyObjectPtr bytes(PyBytes_FromString(str));
  char* buffer1 = PyBytes_AsString(bytes);
  char* buffer2 = PyBytes_AsString(bytes);
  EXPECT_EQ(buffer1, buffer2);
}

TEST_F(BytesExtensionApiTest,
       AsStringAndSizeWithNullBufferReturnsNegativeAndRaisesSystemError) {
  PyObjectPtr bytes(PyBytes_FromString(""));
  Py_ssize_t length;
  EXPECT_EQ(PyBytes_AsStringAndSize(bytes, nullptr, &length), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(BytesExtensionApiTest,
       AsStringAndSizeWithNonBytesReturnsNegativeAndRaisesTypeError) {
  PyObjectPtr num(PyLong_FromLong(0));
  char* buffer;
  EXPECT_EQ(PyBytes_AsStringAndSize(num, &buffer, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, AsStringAndSizeWithBytesReturnsStringAndSize) {
  const char* str = "foo";
  PyObjectPtr bytes(PyBytes_FromString(str));
  char* buffer;
  Py_ssize_t length;
  ASSERT_EQ(PyBytes_AsStringAndSize(bytes, &buffer, &length), 0);
  EXPECT_STREQ(buffer, str);
  EXPECT_EQ(length, 3);
}

TEST_F(BytesExtensionApiTest,
       AsStringAndSizeWithBytesAndNullLengthReturnsString) {
  const char* str = "foo";
  PyObjectPtr bytes(PyBytes_FromString(str));
  char* buffer;
  ASSERT_EQ(PyBytes_AsStringAndSize(bytes, &buffer, nullptr), 0);
  EXPECT_STREQ(buffer, str);
}

TEST_F(BytesExtensionApiTest,
       AsStringAndSizeWithZeroByteAndNullLengthRaisesValueError) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("foo \0 bar", 9));
  char* buffer;
  EXPECT_EQ(PyBytes_AsStringAndSize(bytes, &buffer, nullptr), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(BytesExtensionApiTest, AsStringAndSizeReturnsSameBufferTwice) {
  const char* str = "foo";
  PyObjectPtr bytes(PyBytes_FromString(str));
  char* buffer1;
  char* buffer2;
  ASSERT_EQ(PyBytes_AsStringAndSize(bytes, &buffer1, nullptr), 0);
  ASSERT_EQ(PyBytes_AsStringAndSize(bytes, &buffer2, nullptr), 0);
  EXPECT_EQ(buffer1, buffer2);
}

TEST_F(BytesExtensionApiTest, ConcatWithPointerToNullIsNoop) {
  PyObject* foo = nullptr;
  PyObjectPtr bar(PyLong_FromLong(0));
  PyBytes_Concat(&foo, bar);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(BytesExtensionApiTest, ConcatWithNullSetsFirstArgNull) {
  PyObject* foo = PyBytes_FromString("foo");
  PyBytes_Concat(&foo, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(foo, nullptr);
}

TEST_F(BytesExtensionApiTest, ConcatWithNonBytesFirstArgSetsFirstArgNull) {
  PyObject* foo = PyLong_FromLong(0);
  PyObjectPtr bar(PyBytes_FromString("bar"));
  PyBytes_Concat(&foo, bar);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(foo, nullptr);
}

TEST_F(BytesExtensionApiTest, ConcatWithNonBytesFirstArgRaisesTypeError) {
  PyObject* foo = PyLong_FromLong(0);
  PyObjectPtr bar(PyBytes_FromString("bar"));
  PyBytes_Concat(&foo, bar);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, ConcatWithNonBytesSecondArgSetsFirstArgNull) {
  PyObject* foo = PyBytes_FromString("foo");
  PyObjectPtr bar(PyLong_FromLong(0));
  PyBytes_Concat(&foo, bar);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(foo, nullptr);
}

TEST_F(BytesExtensionApiTest, ConcatWithNonBytesSecondArgRaisesTypeError) {
  PyObject* foo = PyBytes_FromString("foo");
  PyObjectPtr bar(PyLong_FromLong(0));
  PyBytes_Concat(&foo, bar);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, ConcatWithBytesConcatenatesByteStrings) {
  PyObject* foo = PyBytes_FromString("foo");
  PyObjectPtr bar(PyBytes_FromString("bar"));
  PyBytes_Concat(&foo, bar);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyBytes_Size(foo), 6);
}

TEST_F(BytesExtensionApiTest, ConcatDoesNotDecrefSecondArg) {
  PyObject* foo = PyBytes_FromString("foo");
  PyObjectPtr bar(PyBytes_FromString("bar"));
  long refcnt = Py_REFCNT(bar);
  ASSERT_GE(refcnt, 1);
  PyBytes_Concat(&foo, bar);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(Py_REFCNT(bar), refcnt);
}

TEST_F(BytesExtensionApiTest, ConcatAndDelDecrefsSecondArg) {
  PyObject* foo = nullptr;
  PyObject* bar = PyBytes_FromString("bar");
  long refcnt = Py_REFCNT(bar);
  ASSERT_GE(refcnt, 1);
  Py_INCREF(bar);  // prevent bar from being deallocated
  PyBytes_ConcatAndDel(&foo, bar);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(Py_REFCNT(bar), refcnt);
  Py_DECREF(bar);
}

TEST_F(BytesExtensionApiTest, FromFormatWithNoSpecifiersReturnsBytes) {
  PyObjectPtr bytes(PyBytes_FromFormat("hello world"));
  ASSERT_TRUE(PyBytes_Check(bytes));
  EXPECT_STREQ(PyBytes_AsString(bytes), "hello world");
}

TEST_F(BytesExtensionApiTest, FromFormatWithManyArgsReturnsBytes) {
  PyObjectPtr bytes(PyBytes_FromFormat("%%%ih%c%.3s", 2, 'e', "llo world"));
  ASSERT_TRUE(PyBytes_Check(bytes));
  EXPECT_STREQ(PyBytes_AsString(bytes), "%2hello");
}

TEST_F(BytesExtensionApiTest, FromFormatIgnoresWidth) {
  PyObjectPtr bytes(PyBytes_FromFormat("%5d", 42));
  ASSERT_TRUE(PyBytes_Check(bytes));
  EXPECT_STREQ(PyBytes_AsString(bytes), "42");
}

TEST_F(BytesExtensionApiTest, FromFormatIgnoresPrecisionForInt) {
  PyObjectPtr bytes(PyBytes_FromFormat("%5d", 42));
  ASSERT_TRUE(PyBytes_Check(bytes));
  EXPECT_STREQ(PyBytes_AsString(bytes), "42");
}

TEST_F(BytesExtensionApiTest, FromFormatUsesPrecisionForString) {
  PyObjectPtr bytes(PyBytes_FromFormat("%.5s", "hello world"));
  ASSERT_TRUE(PyBytes_Check(bytes));
  EXPECT_STREQ(PyBytes_AsString(bytes), "hello");
}

TEST_F(BytesExtensionApiTest, FromFormatCParsesChar) {
  PyObjectPtr bytes(PyBytes_FromFormat("%c", 42));
  EXPECT_STREQ(PyBytes_AsString(bytes), "*");
}

TEST_F(BytesExtensionApiTest, FromFormatCWithNegativeRaisesOverflowError) {
  ASSERT_EQ(PyBytes_FromFormat("%c", -1), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
}

TEST_F(BytesExtensionApiTest, FromFormatCWithLargeRaisesOverflowError) {
  ASSERT_EQ(PyBytes_FromFormat("%c", 256), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_OverflowError));
}

TEST_F(BytesExtensionApiTest, FromFormatLDParsesLongDecimal) {
  PyObjectPtr bytes(PyBytes_FromFormat("%ld", 42l));
  EXPECT_STREQ(PyBytes_AsString(bytes), "42");
}

TEST_F(BytesExtensionApiTest, FromFormatZDParsesPySsizeT) {
  PyObjectPtr bytes(PyBytes_FromFormat("%zd", static_cast<Py_ssize_t>(42)));
  EXPECT_STREQ(PyBytes_AsString(bytes), "42");
}

TEST_F(BytesExtensionApiTest, FromFormatDParsesDecimal) {
  PyObjectPtr bytes(PyBytes_FromFormat("%d", 42));
  EXPECT_STREQ(PyBytes_AsString(bytes), "42");
}

TEST_F(BytesExtensionApiTest, FromFormatLUParsesNumberTypes) {
  PyObjectPtr bytes(PyBytes_FromFormat("%lu", 42l));
  EXPECT_STREQ(PyBytes_AsString(bytes), "42");
}

TEST_F(BytesExtensionApiTest, FromFormatZUParsesSizeT) {
  PyObjectPtr bytes(PyBytes_FromFormat("%zu", static_cast<size_t>(42)));
  EXPECT_STREQ(PyBytes_AsString(bytes), "42");
}

TEST_F(BytesExtensionApiTest, FromFormatUParsesUnsignedDecimal) {
  PyObjectPtr bytes(PyBytes_FromFormat("%u", 42));
  EXPECT_STREQ(PyBytes_AsString(bytes), "42");
}

TEST_F(BytesExtensionApiTest, FromFormatIParsesInt) {
  PyObjectPtr bytes(PyBytes_FromFormat("%i", 42));
  EXPECT_STREQ(PyBytes_AsString(bytes), "42");
}

TEST_F(BytesExtensionApiTest, FromFormatXParsesHex) {
  PyObjectPtr bytes(PyBytes_FromFormat("%x", 42));
  EXPECT_STREQ(PyBytes_AsString(bytes), "2a");
}

TEST_F(BytesExtensionApiTest, FromFormatSParsesString) {
  PyObjectPtr bytes(PyBytes_FromFormat("%s", "UTF-8"));
  ASSERT_TRUE(PyBytes_Check(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 5);
  EXPECT_STREQ(PyBytes_AsString(bytes), "UTF-8");
}

TEST_F(BytesExtensionApiTest, FromFormatPParsesPointer) {
  long value = 0;
  void* test = &value;
  char buff[18];
  std::snprintf(buff, sizeof(buff), "%p", test);
  Py_ssize_t pointer_len = static_cast<Py_ssize_t>(std::strlen(buff));
  PyObjectPtr bytes(PyBytes_FromFormat("%p", test));
  ASSERT_TRUE(PyBytes_Check(bytes));
  if (buff[1] == 'x') {
    EXPECT_EQ(PyBytes_Size(bytes), pointer_len);
    EXPECT_STREQ(PyBytes_AsString(bytes), buff);
  } else {
    EXPECT_EQ(PyBytes_Size(bytes), pointer_len + 2);
    const char* str = PyBytes_AsString(bytes);
    EXPECT_EQ(str[0], '0');
    EXPECT_EQ(str[1], 'x');
    EXPECT_STREQ(str + 2, buff);
  }
}

TEST_F(BytesExtensionApiTest, FromObjectWithBytesReturnsArgument) {
  PyObjectPtr bytes(PyBytes_FromString("foo"));
  PyObjectPtr copy(PyBytes_FromObject(bytes));
  EXPECT_EQ(copy, bytes);
}

TEST_F(BytesExtensionApiTest, FromObjectWithListReturnsBytes) {
  PyObjectPtr list(PyList_New(3));
  PyList_SetItem(list, 0, PyLong_FromLong('a'));
  PyList_SetItem(list, 1, PyLong_FromLong('b'));
  PyList_SetItem(list, 2, PyLong_FromLong('c'));
  PyObjectPtr bytes(PyBytes_FromObject(list));
  EXPECT_STREQ(PyBytes_AsString(bytes), "abc");
}

TEST_F(BytesExtensionApiTest, FromObjectWithTupleReturnsBytes) {
  PyObjectPtr tuple(PyTuple_New(3));
  PyTuple_SetItem(tuple, 0, PyLong_FromLong('a'));
  PyTuple_SetItem(tuple, 1, PyLong_FromLong('b'));
  PyTuple_SetItem(tuple, 2, PyLong_FromLong('c'));
  PyObjectPtr bytes(PyBytes_FromObject(tuple));
  EXPECT_STREQ(PyBytes_AsString(bytes), "abc");
}

TEST_F(BytesExtensionApiTest, FromObjectWithNonIntTupleRaisesTypeError) {
  PyObjectPtr tuple(PyTuple_New(1));
  PyTuple_SetItem(tuple, 0, Py_None);
  ASSERT_EQ(PyBytes_FromObject(tuple), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, FromObjectWithNonIntIndexTupleRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo:
  def __index__(self): return ''
obj = (Foo(),)
)");
  PyObjectPtr tuple(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyBytes_FromObject(tuple), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, FromObjectWithNegativeIntTupleRaisesValueError) {
  PyObjectPtr tuple(PyTuple_New(1));
  PyTuple_SetItem(tuple, 0, PyLong_FromLong(-1));
  ASSERT_EQ(PyBytes_FromObject(tuple), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(BytesExtensionApiTest, FromObjectWithOverflowTupleRaisesValueError) {
  PyObjectPtr tuple(PyTuple_New(1));
  PyTuple_SetItem(tuple, 0, PyLong_FromLong(256));
  ASSERT_EQ(PyBytes_FromObject(tuple), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(BytesExtensionApiTest, FromObjectWithStringRaisesTypeError) {
  PyObjectPtr str(PyUnicode_FromString("hello"));
  ASSERT_EQ(PyBytes_FromObject(str), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, FromObjectWithNonIterableRaisesTypeError) {
  PyObjectPtr num(PyLong_FromLong(1));
  ASSERT_EQ(PyBytes_FromObject(num), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, FromObjectWithBadIteratorRaisesTypeError) {
  PyRun_SimpleString(R"(
class NotIterator: pass
class HasIter:
  def __iter__(self): return NotIterator()
obj = HasIter()
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyBytes_FromObject(obj), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, FromObjectWithSetReturnsBytes) {
  PyObjectPtr set(PySet_New(nullptr));
  PySet_Add(set, PyLong_FromLong('a'));
  PyObjectPtr bytes(PyBytes_FromObject(set));
  EXPECT_STREQ(PyBytes_AsString(bytes), "a");
}

TEST_F(BytesExtensionApiTest, FromStringAndSizeIncrementsRefCount) {
  PyObject* bytes = PyBytes_FromStringAndSize("foo", 3);
  ASSERT_NE(bytes, nullptr);
  EXPECT_GE(Py_REFCNT(bytes), 1);
  Py_DECREF(bytes);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(BytesExtensionApiTest, FromStringAndSizeReturnsBytes) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("foo", 3));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 3);
}

TEST_F(BytesExtensionApiTest,
       FromStringAndSizeWithEmptyStringReturnsEmptyBytes) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("", 0));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 0);
}

TEST_F(BytesExtensionApiTest, FromStringAndSizeWithNegativeSizeReturnsNull) {
  EXPECT_EQ(PyBytes_FromStringAndSize("foo", -1), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(BytesExtensionApiTest, FromStringAndSizeWithShorterSize) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("foo bar", 5));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 5);
}

TEST_F(BytesExtensionApiTest, FromStringAndSizeWithSizeZero) {
  PyObjectPtr bytes(PyBytes_FromStringAndSize("foo bar", 0));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 0);
}

TEST_F(BytesExtensionApiTest, FromStringIncrementsRefCount) {
  PyObject* bytes = PyBytes_FromString("foo");
  ASSERT_NE(bytes, nullptr);
  EXPECT_GE(Py_REFCNT(bytes), 1);
  Py_DECREF(bytes);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(BytesExtensionApiTest, FromStringReturnsBytes) {
  PyObjectPtr bytes(PyBytes_FromString("foo"));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 3);
}

TEST_F(BytesExtensionApiTest, FromStringWithEmptyStringReturnsEmptyBytes) {
  PyObjectPtr bytes(PyBytes_FromString(""));
  EXPECT_TRUE(PyBytes_CheckExact(bytes));
  EXPECT_EQ(PyBytes_Size(bytes), 0);
}

TEST_F(BytesExtensionApiTest, ResizeWithNonBytesRaisesSystemError) {
  PyObject* num = PyLong_FromLong(0);
  ASSERT_EQ(_PyBytes_Resize(&num, 1), -1);
  ASSERT_EQ(num, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(BytesExtensionApiTest, ResizeWithNegativeNewSizeRaisesSystemError) {
  PyObject* bytes = PyBytes_FromString("hello");
  ASSERT_EQ(_PyBytes_Resize(&bytes, -1), -1);
  ASSERT_EQ(bytes, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(BytesExtensionApiTest, ResizeWithSameSizeKeepsBytes) {
  PyObject* bytes = PyBytes_FromString("hello");
  PyObject** arg = &bytes;
  ASSERT_EQ(_PyBytes_Resize(&bytes, 5), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(&bytes, arg);
  Py_DECREF(bytes);
}

TEST_F(BytesExtensionApiTest, ResizeWithLargerSizeAllocatesLargerBytes) {
  PyObject* bytes = PyBytes_FromString("hello");
  ASSERT_EQ(_PyBytes_Resize(&bytes, 10), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyBytes_Size(bytes), 10);
  EXPECT_STREQ(PyBytes_AsString(bytes), "hello");  // sixth byte is 0
  Py_DECREF(bytes);
}

TEST_F(BytesExtensionApiTest, ResizeWithSmallerSizeAllocatesSmallerBytes) {
  PyObject* bytes = PyBytes_FromString("hello");
  ASSERT_EQ(_PyBytes_Resize(&bytes, 2), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyBytes_Size(bytes), 2);
  EXPECT_STREQ(PyBytes_AsString(bytes), "he");
  Py_DECREF(bytes);
}

TEST_F(BytesExtensionApiTest, SizeWithNonBytesReturnsNegative) {
  PyObjectPtr dict(PyDict_New());

  EXPECT_EQ(PyBytes_Size(dict), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

}  // namespace python
