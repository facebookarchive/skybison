#pragma once

#include <cstdint>

#include "cpython-types.h"

#include "handles.h"
#include "objects.h"

namespace py {

class PointerVisitor;

static const Py_ssize_t kImmediateRefcnt = Py_ssize_t{1} << 62;

class ApiHandle : public PyObject {
 public:
  // Returns a handle for a managed object.  Increments the reference count of
  // the handle.
  static ApiHandle* newReference(Runtime* runtime, RawObject obj);

  // Returns a handle for a managed object.  Does not affect the reference count
  // of the handle.
  static ApiHandle* borrowedReference(Runtime* runtime, RawObject obj);

  // Returns the managed object associated with the handle.  Decrements the
  // reference count of handle.
  static RawObject stealReference(PyObject* py_obj);

  // Returns the managed object associated with the handle checking for
  static RawObject checkFunctionResult(Thread* thread, PyObject* result);

  static ApiHandle* fromPyObject(PyObject* py_obj);

  static ApiHandle* fromPyTypeObject(PyTypeObject* type);

  // WARNING: This function should be called by the garbage collector.
  // Clear out handles which are not referenced by managed objects or by an
  // extension object.
  static void clearNotReferencedHandles(Runtime* runtime);

  // WARNING: This function should be called for shutdown.
  // Dispose all handles, without trying to cleanly deallocate the object for
  // runtime shutdown.
  static void disposeHandles(Runtime* runtime);

  // Visit all reference_ members of live ApiHandles.
  static void visitReferences(Runtime* runtime, PointerVisitor* visitor);

  // Get the object from the handle's reference field.
  RawObject asObject();

  // Return native proxy belonging to an extension object.
  RawNativeProxy asNativeProxy();

  // Each ApiHandle can have one pointer to cached data, which will be freed
  // when the handle is destroyed.
  void* cache(Runtime* runtime);
  void setCache(Runtime* runtime, void* value);

  // Remove the ApiHandle from the dictionary and free its memory
  void dispose(Runtime* runtime);

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
  Py_ssize_t refcnt();

  void setRefcnt(Py_ssize_t count);

  static bool isImmediate(const PyObject* obj);

 private:
  // Returns an owned handle for a managed object. If a handle does not already
  // exist, a new handle is created.
  static ApiHandle* ensure(Runtime* runtime, RawObject obj);

  // Create a new runtime instance based on this ApiHandle
  RawObject asInstance(RawObject type);

  static const Py_ssize_t kManagedBit = Py_ssize_t{1} << 63;
  static const Py_ssize_t kBorrowedBit = Py_ssize_t{1} << 62;

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

struct FreeListNode {
  FreeListNode* next;
};

static_assert(sizeof(FreeListNode) <= sizeof(ApiHandle),
              "Free ApiHandle should be usable as a FreeListNode");

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
  DCHECK((ob_refcnt & ~(kManagedBit | kBorrowedBit)) <
             (std::numeric_limits<Py_ssize_t>::max() &
              ~(kManagedBit | kBorrowedBit)),
         "Reference count overflowed");
  ++ob_refcnt;
}

inline void ApiHandle::decref() {
  if (isImmediate(this)) return;
  DCHECK((ob_refcnt & ~(kManagedBit | kBorrowedBit)) > 0,
         "Reference count underflowed");
  --ob_refcnt;
}

inline Py_ssize_t ApiHandle::refcnt() {
  if (isImmediate(this)) return kBorrowedBit;
  return ob_refcnt & ~(kManagedBit | kBorrowedBit);
}

inline bool ApiHandle::isImmediate(const PyObject* obj) {
  return (reinterpret_cast<uword>(obj) & kImmediateMask) != 0;
}

}  // namespace py
