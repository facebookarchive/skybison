#include "gtest/gtest.h"

#include "capi-handles.h"

#include "runtime.h"

namespace python {

TEST(RuntimeApiHandlesTest, creatingApiHandles) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dictionary> dict(&scope, runtime.apiHandles());

  Handle<Object> obj(&scope, runtime.newInteger(15));
  Object* value = runtime.dictionaryAt(dict, obj);
  EXPECT_EQ(value->isError(), true);

  ApiHandle* integer_handle = ApiHandle::fromObject(*obj);
  value = runtime.dictionaryAt(dict, obj);
  EXPECT_EQ(value->isError(), false);

  ApiHandle* integer_handle2 = ApiHandle::fromObject(*obj);
  EXPECT_EQ(integer_handle, integer_handle2);

  Handle<Integer> integer(&scope, integer_handle->asObject());
  EXPECT_EQ(integer->asWord(), 15);
}

TEST(RuntimeApiHandlesTest, borrowedApiHandles) {
  Runtime runtime;
  HandleScope scope;

  // Create a borrowed handle
  Handle<Object> obj(&scope, runtime.newInteger(15));
  ApiHandle* borrowed_handle = ApiHandle::fromBorrowedObject(*obj);
  EXPECT_TRUE(borrowed_handle->isBorrowed());

  // Check the setting and unsetting of the borrowed bit
  borrowed_handle->clearBorrowed();
  EXPECT_FALSE(borrowed_handle->isBorrowed());
  borrowed_handle->setBorrowed();
  EXPECT_TRUE(borrowed_handle->isBorrowed());

  // Create a normal handle
  Handle<Object> obj2(&scope, runtime.newInteger(20));
  ApiHandle* integer_handle = ApiHandle::fromObject(*obj2);
  EXPECT_FALSE(integer_handle->isBorrowed());

  // Check that a handle that has already been instantiated, just returns
  // the instantiated handle, even if requested as borrowed
  ApiHandle* integer_handle2 = ApiHandle::fromBorrowedObject(*obj);
  EXPECT_TRUE(integer_handle2->isBorrowed());
  ApiHandle* integer_handle3 = ApiHandle::fromBorrowedObject(*obj2);
  EXPECT_FALSE(integer_handle3->isBorrowed());
}

}  // namespace python
