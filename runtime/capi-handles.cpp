#include "capi-handles.h"

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"
#include "object-builtins.h"
#include "runtime.h"
#include "visitor.h"

namespace py {

ApiHandle* ApiHandle::alloc(Thread* thread, RawObject reference) {
  ApiHandle* result = static_cast<ApiHandle*>(std::malloc(sizeof(ApiHandle)));
  result->reference_ = reference.raw();
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

static RawObject identityEqual(Thread*, RawObject a, RawObject b) {
  return Bool::fromBool(a == b);
}

// Look up the value associated with key. Checks for identity equality, not
// structural equality. Returns Error::object() if the key was not found.
RawObject ApiHandle::dictAtIdentityEquals(Thread* thread, const Dict& dict,
                                          const Object& key,
                                          const Object& key_hash) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  bool found = thread->runtime()->dictLookup(thread, data, key, key_hash,
                                             &index, identityEqual);
  if (found) {
    DCHECK(index != -1, "invalid index %ld", index);
    return Dict::Bucket::value(*data, index);
  }
  return Error::notFound();
}

void ApiHandle::dictAtPutIdentityEquals(Thread* thread, const Dict& dict,
                                        const Object& key, const Object& value,
                                        const Object& key_hash) {
  Runtime* runtime = thread->runtime();
  if (dict.capacity() == 0) {
    dict.setData(runtime->newMutableTuple(Runtime::kInitialDictCapacity *
                                          Dict::Bucket::kNumPointers));
    dict.resetNumUsableItems();
  }
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  bool found =
      runtime->dictLookup(thread, data, key, key_hash, &index, identityEqual);
  DCHECK(index != -1, "invalid index %ld", index);
  bool empty_slot = Dict::Bucket::isEmpty(*data, index);
  Dict::Bucket::set(*data, index, *key_hash, *key, *value);
  if (found) {
    return;
  }
  dict.setNumItems(dict.numItems() + 1);
  if (empty_slot) {
    DCHECK(dict.numUsableItems() > 0, "dict.numIsableItems() must be positive");
    dict.decrementNumUsableItems();
    runtime->dictEnsureCapacity(thread, dict);
  }
}

RawObject ApiHandle::dictRemoveIdentityEquals(Thread* thread, const Dict& dict,
                                              const Object& key,
                                              const Object& key_hash) {
  HandleScope scope;
  Tuple data(&scope, dict.data());
  word index = -1;
  Object result(&scope, Error::notFound());
  bool found = thread->runtime()->dictLookup(thread, data, key, key_hash,
                                             &index, identityEqual);
  if (found) {
    DCHECK(index != -1, "unexpected index %ld", index);
    result = Dict::Bucket::value(*data, index);
    Dict::Bucket::setTombstone(*data, index);
    dict.setNumItems(dict.numItems() - 1);
  }
  return *result;
}

ApiHandle* ApiHandle::atIndex(Runtime* runtime, word index) {
  return castFromObject(Dict::Bucket::value(
      Tuple::cast(Dict::cast(runtime->apiHandles()).data()), index));
}

ApiHandle* ApiHandle::ensure(Thread* thread, RawObject obj) {
  Runtime* runtime = thread->runtime();

  // Get the PyObject pointer from extension instances
  if (runtime->isNativeProxy(obj)) {
    return static_cast<ApiHandle*>(runtime->nativeProxyPtr(obj));
  }

  HandleScope scope(thread);
  Dict dict(&scope, runtime->apiHandles());
  Object key(&scope, obj);
  Object key_hash(&scope, runtime->hash(*key));
  Object value(&scope, dictAtIdentityEquals(thread, dict, key, key_hash));

  // Get the handle of a builtin instance
  if (!value.isError()) {
    return castFromObject(*value);
  }

  // Initialize an ApiHandle for a builtin object or runtime instance
  ApiHandle* handle = ApiHandle::alloc(thread, obj);
  handle->setManaged();
  Object object(&scope, runtime->newIntFromCPtr(static_cast<void*>(handle)));
  dictAtPutIdentityEquals(thread, dict, key, object, key_hash);
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

RawObject ApiHandle::stealReference(Thread* thread, PyObject* py_obj) {
  HandleScope scope(thread);
  ApiHandle* handle = ApiHandle::fromPyObject(py_obj);
  Object obj(&scope, handle->asObject());
  handle->decref();
  return *obj;
}

RawObject ApiHandle::checkFunctionResult(Thread* thread, PyObject* result) {
  bool has_pending_exception = thread->hasPendingException();
  if (result == nullptr) {
    if (has_pending_exception) return Error::exception();
    return thread->raiseWithFmt(LayoutId::kSystemError,
                                "NULL return without exception set");
  }
  RawObject result_obj = ApiHandle::stealReference(thread, result);
  if (has_pending_exception) {
    // TODO(T53569173): set the currently pending exception as the cause of the
    // newly raised SystemError
    thread->clearPendingException();
    return thread->raiseWithFmt(LayoutId::kSystemError,
                                "non-NULL return with exception set");
  }
  return result_obj;
}

void ApiHandle::visitReferences(RawObject handles, PointerVisitor* visitor) {
  HandleScope scope;
  Dict dict(&scope, handles);

  // TODO(bsimmers): Since we're reading an object mid-collection, approximate a
  // read barrier until we have a more principled solution in place.
  HeapObject data_raw(&scope, dict.data());
  if (data_raw.isForwarding()) data_raw = data_raw.forward();
  Tuple data(&scope, *data_raw);

  word i = Dict::Bucket::kFirst;
  while (Dict::Bucket::nextItem(*data, &i)) {
    Object value(&scope, Dict::Bucket::value(*data, i));
    // Like above, check for forwarded objects. Most values in this Dict will be
    // SmallInts, but LargeInts are technically possible.
    if (value.isHeapObject()) {
      HeapObject heap_value(&scope, *value);
      if (heap_value.isForwarding()) value = heap_value.forward();
    }
    ApiHandle* handle = castFromObject(*value);
    visitor->visitPointer(reinterpret_cast<RawObject*>(&handle->reference_));
  }
}

ApiHandle* ApiHandle::castFromObject(RawObject value) {
  return static_cast<ApiHandle*>(Int::cast(value).asCPtr());
}

RawObject ApiHandle::asObject() {
  DCHECK(reference_ != 0 || isManaged(this),
         "A handle or native instance must point back to a heap instance");
  return RawObject{reference_};
}

bool ApiHandle::isType() {
  // This works under the assumption only PyType_Type's metaType is itself
  return this->type() == ApiHandle::fromPyObject(type())->type();
}

ApiHandle* ApiHandle::type() { return ApiHandle::fromPyTypeObject(ob_type); }

void* ApiHandle::cache() {
  // Only managed objects can have a cached value
  if (!isManaged(this)) return nullptr;

  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object key(&scope, asObject());
  Object key_hash(&scope, runtime->hash(*key));
  Dict caches(&scope, runtime->apiCaches());
  Object cache(&scope, dictAtIdentityEquals(thread, caches, key, key_hash));
  DCHECK(cache.isInt() || cache.isError(), "unexpected cache type");
  if (!cache.isError()) return Int::cast(*cache).asCPtr();
  return nullptr;
}

void ApiHandle::setCache(void* value) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object key(&scope, asObject());
  Object key_hash(&scope, runtime->hash(*key));
  Dict caches(&scope, runtime->apiCaches());
  Int cache(&scope, runtime->newIntFromCPtr(value));
  dictAtPutIdentityEquals(thread, caches, key, cache, key_hash);
}

void ApiHandle::dispose() {
  DCHECK(isManaged(this), "Dispose should only be called on managed handles");
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // TODO(T46009838): If a module handle is being disposed, this should register
  // a weakref to call the module's m_free once's the module is collected

  Object key(&scope, asObject());
  Object key_hash(&scope, runtime->hash(*key));
  Dict dict(&scope, runtime->apiHandles());
  dictRemoveIdentityEquals(thread, dict, key, key_hash);

  dict = runtime->apiCaches();
  Object cache(&scope, dictRemoveIdentityEquals(thread, dict, key, key_hash));
  DCHECK(cache.isInt() || cache.isError(), "unexpected cache type");
  if (!cache.isError()) std::free(Int::cast(*cache).asCPtr());

  std::free(this);
}

}  // namespace py
