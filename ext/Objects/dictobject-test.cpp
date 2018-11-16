#include "gtest/gtest.h"

#include "Python.h"
#include "runtime/runtime.h"
#include "runtime/test-utils.h"

namespace python {

TEST(ExtensionTest, PyDict_GetItem) {
  Runtime runtime;
  HandleScope scope;
  PyObject* value = nullptr;

  Handle<Dictionary> dict(&scope, runtime.newDictionary());
  Handle<Object> key(&scope, runtime.newInteger(10));
  Handle<Object> value_obj(&scope, runtime.newInteger(20));
  runtime.dictionaryAtPut(dict, key, value_obj);

  // Pass a non dictionary
  value = PyDict_GetItem(
      runtime.asApiHandle(*key)->asPyObject(),
      runtime.asApiHandle(*key)->asPyObject());
  EXPECT_EQ(value, nullptr);

  // Pass a non existing key
  Handle<Object> nonkey(&scope, SmallInteger::fromWord(30));
  value = PyDict_GetItem(
      runtime.asApiHandle(*dict)->asPyObject(),
      runtime.asApiHandle(*nonkey)->asPyObject());
  EXPECT_EQ(value, nullptr);

  // Pass a valid key
  value = PyDict_GetItem(
      runtime.asApiHandle(*dict)->asPyObject(),
      runtime.asApiHandle(*key)->asPyObject());
  EXPECT_NE(value, nullptr);
  Handle<Object> result(&scope, runtime.asObject(value));
  EXPECT_TRUE(result->isSmallInteger());
  EXPECT_EQ(SmallInteger::cast(*result)->value(), 20);

  // Check that key is a normal handle
  ApiHandle* key_handle = runtime.asApiHandle(*key);
  EXPECT_FALSE(key_handle->isBorrowed());

  // Check that value is not one or zero as the borrowed bit is set
  EXPECT_NE(Py_REFCNT(value), 0);
  EXPECT_NE(Py_REFCNT(value), 1);

  // Check that value returned PyDict_GetItem is a borrowed reference
  ApiHandle* value_handle = runtime.asApiHandle(*value_obj);
  EXPECT_TRUE(value_handle->isBorrowed());
}

} // namespace python
