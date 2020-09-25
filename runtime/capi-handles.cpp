#include "capi-handles.h"

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include "event.h"
#include "object-builtins.h"
#include "runtime.h"
#include "visitor.h"

namespace py {

static const int kIdentityDictGrowthFactor = 2;

namespace {

// Helper class for manipulating buckets in the RawTuple that backs
// IdentityDict.
class Bucket {
 public:
  static word bucket(RawMutableTuple data, word hash, word* bucket_mask,
                     uword* perturb) {
    const word nbuckets = data.length() / kNumPointers;
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld is not a power of 2", nbuckets);
    DCHECK(nbuckets > 0, "bucket size <= 0");
    DCHECK(RawSmallInt::isValid(hash), "hash out of range");
    *perturb = static_cast<uword>(hash);
    *bucket_mask = nbuckets - 1;
    return *bucket_mask & hash;
  }

  static word bucketMask(word data_length) {
    word nbuckets = data_length / kNumPointers;
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld is not a power of 2", nbuckets);
    DCHECK(nbuckets > 0, "bucket size <= 0");
    return nbuckets - 1;
  }

  static word reduceIndex(word data_length, uword perturb) {
    return (bucketMask(data_length) & perturb) * kNumPointers;
  }

  static word nextBucket(word current, word bucket_mask, uword* perturb) {
    // Given that current stands for the index of a bucket, this advances
    // current to (5 * bucket + 1 + perturb). Note that it's guaranteed that
    // keeping calling this function returns a permutation of all indices when
    // the number of the buckets is power of two. See
    // https://en.wikipedia.org/wiki/Linear_congruential_generator#c_%E2%89%A0_0.
    *perturb >>= 5;
    return (current * 5 + 1 + *perturb) & bucket_mask;
  }

  static word hash(RawMutableTuple data, word index) {
    return RawSmallInt::cast(data.at(index + kHashOffset)).value();
  }

  static RawObject hashRaw(RawMutableTuple data, word index) {
    return data.at(index + kHashOffset);
  }

  static bool isEmpty(RawMutableTuple data, word index) {
    return data.at(index + kHashOffset).isNoneType();
  }

  static bool isTombstone(RawMutableTuple data, word index) {
    return data.at(index + kHashOffset).isUnbound();
  }

  static RawObject key(RawMutableTuple data, word index) {
    return data.at(index + kKeyOffset);
  }

  static void set(RawMutableTuple data, word index, word hash, RawObject key,
                  RawObject value) {
    data.atPut(index + kHashOffset, RawSmallInt::fromWordTruncated(hash));
    data.atPut(index + kKeyOffset, key);
    data.atPut(index + kValueOffset, value);
  }

  static void setTombstone(RawMutableTuple data, word index) {
    data.atPut(index + kHashOffset, Unbound::object());
    data.atPut(index + kKeyOffset, RawNoneType::object());
    data.atPut(index + kValueOffset, RawNoneType::object());
  }

  static void setValue(RawMutableTuple data, word index, RawObject value) {
    data.atPut(index + kValueOffset, value);
  }

  static RawObject value(RawMutableTuple data, word index) {
    return data.at(index + kValueOffset);
  }

  static bool nextItem(RawMutableTuple data, word* idx) {
    // Calling next on an invalid index should not advance that index.
    if (*idx >= data.length()) {
      return false;
    }
    do {
      *idx += kNumPointers;
    } while (*idx < data.length() && isEmptyOrTombstone(data, *idx));
    return *idx < data.length();
  }

  // Layout.
  static const word kHashOffset = 0;
  static const word kKeyOffset = kHashOffset + 1;
  static const word kValueOffset = kKeyOffset + 1;
  static const word kNumPointers = kValueOffset + 1;
  static const word kFirst = -kNumPointers;

 private:
  static bool isEmptyOrTombstone(RawMutableTuple data, word index) {
    return isEmpty(data, index) || isTombstone(data, index);
  }

  DISALLOW_HEAP_ALLOCATION();
};

}  // namespace

// Insert `key`/`value` into dictionary assuming no bucket with an equivalent
// key and no tombstones exist.
static void identityDictInsertNoUpdate(const MutableTuple& data,
                                       const Object& key, word hash,
                                       const Object& value) {
  DCHECK(data.length() > 0, "table must not be empty");
  uword perturb;
  word bucket_mask;
  for (word current = Bucket::bucket(*data, hash, &bucket_mask, &perturb);;
       current = Bucket::nextBucket(current, bucket_mask, &perturb)) {
    word current_index = current * Bucket::kNumPointers;
    DCHECK(!Bucket::isTombstone(*data, current_index),
           "There should be no tombstones in a newly created dict");
    if (Bucket::isEmpty(*data, current_index)) {
      Bucket::set(*data, current_index, hash, *key, *value);
      return;
    }
  }
}

static void identityDictEnsureCapacity(Thread* thread, IdentityDict* dict) {
  DCHECK(dict->capacity() > 0 && Utils::isPowerOfTwo(dict->capacity()),
         "tabl capacity must be power of two, greater than zero");
  if (dict->numUsableItems() > 0) {
    return;
  }
  // If at least half the space taken up in the dict is tombstones, removing
  // them will free up enough space. Otherwise, the dict must be grown.
  word growth_factor = (dict->numItems() < dict->numTombstones())
                           ? 1
                           : kIdentityDictGrowthFactor;
  // TODO(T44247845): Handle overflow here.
  word new_capacity = dict->capacity() * growth_factor;
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  MutableTuple data(&scope, dict->data());
  MutableTuple new_data(
      &scope, runtime->newMutableTuple(new_capacity * Bucket::kNumPointers));
  // Re-insert items
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  for (word i = Bucket::kFirst; Bucket::nextItem(*data, &i);) {
    key = Bucket::key(*data, i);
    word hash = Bucket::hash(*data, i);
    value = Bucket::value(*data, i);
    identityDictInsertNoUpdate(new_data, key, hash, value);
  }
  dict->setCapacity(new_capacity);
  dict->setData(*new_data);
  // Reset the usable items to 2/3 of the full capacity to guarantee low
  // collision rate.
  dict->setNumUsableItems((dict->capacity() * 2) / 3 - dict->numItems());
}

// TODO(T44244793): Remove these 2 functions when handles have their own
// specialized hash table.
static bool identityDictLookup(RawMutableTuple data, RawObject key, word hash,
                               word* index) {
  if (data.length() == 0) {
    *index = -1;
    return false;
  }
  word next_free_index = -1;
  uword perturb;
  word bucket_mask;
  RawSmallInt hash_int = SmallInt::fromWord(hash);
  for (word current = Bucket::bucket(data, hash, &bucket_mask, &perturb);;
       current = Bucket::nextBucket(current, bucket_mask, &perturb)) {
    word current_index = current * Bucket::kNumPointers;
    if (Bucket::hashRaw(data, current_index) == hash_int) {
      if (Bucket::key(data, current_index) == key) {
        *index = current_index;
        return true;
      }
      continue;
    }
    if (Bucket::isEmpty(data, current_index)) {
      if (next_free_index == -1) {
        next_free_index = current_index;
      }
      *index = next_free_index;
      return false;
    }
    if (Bucket::isTombstone(data, current_index)) {
      if (next_free_index == -1) {
        next_free_index = current_index;
      }
    }
  }
}

void IdentityDict::initialize(Runtime* runtime, word capacity) {
  setCapacity(capacity);
  setData(runtime->newMutableTuple(capacity * Bucket::kNumPointers));
  setNumUsableItems((capacity * 2) / 3);
}

// Look up the value associated with key. Checks for identity equality, not
// structural equality. Returns Error::object() if the key was not found.
RawObject IdentityDict::at(Thread* thread, const Object& key, word hash) {
  HandleScope scope(thread);
  MutableTuple data_tuple(&scope, data());
  word index = -1;
  bool found = identityDictLookup(*data_tuple, *key, hash, &index);
  if (found) {
    DCHECK(index != -1, "invalid index %ld", index);
    return Bucket::value(*data_tuple, index);
  }
  return Error::notFound();
}

bool IdentityDict::includes(Thread* thread, const Object& key, word hash) {
  return !at(thread, key, hash).isErrorNotFound();
}

void IdentityDict::atPut(Thread* thread, const Object& key, word hash,
                         const Object& value) {
  HandleScope scope(thread);
  MutableTuple data_tuple(&scope, data());
  word index = -1;
  bool found = identityDictLookup(*data_tuple, *key, hash, &index);
  DCHECK(index != -1, "invalid index %ld", index);
  bool empty_slot = Bucket::isEmpty(*data_tuple, index);
  Bucket::set(*data_tuple, index, hash, *key, *value);
  if (found) {
    return;
  }
  setNumItems(numItems() + 1);
  if (empty_slot) {
    DCHECK(numUsableItems() > 0, "dict.numUsableItems() must be positive");
    decrementNumUsableItems();
    identityDictEnsureCapacity(thread, this);
    DCHECK(numUsableItems() > 0, "dict.numUsableItems() must be positive");
  }
}

RawObject IdentityDict::remove(Thread* thread, const Object& key, word hash) {
  HandleScope scope(thread);
  MutableTuple data_tuple(&scope, data());
  word index = -1;
  Object result(&scope, Error::notFound());
  bool found = identityDictLookup(*data_tuple, *key, hash, &index);
  if (found) {
    DCHECK(index != -1, "unexpected index %ld", index);
    result = Bucket::value(*data_tuple, index);
    Bucket::setTombstone(*data_tuple, index);
    setNumItems(numItems() - 1);
  }
  return *result;
}

ApiHandle* ApiHandle::atIndex(Runtime* runtime, word index) {
  return castFromObject(
      Bucket::value(MutableTuple::cast(runtime->apiHandles()->data()), index));
}

ApiHandle* ApiHandle::ensure(Thread* thread, RawObject obj) {
  Runtime* runtime = thread->runtime();

  // Get the PyObject pointer from extension instances
  if (runtime->isInstanceOfNativeProxy(obj)) {
    ApiHandle* result = static_cast<ApiHandle*>(
        Int::cast(obj.rawCast<RawNativeProxy>().native()).asCPtr());
    result->incref();
    return result;
  }

  HandleScope scope(thread);
  Object key(&scope, obj);
  word hash = runtime->hash(*key);
  Object value(&scope, runtime->apiHandles()->at(thread, key, hash));

  // Get the handle of a builtin instance
  if (!value.isError()) {
    ApiHandle* result = castFromObject(*value);
    result->incref();
    return result;
  }

  // Initialize an ApiHandle for a builtin object or runtime instance
  EVENT_ID(AllocateCAPIHandle, obj.layoutId());
  ApiHandle* handle = static_cast<ApiHandle*>(std::malloc(sizeof(ApiHandle)));
  Object object(&scope, runtime->newIntFromCPtr(static_cast<void*>(handle)));
  handle->reference_ = NoneType::object().raw();
  handle->ob_refcnt = 1 | kManagedBit;
  runtime->apiHandles()->atPut(thread, key, hash, object);
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

void ApiHandle::clearNotReferencedHandles(Thread* thread, IdentityDict* handles,
                                          IdentityDict* caches) {
  HandleScope scope(thread);
  MutableTuple handle_data(&scope, handles->data());
  Object key(&scope, NoneType::object());
  Object cache_value(&scope, NoneType::object());
  // Loops through the handle table clearing out handles which are not
  // referenced by managed objects or by an extension object.
  for (word i = Bucket::kFirst; Bucket::nextItem(*handle_data, &i);) {
    RawObject value = Bucket::value(*handle_data, i);
    ApiHandle* handle = static_cast<ApiHandle*>(Int::cast(value).asCPtr());
    if (!ApiHandle::hasExtensionReference(handle)) {
      key = Bucket::key(*handle_data, i);
      word hash = Bucket::hash(*handle_data, i);
      // TODO(T56760343): Remove the cache lookup. This should become simpler
      // when it is easier to associate a cache with a handle or when the need
      // for caches is eliminated.
      cache_value = caches->remove(thread, key, hash);
      if (!cache_value.isError()) {
        std::free(Int::cast(*cache_value).asCPtr());
      }
      Bucket::setTombstone(*handle_data, i);
      handles->setNumItems(handles->numItems() - 1);
      std::free(handle);
    }
  }
}

void ApiHandle::disposeHandles(Thread* thread, IdentityDict* api_handles) {
  HandleScope scope(thread);
  MutableTuple data(&scope, api_handles->data());
  Runtime* runtime = thread->runtime();
  for (word i = Bucket::kFirst; Bucket::nextItem(*data, &i);) {
    ApiHandle* handle = ApiHandle::atIndex(runtime, i);
    handle->dispose();
  }
}

void ApiHandle::visitReferences(IdentityDict* handles,
                                PointerVisitor* visitor) {
  HandleScope scope;

  // TODO(bsimmers): Since we're reading an object mid-collection, approximate a
  // read barrier until we have a more principled solution in place.
  HeapObject data_raw(&scope, handles->data());
  if (data_raw.isForwarding()) data_raw = data_raw.forward();
  MutableTuple data(&scope, *data_raw);

  for (word i = Bucket::kFirst; Bucket::nextItem(*data, &i);) {
    Object value(&scope, Bucket::value(*data, i));
    // Like above, check for forwarded objects. Most values in this
    // IdentityDict will be SmallInts, but LargeInts are technically
    // possible.
    if (value.isHeapObject()) {
      HeapObject heap_value(&scope, *value);
      if (heap_value.isForwarding()) value = heap_value.forward();
    }
    ApiHandle* handle = castFromObject(*value);
    if (ApiHandle::hasExtensionReference(handle)) {
      visitor->visitPointer(reinterpret_cast<RawObject*>(&handle->reference_),
                            PointerKind::kApiHandle);
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

RawNativeProxy ApiHandle::asNativeProxy() {
  DCHECK(!isImmediate(this) && reference_ != 0,
         "expected extension object handle");
  return RawObject{reference_}.rawCast<RawNativeProxy>();
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
  IdentityDict* caches = runtime->apiCaches();
  Object cache(&scope, caches->at(thread, key, hash));
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
  IdentityDict* caches = runtime->apiCaches();
  Int cache(&scope, runtime->newIntFromCPtr(value));
  caches->atPut(thread, key, hash, cache);
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
  runtime->apiHandles()->remove(thread, key, hash);

  Object cache(&scope, runtime->apiCaches()->remove(thread, key, hash));
  DCHECK(cache.isInt() || cache.isError(), "unexpected cache type");
  if (!cache.isError()) std::free(Int::cast(*cache).asCPtr());

  std::free(this);
}

}  // namespace py
