#include "capi-handles.h"

#include "runtime.h"

namespace python {

// TODO(T32685074): Handle PyTypeObject as any other ApiHandle
ApiTypeHandle* ApiTypeHandle::newTypeHandle(const char* name,
                                            PyTypeObject* metatype) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  PyTypeObject* pytype = static_cast<PyTypeObject*>(TrackedAllocation::calloc(
      runtime->trackedAllocations(), 1, sizeof(PyTypeObject)));
  pytype->ob_base.ob_base.ob_type = metatype;
  pytype->ob_base.ob_base.ob_refcnt = 1;
  pytype->tp_name = name;
  pytype->tp_flags = ApiTypeHandle::kFlagsBuiltin;
  return ApiTypeHandle::fromPyTypeObject(pytype);
}

ApiHandle* ApiHandle::fromObject(Object* obj) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> key(&scope, obj);
  Handle<Dict> dict(&scope, runtime->apiHandles());
  Object* value = runtime->dictAt(dict, key);

  // Fast path: All initialized builtin objects
  if (!value->isError()) {
    return static_cast<ApiHandle*>(Int::cast(value)->asCPointer());
  }

  // TODO(T32685074): Handle PyTypeObject as any other ApiHandle
  // Get the PyTypeObject pointer from the Type layout
  if (obj->isType()) {
    return ApiHandle::fromPyObject(
        ApiTypeHandle::fromObject(obj)->asPyObject());
  }

  // Get the PyObject pointer from the instance layout
  if (obj->isInstance()) {
    return ApiHandle::fromInstance(obj);
  }

  // Initialize an ApiHandle for a builtin object
  ApiHandle* handle = new ApiHandle(obj, 1);
  Handle<Object> object(
      &scope, runtime->newIntFromCPointer(static_cast<void*>(handle)));
  runtime->dictAtPut(dict, key, object);
  return handle;
}

ApiHandle* ApiHandle::fromBorrowedObject(Object* obj) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> key(&scope, obj);
  Handle<Dict> dict(&scope, runtime->apiHandles());
  Object* value = runtime->dictAt(dict, key);

  // Fast path: All initialized builtin objects
  if (!value->isError()) {
    return static_cast<ApiHandle*>(Int::cast(value)->asCPointer());
  }

  // TODO(T32685074): Handle PyTypeObject as any other ApiHandle
  // Get the PyTypeObject pointer from the Type layout
  if (obj->isType()) {
    return ApiHandle::fromPyObject(
        ApiTypeHandle::fromObject(obj)->asPyObject());
  }

  // Get the PyObject pointer from the instance layout
  if (obj->isInstance()) {
    return ApiHandle::fromInstance(obj);
  }

  // Initialize a Borrowed ApiHandle for a builtin object
  ApiHandle* handle = new ApiHandle(obj, kBorrowedBit);
  Handle<Object> object(
      &scope, runtime->newIntFromCPointer(static_cast<void*>(handle)));
  runtime->dictAtPut(dict, key, object);
  return handle;
}

ApiHandle* ApiHandle::fromInstance(Object* obj) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  DCHECK(obj->isInstance(), "not an instance");
  Handle<HeapObject> instance(&scope, obj);
  Handle<Object> attr_name(&scope, runtime->symbols()->ExtensionPtr());
  Handle<Object> object_ptr(&scope,
                            runtime->instanceAt(thread, instance, attr_name));

  // TODO(T32474341): Handle creation of ApiHandle from runtime instance
  if (object_ptr->isError()) {
    UNIMPLEMENTED(
        "Calling fromObject on a non-extension instance. Can't "
        "materialize PyObject");
  }

  return static_cast<ApiHandle*>(Int::cast(*object_ptr)->asCPointer());
}

Object* ApiHandle::asInstance(Object* obj) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  DCHECK(obj->isType(), "not a Type object");
  Handle<Type> klass(&scope, obj);
  Handle<Layout> layout(&scope, klass->instanceLayout());
  Handle<HeapObject> instance(&scope, runtime->newInstance(layout));
  Handle<Object> object_ptr(
      &scope, runtime->newIntFromCPointer(static_cast<void*>(this)));
  Handle<Object> attr_name(&scope, runtime->symbols()->ExtensionPtr());
  runtime->instanceAtPut(thread, instance, attr_name, object_ptr);

  return *instance;
}

Object* ApiHandle::asObject() {
  // Fast path: All builtin objects except Types
  // TODO(T32474474): Handle the special case of Int values
  if (reference_ != nullptr) {
    return static_cast<Object*>(reference_);
  }

  // Get the Type object from the runtime dict
  DCHECK(type(), "ApiHandles must contain a type pointer");
  if (isType()) {
    return ApiTypeHandle::fromPyObject(asPyObject())->asObject();
  }

  // Create a runtime instance to hold the PyObject pointer
  DCHECK(!type()->isBuiltin(), "Builtins should never be extension instances");
  return asInstance(type()->asObject());
}

bool ApiHandle::isType() {
  // This works under the assumption only PyType_Type's metaType is itself
  return this->type() == ApiHandle::fromPyObject(type()->asPyObject())->type();
}

// TODO(T32685074): Handle PyTypeObject as any other ApiHandle
ApiTypeHandle* ApiHandle::type() {
  return ApiTypeHandle::fromPyTypeObject(ob_type);
}

// TODO(T32685074): Handle PyTypeObject as any other ApiHandle
ApiTypeHandle* ApiTypeHandle::fromObject(Object* type) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  DCHECK(type->isType(), "not a Type object");
  Handle<Type> klass(&scope, type);
  Handle<Object> extension_type_obj(&scope, klass->extensionType());

  if (!extension_type_obj->isInt()) {
    UNIMPLEMENTED("Type object doesn't contain an ApiTypeHandle");
  }

  Handle<Int> extension_type(&scope, *extension_type_obj);
  return static_cast<ApiTypeHandle*>(extension_type->asCPointer());
}

// TODO(T32685074): Handle PyTypeObject as any other ApiHandle
Object* ApiTypeHandle::asObject() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Get type Type
  Handle<Dict> extensions_dict(&scope, runtime->extensionTypes());
  Handle<Object> address(&scope,
                         runtime->newIntFromCPointer(static_cast<void*>(this)));
  Handle<Object> result(&scope, runtime->dictAt(extensions_dict, address));
  CHECK(!result->isError(),
        "Uninitialized PyTypeObject. Run PyType_Ready on it");
  return *result;
}

}  // namespace python
