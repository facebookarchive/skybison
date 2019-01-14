#include "gtest/gtest.h"

#include "Python.h"
#include "capi-fixture.h"
#include "capi-testing.h"

namespace python {

using namespace testing;

using DictExtensionApiTest = ExtensionApi;

TEST_F(DictExtensionApiTest, ClearFreeListReturnsZeroPyro) {
  EXPECT_EQ(PyDict_ClearFreeList(), 0);
}

TEST_F(DictExtensionApiTest, GetItemFromNonDictionaryReturnsNull) {
  // Pass a non dictionary
  PyObject* result = PyDict_GetItem(Py_None, Py_None);
  EXPECT_EQ(result, nullptr);
}

TEST_F(DictExtensionApiTest, GetItemNonExistingKeyReturnsNull) {
  PyObject* dict = PyDict_New();
  PyObject* nonkey = PyLong_FromLong(10);

  // Pass a non existing key
  PyObject* result = PyDict_GetItem(dict, nonkey);
  EXPECT_EQ(result, nullptr);
}

TEST_F(DictExtensionApiTest, GetItemReturnsBorrowedValue) {
  PyObject* dict = PyDict_New();
  PyObject* key = PyLong_FromLong(10);
  PyObject* value = PyLong_FromLong(0);

  // Insert the value into the dictionary
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  // Record the reference count of the value
  long refcnt = Py_REFCNT(value);

  // Get a new reference to the value from the dictionary
  PyObject* value2 = PyDict_GetItem(dict, key);

  // The new reference should be equal to the original reference
  EXPECT_EQ(value2, value);

  // The reference count should not be affected
  EXPECT_EQ(Py_REFCNT(value), refcnt);

  Py_DECREF(value);
  Py_DECREF(key);
  Py_DECREF(dict);
}

TEST_F(DictExtensionApiTest, GetItemWithDictSubclassReturnsValue) {
  PyRun_SimpleString(R"(
class Foo(dict): pass
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  PyObjectPtr key(PyLong_FromLong(1));
  PyObjectPtr val(PyLong_FromLong(2));
  ASSERT_EQ(PyDict_SetItem(obj, key, val), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyObjectPtr result(PyDict_GetItem(obj, key));
  EXPECT_EQ(result, val);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, GetItemStringReturnsValue) {
  PyObjectPtr dict(PyDict_New());
  const char* key_cstr = "key";
  PyObjectPtr key(PyUnicode_FromString(key_cstr));
  PyObjectPtr value(PyLong_FromLong(0));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObject* item = PyDict_GetItemString(dict, key_cstr);
  EXPECT_EQ(item, value);
}

TEST_F(DictExtensionApiTest, SetItemWithNonDictRaisesSystemError) {
  PyObjectPtr set(PySet_New(nullptr));
  PyObjectPtr key(PyLong_FromLong(0));
  PyObjectPtr val(PyLong_FromLong(0));

  ASSERT_EQ(PyDict_SetItem(set, key, val), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, SetItemWithNewDictReturnsZero) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(0));
  PyObjectPtr val(PyLong_FromLong(0));

  EXPECT_EQ(PyDict_SetItem(dict, key, val), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, SetItemWithNewDictSubclassReturnsZero) {
  PyRun_SimpleString(R"(
class Foo(dict): pass
obj = Foo()
  )");

  PyObjectPtr obj(moduleGet("__main__", "obj"));
  PyObjectPtr key(PyLong_FromLong(0));
  PyObjectPtr val(PyLong_FromLong(0));

  EXPECT_EQ(PyDict_SetItem(obj, key, val), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, SizeWithNonDictReturnsNegative) {
  PyObject* list = PyList_New(0);
  EXPECT_EQ(PyDict_Size(list), -1);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));

  Py_DECREF(list);
}

TEST_F(DictExtensionApiTest, SizeWithEmptyDictReturnsZero) {
  PyObject* dict = PyDict_New();
  EXPECT_EQ(PyDict_Size(dict), 0);
  Py_DECREF(dict);
}

TEST_F(DictExtensionApiTest, SizeWithNonEmptyDict) {
  PyObject* dict = PyDict_New();
  PyObject* key1 = PyLong_FromLong(1);
  PyObject* key2 = PyLong_FromLong(2);
  PyObject* value1 = PyLong_FromLong(0);
  PyObject* value2 = PyLong_FromLong(0);
  PyObject* value3 = PyLong_FromLong(0);

  // Dict starts out empty
  EXPECT_EQ(PyDict_Size(dict), 0);

  // Inserting items for two different keys
  ASSERT_EQ(PyDict_SetItem(dict, key1, value1), 0);
  ASSERT_EQ(PyDict_SetItem(dict, key2, value2), 0);
  EXPECT_EQ(PyDict_Size(dict), 2);

  // Replace value for existing key
  ASSERT_EQ(PyDict_SetItem(dict, key1, value3), 0);
  EXPECT_EQ(PyDict_Size(dict), 2);

  Py_DECREF(dict);
  Py_DECREF(key1);
  Py_DECREF(key2);
  Py_DECREF(value1);
  Py_DECREF(value2);
  Py_DECREF(value3);
}

TEST_F(DictExtensionApiTest, ContainsWithKeyInDictReturnsOne) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);
  EXPECT_EQ(PyDict_Contains(dict, key), 1);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, ContainsWithKeyNotInDictReturnsZero) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr key2(PyLong_FromLong(666));
  EXPECT_EQ(PyDict_Contains(dict, key2), 0);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, ItemsWithNonDictRaisesSystemError) {
  ASSERT_EQ(PyDict_Items(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, ItemsWithDictReturnsList) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObjectPtr result(PyDict_Items(dict));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 1);

  PyObjectPtr kv(PyList_GetItem(result, 0));
  ASSERT_TRUE(PyTuple_CheckExact(kv));
  ASSERT_EQ(PyTuple_Size(kv), 2);
  EXPECT_EQ(PyTuple_GetItem(kv, 0), key);
  EXPECT_EQ(PyTuple_GetItem(kv, 1), value);
}

TEST_F(DictExtensionApiTest, KeysWithNonDictRaisesSystemError) {
  EXPECT_EQ(PyDict_Keys(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, KeysWithDictReturnsList) {
  PyObjectPtr dict(PyDict_New());

  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObjectPtr result(PyDict_Keys(dict));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 1);
  EXPECT_EQ(PyList_GetItem(result, 0), key);
}

TEST_F(DictExtensionApiTest, ValuesWithNonDictReturnsNull) {
  EXPECT_EQ(PyDict_Values(Py_None), nullptr);
  ASSERT_NE(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyErr_ExceptionMatches(PyExc_SystemError));
}

TEST_F(DictExtensionApiTest, ValuesWithEmptyDictReturnsEmptyList) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr result(PyDict_Values(dict));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 0);
}

TEST_F(DictExtensionApiTest, ValuesWithNonEmptyDictReturnsNonEmptyList) {
  PyObjectPtr dict(PyDict_New());

  PyObjectPtr key(PyLong_FromLong(10));
  PyObjectPtr value(PyLong_FromLong(11));
  ASSERT_EQ(PyDict_SetItem(dict, key, value), 0);

  PyObjectPtr result(PyDict_Values(dict));
  ASSERT_NE(result, nullptr);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  ASSERT_TRUE(PyList_CheckExact(result));
  EXPECT_EQ(PyList_Size(result), 1);
  EXPECT_EQ(PyList_GetItem(result, 0), value);
}

TEST_F(DictExtensionApiTest, ClearWithNonDictDoesNotThrow) {
  PyDict_Clear(Py_None);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
}

TEST_F(DictExtensionApiTest, ClearRemovesAllItems) {
  PyObjectPtr dict(PyDict_New());
  PyObjectPtr one(PyLong_FromLong(1));
  PyObjectPtr two(PyLong_FromLong(1));
  PyDict_SetItem(dict, one, two);
  ASSERT_EQ(PyErr_Occurred(), nullptr);
  PyObjectPtr three(PyLong_FromLong(1));
  PyObjectPtr four(PyLong_FromLong(1));
  PyDict_SetItem(dict, three, four);
  ASSERT_EQ(PyErr_Occurred(), nullptr);

  PyDict_Clear(dict);
  EXPECT_EQ(PyErr_Occurred(), nullptr);
  EXPECT_EQ(PyDict_Size(dict), 0);
}

}  // namespace python
