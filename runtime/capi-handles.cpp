#include "capi-handles.h"

#include "runtime.h"

namespace python {

ApiTypeHandle* ApiTypeHandle::newTypeHandle(const char* name,
                                            PyTypeObject* metatype) {
  PyTypeObject* pytype =
      static_cast<PyTypeObject*>(std::calloc(1, sizeof(PyTypeObject)));
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
  Handle<Dictionary> dict(&scope, runtime->apiHandles());
  Object* value = runtime->dictionaryAt(dict, key);
  if (value->isError()) {
    ApiHandle* handle = ApiHandle::newHandle(obj);
    Handle<Object> object(
        &scope, runtime->newIntegerFromCPointer(static_cast<void*>(handle)));
    runtime->dictionaryAtPut(dict, key, object);
    return handle;
  }
  return static_cast<ApiHandle*>(Integer::cast(value)->asCPointer());
}

ApiHandle* ApiHandle::fromBorrowedObject(Object* obj) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Handle<Object> key(&scope, obj);
  Handle<Dictionary> dict(&scope, runtime->apiHandles());
  Object* value = runtime->dictionaryAt(dict, key);
  if (value->isError()) {
    ApiHandle* handle = ApiHandle::newBorrowedHandle(obj);
    Handle<Object> object(
        &scope, runtime->newIntegerFromCPointer(static_cast<void*>(handle)));
    runtime->dictionaryAtPut(dict, key, object);
    return handle;
  }
  return static_cast<ApiHandle*>(Integer::cast(value)->asCPointer());
}

Object* ApiHandle::asObject() {
  // Fast path
  if (reference_ != nullptr) {
    return static_cast<Object*>(reference_);
  }

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  // Create runtime instance from extension object
  if (type() && !type()->isBuiltin()) {
    return runtime->newExtensionInstance(this);
  }

  UNIMPLEMENTED("Could not materialize a runtime object from the ApiHandle");
}

}  // namespace python
