#include "scavenger.h"

#include <cstring>

#include "runtime.h"

namespace python {

void ScavengeVisitor::visitPointer(Object** pointer) {
  scavenger_->scavengePointer(pointer);
}

Scavenger::Scavenger(Runtime* runtime)
    : visitor_(this),
      runtime_(runtime),
      from_(runtime->heap()->space()),
      to_(nullptr),
      delayed_references_(None::object()) {}

Scavenger::~Scavenger() {}

void Scavenger::scavenge() {
  to_ = new Space(from_->size());
  processRoots();
  processGrayObjects();
  processDelayedReferences();
  runtime_->heap()->setSpace(to_);
  delete from_;
}

void Scavenger::scavengePointer(Object** pointer) {
  if (!(*pointer)->isHeapObject()) {
    return;
  }
  HeapObject* object = HeapObject::cast(*pointer);
  if (from_->contains(object->address())) {
    if (object->isForwarding()) {
      *pointer = object->forward();
    } else {
      *pointer = transport(object);
    }
  }
}

void Scavenger::processRoots() {
  runtime_->visitRoots(visitor());
}

bool Scavenger::hasWhiteReferent(Object* reference) {
  WeakRef* weak = WeakRef::cast(reference);
  if (!weak->referent()->isHeapObject()) {
    return false;
  }
  return !HeapObject::cast(weak->referent())->isForwarding();
}

void Scavenger::delayReference(Object* reference) {
  DCHECK(hasWhiteReferent(reference), "referent is not white");
  WeakRef::cast(reference)->setLink(delayed_references_);
  delayed_references_ = reference;
}

void Scavenger::processGrayObjects() {
  uword scan = to_->start();
  while (scan < to_->fill()) {
    if (!(*reinterpret_cast<Object**>(scan))->isHeader()) {
      // Skip immediate values for alignment padding or header overflow.
      scan += kPointerSize;
    } else {
      HeapObject* object = HeapObject::fromAddress(scan + Header::kSize);
      uword end = object->baseAddress() + object->size();
      // Scan pointers that follow the header word, if any.
      if (!object->isRoot()) {
        scan = end;
        continue;
      }
      scan += Header::kSize;
      if (object->isWeakRef() && hasWhiteReferent(object)) {
        // Delay the reference object for later processing.
        delayReference(object);
        // Skip over the referent field and continue scavenging.
        scan += kPointerSize;
      }
      for (; scan < end; scan += kPointerSize) {
        scavengePointer(reinterpret_cast<Object**>(scan));
      }
    }
  }
}

void Scavenger::processDelayedReferences() {
  while (delayed_references_ != None::object()) {
    WeakRef* weak = WeakRef::cast(delayed_references_);
    delayed_references_ = weak->link();
    weak->setLink(None::object());
    if (!weak->referent()->isHeapObject()) {
      continue;
    }
    HeapObject* referent = HeapObject::cast(weak->referent());
    if (referent->isForwarding()) {
      weak->setReferent(referent->forward());
    } else {
      weak->setReferent(None::object());
      // TODO: queue the object for invocation of its callback
      if (!weak->callback()->isNone()) {
        UNIMPLEMENTED("weak reference callbacks");
      }
    }
  }
}

Object* Scavenger::transport(Object* old_object) {
  HeapObject* from_object = HeapObject::cast(old_object);
  word size = from_object->size();
  uword address = to_->allocate(size);
  auto dst = reinterpret_cast<void*>(address);
  auto src = reinterpret_cast<void*>(from_object->baseAddress());
  std::memcpy(dst, src, size);
  word offset = from_object->address() - from_object->baseAddress();
  HeapObject* to_object = HeapObject::fromAddress(address + offset);
  from_object->forwardTo(to_object);
  return to_object;
}

} // namespace python
