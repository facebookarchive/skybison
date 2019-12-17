#pragma once

#include "cpython-types.h"

#include "handles.h"
#include "objects.h"

namespace py {

class PointerVisitor;

class ApiHandle : public PyObject {
 public:
  // Returns a handle for a managed object.  Increments the reference count of
  // the handle.
  static ApiHandle* newReference(Thread* thread, RawObject obj);

  // Returns a handle for a managed object.  Does not affect the reference count
  // of the handle.
  static ApiHandle* borrowedReference(Thread* thread, RawObject obj);

  // Returns the handle in Runtime::apiHandles() at index `index`. This is
  // useful when iterating over all of `apiHandles()`.
  static ApiHandle* atIndex(Runtime* runtime, word index);

  // Returns the managed object associated with the handle.  Decrements the
  // reference count of handle.
  static RawObject stealReference(Thread* thread, PyObject* py_obj);

  // Returns the managed object associated with the handle checking for
  static RawObject checkFunctionResult(Thread* thread, PyObject* result);

  static ApiHandle* fromPyObject(PyObject* py_obj) {
    return static_cast<ApiHandle*>(py_obj);
  }

  static ApiHandle* fromPyTypeObject(PyTypeObject* type) {
    return fromPyObject(reinterpret_cast<PyObject*>(type));
  }

  // Visit all reference_ members of live ApiHandles.
  static void visitReferences(RawObject handles, PointerVisitor* visitor);

  // Get the object from the handle's reference pointer. If non-existent
  // Either search the object in the runtime's extension types dictionary
  // or build a new extension instance.
  RawObject asObject();

  // Each ApiHandle can have one pointer to cached data, which will be freed
  // when the handle is destroyed.
  void* cache();
  void setCache(void* value);

  // Remove the ApiHandle from the dictionary and free its memory
  void dispose();

  // Returns true if the PyObject* is an immediate ApiHandle or ApiHandle.
  // Otherwise returns false since the PyObject* is an extension object.
  static bool isManaged(const PyObject* obj) {
    return isImmediate(obj) || (obj->ob_refcnt & kManagedBit) != 0;
  }

  // Returns the reference count of this object by masking the ManagedBit
  // NOTE: This should only be called by the GC.
  static bool hasExtensionReference(const PyObject* obj) {
    DCHECK(!isImmediate(obj),
           "Cannot get hasExtensionReference of immediate handle");
    return (obj->ob_refcnt & ~kManagedBit) > 0;
  }

  // Increments the reference count of the handle to signal the addition of a
  // reference from extension code.
  void incref() {
    if (isImmediate(this)) return;
    DCHECK((refcnt() & ~kManagedBit) < (kManagedBit - 1),
           "Reference count overflowed");
    ++ob_refcnt;
  }

  // Decrements the reference count of the handle to signal the removal of a
  // reference count from extension code.
  void decref() {
    if (isImmediate(this)) return;
    DCHECK((refcnt() & ~kManagedBit) > 0, "Reference count underflowed");
    --ob_refcnt;
  }

  // Returns the number of references to this handle from extension code.
  word refcnt() {
    if (isImmediate(this)) return kBorrowedBit;
    return ob_refcnt;
  }

  static bool isImmediate(const PyObject* obj) {
    return (reinterpret_cast<uword>(obj) & kImmediateMask) != 0;
  }

  // TODO(T44244793): Remove these functions when handles have their own
  // specialized hash table.
  static RawObject dictRemoveIdentityEquals(Thread* thread, const Dict& dict,
                                            const Object& key, word hash);

  // TODO(T44244793): Remove these functions when handles have their own
  // specialized hash table.
  static RawObject dictAtIdentityEquals(Thread* thread, const Dict& dict,
                                        const Object& key, word hash);

 private:
  // Returns an owned handle for a managed object. If a handle does not already
  // exist, a new handle is created.
  static ApiHandle* ensure(Thread* thread, RawObject obj);

  // Cast RawObject to ApiHandle*
  static ApiHandle* castFromObject(RawObject value);

  // Create a new runtime instance based on this ApiHandle
  RawObject asInstance(RawObject type);

  // TODO(T44244793): Remove these functions when handles have their own
  // specialized hash table.
  static void dictAtPutIdentityEquals(Thread* thread, const Dict& dict,
                                      const Object& key, word hash,
                                      const Object& value);

  static const long kManagedBit = 1L << 31;
  static const long kBorrowedBit = 1L << 30;
  static const long kImmediateTag = 0x1;
  static const long kImmediateMask = 0x7;

  static_assert(kImmediateMask < alignof(PyObject*),
                "Stronger alignment guarantees are required for immediate "
                "tagged PyObject* to work.");

  DISALLOW_IMPLICIT_CONSTRUCTORS(ApiHandle);
};

static_assert(sizeof(ApiHandle) == sizeof(PyObject),
              "ApiHandle must not add members to PyObject");

}  // namespace py
