#include "gtest/gtest.h"

#include "capi-handles.h"

#include "runtime.h"

namespace python {

static RawObject initializeExtensionType(PyObject* extension_type) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Initialize Type
  PyObject* pytype_type =
      ApiHandle::newReference(thread, runtime->layoutAt(LayoutId::kType));
  PyObject* pyobj = reinterpret_cast<PyObject*>(extension_type);
  pyobj->ob_type = reinterpret_cast<PyTypeObject*>(pytype_type);
  Type type(&scope, runtime->newType());

  // Compute MRO
  Tuple mro(&scope, runtime->newTuple(0));
  type.setMro(*mro);

  // Initialize instance Layout
  Layout layout_init(&scope, runtime->layoutCreateEmpty(thread));
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  Layout layout(&scope,
                runtime->layoutAddAttribute(thread, layout_init, attr_name, 0));
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  pyobj->reference_ = reinterpret_cast<void*>(type.raw());
  return *type;
}

TEST(CApiHandlesTest, BorrowedApiHandles) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  // Create a new object and a new reference to that object.
  Object obj(&scope, runtime.newTuple(10));
  ApiHandle* new_ref = ApiHandle::newReference(thread, *obj);
  word refcnt = new_ref->refcnt();

  // Create a borrowed reference to the same object.  This should not affect the
  // reference count of the handle.
  ApiHandle* borrowed_ref = ApiHandle::borrowedReference(thread, *obj);
  EXPECT_EQ(borrowed_ref, new_ref);
  EXPECT_EQ(borrowed_ref->refcnt(), refcnt);

  // Create another new reference.  This should increment the reference count
  // of the handle.
  ApiHandle* another_ref = ApiHandle::newReference(thread, *obj);
  EXPECT_EQ(another_ref, new_ref);
  EXPECT_EQ(another_ref->refcnt(), refcnt + 1);
}

TEST(CApiHandlesTest, BuiltinIntObjectReturnsApiHandle) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.apiHandles());

  Object obj(&scope, runtime.newInt(1));
  ASSERT_FALSE(runtime.dictIncludes(dict, obj));

  ApiHandle* handle = ApiHandle::newReference(Thread::currentThread(), *obj);
  EXPECT_NE(handle, nullptr);

  EXPECT_TRUE(runtime.dictIncludes(dict, obj));
}

TEST(CApiHandlesTest, ApiHandleReturnsBuiltinIntObject) {
  Runtime runtime;
  HandleScope scope;

  Object obj(&scope, runtime.newInt(1));
  ApiHandle* handle = ApiHandle::newReference(Thread::currentThread(), *obj);
  Object handle_obj(&scope, handle->asObject());
  ASSERT_TRUE(handle_obj.isInt());

  Int integer(&scope, *handle_obj);
  EXPECT_EQ(integer.asWord(), 1);
}

TEST(CApiHandlesTest, BuiltinObjectReturnsApiHandle) {
  Runtime runtime;
  HandleScope scope;
  Dict dict(&scope, runtime.apiHandles());

  Object obj(&scope, runtime.newList());
  ASSERT_FALSE(runtime.dictIncludes(dict, obj));

  ApiHandle* handle = ApiHandle::newReference(Thread::currentThread(), *obj);
  EXPECT_NE(handle, nullptr);

  EXPECT_TRUE(runtime.dictIncludes(dict, obj));
}

TEST(CApiHandlesTest, BuiltinObjectReturnsSameApiHandle) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Object obj(&scope, runtime.newList());
  ApiHandle* handle = ApiHandle::newReference(thread, *obj);
  ApiHandle* handle2 = ApiHandle::newReference(thread, *obj);
  EXPECT_EQ(handle, handle2);
}

TEST(CApiHandlesTest, ApiHandleReturnsBuiltinObject) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Object obj(&scope, runtime.newList());
  ApiHandle* handle = ApiHandle::newReference(thread, *obj);
  Object handle_obj(&scope, handle->asObject());
  EXPECT_TRUE(handle_obj.isList());
}

TEST(CApiHandlesTest, ExtensionInstanceObjectReturnsPyObject) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyObject extension_type;
  Type type(&scope, initializeExtensionType(&extension_type));

  // Create instance
  Layout layout(&scope, type.instanceLayout());
  Thread* thread = Thread::currentThread();
  Object attr_name(&scope, runtime.symbols()->ExtensionPtr());
  HeapObject instance(&scope, runtime.newInstance(layout));

  PyObject* type_handle = ApiHandle::newReference(thread, *type);
  PyObject pyobj = {nullptr, 1, reinterpret_cast<PyTypeObject*>(type_handle)};
  Object object_ptr(&scope, runtime.newIntFromCPtr(static_cast<void*>(&pyobj)));
  runtime.instanceAtPut(thread, instance, attr_name, object_ptr);

  PyObject* result = ApiHandle::newReference(thread, *instance);
  EXPECT_TRUE(result);
  EXPECT_EQ(result, &pyobj);
}

TEST(CApiHandlesTest, RuntimeInstanceObjectReturnsPyObject) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyObject extension_type;
  Type type(&scope, initializeExtensionType(&extension_type));

  // Initialize instance Layout
  Thread* thread = Thread::currentThread();
  Layout layout(&scope, runtime.layoutCreateEmpty(thread));
  layout.setDescribedType(*type);
  type.setInstanceLayout(*layout);

  // Create instance
  HeapObject instance(&scope, runtime.newInstance(layout));
  PyObject* result = ApiHandle::newReference(thread, *instance);
  ASSERT_NE(result, nullptr);

  Object obj(&scope, ApiHandle::fromPyObject(result)->asObject());
  EXPECT_EQ(*obj, *instance);
}

TEST(CApiHandlesTest, PyObjectReturnsExtensionInstance) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyObject extension_type;
  Type type(&scope, initializeExtensionType(&extension_type));
  PyObject* extension_type_ref =
      ApiHandle::newReference(Thread::currentThread(), *type);

  PyObject pyobj = {nullptr, 1,
                    reinterpret_cast<PyTypeObject*>(extension_type_ref)};
  Object handle_obj(&scope, ApiHandle::fromPyObject(&pyobj)->asObject());
  EXPECT_TRUE(handle_obj.isInstance());
}

TEST(CApiHandlesTest, Cache) {
  Runtime runtime;
  HandleScope scope;

  Thread* thread = Thread::currentThread();

  auto handle1 = ApiHandle::newReference(thread, SmallInt::fromWord(5));
  EXPECT_EQ(handle1->cache(), nullptr);

  Str str(&scope, runtime.newStrFromCStr("this is too long for a RawSmallStr"));
  auto handle2 = ApiHandle::newReference(thread, *str);
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
