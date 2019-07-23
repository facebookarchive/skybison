#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using ObjectExtensionApiTest = ExtensionApi;

using namespace testing;

TEST_F(ObjectExtensionApiTest, PyNoneIdentityIsEqual) {
  // Test Identitiy
  PyObject* none1 = Py_None;
  PyObject* none2 = Py_None;
  EXPECT_EQ(none1, none2);
}

TEST_F(ObjectExtensionApiTest, PyNotImplementedIdentityIsEqual) {
  // Test Identitiy
  PyObject* not_impl1 = Py_NotImplemented;
  PyObject* not_impl2 = Py_NotImplemented;
  EXPECT_EQ(not_impl1, not_impl2);
}

TEST_F(ObjectExtensionApiTest, BytesWithNullReturnsBytes) {
  PyObjectPtr result(PyObject_Bytes(nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_STREQ(PyBytes_AsString(result), "<NULL>");
}

TEST_F(ObjectExtensionApiTest, BytesWithBytesReturnsSameObject) {
  PyObjectPtr bytes(PyBytes_FromString("hello"));
  PyObjectPtr result(PyObject_Bytes(bytes));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, bytes);
}

TEST_F(ObjectExtensionApiTest, BytesWithBadDunderBytesRaisesTypeError) {
  PyRun_SimpleString(R"(
class Foo:
  def __bytes__(self):
    return 1
obj = Foo()
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Bytes(obj), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, BytesWithDunderBytesReturnsBytes) {
  PyRun_SimpleString(R"(
class Foo:
  def __bytes__(self):
    return b'123'
obj = Foo()
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  PyObjectPtr result(PyObject_Bytes(obj));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_STREQ(PyBytes_AsString(result), "123");
}

TEST_F(ObjectExtensionApiTest, BytesWithDunderBytesErrorRaisesValueError) {
  PyRun_SimpleString(R"(
class Foo:
  def __bytes__(self):
    raise ValueError
obj = Foo()
)");
  PyObjectPtr obj(moduleGet("__main__", "obj"));
  ASSERT_EQ(PyObject_Bytes(obj), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(ObjectExtensionApiTest, BytesWithListOfByteReturnsBytes) {
  PyObjectPtr list(PyList_New(2));
  ASSERT_EQ(PyList_SetItem(list, 0, PyLong_FromLong('h')), 0);
  ASSERT_EQ(PyList_SetItem(list, 1, PyLong_FromLong('i')), 0);
  PyObjectPtr result(PyObject_Bytes(list));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_STREQ(PyBytes_AsString(result), "hi");
}

TEST_F(ObjectExtensionApiTest, BytesWithTupleOfByteReturnsBytes) {
  PyObjectPtr tuple(PyTuple_New(2));
  ASSERT_EQ(PyTuple_SetItem(tuple, 0, PyLong_FromLong('h')), 0);
  ASSERT_EQ(PyTuple_SetItem(tuple, 1, PyLong_FromLong('i')), 0);
  PyObjectPtr result(PyObject_Bytes(tuple));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_STREQ(PyBytes_AsString(result), "hi");
}

TEST_F(ObjectExtensionApiTest, BytesWithStringRaisesTypeError) {
  PyObjectPtr str(PyUnicode_FromString("hello"));
  EXPECT_EQ(PyObject_Bytes(str), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, CallableCheckWithNullReturnsZero) {
  EXPECT_EQ(PyCallable_Check(nullptr), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, CallableCheckWithNoneDunderCallReturnsOne) {
  PyRun_SimpleString(R"(
class C:
  __call__ = None
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyCallable_Check(c), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest,
       CallableCheckWithNonCallableDunderCallReturnsOne) {
  PyRun_SimpleString(R"(
class C:
  __call__ = 5
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyCallable_Check(c), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, SetAttrWithInvalidTypeReturnsNegative) {
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(5);
  EXPECT_EQ(PyObject_GenericSetAttr(Py_None, key, value), -1);
}

TEST_F(ObjectExtensionApiTest, SetAttrWithInvalidKeyReturnsNegative) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObject* module = PyModule_Create(&def);
  PyObject* value = PyLong_FromLong(5);
  EXPECT_EQ(PyObject_GenericSetAttr(module, Py_None, value), -1);
}

TEST_F(ObjectExtensionApiTest, SetAttrReturnsZero) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(5);
  EXPECT_EQ(PyObject_GenericSetAttr(module, key, value), 0);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithNoneExistingKeyReturnsNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObject* module = PyModule_Create(&def);

  PyObject* key = PyUnicode_FromString("a_key");
  EXPECT_EQ(PyObject_GenericGetAttr(module, key), nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithInvalidTypeReturnsNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_GenericSetAttr(module, key, value), 0);

  EXPECT_EQ(PyObject_GenericGetAttr(Py_None, key), nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithInvalidKeyReturnsNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_GenericSetAttr(module, key, value), 0);

  EXPECT_EQ(PyObject_GenericGetAttr(module, Py_None), nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrReturnsCorrectValue) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* key = PyUnicode_FromString("a_key");
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_GenericSetAttr(module, key, value), 0);

  PyObject* dict_result = PyObject_GenericGetAttr(module, key);
  ASSERT_NE(dict_result, nullptr);
  EXPECT_EQ(PyLong_AsLong(dict_result), expected_int);
}

TEST_F(ObjectExtensionApiTest, GetAttrStringReturnsCorrectValue) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  const char* key = "a_key";
  int expected_int = 5;
  PyObject* module = PyModule_Create(&def);
  PyObject* value = PyLong_FromLong(expected_int);
  ASSERT_EQ(PyObject_SetAttrString(module, key, value), 0);

  PyObject* dict_result = PyObject_GetAttrString(module, key);
  ASSERT_NE(dict_result, nullptr);
  EXPECT_EQ(PyLong_AsLong(dict_result), expected_int);
}

TEST_F(ObjectExtensionApiTest, HasAttrWithImmediateWithAttributeReturnsTrue) {
  PyObjectPtr num(PyLong_FromLong(6));
  PyObjectPtr name(PyUnicode_FromString("__int__"));
  EXPECT_TRUE(PyObject_HasAttr(num, name));
}

TEST_F(ObjectExtensionApiTest,
       HasAttrStringWithImmediateWithoutAttributeReturnsFalse) {
  PyObjectPtr str(PyUnicode_FromString(""));
  EXPECT_FALSE(PyObject_HasAttrString(str, "foo"));
}

TEST_F(ObjectExtensionApiTest, HasAttrWithNonStringAttrReturnsFalse) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr num(PyLong_FromLong(1));
  EXPECT_FALSE(PyObject_HasAttr(set, num));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, HasAttrWithoutAttrReturnsFalse) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr name(PyUnicode_FromString("foo"));
  EXPECT_FALSE(PyObject_HasAttr(module, name));
}

TEST_F(ObjectExtensionApiTest, HasAttrStringWithoutAttrReturnsFalse) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObjectPtr module(PyModule_Create(&def));
  EXPECT_FALSE(PyObject_HasAttrString(module, "foo"));
}

TEST_F(ObjectExtensionApiTest, HasAttrWithAttrReturnsTrue) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr name(PyUnicode_FromString("foo"));
  PyObjectPtr val(PyLong_FromLong(2));
  ASSERT_EQ(PyObject_GenericSetAttr(module, name, val), 0);
  EXPECT_TRUE(PyObject_HasAttr(module, name));
}

TEST_F(ObjectExtensionApiTest, HasAttrStringWithAttrReturnsTrue) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr name(PyUnicode_FromString("foo"));
  PyObjectPtr val(PyLong_FromLong(2));
  ASSERT_EQ(PyObject_GenericSetAttr(module, name, val), 0);
  EXPECT_TRUE(PyObject_HasAttrString(module, "foo"));
}

TEST_F(ObjectExtensionApiTest, RefCountDecreaseDeallocsHandle) {
  long value = 10;
  PyObject* o = PyLong_FromLong(value);
  Py_DECREF(o);
}

TEST_F(ObjectExtensionApiTest, IncrementDecrementRefCount) {
  PyObject* o = PyLong_FromLong(0);
  long refcnt = Py_REFCNT(o);
  EXPECT_GE(Py_REFCNT(o), 1);
  Py_INCREF(o);
  EXPECT_EQ(Py_REFCNT(o), refcnt + 1);
  Py_DECREF(o);
  EXPECT_EQ(Py_REFCNT(o), refcnt);
}

TEST_F(ObjectExtensionApiTest, IncrementDecrementRefCountWithPyObjectPtr) {
  PyObject* o = PyLong_FromLong(0);
  long refcnt = Py_REFCNT(o);
  {
    Py_INCREF(o);
    EXPECT_EQ(Py_REFCNT(o), refcnt + 1);
    testing::PyObjectPtr h(o);
    static_cast<void>(h);
  }
  EXPECT_EQ(Py_REFCNT(o), refcnt);
  {
    Py_INCREF(o);
    EXPECT_EQ(Py_REFCNT(o), refcnt + 1);
    testing::PyObjectPtr h(o);
    h = nullptr;
    EXPECT_EQ(Py_REFCNT(o), refcnt);
  }
}

TEST_F(ObjectExtensionApiTest, GetAttrIncrementsReferenceCount) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr key(PyUnicode_FromString("test"));
  PyObject* value = PyLong_FromLong(0);
  ASSERT_EQ(PyObject_SetAttr(module, key, value), 0);

  long refcnt = Py_REFCNT(value);
  PyObject* result = PyObject_GenericGetAttr(module, key);
  EXPECT_EQ(Py_REFCNT(result), refcnt + 1);
  Py_DECREF(result);
  result = PyObject_GetAttr(module, key);
  EXPECT_EQ(result, value);
  EXPECT_EQ(Py_REFCNT(result), refcnt + 1);
  Py_DECREF(result);
  Py_DECREF(result);
}

TEST_F(ObjectExtensionApiTest, ReprOnNullReturnsSpecialNullString) {
  PyObjectPtr repr(PyObject_Repr(nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, "<NULL>"));
}

TEST_F(ObjectExtensionApiTest, ReprWithObjectWithBadDunderReprRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __repr__ = None
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  EXPECT_EQ(PyObject_Repr(pyc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, ReprIsCorrectForObjectWithDunderRepr) {
  PyRun_SimpleString(R"(
class C:
  def __repr__(self):
    return "bongo"
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  PyObjectPtr repr(PyObject_Repr(pyc));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, "bongo"));
}

TEST_F(ObjectExtensionApiTest,
       ReprWithRecursiveObjectDoesNotInfinitelyRecurse) {
  PyRun_SimpleString(R"(
a = []
a.append(a)
)");
  PyObjectPtr a(PyObject_GetAttrString(PyImport_AddModule("__main__"), "a"));
  PyObjectPtr repr(PyObject_Repr(a));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(repr, "[[...]]"));
}

TEST_F(ObjectExtensionApiTest, StrOnNullReturnsSpecialNullString) {
  PyObjectPtr str(PyObject_Str(nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(isUnicodeEqualsCStr(str, "<NULL>"));
}

TEST_F(ObjectExtensionApiTest, StrCallsClassDunderReprWhenProvided) {
  PyRun_SimpleString(R"(
class C:
  def __repr__(self):
    return "bongo"
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  PyObjectPtr str(PyObject_Str(pyc));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "bongo"));
}

TEST_F(ObjectExtensionApiTest, StrWithObjectWithBadDunderStrRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __str__ = None
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  EXPECT_EQ(PyObject_Str(pyc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, StrCallsClassDunderStrWhenProvided) {
  PyRun_SimpleString(R"(
class C:
  def __str__(self):
    return "bongo"
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  PyObjectPtr str(PyObject_Str(pyc));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(str, "bongo"));
}

TEST_F(ObjectExtensionApiTest, RichCompareWithNullLhsRaisesSystemError) {
  ASSERT_EQ(PyObject_RichCompare(nullptr, Py_None, 0), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ObjectExtensionApiTest, RichCompareWithNullRhsRaisesSystemError) {
  ASSERT_EQ(PyObject_RichCompare(Py_None, nullptr, 0), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(ObjectExtensionApiTest, RichCompareWithSameType) {
  PyObjectPtr left(PyLong_FromLong(2));
  PyObjectPtr right(PyLong_FromLong(3));
  PyObjectPtr result(PyObject_RichCompare(left, right, Py_LT));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(result, Py_True);
}

TEST_F(ObjectExtensionApiTest, RichCompareNotComparableRaisesTypeError) {
  PyObjectPtr left(PyLong_FromLong(2));
  PyObjectPtr right(PyUnicode_FromString("hello"));
  EXPECT_EQ(PyObject_RichCompare(left, right, Py_LT), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, IsTrueReturnsTrueOnTrue) {
  EXPECT_EQ(PyObject_IsTrue(Py_True), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, IsTrueReturnsFalseOnFalse) {
  EXPECT_EQ(PyObject_IsTrue(Py_False), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, IsTrueReturnsFalseOnNone) {
  EXPECT_EQ(PyObject_IsTrue(Py_None), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest,
       IsTrueWithObjectWithNonCallableDunderBoolRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __bool__ = 4
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  ASSERT_EQ(PyObject_IsTrue(pyc), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest,
       IsTrueWithObjectWithNonCallableDunderLenRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __len__ = 4
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  ASSERT_EQ(PyObject_IsTrue(pyc), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest,
       IsTrueWithObjectWithDunderBoolThatReturnsNonIntRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  def __bool__(self):
    return "bongo"
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  ASSERT_EQ(PyObject_IsTrue(pyc), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest,
       IsTrueWithDunderLenThatReturnsNonIntRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  def __len__(self):
    return "bongo"
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  ASSERT_EQ(PyObject_IsTrue(pyc), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, IsTrueWithEmptyListReturnsFalse) {
  PyObjectPtr empty_list(PyList_New(0));
  ASSERT_EQ(PyObject_IsTrue(empty_list), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, IsTrueWithNonEmptyListReturnsTrue) {
  PyObjectPtr list(PyList_New(0));
  PyList_Append(list, Py_None);

  ASSERT_EQ(PyObject_IsTrue(list), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest,
       IsTrueWithObjectWithDunderLenReturningNegativeOneRaisesValueError) {
  PyRun_SimpleString(R"(
class C:
  def __len__(self):
    return -1
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  ASSERT_EQ(PyObject_IsTrue(pyc), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_ValueError));
}

TEST_F(ObjectExtensionApiTest, ClearWithNullDoesNotRaise) {
  PyObject* null = nullptr;
  Py_CLEAR(null);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(null, nullptr);
}

TEST_F(ObjectExtensionApiTest, ClearWithObjectSetsNull) {
  PyObject* num = PyLong_FromLong(1);
  Py_CLEAR(num);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(num, nullptr);
}

TEST_F(ObjectExtensionApiTest, ClearWithObjectDecrefsObject) {
  PyObject* original = PyLong_FromLong(1);
  PyObject* num = original;
  Py_ssize_t original_count = Py_REFCNT(original);
  Py_CLEAR(num);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(num, nullptr);
  EXPECT_LT(Py_REFCNT(original), original_count);
}

TEST_F(ObjectExtensionApiTest, ASCIIOnNullReturnsSpecialNullString) {
  PyObjectPtr ascii(PyObject_ASCII(nullptr));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(ascii, "<NULL>"));
}

TEST_F(ObjectExtensionApiTest,
       ASCIIWithObjectWithBadDunderReprRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __repr__ = None
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  EXPECT_EQ(PyObject_ASCII(pyc), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, ASCIIcallsDunderRepr) {
  PyRun_SimpleString(R"(
class C:
  def __repr__(self):
    return "bongo"
c = C()
)");
  PyObjectPtr pyc(PyObject_GetAttrString(PyImport_AddModule("__main__"), "c"));
  PyObjectPtr ascii(PyObject_ASCII(pyc));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(isUnicodeEqualsCStr(ascii, "bongo"));
}

TEST_F(ObjectExtensionApiTest, SelfIterIncrementsRefcount) {
  PyObject* o = PyLong_FromLong(0);
  long refcnt = Py_REFCNT(o);
  EXPECT_GE(Py_REFCNT(o), 1);
  PyObject* o2 = PyObject_SelfIter(o);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(Py_REFCNT(o2), refcnt + 1);
}

TEST_F(ObjectExtensionApiTest, NotWithTrueReturnsFalse) {
  EXPECT_EQ(PyObject_Not(Py_True), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, NotWithFalseReturnsTrue) {
  EXPECT_EQ(PyObject_Not(Py_False), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, NotWithNoneReturnsTrue) {
  EXPECT_EQ(PyObject_Not(Py_None), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, NotCallsDunderBool) {
  PyRun_SimpleString(R"(
sideeffect = 0
class C:
  def __bool__(self):
    global sideeffect
    sideeffect = 10
    return False
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyObject_Not(c), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(moduleGet("__main__", "sideeffect"));
  EXPECT_EQ(PyLong_AsLong(sideeffect), 10);
}

TEST_F(ObjectExtensionApiTest,
       NotWithDunderBoolRaisingExceptionRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  def __bool__(self):
    return -10
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyObject_Not(c), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, HashWithUncallableDunderHashRaisesTypeError) {
  PyRun_SimpleString(R"(
class C:
  __hash__ = None
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyObject_Hash(c), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, HashCallsDunderHash) {
  PyRun_SimpleString(R"(
class C:
  def __hash__(self):
    return 7
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyObject_Hash(c), 7);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, HashPropagatesRaisedException) {
  PyRun_SimpleString(R"(
class C:
  def __hash__(self):
    raise IndexError
c = C()
)");
  PyObjectPtr c(moduleGet("__main__", "c"));
  EXPECT_EQ(PyObject_Hash(c), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_IndexError));
}

TEST_F(ObjectExtensionApiTest, HashNotImplementedRaisesTypeError) {
  EXPECT_EQ(PyObject_HashNotImplemented(Py_None), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest,
       RichCompareBoolEqWithLeftEqualsRightReturnsTrue) {
  EXPECT_EQ(PyObject_RichCompareBool(Py_None, Py_None, Py_EQ), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest,
       RichCompareBoolNeWithLeftEqualsRightReturnsFalse) {
  EXPECT_EQ(PyObject_RichCompareBool(Py_None, Py_None, Py_NE), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, RichCompareBoolWithSameTypeReturnsTrue) {
  PyObjectPtr left(PyLong_FromLong(2));
  PyObjectPtr right(PyLong_FromLong(3));
  EXPECT_EQ(PyObject_RichCompareBool(left, right, Py_LT), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, RichCompareBoolNotComparableRaisesTypeError) {
  PyObjectPtr left(PyLong_FromLong(2));
  PyObjectPtr right(PyUnicode_FromString("hello"));
  EXPECT_EQ(PyObject_RichCompareBool(left, right, Py_LT), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, ReprEnterOnceReturnsZero) {
  PyObjectPtr obj(PyLong_FromLong(7));
  EXPECT_EQ(Py_ReprEnter(obj), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, ReprEnterSecondTimeReturnsOne) {
  PyObjectPtr obj(PyLong_FromLong(7));
  ASSERT_EQ(Py_ReprEnter(obj), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(Py_ReprEnter(obj), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, ReprEnterThenLeaveRemovesFromSet) {
  PyObjectPtr obj(PyLong_FromLong(7));
  ASSERT_EQ(Py_ReprEnter(obj), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_EQ(Py_ReprEnter(obj), 1);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  Py_ReprLeave(obj);
  ASSERT_EQ(Py_ReprEnter(obj), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, InitWithNullRaisesNoMemoryError) {
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));
  PyObject_Init(nullptr, reinterpret_cast<PyTypeObject*>(type.get()));
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_MemoryError));
}

TEST_F(ObjectExtensionApiTest, NewReturnsAllocatedObject) {
  struct BarObject {
    PyObject_HEAD int value;
  };
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", sizeof(BarObject), 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  Py_ssize_t refcnt = Py_REFCNT(type.get());
  BarObject* instance =
      PyObject_New(BarObject, reinterpret_cast<PyTypeObject*>(type.get()));
  ASSERT_NE(instance, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(Py_REFCNT(instance), 1);
  EXPECT_EQ(Py_REFCNT(type), refcnt + 1);
  PyObject_Free(instance);
}

TEST_F(ObjectExtensionApiTest, NewVarReturnsAllocatedObject) {
  struct BarObject {
    PyObject_HEAD int value;
  };
  struct BarContainer {
    PyObject_VAR_HEAD BarObject* items[1];
  };
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", sizeof(BarContainer), sizeof(BarObject), Py_TPFLAGS_DEFAULT,
      slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  BarContainer* instance = PyObject_NewVar(
      BarContainer, reinterpret_cast<PyTypeObject*>(type.get()), 5);
  ASSERT_NE(instance, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(Py_REFCNT(instance), 1);
  EXPECT_EQ(Py_SIZE(instance), 5);
  PyObject_Free(instance);
}

TEST_F(ObjectExtensionApiTest, PyEllipsisIdentityIsEqual) {
  // Test Identitiy
  PyObject* ellipsis1 = Py_Ellipsis;
  PyObject* ellipsis2 = Py_Ellipsis;
  EXPECT_EQ(ellipsis1, ellipsis2);
}

}  // namespace python
