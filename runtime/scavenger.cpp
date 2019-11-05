#include "scavenger.h"

#include <cstring>

#include "capi-handles.h"
#include "frame.h"
#include "interpreter.h"
#include "runtime.h"
#include "thread.h"

namespace py {

void ScavengeVisitor::visitPointer(RawObject* pointer) {
  scavenger_->scavengePointer(pointer);
}

Scavenger::Scavenger(Runtime* runtime)
    : visitor_(this),
      runtime_(runtime),
      from_(runtime->heap()->space()),
      to_(nullptr),
      delayed_references_(NoneType::object()),
      delayed_callbacks_(NoneType::object()) {}

Scavenger::~Scavenger() {}

RawObject Scavenger::scavenge() {
  to_ = new Space(from_->size());
  processRoots();
  processGrayObjects();
  processApiHandles();
  processFinalizableReferences();
  processDelayedReferences();
  runtime_->heap()->setSpace(to_);
  delete from_;
  return delayed_callbacks_;
}

void Scavenger::scavengePointer(RawObject* pointer) {
  if (!(*pointer).isHeapObject()) {
    return;
  }
  RawHeapObject object = HeapObject::cast(*pointer);
  if (from_->contains(object.address())) {
    if (object.isForwarding()) {
      *pointer = object.forward();
    } else {
      *pointer = transport(object);
    }
  }
}

void Scavenger::processRoots() { runtime_->visitRoots(visitor()); }

bool Scavenger::hasWhiteReferent(RawObject reference) {
  RawWeakRef weak = WeakRef::cast(reference);
  if (!weak.referent().isHeapObject()) {
    return false;
  }
  return !HeapObject::cast(weak.referent()).isForwarding();
}

void Scavenger::processGrayObjects() {
  uword scan = to_->start();
  while (scan < to_->fill()) {
    if (!(*reinterpret_cast<RawObject*>(scan)).isHeader()) {
      // Skip immediate values for alignment padding or header overflow.
      scan += kPointerSize;
    } else {
      RawHeapObject object = HeapObject::fromAddress(scan + RawHeader::kSize);
      uword end = object.baseAddress() + object.size();
      // Scan pointers that follow the header word, if any.
      if (!object.isRoot()) {
        scan = end;
        continue;
      }
      scan += RawHeader::kSize;
      if (object.isWeakRef() && hasWhiteReferent(object)) {
        // Delay the reference object for later processing.
        WeakRef::enqueue(object, &delayed_references_);
        // Skip over the referent field and continue scavenging.
        scan += kPointerSize;
      }
      for (; scan < end; scan += kPointerSize) {
        scavengePointer(reinterpret_cast<RawObject*>(scan));
      }
    }
  }
}

void Scavenger::processApiHandles() {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Dict handles(&scope, runtime_->apiHandles());
  Tuple handle_data(&scope, handles.data());
  Dict caches(&scope, runtime_->apiCaches());
  Object key(&scope, NoneType::object());
  Object cache_value(&scope, NoneType::object());
  // Loops through the handle table clearing out handles which are not
  // referenced by managed objects or by an extension object.
  for (word i = Dict::Bucket::kFirst;
       Dict::Bucket::nextItem(*handle_data, &i);) {
    RawObject value = Dict::Bucket::value(*handle_data, i);
    ApiHandle* handle = static_cast<ApiHandle*>(Int::cast(value).asCPtr());
    if (ApiHandle::nativeRefcnt(handle) == 0) {
      key = Dict::Bucket::key(*handle_data, i);
      word hash = runtime_->hash(*key);
      // TODO(T56760343): Remove the dict lookup. This should become simpler
      // when it is easier to associate a cache with a handle or when the need
      // for caches is eliminated.
      cache_value =
          ApiHandle::dictRemoveIdentityEquals(thread, caches, key, hash);
      if (!cache_value.isError()) {
        std::free(Int::cast(*cache_value).asCPtr());
      }
      Dict::Bucket::setTombstone(*handle_data, i);
      handles.setNumItems(handles.numItems() - 1);
      handle->type()->decref();
      std::free(handle);
    }
  }
}

// Process the list of weakrefs that had otherwise-unreachable referents during
// processGrayObjects().
//
// If the referent had one or more strong references after all, the weakref is
// updated to point to the relocated object. Otherwise, the weakref's referent
// field is set to None and its callback (if present) is enqueued for running
// later.
void Scavenger::processDelayedReferences() {
  while (delayed_references_ != NoneType::object()) {
    RawWeakRef weak = WeakRef::cast(WeakRef::dequeue(&delayed_references_));
    if (!weak.referent().isHeapObject()) {
      continue;
    }
    RawHeapObject referent = HeapObject::cast(weak.referent());
    if (referent.isForwarding()) {
      weak.setReferent(referent.forward());
    } else {
      weak.setReferent(NoneType::object());
      if (!weak.callback().isNoneType()) {
        WeakRef::enqueue(weak, &delayed_callbacks_);
      }
    }
  }
}

void Scavenger::processFinalizableReferences() {
  for (ListEntry *entry = runtime_->trackedNativeObjects(), *next;
       entry != nullptr; entry = next) {
    next = entry->next;
    ApiHandle* native_instance = reinterpret_cast<ApiHandle*>(entry + 1);
    RawObject native_proxy = native_instance->asObject();
    if (native_instance->refcnt() > 1 ||
        HeapObject::cast(native_proxy).isForwarding()) {
      // The extension object is being kept alive by a reference from an
      // extension object or by a managed reference. Blacken the reference.
      scavengePointer(
          reinterpret_cast<RawObject*>(&native_instance->reference_));
      continue;
    }

    // Deallocate immediately or add to finalization queue
    RawType type = Type::cast(runtime_->typeOf(native_instance->asObject()));
    if (!type.hasFlag(Type::Flag::kHasDefaultDealloc)) {
      scavengePointer(
          reinterpret_cast<RawObject*>(&native_instance->reference_));
      RawNativeProxy::enqueue(native_instance->asObject(),
                              runtime_->finalizableReferences());
      continue;
    }
    auto func = reinterpret_cast<destructor>(
        Int::cast(type.slot(Type::Slot::kDealloc)).asWord());
    (*func)(native_instance);
  }
  // TODO(T55208267): Merge GC extension instances to the native extension list
  for (ListEntry *entry = runtime_->trackedNativeGcObjects(), *next;
       entry != nullptr; entry = next) {
    next = entry->next;
    ApiHandle* native_instance = reinterpret_cast<ApiHandle*>(entry + 1);
    RawObject native_proxy = native_instance->asObject();
    if (native_instance->refcnt() > 1 ||
        HeapObject::cast(native_proxy).isForwarding()) {
      // The extension object is being kept alive by a reference from an
      // extension object or by a managed reference. Blacken the reference.
      scavengePointer(
          reinterpret_cast<RawObject*>(&native_instance->reference_));
      continue;
    }

    // Deallocate immediately or add to finalization queue
    RawType type = Type::cast(runtime_->typeOf(native_instance->asObject()));
    if (!type.hasFlag(Type::Flag::kHasDefaultDealloc)) {
      scavengePointer(
          reinterpret_cast<RawObject*>(&native_instance->reference_));
      RawNativeProxy::enqueue(native_instance->asObject(),
                              runtime_->finalizableReferences());
      continue;
    }
    auto func = reinterpret_cast<destructor>(
        Int::cast(type.slot(Type::Slot::kDealloc)).asWord());
    (*func)(native_instance);
  }
}

RawObject Scavenger::transport(RawObject old_object) {
  RawHeapObject from_object = HeapObject::cast(old_object);
  word size = from_object.size();
  uword address = to_->allocate(size);
  auto dst = reinterpret_cast<void*>(address);
  auto src = reinterpret_cast<void*>(from_object.baseAddress());
  std::memcpy(dst, src, size);
  word offset = from_object.address() - from_object.baseAddress();
  RawHeapObject to_object = HeapObject::fromAddress(address + offset);
  from_object.forwardTo(to_object);
  return to_object;
}

}  // namespace py
