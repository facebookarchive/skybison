#include "gtest/gtest.h"

#include <cstring>

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using BytesExtensionApiTest = ExtensionApi;
using BytesWriterExtensionApiTest = ExtensionApi;

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

TEST_F(BytesExtensionApiTest, AsStringWithBytesSubclassReturnsString) {
  PyRun_SimpleString(R"(
class Foo(bytes): pass
foo = Foo(b"foo")
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  EXPECT_STREQ(PyBytes_AsString(foo), "foo");
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
       AsStringAndSizeWithBytesSubclassReturnsStringAndSize) {
  PyRun_SimpleString(R"(
class Foo(bytes): pass
foo = Foo(b"foo")
)");
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  char* buffer;
  Py_ssize_t length;
  ASSERT_EQ(PyBytes_AsStringAndSize(foo, &buffer, &length), 0);
  EXPECT_STREQ(buffer, "foo");
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

TEST_F(BytesExtensionApiTest, ConcatWithBytesSubclassesReturnsBytes) {
  PyRun_SimpleString(R"(
class Foo(bytes): pass
foo = Foo(b"foo")
bar = Foo(b"bar")
)");
  PyObject* foo = moduleGet("__main__", "foo");
  PyObjectPtr bar(moduleGet("__main__", "bar"));
  PyBytes_Concat(&foo, bar);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_STREQ(PyBytes_AsString(foo), "foobar");
  Py_DECREF(foo);
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

TEST_F(BytesExtensionApiTest, DecodeEscapeReturnsString) {
  PyObjectPtr bytes(
      PyBytes_DecodeEscape("hello \\\nworld", 13, nullptr, 0, nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_STREQ(PyBytes_AsString(bytes), "hello world");
}

TEST_F(BytesExtensionApiTest, UnderDecodeEscapeSetsFirstInvalidEscapeToNull) {
  const char* invalid = reinterpret_cast<const char*>(0x100);
  EXPECT_NE(_PyBytes_DecodeEscape("hello", 5, nullptr, 0, nullptr, &invalid),
            nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(invalid, nullptr);
}

TEST_F(BytesExtensionApiTest, UnderDecodeEscapeReturnsFirstInvalid) {
  const char* invalid;
  PyObjectPtr bytes(_PyBytes_DecodeEscape("hello \\yworld", 13, nullptr, 0,
                                          nullptr, &invalid));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_STREQ(PyBytes_AsString(bytes), "hello \\yworld");
  EXPECT_EQ(*invalid, 'y');
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
  PyObjectPtr bytes(PyBytes_FromFormat("%zd", Py_ssize_t{42}));
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
  PyObjectPtr bytes(PyBytes_FromFormat("%zu", size_t{42}));
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

TEST_F(BytesExtensionApiTest, JoinWithEmptyIteratorReturnsBytes) {
  PyObjectPtr sep(PyBytes_FromString("foo"));
  PyObjectPtr iter(PyTuple_New(0));
  PyObjectPtr join(_PyBytes_Join(sep, iter));
  EXPECT_TRUE(PyBytes_CheckExact(join));
  EXPECT_STREQ(PyBytes_AsString(join), "");
}

TEST_F(BytesExtensionApiTest, JoinWithOneElementIteratorReturnsBytes) {
  PyObjectPtr sep(PyBytes_FromString("foo"));
  PyObjectPtr iter(PyTuple_New(1));
  ASSERT_EQ(PyTuple_SetItem(iter, 0, PyBytes_FromString("bar")), 0);
  PyObjectPtr join(_PyBytes_Join(sep, iter));
  EXPECT_TRUE(PyBytes_CheckExact(join));
  EXPECT_STREQ(PyBytes_AsString(join), "bar");
}

TEST_F(BytesExtensionApiTest, JoinWithEmptySeparatorReturnsBytes) {
  PyObjectPtr sep(PyBytes_FromString(""));
  PyObjectPtr iter(PyList_New(3));
  ASSERT_EQ(PyList_SetItem(iter, 0, PyBytes_FromString("ab")), 0);
  ASSERT_EQ(PyList_SetItem(iter, 1, PyBytes_FromString("cde")), 0);
  ASSERT_EQ(PyList_SetItem(iter, 2, PyBytes_FromString("f")), 0);
  PyObjectPtr join(_PyBytes_Join(sep, iter));
  EXPECT_TRUE(PyBytes_CheckExact(join));
  EXPECT_STREQ(PyBytes_AsString(join), "abcdef");
}

TEST_F(BytesExtensionApiTest, JoinWithNonEmptySeparatorReturnsBytes) {
  PyObjectPtr sep(PyBytes_FromString(" "));
  PyObjectPtr iter(PyList_New(3));
  ASSERT_EQ(PyList_SetItem(iter, 0, PyBytes_FromString("ab")), 0);
  ASSERT_EQ(PyList_SetItem(iter, 1, PyBytes_FromString("cde")), 0);
  ASSERT_EQ(PyList_SetItem(iter, 2, PyBytes_FromString("f")), 0);
  PyObjectPtr join(_PyBytes_Join(sep, iter));
  EXPECT_TRUE(PyBytes_CheckExact(join));
  EXPECT_STREQ(PyBytes_AsString(join), "ab cde f");
}

TEST_F(BytesExtensionApiTest, JoinWithBytesSubclassReturnsBytes) {
  PyRun_SimpleString(R"(
class Foo(bytes):
  def join(self, iterable):
    # we expect to call bytes.join(), not this method
    return 0
sep = Foo(b"-")
first = Foo(b"ab")
second = Foo(b"")
third = Foo(b"123456789")
  )");
  PyObjectPtr sep(moduleGet("__main__", "sep"));
  PyObjectPtr iter(PyList_New(3));
  ASSERT_EQ(PyList_SetItem(iter, 0, moduleGet("__main__", "first")), 0);
  ASSERT_EQ(PyList_SetItem(iter, 1, moduleGet("__main__", "second")), 0);
  ASSERT_EQ(PyList_SetItem(iter, 2, moduleGet("__main__", "third")), 0);
  PyObjectPtr join(_PyBytes_Join(sep, iter));
  EXPECT_TRUE(PyBytes_CheckExact(join));
  EXPECT_STREQ(PyBytes_AsString(join), "ab--123456789");
}

TEST_F(BytesExtensionApiTest, JoinWithMistypedIteratorRaisesTypeError) {
  PyObjectPtr sep(PyBytes_FromString(" "));
  PyObjectPtr iter(PyTuple_New(1));
  ASSERT_EQ(PyTuple_SetItem(iter, 0, PyLong_FromLong(0)), 0);
  ASSERT_EQ(_PyBytes_Join(sep, iter), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, ReprWithNonBytesRaisesTypeErrorPyro) {
  PyObjectPtr array(PyByteArray_FromStringAndSize("", 0));
  ASSERT_EQ(PyBytes_Repr(array, true), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(BytesExtensionApiTest, ReprWithEmptyBytesReturnsEmptyRepr) {
  PyObjectPtr bytes(PyBytes_FromString(""));
  PyObjectPtr repr(PyBytes_Repr(bytes, true));
  ASSERT_TRUE(PyUnicode_Check(repr));
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, "b''"));
}

TEST_F(BytesExtensionApiTest, ReprWithSimpleBytesReturnsRepr) {
  PyObjectPtr bytes(PyBytes_FromString("Hello world!"));
  PyObjectPtr repr(PyBytes_Repr(bytes, true));
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, "b'Hello world!'"));
}

TEST_F(BytesExtensionApiTest, ReprWithBytesSubclassReturnsString) {
  PyRun_SimpleString(R"(
class Foo(bytes): pass
self = Foo(b"Hello world!")
)");
  PyObjectPtr self(moduleGet("__main__", "self"));
  PyObjectPtr repr(PyBytes_Repr(self, true));
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, "b'Hello world!'"));
}

TEST_F(BytesExtensionApiTest, ReprWithDoubleQuoteUsesSingleQuoteDelimiters) {
  PyObjectPtr bytes(PyBytes_FromString(R"(_"_)"));
  PyObjectPtr repr(PyBytes_Repr(bytes, true));
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, R"(b'_"_')"));
}

TEST_F(BytesExtensionApiTest, ReprWithSingleQuoteUsesDoubleQuoteDelimiters) {
  PyObjectPtr bytes(PyBytes_FromString("_'_"));
  PyObjectPtr repr(PyBytes_Repr(bytes, true));
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, R"(b"_'_")"));
}

TEST_F(BytesExtensionApiTest,
       ReprWithSingleQuoteWithoutSmartquotesUsesSingleQuoteDelimiters) {
  PyObjectPtr bytes(PyBytes_FromString("_'_"));
  PyObjectPtr repr(PyBytes_Repr(bytes, false));
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, R"(b'_\'_')"));
}

TEST_F(BytesExtensionApiTest, ReprWithBothQuotesUsesSingleQuoteDelimiters) {
  PyObjectPtr bytes(PyBytes_FromString(R"(_"_'_)"));
  PyObjectPtr repr(PyBytes_Repr(bytes, true));
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, R"(b'_"_\'_')"));
}

TEST_F(BytesExtensionApiTest, ReprWithSpeciaBytesUsesEscapeSequences) {
  PyObjectPtr bytes(PyBytes_FromString("\\\t\n\r"));
  PyObjectPtr repr(PyBytes_Repr(bytes, true));
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, R"(b'\\\t\n\r')"));
}

TEST_F(BytesExtensionApiTest, ReprWithSmallAndLargeBytesUsesHex) {
  char str[4] = {0, 0x1f, static_cast<char>(0x80), static_cast<char>(0xff)};
  Py_ssize_t size = static_cast<Py_ssize_t>(sizeof(str));
  PyObjectPtr bytes(PyBytes_FromStringAndSize(str, size));
  PyObjectPtr repr(PyBytes_Repr(bytes, true));
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, R"(b'\x00\x1f\x80\xff')"));
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

TEST_F(BytesExtensionApiTest, SizeWithBytesSubclassesReturnsLength) {
  PyRun_SimpleString(R"(
class Foo(bytes): pass
empty = Foo()
foo = Foo(b"foo")
)");
  PyObjectPtr empty(moduleGet("__main__", "empty"));
  PyObjectPtr foo(moduleGet("__main__", "foo"));
  EXPECT_EQ(PyBytes_Size(empty), 0);
  EXPECT_EQ(PyBytes_Size(foo), 3);
}

// _PyBytesWriter API

TEST_F(BytesWriterExtensionApiTest, AllocSetsUpBuffer) {
  Py_ssize_t size = 10;
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  EXPECT_NE(_PyBytesWriter_Alloc(&writer, size), nullptr);
  EXPECT_GT(writer.allocated, size);
  EXPECT_EQ(writer.min_size, size);
}

TEST_F(BytesWriterExtensionApiTest, DeallocFreesHeapBuffer) {
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  void* str = _PyBytesWriter_Alloc(&writer, 0);
  EXPECT_NE(str, nullptr);
  str = _PyBytesWriter_Resize(&writer, str, 5000);
  EXPECT_NE(str, nullptr);
  const char* to_write = "Hello world!";
  Py_ssize_t len = std::strlen(to_write);
  str = _PyBytesWriter_WriteBytes(&writer, str, to_write, len);
  ASSERT_NE(str, nullptr);
  EXPECT_EQ(writer.min_size, len);
  PyObjectPtr result(_PyBytesWriter_Finish(&writer, str));
  ASSERT_TRUE(PyBytes_CheckExact(result));
  _PyBytesWriter_Dealloc(&writer);
}

TEST_F(BytesWriterExtensionApiTest, FinishWithEmptyWriterReturnsEmptyBytes) {
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  void* str = _PyBytesWriter_Alloc(&writer, 0);
  ASSERT_NE(str, nullptr);
  PyObjectPtr result(_PyBytesWriter_Finish(&writer, str));
  _PyBytesWriter_Dealloc(&writer);
  EXPECT_TRUE(PyBytes_CheckExact(result));
  EXPECT_EQ(PyBytes_Size(result), 0);
}

TEST_F(BytesWriterExtensionApiTest,
       FinishWithEmptyWriterUseByteArrayReturnsEmptyByteArray) {
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  void* str = _PyBytesWriter_Alloc(&writer, 0);
  ASSERT_NE(str, nullptr);
  writer.use_bytearray = true;
  PyObjectPtr result(_PyBytesWriter_Finish(&writer, str));
  _PyBytesWriter_Dealloc(&writer);
  EXPECT_TRUE(PyByteArray_CheckExact(result));
  EXPECT_EQ(PyByteArray_Size(result), 0);
}

TEST_F(BytesWriterExtensionApiTest, FinishReturnsBytes) {
  Py_ssize_t size = 10;
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  char* str = reinterpret_cast<char*>(_PyBytesWriter_Alloc(&writer, 0));
  ASSERT_NE(str, nullptr);
  for (char c = 'a'; c < 'a' + size; c++) {
    *str++ = c;
  }
  PyObjectPtr result(_PyBytesWriter_Finish(&writer, str));
  _PyBytesWriter_Dealloc(&writer);
  EXPECT_TRUE(PyBytes_CheckExact(result));
  EXPECT_EQ(PyBytes_Size(result), size);
  EXPECT_STREQ(PyBytes_AsString(result), "abcdefghij");
}

TEST_F(BytesWriterExtensionApiTest, FinishUseByteArrayReturnsByteArray) {
  Py_ssize_t size = 10;
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  char* str = reinterpret_cast<char*>(_PyBytesWriter_Alloc(&writer, 0));
  ASSERT_NE(str, nullptr);
  for (char c = 'a'; c < 'a' + size; c++) {
    *str++ = c;
  }
  writer.use_bytearray = true;
  PyObjectPtr result(_PyBytesWriter_Finish(&writer, str));
  _PyBytesWriter_Dealloc(&writer);
  EXPECT_TRUE(PyByteArray_CheckExact(result));
  EXPECT_EQ(PyByteArray_Size(result), size);
  EXPECT_STREQ(PyByteArray_AsString(result), "abcdefghij");
}

TEST_F(BytesWriterExtensionApiTest, InitSetsFieldsToZero) {
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  EXPECT_EQ(writer.allocated, 0);
  EXPECT_EQ(writer.min_size, 0);
  EXPECT_FALSE(writer.overallocate);
  EXPECT_FALSE(writer.use_bytearray);
}

TEST_F(BytesWriterExtensionApiTest, PrepareWithZeroReturnsSamePointer) {
  Py_ssize_t initial_size = 10;
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  void* str = _PyBytesWriter_Alloc(&writer, initial_size);
  ASSERT_NE(str, nullptr);
  ASSERT_EQ(writer.min_size, initial_size);
  EXPECT_EQ(_PyBytesWriter_Prepare(&writer, str, 0), str);
  EXPECT_EQ(writer.min_size, initial_size);
}

TEST_F(BytesWriterExtensionApiTest, PrepareWithNonZeroIncreasesMinSize) {
  Py_ssize_t initial_size = 10;
  Py_ssize_t growth = 20;
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  void* str = _PyBytesWriter_Alloc(&writer, initial_size);
  ASSERT_NE(str, nullptr);
  ASSERT_EQ(writer.min_size, initial_size);
  EXPECT_NE(_PyBytesWriter_Prepare(&writer, str, growth), nullptr);
  EXPECT_EQ(writer.min_size, initial_size + growth);
}

TEST_F(BytesWriterExtensionApiTest, PrepareWithLargeIntRaisesMemoryError) {
  Py_ssize_t initial_size = 10;
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  void* str = _PyBytesWriter_Alloc(&writer, initial_size);
  ASSERT_NE(str, nullptr);
  EXPECT_EQ(_PyBytesWriter_Prepare(&writer, str, PY_SSIZE_T_MAX), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_MemoryError));
}

TEST_F(BytesWriterExtensionApiTest, ResizeAllocatesNewBuffer) {
  Py_ssize_t initial_size = 10;
  Py_ssize_t new_size = 1000;
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  void* str = _PyBytesWriter_Alloc(&writer, initial_size);
  ASSERT_NE(str, nullptr);
  ASSERT_LT(writer.allocated, new_size);
  void* new_str = _PyBytesWriter_Resize(&writer, str, new_size);
  EXPECT_NE(new_str, nullptr);
  EXPECT_NE(new_str, str);  // buffer has been reallocated
  EXPECT_EQ(writer.allocated, new_size);
  _PyBytesWriter_Dealloc(&writer);
}

TEST_F(BytesWriterExtensionApiTest, ResizeWithOverallocateAllocatesOversized) {
  Py_ssize_t initial_size = 10;
  Py_ssize_t new_size = 1000;
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  void* str = _PyBytesWriter_Alloc(&writer, initial_size);
  ASSERT_NE(str, nullptr);
  ASSERT_LT(writer.allocated, new_size);
  writer.overallocate = true;
  void* new_str = _PyBytesWriter_Resize(&writer, str, new_size);
  EXPECT_NE(new_str, nullptr);
  EXPECT_NE(new_str, str);  // buffer has been reallocated
  EXPECT_GT(writer.allocated, new_size);
  _PyBytesWriter_Dealloc(&writer);
}

TEST_F(BytesWriterExtensionApiTest,
       ResizeWithUseByteArrayAllocatesOversizedPyro) {
  Py_ssize_t initial_size = 10;
  Py_ssize_t new_size = 1000;
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  void* str = _PyBytesWriter_Alloc(&writer, initial_size);
  ASSERT_NE(str, nullptr);
  ASSERT_LT(writer.allocated, new_size);
  writer.use_bytearray = true;
  void* new_str = _PyBytesWriter_Resize(&writer, str, new_size);
  EXPECT_NE(new_str, nullptr);
  EXPECT_NE(new_str, str);  // buffer has been reallocated
  EXPECT_GT(writer.allocated, new_size);
  _PyBytesWriter_Dealloc(&writer);
}

TEST_F(BytesWriterExtensionApiTest, WriteBytesGrowsAndWrites) {
  Py_ssize_t initial_size = 10;
  Py_ssize_t growth = 5;
  _PyBytesWriter writer;
  _PyBytesWriter_Init(&writer);
  void* str = _PyBytesWriter_Alloc(&writer, initial_size);
  ASSERT_NE(str, nullptr);
  ASSERT_EQ(writer.min_size, initial_size);
  str = _PyBytesWriter_WriteBytes(&writer, str, "Hello world!", growth);
  ASSERT_NE(str, nullptr);
  EXPECT_EQ(writer.min_size, initial_size + growth);
  PyObjectPtr result(_PyBytesWriter_Finish(&writer, str));
  _PyBytesWriter_Dealloc(&writer);
  ASSERT_TRUE(PyBytes_CheckExact(result));
  EXPECT_EQ(PyBytes_Size(result), 5);
  EXPECT_STREQ(PyBytes_AsString(result), "Hello");
}

}  // namespace python
