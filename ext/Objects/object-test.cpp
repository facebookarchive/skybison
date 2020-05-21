#include "Python.h"
#include "gtest/gtest.h"

#include "capi-fixture.h"
#include "capi-testing.h"

namespace py {
namespace testing {

using ObjectExtensionApiTest = ExtensionApi;

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
  PyObjectPtr obj(mainModuleGet("obj"));
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
  PyObjectPtr obj(mainModuleGet("obj"));
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
  PyObjectPtr obj(mainModuleGet("obj"));
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
  PyObjectPtr c(mainModuleGet("c"));
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
  PyObjectPtr c(mainModuleGet("c"));
  EXPECT_EQ(PyCallable_Check(c), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(ObjectExtensionApiTest, DelAttrStringRemovesAttribute) {
  PyRun_SimpleString(R"(
class C:
  pass
obj = C()
obj.a = 42
obj.b = 13
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  ASSERT_TRUE(PyObject_HasAttrString(obj, "a"));
  ASSERT_TRUE(PyObject_HasAttrString(obj, "b"));
  EXPECT_EQ(PyObject_DelAttrString(obj, "a"), 0);
  EXPECT_FALSE(PyObject_HasAttrString(obj, "a"));
  EXPECT_TRUE(PyObject_HasAttrString(obj, "b"));
}

TEST_F(ObjectExtensionApiTest, DelAttrRemovesAttribute) {
  PyRun_SimpleString(R"(
class C:
  pass
obj = C()
obj.a = 42
obj.b = 13
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  ASSERT_TRUE(PyObject_HasAttrString(obj, "a"));
  ASSERT_TRUE(PyObject_HasAttrString(obj, "b"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  EXPECT_EQ(PyObject_DelAttr(obj, name), 0);
  EXPECT_FALSE(PyObject_HasAttrString(obj, "a"));
  EXPECT_TRUE(PyObject_HasAttrString(obj, "b"));
}

TEST_F(ObjectExtensionApiTest, DelAttrRaisesAttributeError) {
  Py_INCREF(Py_None);
  PyObjectPtr obj(Py_None);
  EXPECT_EQ(PyObject_DelAttrString(obj, "does_not_exist"), -1);
  EXPECT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(ObjectExtensionApiTest, LookupAttrWithNonStrNameRaisesTypeError) {
  Py_INCREF(Py_None);
  PyObjectPtr obj(Py_None);
  Py_INCREF(Py_None);
  PyObjectPtr name(Py_None);
  PyObject* result = name.get();  // some non-NULL value
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, LookupAttrWithExtantAttrReturnsOne) {
  PyRun_SimpleString(R"(
class C:
  pass
obj = C()
obj.a = 42
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  ASSERT_TRUE(PyObject_HasAttr(obj, name));
  PyObject* result = nullptr;
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 42);
  Py_DECREF(result);
}

TEST_F(ObjectExtensionApiTest, LookupAttrWithNonexistentAttrReturnsZero) {
  Py_INCREF(Py_None);
  PyObjectPtr obj(Py_None);
  PyObjectPtr name(PyUnicode_FromString("a"));
  ASSERT_FALSE(PyObject_HasAttr(obj, name));
  PyObject* result = name.get();  // some non-NULL value
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, nullptr);
}

TEST_F(ObjectExtensionApiTest,
       LookupAttrWithSuccessfulDunderGetAttributeReturnsOne) {
  PyRun_SimpleString(R"(
class C:
  def __getattribute__(self, key):
    return 42
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  PyObject* result = nullptr;
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 42);
  Py_DECREF(result);
}

TEST_F(ObjectExtensionApiTest,
       LookupAttrWithRaisingDunderGetAttributeReturnsNegativeOne) {
  PyRun_SimpleString(R"(
class C:
  def __getattribute__(self, key):
    raise TypeError("foo")
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  PyObject* result = name.get();  // some non-NULL value
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest,
       LookupAttrWithAttributeErrorRaisingDunderGetAttributeReturnsZero) {
  PyRun_SimpleString(R"(
class C:
  def __getattribute__(self, key):
    raise AttributeError("foo")
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  PyObject* result = name.get();  // some non-NULL value
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, nullptr);
}

TEST_F(ObjectExtensionApiTest,
       LookupAttrWithSuccessfulDunderGetAttrReturnsOne) {
  PyRun_SimpleString(R"(
class C:
  def __getattr__(self, key):
    return 42
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  PyObject* result = nullptr;
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 42);
  Py_DECREF(result);
}

TEST_F(ObjectExtensionApiTest,
       LookupAttrWithRaisingDunderGetAttrReturnsNegativeOne) {
  PyRun_SimpleString(R"(
class C:
  def __getattr__(self, key):
    raise TypeError("foo")
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  PyObject* result = name.get();  // some non-NULL value
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest,
       LookupAttrWithAttributeErrorRaisingDunderGetAttrReturnsZero) {
  PyRun_SimpleString(R"(
class C:
  def __getattr__(self, key):
    raise AttributeError("foo")
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  PyObject* result = name.get();  // some non-NULL value
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, nullptr);
}

TEST_F(
    ObjectExtensionApiTest,
    LookupAttrWithDunderGetAttributeAndDunderGetAttrCallsDunderGetAttribute) {
  PyRun_SimpleString(R"(
class C:
  def __getattr__(self, key):
    return 5
  def __getattribute__(self, key):
    return 10
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  PyObject* result = nullptr;
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 10);
  Py_DECREF(result);
}

TEST_F(
    ObjectExtensionApiTest,
    LookupAttrWithRaisingDunderGetAttributeAndDunderGetAttrCallsDunderGetAttr) {
  PyRun_SimpleString(R"(
class C:
  def __getattr__(self, key):
    return 5
  def __getattribute__(self, key):
    raise AttributeError("foo")
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  PyObject* result = nullptr;
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_NE(result, nullptr);
  EXPECT_EQ(PyLong_AsLong(result), 5);
  Py_DECREF(result);
}

TEST_F(ObjectExtensionApiTest,
       LookupAttrWithRaisingDescrAttrReturnsNegativeOne) {
  PyRun_SimpleString(R"(
class Desc:
  def __get__(self, instance, owner):
    raise TypeError("foo")

class C:
  a = Desc()

obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  PyObject* result = name.get();  // some non-NULL value
  EXPECT_EQ(_PyObject_LookupAttr(obj, name, &result), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_EQ(result, nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, PySizeReturnsLvalue) {
  PyType_Slot slots[] = {
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", sizeof(PyObject) + 10, 5, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  ASSERT_TRUE(PyType_CheckExact(type));

  PyObjectPtr result(
      PyType_GenericAlloc(reinterpret_cast<PyTypeObject*>(type.get()), 5));
  EXPECT_EQ(Py_SIZE(result.get()), 5);

  Py_SIZE(result.get()) = 4;
  EXPECT_EQ(Py_SIZE(result.get()), 4);
}

TEST_F(ObjectExtensionApiTest, SetAttrWithInvalidTypeReturnsNegative) {
  PyObjectPtr key(PyUnicode_FromString("a_key"));
  PyObjectPtr value(PyLong_FromLong(5));
  EXPECT_EQ(PyObject_SetAttr(Py_None, key, value), -1);
}

TEST_F(ObjectExtensionApiTest, SetAttrWithInvalidKeyReturnsNegative) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr value(PyLong_FromLong(5));
  EXPECT_EQ(PyObject_SetAttr(module, Py_None, value), -1);
}

TEST_F(ObjectExtensionApiTest, SetAttrReturnsZero) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr key(PyUnicode_FromString("a_key"));
  PyObjectPtr value(PyLong_FromLong(5));
  EXPECT_EQ(PyObject_SetAttr(module, key, value), 0);
}

TEST_F(ObjectExtensionApiTest, SetAttrStringWithNullRemovesAttribute) {
  PyRun_SimpleString(R"(
class C:
  pass
obj = C()
obj.a = 42
obj.b = 13
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  ASSERT_TRUE(PyObject_HasAttrString(obj, "a"));
  ASSERT_TRUE(PyObject_HasAttrString(obj, "b"));
  EXPECT_EQ(PyObject_SetAttrString(obj, "a", nullptr), 0);
  EXPECT_FALSE(PyObject_HasAttrString(obj, "a"));
  EXPECT_TRUE(PyObject_HasAttrString(obj, "b"));
}

TEST_F(ObjectExtensionApiTest, SetAttrWithNullRemovesAttribute) {
  PyRun_SimpleString(R"(
class C:
  pass
obj = C()
obj.a = 42
obj.b = 13
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  ASSERT_TRUE(PyObject_HasAttrString(obj, "a"));
  ASSERT_TRUE(PyObject_HasAttrString(obj, "b"));
  PyObjectPtr name(PyUnicode_FromString("a"));
  EXPECT_EQ(PyObject_SetAttr(obj, name, nullptr), 0);
  EXPECT_FALSE(PyObject_HasAttrString(obj, "a"));
  EXPECT_TRUE(PyObject_HasAttrString(obj, "b"));
}

TEST_F(ObjectExtensionApiTest, GetAttrWithNoneExistingKeyReturnsNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObjectPtr module(PyModule_Create(&def));

  PyObjectPtr key(PyUnicode_FromString("a_key"));
  EXPECT_EQ(PyObject_GetAttr(module, key), nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithInvalidTypeReturnsNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  int expected_int = 5;
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr key(PyUnicode_FromString("a_key"));
  PyObjectPtr value(PyLong_FromLong(expected_int));
  ASSERT_EQ(PyObject_SetAttr(module, key, value), 0);

  EXPECT_EQ(PyObject_GetAttr(Py_None, key), nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrWithInvalidKeyReturnsNull) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  int expected_int = 5;
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr key(PyUnicode_FromString("a_key"));
  PyObjectPtr value(PyLong_FromLong(expected_int));
  ASSERT_EQ(PyObject_SetAttr(module, key, value), 0);

  EXPECT_EQ(PyObject_GetAttr(module, Py_None), nullptr);
}

TEST_F(ObjectExtensionApiTest, GetAttrReturnsCorrectValue) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  int expected_int = 5;
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr key(PyUnicode_FromString("a_key"));
  PyObjectPtr value(PyLong_FromLong(expected_int));
  ASSERT_EQ(PyObject_SetAttr(module, key, value), 0);

  PyObjectPtr dict_result(PyObject_GetAttr(module, key));
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
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr value(PyLong_FromLong(expected_int));
  ASSERT_EQ(PyObject_SetAttrString(module, key, value), 0);

  PyObjectPtr dict_result(PyObject_GetAttrString(module, key));
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
  ASSERT_EQ(PyObject_SetAttr(module, name, val), 0);
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
  ASSERT_EQ(PyObject_SetAttr(module, name, val), 0);
  EXPECT_TRUE(PyObject_HasAttrString(module, "foo"));
}

TEST_F(ObjectExtensionApiTest, PrintWithNullObjPrintsNil) {
  CaptureStdStreams streams;
  int result = PyObject_Print(nullptr, stdout, 0);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(streams.out(), "<nil>");
}

TEST_F(ObjectExtensionApiTest, PrintWithZeroFlagsCallsDunderRepr) {
  ASSERT_EQ(PyRun_SimpleString(R"(
class C:
  def __repr__(self):
    return "foo"
  def __str__(self):
    return "bar"
obj = C()
  )"),
            0);
  CaptureStdStreams streams;
  PyObjectPtr obj(mainModuleGet("obj"));
  int result = PyObject_Print(obj, stdout, 0);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(streams.out(), "foo");
}

TEST_F(ObjectExtensionApiTest, PrintWithRawFlagsCallsDunderStr) {
  ASSERT_EQ(PyRun_SimpleString(R"(
class C:
  def __repr__(self):
    return "foo"
  def __str__(self):
    return "bar"
obj = C()
  )"),
            0);
  CaptureStdStreams streams;
  PyObjectPtr obj(mainModuleGet("obj"));
  int result = PyObject_Print(obj, stdout, Py_PRINT_RAW);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(streams.out(), "bar");
}

TEST_F(ObjectExtensionApiTest, PrintReplacesBackslashes) {
  ASSERT_EQ(PyRun_SimpleString(R"(
class C:
  def __repr__(self):
    return r"foo\bar"
obj = C()
  )"),
            0);
  CaptureStdStreams streams;
  PyObjectPtr obj(mainModuleGet("obj"));
  int result = PyObject_Print(obj, stdout, 0);
  EXPECT_EQ(result, 0);
  EXPECT_EQ(streams.out(), "foo\\bar");
}

TEST_F(ObjectExtensionApiTest, RefCountDecreaseDeallocsHandle) {
  long value = 10;
  PyObject* o = PyLong_FromLong(value);
  Py_DECREF(o);
}

TEST_F(ObjectExtensionApiTest, IncrementDecrementRefCount) {
  PyObject* o = PyTuple_New(1);
  long refcnt = Py_REFCNT(o);
  EXPECT_GE(Py_REFCNT(o), 1);
  Py_INCREF(o);
  EXPECT_EQ(Py_REFCNT(o), refcnt + 1);
  Py_DECREF(o);
  EXPECT_EQ(Py_REFCNT(o), refcnt);
  Py_DECREF(o);
}

TEST_F(ObjectExtensionApiTest, IncrementDecrementRefCountWithPyObjectPtr) {
  PyObject* o = PyTuple_New(1);
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
  Py_DECREF(o);
}

TEST_F(ObjectExtensionApiTest, CallFinalizerFromDeallocWithNonZeroRefcntDies) {
  PyObject* obj = Py_None;
  Py_INCREF(obj);  // definitely has a non-zero refcount
  EXPECT_DEATH(PyObject_CallFinalizerFromDealloc(obj),
               "PyObject_CallFinalizerFromDealloc called on object with a "
               "non-zero refcount");
}

TEST_F(ObjectExtensionApiTest,
       CallFinalizerFromDeallocWithoutTpFinalizeFlagDoesNotCallTpFinalize) {
  static bool dealloc_called;
  static bool finalizer_called;
  dealloc_called = false;
  finalizer_called = false;
  destructor dealloc_func = [](PyObject* self) {
    dealloc_called = true;
    if (PyObject_CallFinalizerFromDealloc(self) < 0) return;
    PyTypeObject* type = Py_TYPE(self);
    PyObject_Del(self);
    Py_DECREF(type);
  };
  destructor finalizer_func = [](PyObject*) { finalizer_called = true; };
  PyType_Slot slots[] = {
      {Py_tp_dealloc, reinterpret_cast<void*>(dealloc_func)},
      {Py_tp_finalize, reinterpret_cast<void*>(finalizer_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  allocfunc func = reinterpret_cast<allocfunc>(
      PyType_GetSlot(type.asTypeObject(), Py_tp_alloc));
  ASSERT_NE(func, nullptr);
  PyObject* obj = (*func)(type.asTypeObject(), 0);
  ASSERT_NE(obj, nullptr);
  EXPECT_GE(Py_REFCNT(obj), 1);
  Py_DECREF(obj);  // Drop the reference to it
  // Trigger a GC. PyObject_CallFinalizerFromDealloc is called during GC in
  // Pyro and immmediately in the decref in CPython.
  collectGarbage();
  EXPECT_TRUE(dealloc_called);
  EXPECT_FALSE(finalizer_called);
}

TEST_F(ObjectExtensionApiTest,
       CallFinalizerFromDeallocWithTpFinalizeFlagCallsTpFinalize) {
  static bool dealloc_called;
  static bool finalizer_called;
  dealloc_called = false;
  finalizer_called = false;
  destructor dealloc_func = [](PyObject* self) {
    dealloc_called = true;
    if (PyObject_CallFinalizerFromDealloc(self) < 0) return;
    PyTypeObject* type = Py_TYPE(self);
    PyObject_Del(self);
    Py_DECREF(type);
  };
  destructor finalizer_func = [](PyObject*) { finalizer_called = true; };
  PyType_Slot slots[] = {
      {Py_tp_dealloc, reinterpret_cast<void*>(dealloc_func)},
      {Py_tp_finalize, reinterpret_cast<void*>(finalizer_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_FINALIZE, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  allocfunc func = reinterpret_cast<allocfunc>(
      PyType_GetSlot(type.asTypeObject(), Py_tp_alloc));
  ASSERT_NE(func, nullptr);
  PyObject* obj = (*func)(type.asTypeObject(), 0);
  ASSERT_NE(obj, nullptr);
  EXPECT_GE(Py_REFCNT(obj), 1);
  Py_DECREF(obj);  // Drop the reference to it
  // Trigger a GC. PyObject_CallFinalizerFromDealloc is called during GC in
  // Pyro and immmediately in the decref in CPython.
  collectGarbage();
  EXPECT_TRUE(dealloc_called);
  EXPECT_TRUE(finalizer_called);
}

TEST_F(
    ObjectExtensionApiTest,
    CallFinalizerFromDeallocWithTpFinalizeResurrectingObjectDoesNotGCObject) {
  static bool dealloc_called;
  static bool finalizer_called;
  dealloc_called = false;
  finalizer_called = false;
  destructor dealloc_func = [](PyObject* self) {
    dealloc_called = true;
    if (PyObject_CallFinalizerFromDealloc(self) < 0) return;
    PyTypeObject* type = Py_TYPE(self);
    PyObject_Del(self);
    Py_DECREF(type);
  };
  destructor finalizer_func = [](PyObject* self) {
    finalizer_called = true;
    Py_INCREF(self);
  };
  PyType_Slot slots[] = {
      {Py_tp_dealloc, reinterpret_cast<void*>(dealloc_func)},
      {Py_tp_finalize, reinterpret_cast<void*>(finalizer_func)},
      {0, nullptr},
  };
  static PyType_Spec spec;
  spec = {
      "foo.Bar", 0, 0, Py_TPFLAGS_DEFAULT | Py_TPFLAGS_HAVE_FINALIZE, slots,
  };
  PyObjectPtr type(PyType_FromSpec(&spec));
  ASSERT_NE(type, nullptr);
  allocfunc func = reinterpret_cast<allocfunc>(
      PyType_GetSlot(type.asTypeObject(), Py_tp_alloc));
  ASSERT_NE(func, nullptr);
  PyObject* obj = (*func)(type.asTypeObject(), 0);
  ASSERT_NE(obj, nullptr);
  EXPECT_GE(Py_REFCNT(obj), 1);
  Py_DECREF(obj);  // Drop the reference to it
  // Trigger a GC. PyObject_CallFinalizerFromDealloc is called during GC in
  // Pyro and immmediately in the decref in CPython.
  collectGarbage();
  EXPECT_TRUE(dealloc_called);
  EXPECT_TRUE(finalizer_called);
  EXPECT_GE(Py_REFCNT(obj), 1);
}

TEST_F(ObjectExtensionApiTest, GenericGetAttrFindsCorrectlySetValue) {
  ASSERT_EQ(PyRun_SimpleString(R"(
class C: pass
i = C()
)"),
            0);

  PyObjectPtr i(mainModuleGet("i"));
  ASSERT_NE(i, nullptr);
  PyObjectPtr key(PyUnicode_FromString("key"));
  PyObjectPtr value(PyUnicode_FromString("value"));
  EXPECT_EQ(PyObject_GenericSetAttr(i, key, value), 0);
  PyObjectPtr get_val(PyObject_GenericGetAttr(i, key));
  EXPECT_TRUE(isUnicodeEqualsCStr(get_val, "value"));
}

TEST_F(ObjectExtensionApiTest, GenericSetAttrWithSealedTypeReturnsNegOne) {
  ASSERT_EQ(PyRun_SimpleString(R"(
i = 3
)"),
            0);

  PyObjectPtr i(mainModuleGet("i"));
  ASSERT_NE(i, nullptr);
  PyObjectPtr key(PyUnicode_FromString("key"));
  PyObjectPtr value(PyUnicode_FromString("value"));
  EXPECT_EQ(PyObject_GenericSetAttr(i, key, value), -1);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_AttributeError));
}

TEST_F(ObjectExtensionApiTest, GetAttrIncrementsReferenceCount) {
  static PyModuleDef def;
  def = {
      PyModuleDef_HEAD_INIT,
      "test",
  };
  PyObjectPtr module(PyModule_Create(&def));
  PyObjectPtr key(PyUnicode_FromString("test"));
  PyObject* value = PyTuple_New(1);
  ASSERT_EQ(PyObject_SetAttr(module, key, value), 0);

  long refcnt = Py_REFCNT(value);
  PyObject* result = PyObject_GetAttr(module, key);
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
  PyObject* original = PyTuple_New(1);
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
  PyObject* o = PyTuple_New(1);
  long refcnt = Py_REFCNT(o);
  EXPECT_GE(Py_REFCNT(o), 1);
  PyObject* o2 = PyObject_SelfIter(o);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(Py_REFCNT(o2), refcnt + 1);
  Py_DECREF(o);
  Py_DECREF(o);
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
  PyObjectPtr c(mainModuleGet("c"));
  EXPECT_EQ(PyObject_Not(c), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr sideeffect(mainModuleGet("sideeffect"));
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
  PyObjectPtr c(mainModuleGet("c"));
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
  PyObjectPtr c(mainModuleGet("c"));
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
  PyObjectPtr c(mainModuleGet("c"));
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
  PyObjectPtr c(mainModuleGet("c"));
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
    PyObject_HEAD
    int value;
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
  PyObjectPtr instance(reinterpret_cast<PyObject*>(
      PyObject_New(BarObject, type.asTypeObject())));
  ASSERT_NE(instance, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  // TODO(T53456038): Switch back to EXPECT_EQ, once initial refcount is fixed
  EXPECT_GE(Py_REFCNT(instance), 1);
  EXPECT_EQ(Py_REFCNT(type), refcnt + 1);
}

TEST_F(ObjectExtensionApiTest, NewVarReturnsAllocatedObject) {
  struct BarObject {
    PyObject_HEAD
    int value;
  };
  struct BarContainer {
    PyObject_VAR_HEAD
    BarObject* items[1];
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
  PyObjectPtr instance(reinterpret_cast<PyObject*>(
      PyObject_NewVar(BarContainer, type.asTypeObject(), 5)));
  ASSERT_NE(instance, nullptr);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  // TODO(T53456038): Switch back to EXPECT_EQ, once initial refcount is fixed
  EXPECT_GE(Py_REFCNT(instance), 1);
  EXPECT_EQ(Py_SIZE(instance.get()), 5);
}

TEST_F(ObjectExtensionApiTest, PyEllipsisIdentityIsEqual) {
  // Test Identitiy
  PyObject* ellipsis1 = Py_Ellipsis;
  PyObject* ellipsis2 = Py_Ellipsis;
  EXPECT_EQ(ellipsis1, ellipsis2);
}

TEST_F(ObjectExtensionApiTest, DirWithoutExecutionFrameReturnsNull) {
  EXPECT_EQ(PyObject_Dir(nullptr), nullptr);
}

TEST_F(ObjectExtensionApiTest, DirReturnsLocals) {
  PyRun_SimpleString(R"(
class C: pass
)");
  PyObjectPtr c_type(mainModuleGet("C"));
  binaryfunc meth = [](PyObject*, PyObject*) { return PyObject_Dir(nullptr); };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, c_type, nullptr));
  ASSERT_NE(func, nullptr);
  PyObject_SetAttrString(c_type, "foo", func);

  PyRun_SimpleString(R"(
foo = 123
c = C()
obj = c.foo()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  ASSERT_EQ(PyList_Check(obj), 1);
  PyObjectPtr foo(PyUnicode_FromString("foo"));
  EXPECT_EQ(PySequence_Contains(obj, foo), 1);
}

TEST_F(ObjectExtensionApiTest, DirOnInstanceReturnsListOfAttributes) {
  PyRun_SimpleString(R"(
class C:
  def __init__(self):
    self.foo = 123
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr result(PyObject_Dir(obj));
  ASSERT_EQ(PyList_Check(result), 1);
  PyObjectPtr foo(PyUnicode_FromString("foo"));
  EXPECT_EQ(PySequence_Contains(result, foo), 1);
}

TEST_F(ObjectExtensionApiTest, DirOnInstanceWithDunderDirRaisingReturnsNull) {
  PyRun_SimpleString(R"(
class C:
  def __init__(self):
    self.foo = 123
  def __dir__(self):
      raise TypeError("no dir on this type")
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr result(PyObject_Dir(obj));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest,
       DirOnInstanceWithNonIterableDunderDirReturnsNull) {
  PyRun_SimpleString(R"(
class C:
  def __init__(self):
    self.foo = 123
  def __dir__(self):
      return 123
obj = C()
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr result(PyObject_Dir(obj));
  ASSERT_EQ(result, nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_TypeError));
}

TEST_F(ObjectExtensionApiTest, DirOnInstanceIgnoresInstanceDictionary) {
  PyRun_SimpleString(R"(
class C:
  def __init__(self):
    self.foo = 123

def new_dir(self):
    return ("bar")

obj = C()
obj.__dir__ = new_dir.__get__(obj, C)
)");
  PyObjectPtr obj(mainModuleGet("obj"));
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr result(PyObject_Dir(obj));
  ASSERT_EQ(PyList_Check(result), 1);
  PyObjectPtr foo(PyUnicode_FromString("foo"));
  EXPECT_EQ(PySequence_Contains(result, foo), 1);
}

TEST_F(ObjectExtensionApiTest, PyReturnNoneReturnsNone) {
  PyObjectPtr module(PyModule_New("mod"));
  binaryfunc meth = [](PyObject*, PyObject*) { Py_RETURN_NONE; };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, nullptr, module));
  PyObjectPtr result(_PyObject_CallNoArg(func));
  EXPECT_EQ(result, Py_None);
}

TEST_F(ObjectExtensionApiTest, PyReturnNotImplementedReturnsNotImplemented) {
  PyObjectPtr module(PyModule_New("mod"));
  binaryfunc meth = [](PyObject*, PyObject*) { Py_RETURN_NOTIMPLEMENTED; };
  static PyMethodDef foo_func = {"foo", meth, METH_NOARGS};
  PyObjectPtr func(PyCFunction_NewEx(&foo_func, nullptr, module));
  PyObjectPtr result(_PyObject_CallNoArg(func));
  EXPECT_EQ(result, Py_NotImplemented);
}

}  // namespace testing
}  // namespace py
