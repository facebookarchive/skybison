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

struct IndexProbe {
  word index;
  word mask;
  uword perturb;
};

// Compute hash value suitable for `RawObject::operator==` (aka `a is b`)
// equality tests.
static uword handleHash(RawObject obj) {
  if (obj.isHeapObject()) {
    return HeapObject::cast(obj).address() >> kObjectAlignmentLog2;
  }
  return obj.raw();
}

static void itemAtPut(RawObject* keys, void** values, word index, RawObject key,
                      void* value) {
  DCHECK(!key.isUnbound(), "Unbound represents tombstone");
  DCHECK(!key.isNoneType(), "None represents empty");
  DCHECK(value != nullptr, "key must be associated with a C-API handle");
  keys[index] = key;
  values[index] = value;
}

static void itemAtPutTombstone(RawObject* keys, void** values, word index) {
  keys[index] = Unbound::object();
  values[index] = nullptr;
}

static bool itemIsEmpty(RawObject* keys, word index) {
  return keys[index].isNoneType();
}

static bool itemIsTombstone(RawObject* keys, word index) {
  return keys[index].isUnbound();
}

static RawObject itemKeyAt(RawObject* keys, word index) { return keys[index]; }

static void* itemValueAt(void** values, word index) { return values[index]; }

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

static bool nextItem(RawObject* keys, void** values, word* idx, word end,
                     RawObject* key_out, void** value_out) {
  for (word i = *idx; i < end; i++) {
    RawObject key = itemKeyAt(keys, i);
    if (key.isNoneType() || key.isUnbound()) continue;
    *key_out = key;
    *value_out = itemValueAt(values, i);
    *idx = i + 1;
    return true;
  }
  *idx = end;
  return false;
}

static IndexProbe probeBegin(word num_indices, uword hash) {
  DCHECK(num_indices > 0 && Utils::isPowerOfTwo(num_indices),
         "number of indices must be a power of two, got %ld", num_indices);
  word mask = num_indices - 1;
  return {static_cast<word>(hash) & mask, mask, hash};
}

static void probeNext(IndexProbe* probe) {
  // Note that repeated calls to this function guarantee a permutation of all
  // indices when the number of indices is power of two. See
  // https://en.wikipedia.org/wiki/Linear_congruential_generator#c_%E2%89%A0_0.
  probe->perturb >>= 5;
  probe->index = (probe->index * 5 + 1 + probe->perturb) & probe->mask;
}

void* IdentityDict::at(RawObject key) {
  word index = -1;
  bool found = lookup(key, &index);
  if (found) {
    DCHECK(index != -1, "invalid index %ld", index);
    return itemValueAt(values(), index);
  }
  return nullptr;
}

void IdentityDict::atPut(RawObject key, void* value) {
  DCHECK(value != nullptr, "nullptr indicates not found");
  word index = -1;
  bool found = lookup(key, &index);
  DCHECK(index != -1, "invalid index %ld", index);
  RawObject* keys = this->keys();
  bool empty_slot = itemIsEmpty(keys, index);
  itemAtPut(keys, values(), index, key, value);
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

bool IdentityDict::includes(RawObject key) {
  word ignored;
  return lookup(key, &ignored);
}

void IdentityDict::initialize(word capacity) {
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
  uword hash = handleHash(key);
  for (IndexProbe probe = probeBegin(capacity, hash);; probeNext(&probe)) {
    RawObject current_key = itemKeyAt(keys, probe.index);
    if (handleHash(current_key) == hash) {
      if (current_key == key) {
        *index = probe.index;
        return true;
      }
      continue;
    }
    if (itemIsEmpty(keys, probe.index)) {
      if (next_free_index == -1) {
        next_free_index = probe.index;
      }
      *index = next_free_index;
      return false;
    }
    if (itemIsTombstone(keys, probe.index)) {
      if (next_free_index == -1) {
        next_free_index = probe.index;
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
  RawObject key = NoneType::object();
  void* value;
  for (word i = 0; nextItem(keys, values, &i, capacity, &key, &value);) {
    uword hash = handleHash(key);
    for (IndexProbe probe = probeBegin(new_capacity, hash);;
         probeNext(&probe)) {
      DCHECK(!itemIsTombstone(new_keys, probe.index),
             "There should be no tombstones in a newly created dict");
      if (itemIsEmpty(new_keys, probe.index)) {
        itemAtPut(new_keys, new_values, probe.index, key, value);
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

void* IdentityDict::remove(RawObject key) {
  word index = -1;
  void* result = nullptr;
  bool found = lookup(key, &index);
  if (found) {
    DCHECK(index != -1, "unexpected index %ld", index);
    void** values = this->values();
    result = itemValueAt(values, index);
    itemAtPutTombstone(keys(), values, index);
    decrementNumItems();
  }
  return result;
}

void IdentityDict::shrink() {
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

  // Get the handle of a builtin instance
  IdentityDict* handles = capiHandles(runtime);
  void* value = handles->at(obj);
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

  handles->atPut(obj, handle);
  handle->reference_ = obj.raw();
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

void ApiHandle::clearNotReferencedHandles(IdentityDict* handles,
                                          IdentityDict* caches) {
  // Objects have moved; rehash the caches first to reflect new addresses
  caches->rehash(caches->capacity());

  // Now caches can be removed with ->remove()
  word capacity = handles->capacity();
  RawObject* keys = handles->keys();
  void** values = handles->values();

  // Loops through the handle table clearing out handles which are not
  // referenced by managed objects or by an extension object.
  RawObject key = NoneType::object();
  void* value;
  for (word i = 0; nextItem(keys, values, &i, capacity, &key, &value);) {
    ApiHandle* handle = static_cast<ApiHandle*>(value);
    if (!ApiHandle::hasExtensionReference(handle)) {
      // TODO(T56760343): Remove the cache lookup. This should become simpler
      // when it is easier to associate a cache with a handle or when the need
      // for caches is eliminated.
      void* cache = caches->remove(key);
      itemAtPutTombstone(keys, values, i - 1);
      handles->decrementNumItems();
      std::free(handle);
      std::free(cache);
    }
  }

  handles->rehash(handles->capacity());
}

void ApiHandle::disposeHandles(IdentityDict* handles) {
  word capacity = handles->capacity();
  RawObject* keys = handles->keys();
  void** values = handles->values();

  RawObject key = NoneType::object();
  void* value;
  for (word i = 0; nextItem(keys, values, &i, capacity, &key, &value);) {
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
  RawObject key = NoneType::object();
  void* value;
  for (word i = 0; nextItem(keys, values, &i, capacity, &key, &value);) {
    ApiHandle* handle = reinterpret_cast<ApiHandle*>(value);
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
  IdentityDict* caches = capiCaches(runtime);
  RawObject obj = asObject();
  return caches->at(obj);
}

void ApiHandle::setCache(void* value) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  IdentityDict* caches = capiCaches(runtime);
  RawObject obj = asObject();
  caches->atPut(obj, value);
}

void ApiHandle::dispose() {
  DCHECK(isManaged(this), "Dispose should only be called on managed handles");
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();

  // TODO(T46009838): If a module handle is being disposed, this should register
  // a weakref to call the module's m_free once's the module is collected

  RawObject obj = asObject();
  capiHandles(runtime)->remove(obj);

  void* cache = capiCaches(runtime)->remove(obj);
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

void capiHandlesClearNotReferenced(Runtime* runtime) {
  ApiHandle::clearNotReferencedHandles(capiHandles(runtime),
                                       capiCaches(runtime));
}

void capiHandlesDispose(Runtime* runtime) {
  ApiHandle::disposeHandles(capiHandles(runtime));
}

void capiHandlesShrink(Runtime* runtime) { capiHandles(runtime)->shrink(); }

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
