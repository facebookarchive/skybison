#include "gtest/gtest.h"

#include "Python.h"
#include "runtime/runtime.h"
#include "runtime/test-utils.h"

namespace python {

TEST(DictObject, GetItemFromNonDictionaryReturnsNull) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> non_dict(&scope, runtime.newInteger(10));

  // Pass a non dictionary
  PyObject* result =
      PyDict_GetItem(runtime.asApiHandle(*non_dict)->asPyObject(),
                     runtime.asApiHandle(*non_dict)->asPyObject());
  EXPECT_EQ(result, nullptr);
}

TEST(DictObject, GetItemNonExistingKeyReturnsNull) {
  Runtime runtime;
  HandleScope scope;

  Handle<Dictionary> dict(&scope, runtime.newDictionary());
  Handle<Object> nonkey(&scope, SmallInteger::fromWord(30));

  // Pass a non existing key
  PyObject* result = PyDict_GetItem(runtime.asApiHandle(*dict)->asPyObject(),
                                    runtime.asApiHandle(*nonkey)->asPyObject());
  EXPECT_EQ(result, nullptr);
}

TEST(DictObject, GetItemReturnsValue) {
  Runtime runtime;
  HandleScope scope;

  Handle<Dictionary> dict(&scope, runtime.newDictionary());
  Handle<Object> key(&scope, runtime.newInteger(10));
  Handle<Object> value(&scope, runtime.newInteger(20));
  runtime.dictionaryAtPut(dict, key, value);

  // Pass existing key
  PyObject* result = PyDict_GetItem(runtime.asApiHandle(*dict)->asPyObject(),
                                    runtime.asApiHandle(*key)->asPyObject());
  ASSERT_NE(result, nullptr);

  // Check result value
  Handle<Object> result_obj(&scope,
                            ApiHandle::fromPyObject(result)->asObject());
  ASSERT_TRUE(result_obj->isInteger());
  EXPECT_EQ(Integer::cast(*result_obj)->asWord(), 20);
}

TEST(DictObject, GetItemReturnsBorrowedReference) {
  Runtime runtime;
  HandleScope scope;

  Handle<Dictionary> dict(&scope, runtime.newDictionary());
  Handle<Object> key(&scope, runtime.newInteger(10));
  Handle<Object> value(&scope, runtime.newInteger(20));
  runtime.dictionaryAtPut(dict, key, value);

  // Pass existing key
  PyObject* result = PyDict_GetItem(runtime.asApiHandle(*dict)->asPyObject(),
                                    runtime.asApiHandle(*key)->asPyObject());
  ASSERT_NE(result, nullptr);

  // Check that key is a normal handle
  ApiHandle* key_handle = runtime.asApiHandle(*key);
  EXPECT_FALSE(key_handle->isBorrowed());

  // Check that value returned PyDict_GetItem is a borrowed reference
  ApiHandle* value_handle = runtime.asApiHandle(*value);
  EXPECT_TRUE(value_handle->isBorrowed());
}

}  // namespace python
