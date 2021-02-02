#include "capi-handles.h"

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include "capi-state.h"
#include "capi.h"
#include "event.h"
#include "globals.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "visitor.h"

namespace py {

namespace {

// Compute hash value suitable for `RawObject::operator==` (aka `a is b`)
// equality tests.
static word handleHash(RawObject obj) {
  if (obj.isHeapObject()) {
    return HeapObject::cast(obj).address() >> kObjectAlignmentLog2;
  }
  return static_cast<word>(obj.raw());
}

static RawObject* newKeys(word capacity) {
  word size = capacity * sizeof(RawObject);
  void* result = std::malloc(size);
  DCHECK(result != nullptr, "malloc failed");
  std::memset(result, -1, size);  // fill with NoneType::object()
  return reinterpret_cast<RawObject*>(result);
}

static void** newValues(word capacity) {
  void* result = std::calloc(capacity, kPointerSize);
  DCHECK(result != nullptr, "calloc failed");
  return reinterpret_cast<void**>(result);
}

// Helper class for manipulating buckets in the RawTuple that backs
// IdentityDict.
class Bucket {
 public:
  static word bucket(word nbuckets, word hash, word* bucket_mask,
                     uword* perturb) {
    DCHECK(Utils::isPowerOfTwo(nbuckets), "%ld is not a power of 2", nbuckets);
    DCHECK(nbuckets > 0, "bucket size <= 0");
    *perturb = static_cast<uword>(hash);
    *bucket_mask = nbuckets - 1;
    return *bucket_mask & hash;
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

  static bool isEmpty(RawObject* keys, word index) {
    return key(keys, index).isNoneType();
  }

  static bool isTombstone(RawObject* keys, word index) {
    return key(keys, index).isUnbound();
  }

  static RawObject key(RawObject* keys, word index) { return keys[index]; }

  static void set(RawObject* keys, void** values, word index, RawObject key,
                  void* value) {
    DCHECK(!key.isUnbound(),
           "Unbound should be an immediate C-API handle and represents "
           "tombstones");
    DCHECK(!key.isNoneType(),
           "None should be an immediate C-API handle and represents empties");
    keys[index] = key;
    values[index] = value;
  }

  static void setTombstone(RawObject* keys, void** values, word index) {
    keys[index] = Unbound::object();
    values[index] = nullptr;
  }

  static void* value(void** values, word index) { return values[index]; }

  static bool nextItem(RawObject* keys, word* idx, word end) {
    for (word i = *idx + 1; i < end; i++) {
      if (isEmptyOrTombstone(keys, i)) continue;
      *idx = i;
      return true;
    }
    *idx = end;
    return false;
  }

  // Layout.
  static const word kFirst = -1;

 private:
  static bool isEmptyOrTombstone(RawObject* keys, word index) {
    return isEmpty(keys, index) || isTombstone(keys, index);
  }

  DISALLOW_HEAP_ALLOCATION();
};

}  // namespace

void* IdentityDict::at(Thread*, const Object& key) {
  word index = -1;
  bool found = lookup(*key, &index);
  if (found) {
    DCHECK(index != -1, "invalid index %ld", index);
    return Bucket::value(values(), index);
  }
  return nullptr;
}

void IdentityDict::atPut(Thread*, const Object& key, void* value) {
  DCHECK(value != nullptr, "nullptr indicates not found");
  word index = -1;
  bool found = lookup(*key, &index);
  DCHECK(index != -1, "invalid index %ld", index);
  RawObject* keys = this->keys();
  bool empty_slot = Bucket::isEmpty(keys, index);
  Bucket::set(keys, values(), index, *key, value);
  if (found) {
    return;
  }
  incrementNumItems();
  if (empty_slot) {
    DCHECK(numUsableItems() > 0, "dict.numUsableItems() must be positive");
    decrementNumUsableItems();
    DCHECK(capacity() > 0 && Utils::isPowerOfTwo(capacity()),
           "table capacity must be power of two, greater than zero");
    if (numUsableItems() > 0) {
      return;
    }
    // If at least half the space taken up in the dict is tombstones, removing
    // them will free up enough space. Otherwise, the dict must be grown.
    word growth_factor = (numItems() < numTombstones()) ? 1 : kGrowthFactor;
    // TODO(T44247845): Handle overflow here.
    word new_capacity = capacity() * growth_factor;
    rehash(new_capacity);
    DCHECK(numUsableItems() > 0, "dict.numUsableItems() must be positive");
  }
}

bool IdentityDict::includes(Thread*, const Object& key) {
  word ignored;
  return lookup(*key, &ignored);
}

void IdentityDict::initialize(Runtime*, word capacity) {
  setCapacity(capacity);
  setKeys(newKeys(capacity));
  setNumUsableItems((capacity * 2) / 3);
  setValues(newValues(capacity));
}

bool IdentityDict::lookup(RawObject key, word* index) {
  word capacity = this->capacity();
  RawObject* keys = this->keys();
  if (capacity == 0) {
    *index = -1;
    return false;
  }
  word next_free_index = -1;
  uword perturb;
  word bucket_mask;
  word hash = handleHash(key);
  for (word current = Bucket::bucket(capacity, hash, &bucket_mask, &perturb);;
       current = Bucket::nextBucket(current, bucket_mask, &perturb)) {
    RawObject current_key = Bucket::key(keys, current);
    if (handleHash(current_key) == hash) {
      if (current_key == key) {
        *index = current;
        return true;
      }
      continue;
    }
    if (Bucket::isEmpty(keys, current)) {
      if (next_free_index == -1) {
        next_free_index = current;
      }
      *index = next_free_index;
      return false;
    }
    if (Bucket::isTombstone(keys, current)) {
      if (next_free_index == -1) {
        next_free_index = current;
      }
    }
  }
}

void IdentityDict::rehash(word new_capacity) {
  word capacity = this->capacity();
  RawObject* keys = this->keys();
  void** values = this->values();

  RawObject* new_keys = newKeys(new_capacity);
  void** new_values = newValues(new_capacity);

  // Re-insert items
  for (word i = Bucket::kFirst; Bucket::nextItem(keys, &i, capacity);) {
    RawObject key = Bucket::key(keys, i);
    void* value = Bucket::value(values, i);

    uword perturb;
    word bucket_mask;
    word hash = handleHash(key);
    for (word j = Bucket::bucket(new_capacity, hash, &bucket_mask, &perturb);;
         j = Bucket::nextBucket(j, bucket_mask, &perturb)) {
      DCHECK(!Bucket::isTombstone(new_keys, j),
             "There should be no tombstones in a newly created dict");
      if (Bucket::isEmpty(new_keys, j)) {
        Bucket::set(new_keys, new_values, j, key, value);
        break;
      }
    }
  }

  setCapacity(new_capacity);
  setKeys(new_keys);
  // Reset the usable items to 2/3 of the full capacity to guarantee low
  // collision rate.
  setNumUsableItems((new_capacity * 2) / 3 - numItems());
  setValues(new_values);

  std::free(keys);
  std::free(values);
}

void* IdentityDict::remove(Thread*, const Object& key) {
  word index = -1;
  void* result = nullptr;
  bool found = lookup(*key, &index);
  if (found) {
    DCHECK(index != -1, "unexpected index %ld", index);
    void** values = this->values();
    result = Bucket::value(values, index);
    Bucket::setTombstone(keys(), values, index);
    decrementNumItems();
  }
  return result;
}

void IdentityDict::shrink(Thread*) {
  if (numItems() < capacity() / kShrinkFactor) {
    // TODO(T44247845): Handle overflow here.
    // Ensure numItems is no more than 2/3 of available slots (ensure capacity
    // is at least 3/2 numItems).
    word new_capacity = Utils::nextPowerOfTwo((numItems() * 3) / 2 + 1);
    rehash(new_capacity);
  }
}

void IdentityDict::visit(PointerVisitor* visitor) {
  RawObject* keys = this->keys();
  if (keys == nullptr) return;
  word keys_length = capacity();
  for (word i = 0; i < keys_length; i++) {
    visitor->visitPointer(&keys[i], PointerKind::kRuntime);
  }
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
  void* value = capiHandles(runtime)->at(thread, key);

  // Get the handle of a builtin instance
  if (value != nullptr) {
    ApiHandle* result = reinterpret_cast<ApiHandle*>(value);
    result->incref();
    return result;
  }

  // Initialize an ApiHandle for a builtin object or runtime instance
  EVENT_ID(AllocateCAPIHandle, obj.layoutId());
  ApiHandle* handle = static_cast<ApiHandle*>(std::malloc(sizeof(ApiHandle)));
  handle->reference_ = NoneType::object().raw();
  handle->ob_refcnt = 1 | kManagedBit;
  capiHandles(runtime)->atPut(thread, key, handle);
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
  // Objects have moved; rehash the caches first to reflect new addresses
  caches->rehash(caches->capacity());

  // Now caches can be removed with ->remove()
  HandleScope scope(thread);
  word capacity = handles->capacity();
  RawObject* keys = handles->keys();
  void** values = handles->values();
  Object key(&scope, NoneType::object());
  // Loops through the handle table clearing out handles which are not
  // referenced by managed objects or by an extension object.
  for (word i = Bucket::kFirst; Bucket::nextItem(keys, &i, capacity);) {
    ApiHandle* handle = static_cast<ApiHandle*>(Bucket::value(values, i));
    if (!ApiHandle::hasExtensionReference(handle)) {
      key = Bucket::key(keys, i);
      // TODO(T56760343): Remove the cache lookup. This should become simpler
      // when it is easier to associate a cache with a handle or when the need
      // for caches is eliminated.
      void* cache = caches->remove(thread, key);
      Bucket::setTombstone(keys, values, i);
      handles->decrementNumItems();
      std::free(handle);
      std::free(cache);
    }
  }

  handles->rehash(handles->capacity());
}

void ApiHandle::disposeHandles(Thread*, IdentityDict* handles) {
  word capacity = handles->capacity();
  RawObject* keys = handles->keys();
  void** values = handles->values();
  for (word i = Bucket::kFirst; Bucket::nextItem(keys, &i, capacity);) {
    void* value = Bucket::value(values, i);
    ApiHandle* handle = reinterpret_cast<ApiHandle*>(value);
    handle->dispose();
  }
}

void ApiHandle::visitReferences(IdentityDict* handles,
                                PointerVisitor* visitor) {
  word capacity = handles->capacity();
  RawObject* keys = handles->keys();
  void** values = handles->values();

  // TODO(bsimmers): Since we're reading an object mid-collection, approximate a
  // read barrier until we have a more principled solution in place.
  for (word i = Bucket::kFirst; Bucket::nextItem(keys, &i, capacity);) {
    ApiHandle* handle = reinterpret_cast<ApiHandle*>(Bucket::value(values, i));
    if (ApiHandle::hasExtensionReference(handle)) {
      visitor->visitPointer(reinterpret_cast<RawObject*>(&handle->reference_),
                            PointerKind::kApiHandle);
    }
  }
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
  IdentityDict* caches = capiCaches(runtime);
  return caches->at(thread, key);
}

void ApiHandle::setCache(void* value) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Object key(&scope, asObject());
  IdentityDict* caches = capiCaches(runtime);
  caches->atPut(thread, key, value);
}

void ApiHandle::dispose() {
  DCHECK(isManaged(this), "Dispose should only be called on managed handles");
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // TODO(T46009838): If a module handle is being disposed, this should register
  // a weakref to call the module's m_free once's the module is collected

  Object key(&scope, asObject());
  capiHandles(runtime)->remove(thread, key);

  void* cache = capiCaches(runtime)->remove(thread, key);
  std::free(cache);
  std::free(this);
}

RawObject capiHandleAsObject(void* handle) {
  return reinterpret_cast<ApiHandle*>(handle)->asObject();
}

bool capiHandleFinalizableReference(void* handle, RawObject** out) {
  ApiHandle* api_handle = reinterpret_cast<ApiHandle*>(handle);
  *out = reinterpret_cast<RawObject*>(&api_handle->reference_);
  return api_handle->refcnt() > 1 ||
         HeapObject::cast(api_handle->asObject()).isForwarding();
}

void capiHandlesClearNotReferenced(Thread* thread) {
  Runtime* runtime = thread->runtime();
  ApiHandle::clearNotReferencedHandles(thread, capiHandles(runtime),
                                       capiCaches(runtime));
}

void capiHandlesDispose(Thread* thread) {
  ApiHandle::disposeHandles(thread, capiHandles(thread->runtime()));
}

void capiHandlesShrink(Thread* thread) {
  capiHandles(thread->runtime())->shrink(thread);
}

void* objectBorrowedReference(Thread* thread, RawObject obj) {
  return ApiHandle::borrowedReference(thread, obj);
}

RawObject objectGetMember(Thread* thread, RawObject ptr, RawObject name) {
  ApiHandle* value = *reinterpret_cast<ApiHandle**>(Int::cast(ptr).asCPtr());
  if (value != nullptr) {
    return value->asObject();
  }
  if (name.isNoneType()) {
    return NoneType::object();
  }
  HandleScope scope(thread);
  Str name_str(&scope, name);
  return thread->raiseWithFmt(LayoutId::kAttributeError,
                              "Object attribute '%S' is nullptr", &name_str);
}

bool objectHasHandleCache(Thread* thread, RawObject obj) {
  return ApiHandle::borrowedReference(thread, obj)->cache() != nullptr;
}

void* objectNewReference(Thread* thread, RawObject obj) {
  return ApiHandle::newReference(thread, obj);
}

void objectSetMember(Thread* thread, RawObject old_ptr, RawObject new_val) {
  ApiHandle** old = reinterpret_cast<ApiHandle**>(Int::cast(old_ptr).asCPtr());
  (*old)->decref();
  *old = ApiHandle::newReference(thread, new_val);
}

}  // namespace py
