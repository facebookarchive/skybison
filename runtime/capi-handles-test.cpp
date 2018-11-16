#include "gtest/gtest.h"

#include "capi-handles.h"

#include "runtime.h"

namespace python {

static Object* initializeExtensionType(PyTypeObject* extension_type) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Initialize Type
  PyObject* pytype_type =
      ApiHandle::fromObject(runtime->layoutAt(LayoutId::kType));
  PyObject* pyobj = reinterpret_cast<PyObject*>(extension_type);
  pyobj->ob_type = reinterpret_cast<PyTypeObject*>(pytype_type);
  Handle<Type> type_class(&scope, runtime->newClass());

  // Compute MRO
  Handle<ObjectArray> mro(&scope, runtime->newObjectArray(0));
  type_class->setMro(*mro);

  // Initialize instance Layout
  Handle<Layout> layout_init(&scope, runtime->layoutCreateEmpty(thread));
  Handle<Object> attr_name(&scope, runtime->symbols()->ExtensionPtr());
  Handle<Layout> layout(
      &scope, runtime->layoutAddAttribute(thread, layout_init, attr_name, 0));
  layout->setDescribedClass(*type_class);
  type_class->setInstanceLayout(*layout);

  pyobj->reference_ = *type_class;
  return *type_class;
}

TEST(CApiHandlesTest, BorrowedApiHandles) {
  Runtime runtime;
  HandleScope scope;

  // Create a borrowed handle
  Handle<Object> obj(&scope, runtime.newInt(15));
  ApiHandle* borrowed_handle = ApiHandle::fromBorrowedObject(*obj);
  EXPECT_TRUE(borrowed_handle->isBorrowed());

  // Check the setting and unsetting of the borrowed bit
  borrowed_handle->clearBorrowed();
  EXPECT_FALSE(borrowed_handle->isBorrowed());
  borrowed_handle->setBorrowed();
  EXPECT_TRUE(borrowed_handle->isBorrowed());

  // Create a normal handle
  Handle<Object> obj2(&scope, runtime.newInt(20));
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
  Handle<Dict> dict(&scope, runtime.apiHandles());

  Handle<Object> obj(&scope, runtime.newInt(1));
  ASSERT_FALSE(runtime.dictIncludes(dict, obj));

  ApiHandle* handle = ApiHandle::fromObject(*obj);
  EXPECT_NE(handle, nullptr);

  EXPECT_TRUE(runtime.dictIncludes(dict, obj));
}

TEST(CApiHandlesTest, ApiHandleReturnsBuiltinIntObject) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> obj(&scope, runtime.newInt(1));
  ApiHandle* handle = ApiHandle::fromObject(*obj);
  Handle<Object> handle_obj(&scope, handle->asObject());
  ASSERT_TRUE(handle_obj->isInt());

  Handle<Int> integer(&scope, *handle_obj);
  EXPECT_EQ(integer->asWord(), 1);
}

TEST(CApiHandlesTest, BuiltinObjectReturnsApiHandle) {
  Runtime runtime;
  HandleScope scope;
  Handle<Dict> dict(&scope, runtime.apiHandles());

  Handle<Object> obj(&scope, runtime.newList());
  ASSERT_FALSE(runtime.dictIncludes(dict, obj));

  ApiHandle* handle = ApiHandle::fromObject(*obj);
  EXPECT_NE(handle, nullptr);

  EXPECT_TRUE(runtime.dictIncludes(dict, obj));
}

TEST(CApiHandlesTest, BuiltinObjectReturnsSameApiHandle) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> obj(&scope, runtime.newList());
  ApiHandle* handle = ApiHandle::fromObject(*obj);
  ApiHandle* handle2 = ApiHandle::fromObject(*obj);
  EXPECT_EQ(handle, handle2);
}

TEST(CApiHandlesTest, ApiHandleReturnsBuiltinObject) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> obj(&scope, runtime.newList());
  ApiHandle* handle = ApiHandle::fromObject(*obj);
  Handle<Object> handle_obj(&scope, handle->asObject());
  EXPECT_TRUE(handle_obj->isList());
}

TEST(CApiHandlesTest, PyTypeObjectReturnsExtensionType) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyObject_HEAD_INIT(nullptr)};
  Handle<Type> type_class(&scope, initializeExtensionType(&extension_type));

  Handle<Object> handle_obj(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(&extension_type))
          ->asObject());
  EXPECT_TRUE(handle_obj->isType());
  EXPECT_EQ(*type_class, *handle_obj);
}

TEST(CApiHandlesTest, ExtensionInstanceObjectReturnsPyObject) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyObject_HEAD_INIT(nullptr)};
  Handle<Type> type_class(&scope, initializeExtensionType(&extension_type));

  // Create instance
  Handle<Layout> layout(&scope, type_class->instanceLayout());
  Thread* thread = Thread::currentThread();
  Handle<Object> attr_name(&scope, runtime.symbols()->ExtensionPtr());
  Handle<HeapObject> instance(&scope, runtime.newInstance(layout));

  PyObject* type_handle = ApiHandle::fromObject(*type_class);
  PyObject pyobj = {nullptr, 1, reinterpret_cast<PyTypeObject*>(type_handle)};
  Handle<Object> object_ptr(&scope,
                            runtime.newIntFromCPtr(static_cast<void*>(&pyobj)));
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
  Handle<Type> type_class(&scope, initializeExtensionType(&extension_type));

  // Initialize instance Layout
  Thread* thread = Thread::currentThread();
  Handle<Layout> layout(&scope, runtime.layoutCreateEmpty(thread));
  layout->setDescribedClass(*type_class);
  type_class->setInstanceLayout(*layout);

  // Create instance
  Handle<HeapObject> instance(&scope, runtime.newInstance(layout));
  PyObject* result = ApiHandle::fromObject(*instance);
  ASSERT_NE(result, nullptr);

  Handle<Object> obj(&scope, ApiHandle::fromPyObject(result)->asObject());
  EXPECT_EQ(*obj, *instance);
}

TEST(CApiHandlesTest, PyObjectReturnsExtensionInstance) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyObject_HEAD_INIT(nullptr)};
  Handle<Type> type_class(&scope, initializeExtensionType(&extension_type));

  PyObject pyobj = {nullptr, 1,
                    reinterpret_cast<PyTypeObject*>(&extension_type)};
  Handle<Object> handle_obj(&scope,
                            ApiHandle::fromPyObject(&pyobj)->asObject());
  EXPECT_TRUE(handle_obj->isInstance());
}

}  // namespace python
