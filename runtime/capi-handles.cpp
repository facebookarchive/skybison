#include "capi-handles.h"

#include "cpython-func.h"
#include "cpython-types.h"
#include "object-builtins.h"
#include "runtime.h"
#include "visitor.h"

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

static bool identityEqual(RawObject a, RawObject b) { return a == b; }

// Look up the value associated with key. Checks for identity equality, not
// structural equality. Returns Error::object() if the key was not found.
RawObject ApiHandle::dictAtIdentityEquals(Thread* thread, const Dict& dict,
                                          const Object& key,
                                          const Object& key_hash) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  bool found =
      thread->runtime()->dictLookup(data, key, key_hash, &index, identityEqual);
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
    dict.setData(runtime->newTuple(Runtime::kInitialDictCapacity *
                                   Dict::Bucket::kNumPointers));
    dict.resetNumUsableItems();
  }
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  bool found = runtime->dictLookup(data, key, key_hash, &index, identityEqual);
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
    dictEnsureCapacity(thread, dict);
  }
}

void ApiHandle::dictEnsureCapacity(Thread* thread, const Dict& dict) {
  // TODO(T44245141): Move initialization of an empty dict here.
  DCHECK(dict.capacity() > 0 && Utils::isPowerOfTwo(dict.capacity()),
         "dict capacity must be power of two, greater than zero");
  if (dict.numUsableItems() > 0) {
    return;
  }
  // TODO(T44247845): Handle overflow here.
  word new_capacity = dict.capacity() * Runtime::kDictGrowthFactor;
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple data(&scope, dict.data());
  Tuple new_data(&scope,
                 runtime->newTuple(new_capacity * Dict::Bucket::kNumPointers));
  // Re-insert items
  for (word i = Dict::Bucket::kFirst; Dict::Bucket::nextItem(*data, &i);) {
    Object key(&scope, Dict::Bucket::key(*data, i));
    Object hash(&scope, Dict::Bucket::hash(*data, i));
    word index = -1;
    runtime->dictLookup(new_data, key, hash, &index, identityEqual);
    DCHECK(index != -1, "invalid index %ld", index);
    Dict::Bucket::set(*new_data, index, *hash, *key,
                      Dict::Bucket::value(*data, i));
  }
  dict.setData(*new_data);
  dict.resetNumUsableItems();
}

RawObject ApiHandle::dictRemoveIdentityEquals(Thread* thread, const Dict& dict,
                                              const Object& key,
                                              const Object& key_hash) {
  HandleScope scope;
  Tuple data(&scope, dict.data());
  word index = -1;
  Object result(&scope, Error::notFound());
  bool found =
      thread->runtime()->dictLookup(data, key, key_hash, &index, identityEqual);
  if (found) {
    DCHECK(index != -1, "unexpected index %ld", index);
    result = Dict::Bucket::value(*data, index);
    Dict::Bucket::setTombstone(*data, index);
    dict.setNumItems(dict.numItems() - 1);
  }
  return *result;
}

ApiHandle* ApiHandle::ensure(Thread* thread, RawObject obj) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Dict dict(&scope, runtime->apiHandles());
  Object key(&scope, obj);
  Object key_hash(&scope, runtime->hash(*key));
  Object value(&scope, dictAtIdentityEquals(thread, dict, key, key_hash));

  // Fast path: All initialized builtin objects
  if (!value.isError()) {
    return castFromObject(*value);
  }

  // Get the PyObject pointer from extension instances
  RawObject extension_ptr = getExtensionPtrAttr(thread, key);
  if (!extension_ptr.isError()) {
    return castFromObject(extension_ptr);
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
  if (py_obj == nullptr) return Error::exception();

  HandleScope scope(thread);
  // Using ApiHandle::fromPyObject() here is sketchy since the provenance of
  // py_obj is unknown, but it's the best we can do for now.
  Object obj(&scope, ApiHandle::fromPyObject(py_obj)->asObject());
  // TODO(T42827325): We should decref py_obj before returning.
  return *obj;
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

RawObject ApiHandle::getExtensionPtrAttr(Thread* thread, const Object& obj) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  if (!obj.isInstance()) return Error::notFound();

  HeapObject instance(&scope, *obj);
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  return instanceGetAttribute(thread, instance, attr_name);
}

RawObject ApiHandle::asInstance(RawObject obj) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  DCHECK(obj.isType(), "not a RawType object");
  Type type(&scope, obj);
  Layout layout(&scope, type.instanceLayout());
  HeapObject instance(&scope, runtime->newInstance(layout));
  Object object_ptr(&scope, runtime->newIntFromCPtr(static_cast<void*>(this)));
  Object attr_name(&scope, runtime->symbols()->ExtensionPtr());
  instanceSetAttr(thread, instance, attr_name, object_ptr);
  // TODO(T42827325): incref this before returning to represent the reference
  // stored in ___extension___.

  return *instance;
}

RawObject ApiHandle::asObject() {
  // Fast path: All builtin objects except Types
  if (isManaged(this)) return RawObject{reinterpret_cast<uword>(reference_)};

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

}  // namespace python
