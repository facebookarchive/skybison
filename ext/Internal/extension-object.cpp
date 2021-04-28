#include "extension-object.h"

#include "api-handle.h"
#include "capi-state.h"
#include "capi-typeslots.h"
#include "capi.h"
#include "linked-list.h"
#include "object.h"
#include "runtime.h"
#include "scavenger.h"

namespace py {

void disposeExtensionObjects(Runtime* runtime) {
  CAPIState* state = capiState(runtime);
  while (ListEntry* entry = state->extension_objects) {
    untrackExtensionObject(runtime, entry);
    std::free(entry);
  }
}

void finalizeExtensionObject(Thread* thread, RawObject object) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  NativeProxy proxy(&scope, object);
  Type type(&scope, runtime->typeOf(*proxy));
  DCHECK(type.hasNativeData(),
         "A native instance must come from an extension type");
  destructor tp_dealloc =
      reinterpret_cast<destructor>(typeSlotAt(type, Py_tp_dealloc));
  DCHECK(tp_dealloc != nullptr, "Extension types must have a dealloc slot");
  ApiHandle* handle = ApiHandle::fromPyObject(
      reinterpret_cast<PyObject*>(Int::cast(proxy.native()).asCPtr()));
  CHECK(handle->refcnt() == 1,
        "The runtime must hold the last reference to the PyObject* (%p). "
        "Expecting a refcount of 1, but found %ld\n",
        reinterpret_cast<void*>(handle), handle->refcnt());
  handle->setRefcnt(0);
  handle->setBorrowedNoImmediate();
  (*tp_dealloc)(handle);
  if (!proxy.native().isNoneType() && handle->refcnt() == 0) {
    // `proxy.native()` being `None` indicates the extension object memory was
    // not freed. `ob_refcnt == 0` means the object was not resurrected.
    // This typically indicates that the user maintains a free-list and wants to
    // call `PyObject_Init` on the memory again, we have to untrack it!
    ListEntry* entry = reinterpret_cast<ListEntry*>(handle) - 1;
    untrackExtensionObject(runtime, entry);
  }
}

PyObject* initializeExtensionObject(Thread* thread, PyObject* obj,
                                    PyTypeObject* typeobj,
                                    const Object& instance) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  NativeProxy proxy(&scope, *instance);
  proxy.setNative(runtime->newIntFromCPtr(obj));
  trackExtensionObject(runtime, reinterpret_cast<ListEntry*>(obj) - 1);

  // Initialize the native object
  obj->reference_ = proxy.raw();
  Py_INCREF(typeobj);
  obj->ob_refcnt = 2;
  return obj;
}

word numExtensionObjects(Runtime* runtime) {
  return capiState(runtime)->num_extension_objects;
}

bool trackExtensionObject(Runtime* runtime, ListEntry* entry) {
  CAPIState* state = capiState(runtime);
  bool did_insert = listEntryInsert(entry, &state->extension_objects);
  if (did_insert) state->num_extension_objects++;
  return did_insert;
}

bool untrackExtensionObject(Runtime* runtime, ListEntry* entry) {
  CAPIState* state = capiState(runtime);
  bool did_remove = listEntryRemove(entry, &state->extension_objects);
  if (did_remove) state->num_extension_objects--;
  return did_remove;
}

void visitExtensionObjects(Runtime* runtime, Scavenger* scavenger,
                           PointerVisitor* visitor) {
  CAPIState* state = capiState(runtime);
  for (ListEntry *next, *entry = state->extension_objects; entry != nullptr;
       entry = next) {
    next = entry->next;
    void* native_instance = entry + 1;
    ApiHandle* handle = reinterpret_cast<ApiHandle*>(native_instance);
    RawObject object = handle->asObjectNoImmediate();
    bool alive = handle->refcnt() > 1 ||
                 !isWhiteObject(scavenger, HeapObject::cast(object));
    visitor->visitPointer(&object, PointerKind::kApiHandle);
    handle->reference_ = reinterpret_cast<uintptr_t>(object.raw());

    // TODO(T58548736): Run safe dealloc slots here when possible rather than
    // putting everything on the queue.
    if (!alive) {
      NativeProxy::enqueue(object, runtime->finalizableReferences());
    }
  }
}

}  // namespace py
