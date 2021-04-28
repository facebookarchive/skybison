#pragma once

#include <cstdint>

#include "cpython-types.h"

#include "handles.h"
#include "objects.h"

namespace py {

class PointerVisitor;

static const Py_ssize_t kImmediateRefcnt = Py_ssize_t{1} << 63;

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

  RawObject asObjectImmediate();

  RawObject asObjectNoImmediate();

  // Return native proxy belonging to an extension object.
  RawNativeProxy asNativeProxy();

  // Each ApiHandle can have one pointer to cached data, which will be freed
  // when the handle is destroyed.
  void* cache(Runtime* runtime);
  void setCache(Runtime* runtime, void* value);

  // Decrements the reference count of the handle to signal the removal of a
  // reference count from extension code.
  void decref();

  void decrefNoImmediate();

  // Remove the ApiHandle from the dictionary and free its memory
  void dispose();
  void disposeWithRuntime(Runtime* runtime);

  bool isImmediate();

  // Increments the reference count of the handle to signal the addition of a
  // reference from extension code.
  void incref();

  void increfNoImmediate();

  // Returns the number of references to this handle from extension code.
  Py_ssize_t refcnt();

  Py_ssize_t refcntNoImmediate();

  void setRefcnt(Py_ssize_t count);

  void setBorrowedNoImmediate();
  bool isBorrowedNoImmediate();

 private:
  static bool isEncodeableAsImmediate(RawObject obj);

  static const Py_ssize_t kBorrowedBit = Py_ssize_t{1} << 63;

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
  if (isImmediate()) return asObjectImmediate();
  return asObjectNoImmediate();
}

inline RawObject ApiHandle::asObjectImmediate() {
  DCHECK(isImmediate(), "expected immediate");
  return RawObject{reinterpret_cast<uword>(this) ^ kImmediateTag};
}

inline RawObject ApiHandle::asObjectNoImmediate() {
  DCHECK(!isImmediate(), "must not be called with immediate object");
  return RawObject{reference_};
}

inline void ApiHandle::decref() {
  if (isImmediate()) return;
  decrefNoImmediate();
}

inline void ApiHandle::decrefNoImmediate() {
  DCHECK(!isImmediate(), "must not be called with immediate object");
  DCHECK((ob_refcnt & ~kBorrowedBit) > 0, "reference count underflow");
  --ob_refcnt;
  // Dispose `ApiHandle`s without `kBorrowedBit` when they reach refcount zero.
  if (ob_refcnt == 0) {
    dispose();
  }
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

inline void ApiHandle::incref() {
  if (isImmediate()) return;
  increfNoImmediate();
}

inline void ApiHandle::increfNoImmediate() {
  DCHECK(!isImmediate(), "must not be called with immediate object");
  DCHECK((ob_refcnt & ~kBorrowedBit) <
             (std::numeric_limits<Py_ssize_t>::max() & ~kBorrowedBit),
         "Reference count overflowed");
  ++ob_refcnt;
}

inline bool ApiHandle::isImmediate() {
  return (reinterpret_cast<uword>(this) & kImmediateMask) != 0;
}

inline Py_ssize_t ApiHandle::refcnt() {
  if (isImmediate()) return kImmediateRefcnt;
  return refcntNoImmediate();
}

inline Py_ssize_t ApiHandle::refcntNoImmediate() {
  DCHECK(!isImmediate(), "must not be called with immediate object");
  return ob_refcnt & ~kBorrowedBit;
}

inline void ApiHandle::setBorrowedNoImmediate() {
  DCHECK(!isImmediate(), "must not be called with immediate object");
  ob_refcnt |= kBorrowedBit;
}

inline bool ApiHandle::isBorrowedNoImmediate() {
  DCHECK(!isImmediate(), "must not be called with immediate object");
  return (ob_refcnt & kBorrowedBit) != 0;
}

inline RawObject ApiHandle::stealReference(PyObject* py_obj) {
  ApiHandle* handle = ApiHandle::fromPyObject(py_obj);
  if (handle->isImmediate()) return handle->asObjectImmediate();
  DCHECK((handle->ob_refcnt & ~kBorrowedBit) > 0, "refcount underflow");
  // Mark stolen reference as borrowed. This is to support code like this that
  // increases refcount after the fact:
  //     PyModule_AddObject(..., x);
  //     Py_INCREF(x);
  handle->ob_refcnt |= kBorrowedBit;
  handle->ob_refcnt--;
  return handle->asObjectNoImmediate();
}

}  // namespace py
