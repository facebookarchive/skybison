#include "capi-handles.h"

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"
#include "dict-builtins.h"
#include "object-builtins.h"
#include "runtime.h"
#include "visitor.h"

namespace py {

// TODO(T44244793): Remove these functions when handles have their own
// specialized hash table.
static bool dictLookupIdentityEquals(RawTuple data, RawObject key, word hash,
                                     word* index) {
  if (data.length() == 0) {
    *index = -1;
    return false;
  }
  word next_free_index = -1;
  uword perturb;
  word bucket_mask;
  RawSmallInt hash_int = SmallInt::fromWord(hash);
  for (word current = Dict::Bucket::bucket(data, hash, &bucket_mask, &perturb);;
       current = Dict::Bucket::nextBucket(current, bucket_mask, &perturb)) {
    word current_index = current * Dict::Bucket::kNumPointers;
    if (Dict::Bucket::hashRaw(data, current_index) == hash_int) {
      if (Dict::Bucket::key(data, current_index) == key) {
        *index = current_index;
        return true;
      }
      continue;
    }
    if (Dict::Bucket::isEmpty(data, current_index)) {
      if (next_free_index == -1) {
        next_free_index = current_index;
      }
      *index = next_free_index;
      return false;
    }
    if (Dict::Bucket::isTombstone(data, current_index)) {
      if (next_free_index == -1) {
        next_free_index = current_index;
      }
    }
  }
}

// Look up the value associated with key. Checks for identity equality, not
// structural equality. Returns Error::object() if the key was not found.
RawObject ApiHandle::dictAtIdentityEquals(Thread* thread, const Dict& dict,
                                          const Object& key, word hash) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  bool found = dictLookupIdentityEquals(*data, *key, hash, &index);
  if (found) {
    DCHECK(index != -1, "invalid index %ld", index);
    return Dict::Bucket::value(*data, index);
  }
  return Error::notFound();
}

void ApiHandle::dictAtPutIdentityEquals(Thread* thread, const Dict& dict,
                                        const Object& key, word hash,
                                        const Object& value) {
  Runtime* runtime = thread->runtime();
  if (dict.capacity() == 0) {
    dict.setData(runtime->newMutableTuple(Runtime::kInitialDictCapacity *
                                          Dict::Bucket::kNumPointers));
    dict.resetNumUsableItems();
  }
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  bool found = dictLookupIdentityEquals(*data, *key, hash, &index);
  DCHECK(index != -1, "invalid index %ld", index);
  bool empty_slot = Dict::Bucket::isEmpty(*data, index);
  Dict::Bucket::set(*data, index, hash, *key, *value);
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

RawObject ApiHandle::dictRemoveIdentityEquals(Thread* thread, const Dict& dict,
                                              const Object& key, word hash) {
  HandleScope scope(thread);
  Tuple data(&scope, dict.data());
  word index = -1;
  Object result(&scope, Error::notFound());
  bool found = dictLookupIdentityEquals(*data, *key, hash, &index);
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
    ApiHandle* result = static_cast<ApiHandle*>(runtime->nativeProxyPtr(obj));
    result->incref();
    return result;
  }

  HandleScope scope(thread);
  Dict dict(&scope, runtime->apiHandles());
  Object key(&scope, obj);
  word hash = runtime->hash(*key);
  Object value(&scope, dictAtIdentityEquals(thread, dict, key, hash));

  // Get the handle of a builtin instance
  if (!value.isError()) {
    ApiHandle* result = castFromObject(*value);
    result->incref();
    return result;
  }

  // Initialize an ApiHandle for a builtin object or runtime instance
  ApiHandle* handle = static_cast<ApiHandle*>(std::malloc(sizeof(ApiHandle)));
  Object object(&scope, runtime->newIntFromCPtr(static_cast<void*>(handle)));
  handle->reference_ = NoneType::object().raw();
  handle->ob_refcnt = 1 | kManagedBit;
  dictAtPutIdentityEquals(thread, dict, key, hash, object);
  handle->reference_ = key.raw();
  return handle;
}

// TODO(T58710656): Allow immediate handles for SmallStr
// TODO(T58710677): Allow immediate handles for SmallBytes
static bool isEncodeableAsImmediate(RawObject obj) {
  DCHECK(!obj.isHeapObject(),
         "this function should only be called on immediates");
  // SmallStr and SmallBytes require solutions for C-API functions that read
  // out char* whose lifetimes depend on the lifetimes of the PyObject*s.
  return !obj.isSmallStr() && !obj.isSmallBytes();
}

ApiHandle* ApiHandle::newReference(Thread* thread, RawObject obj) {
  if (!obj.isHeapObject() && isEncodeableAsImmediate(obj)) {
    return reinterpret_cast<ApiHandle*>(obj.raw() ^ kImmediateTag);
  }
  return ApiHandle::ensure(thread, obj);
}

ApiHandle* ApiHandle::borrowedReference(Thread* thread, RawObject obj) {
  if (!obj.isHeapObject() && isEncodeableAsImmediate(obj)) {
    return reinterpret_cast<ApiHandle*>(obj.raw() ^ kImmediateTag);
  }
  ApiHandle* result = ApiHandle::ensure(thread, obj);
  result->decref();
  return result;
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
    if (ApiHandle::hasExtensionReference(handle)) {
      visitor->visitPointer(reinterpret_cast<RawObject*>(&handle->reference_));
    }
  }
}

ApiHandle* ApiHandle::castFromObject(RawObject value) {
  return static_cast<ApiHandle*>(Int::cast(value).asCPtr());
}

RawObject ApiHandle::asObject() {
  if (isImmediate(this)) {
    return RawObject(reinterpret_cast<uword>(this) ^ kImmediateTag);
  }
  DCHECK(reference_ != 0 || isManaged(this),
         "A handle or native instance must point back to a heap instance");
  return RawObject{reference_};
}

void* ApiHandle::cache() {
  // Only managed objects can have a cached value
  DCHECK(!isImmediate(this), "immediate handles do not have caches");
  if (!isManaged(this)) return nullptr;

  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object key(&scope, asObject());
  word hash = runtime->hash(*key);
  Dict caches(&scope, runtime->apiCaches());
  Object cache(&scope, dictAtIdentityEquals(thread, caches, key, hash));
  DCHECK(cache.isInt() || cache.isError(), "unexpected cache type");
  if (!cache.isError()) return Int::cast(*cache).asCPtr();
  return nullptr;
}

void ApiHandle::setCache(void* value) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object key(&scope, asObject());
  word hash = runtime->hash(*key);
  Dict caches(&scope, runtime->apiCaches());
  Int cache(&scope, runtime->newIntFromCPtr(value));
  dictAtPutIdentityEquals(thread, caches, key, hash, cache);
}

void ApiHandle::dispose() {
  DCHECK(isManaged(this), "Dispose should only be called on managed handles");
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // TODO(T46009838): If a module handle is being disposed, this should register
  // a weakref to call the module's m_free once's the module is collected

  Object key(&scope, asObject());
  word hash = runtime->hash(*key);
  Dict dict(&scope, runtime->apiHandles());
  dictRemoveIdentityEquals(thread, dict, key, hash);

  dict = runtime->apiCaches();
  Object cache(&scope, dictRemoveIdentityEquals(thread, dict, key, hash));
  DCHECK(cache.isInt() || cache.isError(), "unexpected cache type");
  if (!cache.isError()) std::free(Int::cast(*cache).asCPtr());

  std::free(this);
}

}  // namespace py
