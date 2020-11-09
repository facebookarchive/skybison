#pragma once

#include "cpython-types.h"

#include "handles.h"
#include "objects.h"

namespace py {

class PointerVisitor;

class IdentityDict {
 public:
  IdentityDict()
      : capacity_(0),
        num_items_(0),
        num_usable_items_(0),
        data_(NoneType::object()) {}

  void initialize(Runtime* runtime, word capacity);

  word capacity() { return capacity_; }
  void setCapacity(word capacity) { capacity_ = capacity; }

  word numItems() { return num_items_; }
  void setNumItems(word num_items) { num_items_ = num_items; }

  word numUsableItems() { return num_usable_items_; }
  void setNumUsableItems(word num_usable_items) {
    num_usable_items_ = num_usable_items;
  }
  void decrementNumUsableItems() {
    DCHECK(num_usable_items_ > 0, "num_usable_items must be > 0");
    num_usable_items_--;
  }
  word numTombstones() {
    return (capacity() * 2) / 3 - numItems() - numUsableItems();
  }

  RawObject data() { return data_; }
  void setData(RawObject data) { data_ = data; }

  void visit(PointerVisitor* visitor) {
    visitor->visitPointer(&data_, PointerKind::kRuntime);
  }

  RawObject at(Thread* thread, const Object& key);

  bool includes(Thread* thread, const Object& key);

  void atPut(Thread* thread, const Object& key, const Object& value);

  RawObject remove(Thread* thread, const Object& key);

  void shrink(Thread* thread);

 private:
  word capacity_;
  word num_items_;
  word num_usable_items_;
  RawObject data_;

  DISALLOW_HEAP_ALLOCATION();
  DISALLOW_COPY_AND_ASSIGN(IdentityDict);
};

static const word kImmediateRefcnt = 1L << 30;

class ApiHandle : public PyObject {
 public:
  // Returns a handle for a managed object.  Increments the reference count of
  // the handle.
  static ApiHandle* newReference(Thread* thread, RawObject obj);

  // Returns a handle for a managed object.  Does not affect the reference count
  // of the handle.
  static ApiHandle* borrowedReference(Thread* thread, RawObject obj);

  // Returns the handle in capiHandles(runtime) at index `index`. This is
  // useful when iterating over all of `capiHandles()`.
  static ApiHandle* atIndex(Runtime* runtime, word index);

  // Returns the managed object associated with the handle.  Decrements the
  // reference count of handle.
  static RawObject stealReference(Thread* thread, PyObject* py_obj);

  // Returns the managed object associated with the handle checking for
  static RawObject checkFunctionResult(Thread* thread, PyObject* result);

  static ApiHandle* fromPyObject(PyObject* py_obj);

  static ApiHandle* fromPyTypeObject(PyTypeObject* type);

  // WARNING: This function should be called by the garbage collector.
  // Clear out handles which are not referenced by managed objects or by an
  // extension object.
  static void clearNotReferencedHandles(Thread* thread);

  // WARNING: This function should be called for shutdown.
  // Dispose all handles, without trying to cleanly deallocate the object for
  // runtime shutdown.
  static void disposeHandles(Thread* thread, IdentityDict* api_handles);

  // Visit all reference_ members of live ApiHandles.
  static void visitReferences(IdentityDict* handles, PointerVisitor* visitor);

  // Get the object from the handle's reference field.
  RawObject asObject();

  // Return native proxy belonging to an extension object.
  RawNativeProxy asNativeProxy();

  // Each ApiHandle can have one pointer to cached data, which will be freed
  // when the handle is destroyed.
  void* cache();
  void setCache(void* value);

  // Remove the ApiHandle from the dictionary and free its memory
  void dispose();

  // Returns true if the PyObject* is an immediate ApiHandle or ApiHandle.
  // Otherwise returns false since the PyObject* is an extension object.
  static bool isManaged(const PyObject* obj);

  // Returns the reference count of this object by masking the ManagedBit
  // NOTE: This should only be called by the GC.
  static bool hasExtensionReference(const PyObject* obj);

  // Increments the reference count of the handle to signal the addition of a
  // reference from extension code.
  void incref();

  // Decrements the reference count of the handle to signal the removal of a
  // reference count from extension code.
  void decref();

  // Returns the number of references to this handle from extension code.
  word refcnt();

  static bool isImmediate(const PyObject* obj);

 private:
  // Returns an owned handle for a managed object. If a handle does not already
  // exist, a new handle is created.
  static ApiHandle* ensure(Thread* thread, RawObject obj);

  // Cast RawObject to ApiHandle*
  static ApiHandle* castFromObject(RawObject value);

  // Create a new runtime instance based on this ApiHandle
  RawObject asInstance(RawObject type);

  static const long kManagedBit = 1L << 31;
  static const long kBorrowedBit = 1L << 30;
  static const long kImmediateTag = 0x1;
  static const long kImmediateMask = 0x7;

  static_assert(kBorrowedBit == kImmediateRefcnt,
                "keep kBorrowedBit and kImmediateRefcnt in sync");
  static_assert(kImmediateMask < alignof(PyObject*),
                "Stronger alignment guarantees are required for immediate "
                "tagged PyObject* to work.");

  DISALLOW_IMPLICIT_CONSTRUCTORS(ApiHandle);
};

static_assert(sizeof(ApiHandle) == sizeof(PyObject),
              "ApiHandle must not add members to PyObject");

inline ApiHandle* ApiHandle::fromPyObject(PyObject* py_obj) {
  return static_cast<ApiHandle*>(py_obj);
}

inline ApiHandle* ApiHandle::fromPyTypeObject(PyTypeObject* type) {
  return fromPyObject(reinterpret_cast<PyObject*>(type));
}

inline RawObject ApiHandle::asObject() {
  if (isImmediate(this)) {
    return RawObject{reinterpret_cast<uword>(this) ^ kImmediateTag};
  }
  DCHECK(reference_ != 0 || isManaged(this),
         "A handle or native instance must point back to a heap instance");
  return RawObject{reference_};
}

inline bool ApiHandle::isManaged(const PyObject* obj) {
  return isImmediate(obj) || (obj->ob_refcnt & kManagedBit) != 0;
}

inline bool ApiHandle::hasExtensionReference(const PyObject* obj) {
  DCHECK(!isImmediate(obj),
         "Cannot get hasExtensionReference of immediate handle");
  return (obj->ob_refcnt & ~kManagedBit) > 0;
}

inline void ApiHandle::incref() {
  if (isImmediate(this)) return;
  DCHECK((refcnt() & ~kManagedBit) < (kManagedBit - 1),
         "Reference count overflowed");
  ++ob_refcnt;
}

inline void ApiHandle::decref() {
  if (isImmediate(this)) return;
  DCHECK((refcnt() & ~kManagedBit) > 0, "Reference count underflowed");
  --ob_refcnt;
}

inline word ApiHandle::refcnt() {
  if (isImmediate(this)) return kBorrowedBit;
  return ob_refcnt;
}

inline bool ApiHandle::isImmediate(const PyObject* obj) {
  return (reinterpret_cast<uword>(obj) & kImmediateMask) != 0;
}

}  // namespace py
