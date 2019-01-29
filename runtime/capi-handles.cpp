#include "capi-handles.h"
#include "cpython-types.h"
#include "runtime.h"

namespace python {

ApiHandle* ApiHandle::alloc(Thread* thread, RawObject reference) {
  ApiHandle* result = static_cast<ApiHandle*>(std::malloc(sizeof(ApiHandle)));
  result->reference_ = reinterpret_cast<void*>(reference.raw());
  result->ob_refcnt = 0;
  RawObject type = thread->runtime()->typeOf(reference);
  if (reference == type) {
    result->ob_type = reinterpret_cast<PyTypeObject*>(result);
  } else {
    result->ob_type =
        reinterpret_cast<PyTypeObject*>(ApiHandle::newReference(thread, type));
  }
  return result;
}

ApiHandle* ApiHandle::ensure(Thread* thread, RawObject obj) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Dict dict(&scope, runtime->apiHandles());
  Object key(&scope, obj);
  Object value(&scope, runtime->dictAt(dict, key));

  // Fast path: All initialized builtin objects
  if (!value->isError()) {
    return castFromObject(*value);
  }

  // Get the PyObject pointer from extension instances
  RawObject extension_ptr = getExtensionPtrAttr(thread, key);
  if (!extension_ptr->isError()) {
    return castFromObject(extension_ptr);
  }

  // Initialize an ApiHandle for a builtin object or runtime instance
  ApiHandle* handle = ApiHandle::alloc(thread, obj);
  handle->setManaged();
  Object object(&scope, runtime->newIntFromCPtr(static_cast<void*>(handle)));
  runtime->dictAtPut(dict, key, object);
  return handle;
}

ApiHandle* ApiHandle::newReference(Thread* thread, RawObject obj) {
  ApiHandle* result = ApiHandle::ensure(thread, obj);
  result->incref();
  return result;
}

ApiHandle* ApiHandle::borrowedReference(Thread* thread, RawObject obj) {
  return ApiHandle::ensure(thread, obj);
}

ApiHandle* ApiHandle::castFromObject(RawObject value) {
  return static_cast<ApiHandle*>(RawInt::cast(value)->asCPtr());
}

RawObject ApiHandle::getExtensionPtrAttr(Thread* thread, const Object& obj) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  if (!obj->isInstance()) return Error::object();

  HeapObject instance(&scope, *obj);
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  return runtime->instanceAt(thread, instance, attr_name);
}

RawObject ApiHandle::asInstance(RawObject obj) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  DCHECK(obj->isType(), "not a RawType object");
  Type type(&scope, obj);
  Layout layout(&scope, type->instanceLayout());
  HeapObject instance(&scope, runtime->newInstance(layout));
  Object object_ptr(&scope, runtime->newIntFromCPtr(static_cast<void*>(this)));
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  runtime->instanceAtPut(thread, instance, attr_name, object_ptr);

  return *instance;
}

RawObject ApiHandle::asObject() {
  // Fast path: All builtin objects except Types
  if (isManaged()) return RawObject{reinterpret_cast<uword>(reference_)};

  // Create a runtime instance to hold the PyObject pointer
  DCHECK(type(), "ApiHandles must have a type to create an instance");
  return asInstance(type()->asObject());
}

bool ApiHandle::isType() {
  // This works under the assumption only PyType_Type's metaType is itself
  return this->type() == ApiHandle::fromPyObject(type())->type();
}

ApiHandle* ApiHandle::type() {
  return ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(ob_type));
}

void* ApiHandle::cache() {
  // Only managed objects can have a cached value
  if (!isManaged()) return nullptr;

  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object key(&scope, asObject());
  Dict caches(&scope, runtime->apiCaches());
  Object cache(&scope, runtime->dictAt(caches, key));
  DCHECK(cache->isInt() || cache->isError(), "unexpected cache type");
  if (!cache->isError()) return RawInt::cast(*cache)->asCPtr();
  return nullptr;
}

void ApiHandle::setCache(void* value) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object key(&scope, asObject());
  Dict caches(&scope, runtime->apiCaches());
  Int cache(&scope, runtime->newIntFromCPtr(value));
  runtime->dictAtPut(caches, key, cache);
}

void ApiHandle::dispose() {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object key(&scope, asObject());
  Dict dict(&scope, runtime->apiHandles());
  runtime->dictRemove(dict, key);

  dict = runtime->apiCaches();
  Object cache(&scope, runtime->dictRemove(dict, key));
  DCHECK(cache->isInt() || cache->isError(), "unexpected cache type");
  if (!cache->isError()) std::free(RawInt::cast(*cache)->asCPtr());

  std::free(this);
}

}  // namespace python
