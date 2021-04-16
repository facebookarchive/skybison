#include "api-handle.h"

#include <cstdint>

#include "cpython-data.h"
#include "cpython-func.h"
#include "cpython-types.h"

#include "capi-state.h"
#include "capi.h"
#include "debugging.h"
#include "event.h"
#include "globals.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "visitor.h"

namespace py {

static const int32_t kEmptyIndex = -1;
static const int32_t kTombstoneIndex = -2;

struct IndexProbe {
  word index;
  word mask;
  uword perturb;
};

// Compute hash value suitable for `RawObject::operator==` (aka `a is b`)
// equality tests.
static inline uword handleHash(RawObject obj) {
  if (obj.isHeapObject()) {
    return obj.raw() >> kObjectAlignmentLog2;
  }
  return obj.raw();
}

static int32_t indexAt(int32_t* indices, word index) { return indices[index]; }

static void indexAtPut(int32_t* indices, word index, int32_t item_index) {
  indices[index] = item_index;
}

static void indexAtPutTombstone(int32_t* indices, word index) {
  indices[index] = kTombstoneIndex;
}

static void itemAtPut(RawObject* keys, void** values, int32_t index,
                      RawObject key, void* value) {
  DCHECK(!key.isNoneType(), "None represents empty and tombstone");
  DCHECK(value != nullptr, "key must be associated with a C-API handle");
  keys[index] = key;
  values[index] = value;
}

static void itemAtPutTombstone(RawObject* keys, void** values, int32_t index) {
  keys[index] = NoneType::object();
  values[index] = nullptr;
}

static RawObject itemKeyAt(RawObject* keys, int32_t index) {
  return keys[index];
}

static void* itemValueAt(void** values, int32_t index) { return values[index]; }

static int32_t maxCapacity(word num_indices) {
  DCHECK(num_indices <= kMaxInt32, "cannot fit %ld indices into 4-byte int",
         num_indices);
  return static_cast<int32_t>((num_indices * 2) / 3);
}

static int32_t* newIndices(word num_indices) {
  word size = num_indices * sizeof(int32_t);
  void* result = std::malloc(size);
  DCHECK(result != nullptr, "malloc failed");
  std::memset(result, -1, size);  // fill with kEmptyIndex
  return reinterpret_cast<int32_t*>(result);
}

static RawObject* newKeys(int32_t capacity) {
  word size = capacity * sizeof(RawObject);
  void* result = std::malloc(size);
  DCHECK(result != nullptr, "malloc failed");
  std::memset(result, -1, size);  // fill with NoneType::object()
  return reinterpret_cast<RawObject*>(result);
}

static void** newValues(int32_t capacity) {
  void* result = std::calloc(capacity, kPointerSize);
  DCHECK(result != nullptr, "calloc failed");
  return reinterpret_cast<void**>(result);
}

static bool nextItem(RawObject* keys, void** values, int32_t* idx, int32_t end,
                     RawObject* key_out, void** value_out) {
  for (int32_t i = *idx; i < end; i++) {
    RawObject key = itemKeyAt(keys, i);
    if (key.isNoneType()) continue;
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

void* ApiHandleDict::at(RawObject key) {
  word index;
  int32_t item_index;
  if (lookup(key, &index, &item_index)) {
    return itemValueAt(values(), item_index);
  }
  return nullptr;
}

void ApiHandleDict::atPut(RawObject key, void* value) {
  RawObject* keys = this->keys();
  void** values = this->values();

  word index;
  int32_t item_index;
  if (lookupForInsertion(key, &index, &item_index)) {
    itemAtPut(keys, values, item_index, key, value);
    return;
  }

  item_index = nextIndex();
  indexAtPut(indices(), index, item_index);
  itemAtPut(keys, values, item_index, key, value);
  incrementNumItems();
  setNextIndex(item_index + 1);

  // Maintain the invariant that we have space for at least one more item.
  if (hasUsableItem()) return;

  // If at least half of the items in the dense array are tombstones, removing
  // them will free up plenty of space. Otherwise, the dict must be grown.
  word growth_factor = (numItems() < capacity() / 2) ? 1 : kGrowthFactor;
  word new_num_indices = numIndices() * growth_factor;
  rehash(new_num_indices);
  DCHECK(hasUsableItem(), "dict must have space for another item");
}

void ApiHandleDict::initialize(word num_indices) {
  setIndices(newIndices(num_indices));
  setNumIndices(num_indices);

  int32_t capacity = maxCapacity(num_indices);
  setCapacity(capacity);
  setKeys(newKeys(capacity));
  setValues(newValues(capacity));
}

bool ApiHandleDict::lookup(RawObject key, word* sparse, int32_t* dense) {
  uword hash = handleHash(key);
  int32_t* indices = this->indices();
  RawObject* keys = this->keys();
  word num_indices = this->numIndices();

  for (IndexProbe probe = probeBegin(num_indices, hash);; probeNext(&probe)) {
    int32_t item_index = indexAt(indices, probe.index);
    if (item_index >= 0) {
      if (itemKeyAt(keys, item_index) == key) {
        *sparse = probe.index;
        *dense = item_index;
        return true;
      }
      continue;
    }
    if (item_index == kEmptyIndex) {
      return false;
    }
  }
}

bool ApiHandleDict::lookupForInsertion(RawObject key, word* sparse,
                                       int32_t* dense) {
  uword hash = handleHash(key);
  int32_t* indices = this->indices();
  RawObject* keys = this->keys();
  word num_indices = this->numIndices();

  word next_free_index = -1;
  for (IndexProbe probe = probeBegin(num_indices, hash);; probeNext(&probe)) {
    int32_t item_index = indexAt(indices, probe.index);
    if (item_index >= 0) {
      if (itemKeyAt(keys, item_index) == key) {
        *sparse = probe.index;
        *dense = item_index;
        return true;
      }
      continue;
    }
    if (next_free_index == -1) {
      next_free_index = probe.index;
    }
    if (item_index == kEmptyIndex) {
      *sparse = next_free_index;
      return false;
    }
  }
}

void ApiHandleDict::rehash(word new_num_indices) {
  int32_t end = nextIndex();
  int32_t* indices = this->indices();
  RawObject* keys = this->keys();
  void** values = this->values();

  int32_t new_capacity = maxCapacity(new_num_indices);
  int32_t* new_indices = newIndices(new_num_indices);
  RawObject* new_keys = newKeys(new_capacity);
  void** new_values = newValues(new_capacity);

  // Re-insert items
  RawObject key = NoneType::object();
  void* value;
  for (int32_t i = 0, count = 0; nextItem(keys, values, &i, end, &key, &value);
       count++) {
    uword hash = handleHash(key);
    for (IndexProbe probe = probeBegin(new_num_indices, hash);;
         probeNext(&probe)) {
      if (indexAt(new_indices, probe.index) == kEmptyIndex) {
        indexAtPut(new_indices, probe.index, count);
        itemAtPut(new_keys, new_values, count, key, value);
        break;
      }
    }
  }

  setCapacity(new_capacity);
  setIndices(new_indices);
  setKeys(new_keys);
  setNextIndex(numItems());
  setNumIndices(new_num_indices);
  setValues(new_values);

  std::free(indices);
  std::free(keys);
  std::free(values);
}

void* ApiHandleDict::remove(RawObject key) {
  word index;
  int32_t item_index;
  if (!lookup(key, &index, &item_index)) {
    return nullptr;
  }

  void** values = this->values();
  void* result = itemValueAt(values, item_index);
  indexAtPutTombstone(indices(), index);
  itemAtPutTombstone(keys(), values, item_index);
  decrementNumItems();
  return result;
}

void ApiHandleDict::shrink() {
  int32_t num_items = numItems();
  if (num_items < capacity() / kShrinkFactor) {
    // Ensure that the indices array is large enough to limit collisions.
    word new_num_indices = Utils::nextPowerOfTwo((num_items * 3) / 2 + 1);
    rehash(new_num_indices);
  }
}

void ApiHandleDict::visitKeys(PointerVisitor* visitor) {
  RawObject* keys = this->keys();
  if (keys == nullptr) return;
  word keys_length = capacity();
  for (word i = 0; i < keys_length; i++) {
    visitor->visitPointer(&keys[i], PointerKind::kRuntime);
  }
}

// Reserves a new handle in the given runtime's handle buffer.
static ApiHandle* allocateHandle(Runtime* runtime) {
  FreeListNode** free_handles = capiFreeHandles(runtime);
  ApiHandle* result = reinterpret_cast<ApiHandle*>(*free_handles);

  FreeListNode* next = (*free_handles)->next;
  if (next != nullptr) {
    *free_handles = next;
  } else {
    // No handles left to recycle; advance the frontier
    *free_handles = reinterpret_cast<FreeListNode*>(result + 1);
  }

  return result;
}

// Frees the handle for future re-use by the given runtime.
static void freeHandle(Runtime* runtime, ApiHandle* handle) {
  FreeListNode** free_handles = capiFreeHandles(runtime);
  FreeListNode* node = reinterpret_cast<FreeListNode*>(handle);
  node->next = *free_handles;
  *free_handles = node;
}

RawNativeProxy ApiHandle::asNativeProxy() {
  DCHECK(!isImmediate() && reference_ != 0, "expected extension object handle");
  return RawObject{reference_}.rawCast<RawNativeProxy>();
}

ApiHandle* ApiHandle::newReference(Runtime* runtime, RawObject obj) {
  if (isEncodeableAsImmediate(obj)) {
    return handleFromImmediate(obj);
  }
  if (runtime->isInstanceOfNativeProxy(obj)) {
    ApiHandle* result = static_cast<ApiHandle*>(
        Int::cast(obj.rawCast<RawNativeProxy>().native()).asCPtr());
    result->incref();
    return result;
  }
  return ApiHandle::newReferenceWithManaged(runtime, obj);
}

ApiHandle* ApiHandle::newReferenceWithManaged(Runtime* runtime, RawObject obj) {
  DCHECK(!isEncodeableAsImmediate(obj), "immediates not handled here");
  DCHECK(!runtime->isInstanceOfNativeProxy(obj),
         "native proxy not handled here");

  // Get the handle of a builtin instance
  ApiHandleDict* handles = capiHandles(runtime);
  void* value = handles->at(obj);
  if (value != nullptr) {
    ApiHandle* result = reinterpret_cast<ApiHandle*>(value);
    result->incref();
    return result;
  }

  // Initialize an ApiHandle for a builtin object or runtime instance
  EVENT_ID(AllocateCAPIHandle, obj.layoutId());
  ApiHandle* handle = allocateHandle(runtime);
  handle->reference_ = NoneType::object().raw();
  handle->ob_refcnt = 1 | kManagedBit;

  handles->atPut(obj, handle);
  handle->reference_ = obj.raw();
  return handle;
}

ApiHandle* ApiHandle::borrowedReference(Runtime* runtime, RawObject obj) {
  if (isEncodeableAsImmediate(obj)) {
    return handleFromImmediate(obj);
  }
  if (runtime->isInstanceOfNativeProxy(obj)) {
    return static_cast<ApiHandle*>(
        Int::cast(obj.rawCast<RawNativeProxy>().native()).asCPtr());
  }
  ApiHandle* result = ApiHandle::newReferenceWithManaged(runtime, obj);
  result->decref();
  return result;
}

RawObject ApiHandle::checkFunctionResult(Thread* thread, PyObject* result) {
  bool has_pending_exception = thread->hasPendingException();
  if (result == nullptr) {
    if (has_pending_exception) return Error::exception();
    return thread->raiseWithFmt(LayoutId::kSystemError,
                                "NULL return without exception set");
  }
  RawObject result_obj = ApiHandle::stealReference(result);
  if (has_pending_exception) {
    // TODO(T53569173): set the currently pending exception as the cause of the
    // newly raised SystemError
    thread->clearPendingException();
    return thread->raiseWithFmt(LayoutId::kSystemError,
                                "non-NULL return with exception set");
  }
  return result_obj;
}

void* ApiHandle::cache(Runtime* runtime) {
  // Only managed objects can have a cached value
  DCHECK(!isImmediate(), "immediate handles do not have caches");
  if (!isManaged()) return nullptr;

  ApiHandleDict* caches = capiCaches(runtime);
  RawObject obj = asObject();
  return caches->at(obj);
}

void ApiHandle::dispose(Runtime* runtime) {
  DCHECK(isManaged(), "Dispose should only be called on managed handles");

  // TODO(T46009838): If a module handle is being disposed, this should register
  // a weakref to call the module's m_free once's the module is collected

  RawObject obj = asObject();
  capiHandles(runtime)->remove(obj);

  void* cache = capiCaches(runtime)->remove(obj);
  std::free(cache);
  freeHandle(runtime, this);
}

// TODO(T58710656): Allow immediate handles for SmallStr
// TODO(T58710677): Allow immediate handles for SmallBytes
bool ApiHandle::isEncodeableAsImmediate(RawObject obj) {
  // SmallStr and SmallBytes require solutions for C-API functions that read
  // out char* whose lifetimes depend on the lifetimes of the PyObject*s.
  return !obj.isHeapObject() && !obj.isSmallStr() && !obj.isSmallBytes();
}

void ApiHandle::setCache(Runtime* runtime, void* value) {
  ApiHandleDict* caches = capiCaches(runtime);
  RawObject obj = asObject();
  caches->atPut(obj, value);
}

RawObject ApiHandle::stealReference(PyObject* py_obj) {
  ApiHandle* handle = ApiHandle::fromPyObject(py_obj);
  handle->decref();
  return handle->asObject();
}

void ApiHandle::setRefcnt(Py_ssize_t count) {
  if (isImmediate()) return;
  DCHECK((count & (kManagedBit | kBorrowedBit)) == 0,
         "count must not have high bits set");
  Py_ssize_t flags = ob_refcnt & (kManagedBit | kBorrowedBit);
  ob_refcnt = count | flags;
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
  ApiHandleDict* handles = capiHandles(runtime);
  ApiHandleDict* caches = capiCaches(runtime);

  // Objects have moved; rehash the caches first to reflect new addresses
  caches->rehash(caches->numIndices());

  // Now caches can be removed with ->remove()
  int32_t end = handles->nextIndex();
  RawObject* keys = handles->keys();
  void** values = handles->values();

  // Loops through the handle table clearing out handles which are not
  // referenced by managed objects or by an extension object.
  RawObject key = NoneType::object();
  void* value;
  for (int32_t i = 0; nextItem(keys, values, &i, end, &key, &value);) {
    ApiHandle* handle = static_cast<ApiHandle*>(value);
    if (!handle->hasExtensionReference()) {
      // TODO(T56760343): Remove the cache lookup. This should become simpler
      // when it is easier to associate a cache with a handle or when the need
      // for caches is eliminated.
      void* cache = caches->remove(key);
      itemAtPutTombstone(keys, values, i - 1);
      freeHandle(runtime, handle);
      std::free(cache);
    }
  }

  handles->rehash(handles->numIndices());
}

void capiHandlesDispose(Runtime* runtime) {
  ApiHandleDict* handles = capiHandles(runtime);
  int32_t end = handles->nextIndex();
  RawObject* keys = handles->keys();
  void** values = handles->values();
  RawObject key = NoneType::object();
  void* value;

  for (int32_t i = 0; nextItem(keys, values, &i, end, &key, &value);) {
    ApiHandle* handle = reinterpret_cast<ApiHandle*>(value);
    handle->dispose(runtime);
  }
}

void capiHandlesShrink(Runtime* runtime) { capiHandles(runtime)->shrink(); }

void capiHandlesVisit(Runtime* runtime, PointerVisitor* visitor) {
  ApiHandleDict* handles = capiHandles(runtime);
  handles->visitKeys(visitor);

  int32_t end = handles->nextIndex();
  RawObject* keys = handles->keys();
  void** values = handles->values();
  RawObject key = NoneType::object();
  void* value;
  for (int32_t i = 0; nextItem(keys, values, &i, end, &key, &value);) {
    ApiHandle* handle = reinterpret_cast<ApiHandle*>(value);
    if (handle->hasExtensionReference()) {
      visitor->visitPointer(reinterpret_cast<RawObject*>(&handle->reference_),
                            PointerKind::kApiHandle);
    }
  }

  ApiHandleDict* caches = capiCaches(runtime);
  caches->visitKeys(visitor);
}

void* objectBorrowedReference(Runtime* runtime, RawObject obj) {
  return ApiHandle::borrowedReference(runtime, obj);
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

bool objectHasHandleCache(Runtime* runtime, RawObject obj) {
  return ApiHandle::borrowedReference(runtime, obj)->cache(runtime) != nullptr;
}

void* objectNewReference(Runtime* runtime, RawObject obj) {
  return ApiHandle::newReference(runtime, obj);
}

void objectSetMember(Runtime* runtime, RawObject old_ptr, RawObject new_val) {
  ApiHandle** old = reinterpret_cast<ApiHandle**>(Int::cast(old_ptr).asCPtr());
  (*old)->decref();
  *old = ApiHandle::newReference(runtime, new_val);
}

void dump(PyObject* obj) {
  if (obj == nullptr) {
    std::fprintf(stderr, "<nullptr>\n");
    return;
  }
  dump(ApiHandle::fromPyObject(obj)->asObject());
}

}  // namespace py
