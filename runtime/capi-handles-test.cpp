#include "gtest/gtest.h"

#include "capi-handles.h"

#include "runtime.h"

namespace python {

static RawObject initializeExtensionType(PyTypeObject* extension_type) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Initialize Type
  PyObject* pytype_type =
      ApiHandle::fromObject(runtime->layoutAt(LayoutId::kType));
  PyObject* pyobj = reinterpret_cast<PyObject*>(extension_type);
  pyobj->ob_type = reinterpret_cast<PyTypeObject*>(pytype_type);
  Type type(&scope, runtime->newType());

  // Compute MRO
  Tuple mro(&scope, runtime->newTuple(0));
  type->setMro(*mro);

  // Initialize instance Layout
  Layout layout_init(&scope, runtime->layoutCreateEmpty(thread));
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  Layout layout(&scope,
                runtime->layoutAddAttribute(thread, layout_init, attr_name, 0));
  layout->setDescribedType(*type);
  type->setInstanceLayout(*layout);

  pyobj->reference_ = reinterpret_cast<void*>(type->raw());
  return *type;
}

TEST(CApiHandlesTest, BorrowedApiHandles) {
  Runtime runtime;
  HandleScope scope;

  // Create a borrowed handle
  Object obj(&scope, runtime.newInt(15));
  ApiHandle* borrowed_handle = ApiHandle::fromBorrowedObject(*obj);
  EXPECT_TRUE(borrowed_handle->isBorrowed());

  // Check the setting and unsetting of the borrowed bit
  borrowed_handle->clearBorrowed();
  EXPECT_FALSE(borrowed_handle->isBorrowed());
  borrowed_handle->setBorrowed();
  EXPECT_TRUE(borrowed_handle->isBorrowed());

  // Create a normal handle
  Object obj2(&scope, runtime.newInt(20));
  ApiHandle* int_handle1 = ApiHandle::fromObject(*obj2);
  EXPECT_FALSE(int_handle1->isBorrowed());

  // A handle once set as borrowed will persist through all its pointers
  ApiHandle* int_handle2 = ApiHandle::fromBorrowedObject(*obj2);
  EXPECT_TRUE(int_handle2->isBorrowed());
  EXPECT_TRUE(int_handle1->isBorrowed());
}

TEST(CApiHandlesTest, BuiltinIntObjectReturnsApiHandle) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.apiHandles());

  Object obj(&scope, runtime.newInt(1));
  ASSERT_FALSE(runtime.dictIncludes(dict, obj));

  ApiHandle* handle = ApiHandle::fromObject(*obj);
  EXPECT_NE(handle, nullptr);

  EXPECT_TRUE(runtime.dictIncludes(dict, obj));
}

TEST(CApiHandlesTest, ApiHandleReturnsBuiltinIntObject) {
  Runtime runtime;
  HandleScope scope;

  Object obj(&scope, runtime.newInt(1));
  ApiHandle* handle = ApiHandle::fromObject(*obj);
  Object handle_obj(&scope, handle->asObject());
  ASSERT_TRUE(handle_obj->isInt());

  Int integer(&scope, *handle_obj);
  EXPECT_EQ(integer->asWord(), 1);
}

TEST(CApiHandlesTest, BuiltinObjectReturnsApiHandle) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.apiHandles());

  Object obj(&scope, runtime.newList());
  ASSERT_FALSE(runtime.dictIncludes(dict, obj));

  ApiHandle* handle = ApiHandle::fromObject(*obj);
  EXPECT_NE(handle, nullptr);

  EXPECT_TRUE(runtime.dictIncludes(dict, obj));
}

TEST(CApiHandlesTest, BuiltinObjectReturnsSameApiHandle) {
  Runtime runtime;
  HandleScope scope;

  Object obj(&scope, runtime.newList());
  ApiHandle* handle = ApiHandle::fromObject(*obj);
  ApiHandle* handle2 = ApiHandle::fromObject(*obj);
  EXPECT_EQ(handle, handle2);
}

TEST(CApiHandlesTest, ApiHandleReturnsBuiltinObject) {
  Runtime runtime;
  HandleScope scope;

  Object obj(&scope, runtime.newList());
  ApiHandle* handle = ApiHandle::fromObject(*obj);
  Object handle_obj(&scope, handle->asObject());
  EXPECT_TRUE(handle_obj->isList());
}

TEST(CApiHandlesTest, PyTypeObjectReturnsExtensionType) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyObject_HEAD_INIT(nullptr)};
  Type type(&scope, initializeExtensionType(&extension_type));

  Object handle_obj(&scope, ApiHandle::fromPyObject(
                                reinterpret_cast<PyObject*>(&extension_type))
                                ->asObject());
  EXPECT_TRUE(handle_obj->isType());
  EXPECT_EQ(*type, *handle_obj);
}

TEST(CApiHandlesTest, ExtensionInstanceObjectReturnsPyObject) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyObject_HEAD_INIT(nullptr)};
  Type type(&scope, initializeExtensionType(&extension_type));

  // Create instance
  Layout layout(&scope, type->instanceLayout());
  Thread* thread = Thread::currentThread();
  Object attr_name(&scope, runtime.symbols()->ExtensionPtr());
  HeapObject instance(&scope, runtime.newInstance(layout));

  PyObject* type_handle = ApiHandle::fromObject(*type);
  PyObject pyobj = {nullptr, 1, reinterpret_cast<PyTypeObject*>(type_handle)};
  Object object_ptr(&scope, runtime.newIntFromCPtr(static_cast<void*>(&pyobj)));
  runtime.instanceAtPut(thread, instance, attr_name, object_ptr);

  PyObject* result = ApiHandle::fromObject(*instance);
  EXPECT_TRUE(result);
  EXPECT_EQ(result, &pyobj);
}

TEST(CApiHandlesTest, RuntimeInstanceObjectReturnsPyObject) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyObject_HEAD_INIT(nullptr)};
  Type type(&scope, initializeExtensionType(&extension_type));

  // Initialize instance Layout
  Thread* thread = Thread::currentThread();
  Layout layout(&scope, runtime.layoutCreateEmpty(thread));
  layout->setDescribedType(*type);
  type->setInstanceLayout(*layout);

  // Create instance
  HeapObject instance(&scope, runtime.newInstance(layout));
  PyObject* result = ApiHandle::fromObject(*instance);
  ASSERT_NE(result, nullptr);

  Object obj(&scope, ApiHandle::fromPyObject(result)->asObject());
  EXPECT_EQ(*obj, *instance);
}

TEST(CApiHandlesTest, PyObjectReturnsExtensionInstance) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyObject_HEAD_INIT(nullptr)};
  Type type(&scope, initializeExtensionType(&extension_type));

  PyObject pyobj = {nullptr, 1,
                    reinterpret_cast<PyTypeObject*>(&extension_type)};
  Object handle_obj(&scope, ApiHandle::fromPyObject(&pyobj)->asObject());
  EXPECT_TRUE(handle_obj->isInstance());
}

TEST(CApiHandlesTest, Cache) {
  Runtime runtime;
  HandleScope scope;

  auto handle1 = ApiHandle::fromObject(SmallInt::fromWord(5));
  EXPECT_EQ(handle1->cache(), nullptr);

  Str str(&scope, runtime.newStrFromCStr("this is too long for a RawSmallStr"));
  auto handle2 = ApiHandle::fromObject(*str);
  EXPECT_EQ(handle2->cache(), nullptr);

  void* buffer1 = std::malloc(16);
  handle1->setCache(buffer1);
  EXPECT_EQ(handle1->cache(), buffer1);
  EXPECT_EQ(handle2->cache(), nullptr);

  void* buffer2 = std::malloc(16);
  handle2->setCache(buffer2);
  EXPECT_EQ(handle2->cache(), buffer2);
  EXPECT_EQ(handle1->cache(), buffer1);

  handle1->setCache(buffer2);
  handle2->setCache(buffer1);
  EXPECT_EQ(handle1->cache(), buffer2);
  EXPECT_EQ(handle2->cache(), buffer1);

  Object key(&scope, handle1->asObject());
  handle1->dispose();
  Dict caches(&scope, runtime.apiCaches());
  EXPECT_EQ(runtime.dictAt(caches, key), Error::object());
  EXPECT_EQ(handle2->cache(), buffer1);
}

}  // namespace python
