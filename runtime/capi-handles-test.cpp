#include "gtest/gtest.h"

#include "capi-handles.h"

#include "runtime.h"

namespace python {

static Object* initializeExtensionType(PyTypeObject* extension_type) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Initialize Type
  extension_type->ob_base.ob_base.ob_type =
      runtime->builtinTypeHandles(ExtensionTypes::kType)->asPyTypeObject();
  Handle<Type> type_class(&scope, runtime->newClass());
  type_class->setExtensionType(
      runtime->newIntFromCPointer(static_cast<void*>(extension_type)));

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

// TODO(T32685074): Handle PyTypeObject as any other ApiHandle
TEST(CApiHandlesDeathTest, BuiltinTypeObjectReturnsPyTypeObject) {
  Runtime runtime;
  HandleScope scope;

  Handle<Object> list_class(&scope, runtime.typeAt(LayoutId::kList));
  ASSERT_TRUE(list_class->isType());
  EXPECT_DEATH(ApiHandle::fromObject(*list_class),
               "unimplemented: Type object doesn't contain an ApiTypeHandle");
}

TEST(CApiHandlesDeathTest, PyTypeObjectReturnsBuiltinType) {
  Runtime runtime;

  PyTypeObject* pytypeobject =
      runtime.builtinTypeHandles(ExtensionTypes::kLong)->asPyTypeObject();
  ASSERT_TRUE(pytypeobject);
  EXPECT_DEATH(ApiHandle::fromPyTypeObject(pytypeobject)->asObject(),
               "failed: Uninitialized PyTypeObject. Run PyType_Ready on it");
}

// TODO(T32685074): Handle PyTypeObject as any other ApiHandle
TEST(CApiHandlesTest, ExtensionTypeObjectReturnsPyTypeObject) {
  Runtime runtime;
  HandleScope scope;

  PyTypeObject extension_type{PyVarObject_HEAD_INIT(nullptr, 0)};
  extension_type.ob_base.ob_base.ob_type =
      runtime.builtinTypeHandles(ExtensionTypes::kType)->asPyTypeObject();
  Handle<Type> type_class(&scope, runtime.newClass());
  type_class->setExtensionType(
      runtime.newIntFromCPointer(static_cast<void*>(&extension_type)));

  PyTypeObject* type = ApiHandle::fromObject(*type_class)->asPyTypeObject();
  EXPECT_EQ(type, &extension_type);
}

TEST(CApiHandlesTest, PyTypeObjectReturnsExtensionType) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyVarObject_HEAD_INIT(nullptr, 0)};
  extension_type.ob_base.ob_base.ob_type =
      runtime.builtinTypeHandles(ExtensionTypes::kType)->asPyTypeObject();
  Handle<Type> type_class(&scope, runtime.newClass());

  // Save to runtime dict
  Handle<Dict> extensions_dict(&scope, runtime.extensionTypes());
  Handle<Object> type_class_obj(&scope, *type_class);
  Handle<Object> type_class_id(
      &scope, runtime.newIntFromCPointer(static_cast<void*>(&extension_type)));
  runtime.dictAtPut(extensions_dict, type_class_id, type_class_obj);

  Handle<Object> handle_obj(
      &scope, ApiHandle::fromPyTypeObject(&extension_type)->asObject());
  EXPECT_TRUE(handle_obj->isType());
  EXPECT_EQ(*type_class, *handle_obj);
}

TEST(CApiHandlesTest, ExtensionInstanceObjectReturnsPyObject) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyVarObject_HEAD_INIT(nullptr, 0)};
  Handle<Type> type_class(&scope, initializeExtensionType(&extension_type));

  // Create instance
  Handle<Layout> layout(&scope, type_class->instanceLayout());
  Thread* thread = Thread::currentThread();
  Handle<Object> attr_name(&scope, runtime.symbols()->ExtensionPtr());
  Handle<HeapObject> instance(&scope, runtime.newInstance(layout));
  PyObject pyobj = {nullptr, 1, &extension_type};
  Handle<Object> object_ptr(
      &scope, runtime.newIntFromCPointer(static_cast<void*>(&pyobj)));
  runtime.instanceAtPut(thread, instance, attr_name, object_ptr);

  PyObject* result = ApiHandle::fromObject(*instance)->asPyObject();
  EXPECT_TRUE(result);
  EXPECT_EQ(result, &pyobj);
}

TEST(CApiHandlesDeathTest, RuntimeInstanceObjectReturnsPyObject) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyVarObject_HEAD_INIT(nullptr, 0)};
  Handle<Type> type_class(&scope, initializeExtensionType(&extension_type));

  // Initialize instance Layout
  Thread* thread = Thread::currentThread();
  Handle<Layout> layout(&scope, runtime.layoutCreateEmpty(thread));
  layout->setDescribedClass(*type_class);
  type_class->setInstanceLayout(*layout);

  // Create instance
  Handle<HeapObject> instance(&scope, runtime.newInstance(layout));
  EXPECT_DEATH(ApiHandle::fromObject(*instance)->asPyObject(),
               "unimplemented: Calling fromObject on a non-extension "
               "instance. Can't materialize PyObject");
}

TEST(CApiHandlesTest, PyObjectReturnsExtensionInstance) {
  Runtime runtime;
  HandleScope scope;

  // Create type
  PyTypeObject extension_type{PyVarObject_HEAD_INIT(nullptr, 0)};
  Handle<Type> type_class(&scope, initializeExtensionType(&extension_type));

  // Save to runtime extension dict
  Handle<Dict> extensions_dict(&scope, runtime.extensionTypes());
  Handle<Object> type_class_obj(&scope, *type_class);
  Handle<Object> type_class_id(
      &scope, runtime.newIntFromCPointer(static_cast<void*>(&extension_type)));
  runtime.dictAtPut(extensions_dict, type_class_id, type_class_obj);

  PyObject pyobj = {nullptr, 1, &extension_type};
  Handle<Object> handle_obj(&scope,
                            ApiHandle::fromPyObject(&pyobj)->asObject());
  EXPECT_TRUE(handle_obj->isInstance());
}

}  // namespace python
