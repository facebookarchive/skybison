#include "gtest/gtest.h"

#include "Python.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

TEST(DictObject, GetItemFromNonDictReturnsNull) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> non_dict(&scope, runtime.newInt(10));

  // Pass a non dict
  PyObject* result = PyDict_GetItem(ApiHandle::fromObject(*non_dict),
                                    ApiHandle::fromObject(*non_dict));
  EXPECT_EQ(result, nullptr);
}

TEST(DictObject, GetItemNonExistingKeyReturnsNull) {
  Runtime runtime;
  HandleScope scope;

  Handle<Dict> dict(&scope, runtime.newDict());
  Handle<Object> nonkey(&scope, SmallInt::fromWord(30));

  // Pass a non existing key
  PyObject* result = PyDict_GetItem(ApiHandle::fromObject(*dict),
                                    ApiHandle::fromObject(*nonkey));
  EXPECT_EQ(result, nullptr);
}

TEST(DictObject, GetItemReturnsValue) {
  Runtime runtime;
  HandleScope scope;

  Handle<Dict> dict(&scope, runtime.newDict());
  Handle<Object> key(&scope, runtime.newInt(10));
  Handle<Object> value(&scope, runtime.newInt(20));
  runtime.dictAtPut(dict, key, value);

  // Pass existing key
  PyObject* result =
      PyDict_GetItem(ApiHandle::fromObject(*dict), ApiHandle::fromObject(*key));
  ASSERT_NE(result, nullptr);

  // Check result value
  Handle<Object> result_obj(&scope,
                            ApiHandle::fromPyObject(result)->asObject());
  ASSERT_TRUE(result_obj->isInt());
  EXPECT_EQ(Int::cast(*result_obj)->asWord(), 20);
}

TEST(DictObject, GetItemReturnsBorrowedReference) {
  Runtime runtime;
  HandleScope scope;

  Handle<Dict> dict(&scope, runtime.newDict());
  Handle<Object> key(&scope, runtime.newInt(10));
  Handle<Object> value(&scope, runtime.newInt(20));
  runtime.dictAtPut(dict, key, value);

  // Pass existing key
  PyObject* result =
      PyDict_GetItem(ApiHandle::fromObject(*dict), ApiHandle::fromObject(*key));
  ASSERT_NE(result, nullptr);

  // Check that key is a normal handle
  ApiHandle* key_handle = ApiHandle::fromObject(*key);
  EXPECT_FALSE(key_handle->isBorrowed());

  // Check that value returned PyDict_GetItem is a borrowed reference
  ApiHandle* value_handle = ApiHandle::fromObject(*value);
  EXPECT_TRUE(value_handle->isBorrowed());
}

}  // namespace python
