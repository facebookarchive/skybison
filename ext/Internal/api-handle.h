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

  // Returns a handle for a managed object. This must not be called with an
  // extension object or an object for which `isEncodeableAsImmediate` is true.
  static ApiHandle* newReferenceWithManaged(Runtime* runtime, RawObject obj);

  // Returns a handle for a managed object.  Does not affect the reference count
  // of the handle.
  static ApiHandle* borrowedReference(Runtime* runtime, RawObject obj);

  static ApiHandle* handleFromImmediate(RawObject obj);

  // Returns the managed object associated with the handle.  Decrements the
  // reference count of handle.
  static RawObject stealReference(PyObject* py_obj);

  // Returns the managed object associated with the handle checking for
  static RawObject checkFunctionResult(Thread* thread, PyObject* result);

  static ApiHandle* fromPyObject(PyObject* py_obj);

  static ApiHandle* fromPyTypeObject(PyTypeObject* type);

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

  // Returns true if the handle has `kManagedBit` set. This means it references
  // an object on the managed heap that is not a NativeProxy, meaning exccept
  // for the `ApiHandle` there is no associated object in the C/C++ heap.
  bool isManaged();

  bool isImmediate();

  // Returns true if the object had its reference count increased by C-API
  // code. Internally this reads the refcount and ignores the borrowed/managed
  // bits.
  bool hasExtensionReference();

  // Increments the reference count of the handle to signal the addition of a
  // reference from extension code.
  void incref();

  // Decrements the reference count of the handle to signal the removal of a
  // reference count from extension code.
  void decref();

  // Returns the number of references to this handle from extension code.
  Py_ssize_t refcnt();

  void setRefcnt(Py_ssize_t count);

 private:
  static bool isEncodeableAsImmediate(RawObject obj);

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

inline RawObject ApiHandle::asObject() {
  if (isImmediate()) {
    return RawObject{reinterpret_cast<uword>(this) ^ kImmediateTag};
  }
  DCHECK(reference_ != 0 || isManaged(),
         "A handle or native instance must point back to a heap instance");
  return RawObject{reference_};
}

inline ApiHandle* ApiHandle::fromPyObject(PyObject* py_obj) {
  return static_cast<ApiHandle*>(py_obj);
}

inline ApiHandle* ApiHandle::fromPyTypeObject(PyTypeObject* type) {
  return fromPyObject(reinterpret_cast<PyObject*>(type));
}

inline ApiHandle* ApiHandle::handleFromImmediate(RawObject obj) {
  DCHECK(isEncodeableAsImmediate(obj), "expected immediate");
  return reinterpret_cast<ApiHandle*>(obj.raw() ^ kImmediateTag);
}

inline bool ApiHandle::hasExtensionReference() {
  DCHECK(!isImmediate(),
         "Cannot get hasExtensionReference of immediate handle");
  return (ob_refcnt & ~kManagedBit) > 0;
}

inline void ApiHandle::incref() {
  if (isImmediate()) return;
  DCHECK((ob_refcnt & ~(kManagedBit | kBorrowedBit)) <
             (std::numeric_limits<Py_ssize_t>::max() &
              ~(kManagedBit | kBorrowedBit)),
         "Reference count overflowed");
  ++ob_refcnt;
}

inline bool ApiHandle::isImmediate() {
  return (reinterpret_cast<uword>(this) & kImmediateMask) != 0;
}

inline bool ApiHandle::isManaged() {
  DCHECK(!isImmediate(), "must not be called with an immediate");
  return (ob_refcnt & kManagedBit) != 0;
}

inline void ApiHandle::decref() {
  if (isImmediate()) return;
  DCHECK((ob_refcnt & ~(kManagedBit | kBorrowedBit)) > 0,
         "Reference count underflowed");
  --ob_refcnt;
}

inline Py_ssize_t ApiHandle::refcnt() {
  if (isImmediate()) return kBorrowedBit;
  return ob_refcnt & ~(kManagedBit | kBorrowedBit);
}

}  // namespace py
