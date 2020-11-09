#include "scavenger.h"

#include <cstring>

#include "capi-handles.h"
#include "runtime.h"

namespace py {

class Scavenger : public PointerVisitor {
 public:
  explicit Scavenger(Runtime* runtime);

  RawObject scavenge();

  void visitPointer(RawObject* pointer, PointerKind kind) override;

 private:
  void scavengePointer(RawObject* pointer);

  RawObject transport(RawObject old_object);

  bool hasWhiteReferent(RawObject reference);

  void processDelayedReferences();

  void processFinalizableReferences();

  void processGrayObjects();

  void processRoots();

  void processLayouts();

  void compactLayoutTypeTransitions();

  Runtime* runtime_;
  Space* from_;
  Space* to_;
  uword scan_;
  RawMutableTuple layouts_;
  RawMutableTuple layout_type_transitions_;
  RawObject delayed_references_;
  RawObject delayed_callbacks_;
};

Scavenger::Scavenger(Runtime* runtime)
    : runtime_(runtime),
      from_(runtime->heap()->space()),
      to_(nullptr),
      layouts_(MutableTuple::cast(runtime->layouts())),
      layout_type_transitions_(
          MutableTuple::cast(runtime->layoutTypeTransitions())),
      delayed_references_(NoneType::object()),
      delayed_callbacks_(NoneType::object()) {}

RawObject Scavenger::scavenge() {
  DCHECK(runtime_->heap()->verify(), "Heap failed to verify before GC");
  to_ = new Space(from_->size());
  scan_ = to_->start();
  // Nothing should be allocating during a GC.
  runtime_->heap()->setSpace(nullptr);
  processRoots();
  processGrayObjects();
  ApiHandle::clearNotReferencedHandles(Thread::current());
  processFinalizableReferences();
  processGrayObjects();
  processDelayedReferences();
  processLayouts();
  runtime_->heap()->setSpace(to_);
  DCHECK(runtime_->heap()->verify(), "Heap failed to verify after GC");
  delete from_;
  return delayed_callbacks_;
}

void Scavenger::visitPointer(RawObject* pointer, PointerKind) {
  scavengePointer(pointer);
}

void Scavenger::scavengePointer(RawObject* pointer) {
  if (!(*pointer).isHeapObject()) {
    return;
  }
  RawHeapObject object = HeapObject::cast(*pointer);
  if (!from_->contains(object.address())) {
    DCHECK(object.header().isHeader(), "object must have a header");
    DCHECK(to_->contains(object.address()),
           "object must be in 'from' or 'to' space");
  } else if (object.isForwarding()) {
    DCHECK(to_->contains(HeapObject::cast(object.forward()).address()),
           "transported object must be located in 'to' space");
    *pointer = object.forward();
  } else {
    *pointer = transport(object);
  }
}

void Scavenger::processRoots() { runtime_->visitRoots(this); }

bool Scavenger::hasWhiteReferent(RawObject reference) {
  RawWeakRef weak = WeakRef::cast(reference);
  if (!weak.referent().isHeapObject()) {
    return false;
  }
  return !HeapObject::cast(weak.referent()).isForwarding();
}

void Scavenger::processGrayObjects() {
  uword scan = scan_;
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
  scan_ = scan;
}

// Do a final pass through the Layouts Tuple, treating all non-builtin entries
// as weak roots.
void Scavenger::processLayouts() {
  for (word i = static_cast<word>(LayoutId::kLastBuiltinId) + 1,
            end = layouts_.length();
       i < end; ++i) {
    RawObject layout = layouts_.at(i);
    if (layout.isNoneType()) continue;
    RawHeapObject heap_obj = HeapObject::cast(layout);
    if (to_->contains(heap_obj.address())) continue;

    if (heap_obj.isForwarding()) {
      DCHECK(heap_obj.forward().isLayout(), "Bad Layout forwarded value");
      layouts_.atPut(i, heap_obj.forward());
    } else {
      layouts_.atPut(i, NoneType::object());
    }
  }

  // TODO(T59281894): We can skip this step if the Layouts table doesn't live
  // in the managed heap.
  runtime_->setLayouts(transport(layouts_));

  // Remove dead empty entries (triples (A, B, C) where either A or C is dead).
  // Post-condition: all entries in the tuple will either be references to
  // to-space or None.
  word length = layout_type_transitions_.length();
  DCHECK(!to_->contains(layout_type_transitions_.address()),
         "object should not have been moved");
  for (word i = 0; i < length; i += LayoutTypeTransition::kTransitionSize) {
    RawObject from_obj =
        layout_type_transitions_.at(i + LayoutTypeTransition::kFrom);
    if (from_obj.isNoneType()) continue;

    RawHeapObject from = HeapObject::cast(from_obj);
    RawHeapObject to = HeapObject::cast(
        layout_type_transitions_.at(i + LayoutTypeTransition::kTo));
    RawHeapObject result = HeapObject::cast(
        layout_type_transitions_.at(i + LayoutTypeTransition::kResult));
    DCHECK(!to_->contains(to.address()),
           "reference should not have been moved");
    DCHECK(!to_->contains(from.address()),
           "reference should not have been moved");
    DCHECK(!to_->contains(result.address()),
           "reference should not have been moved");
    if (from.isForwarding() && result.isForwarding()) {
      layout_type_transitions_.atPut(i + LayoutTypeTransition::kFrom,
                                     from.forward());
      layout_type_transitions_.atPut(i + LayoutTypeTransition::kTo,
                                     to.forward());
      layout_type_transitions_.atPut(i + LayoutTypeTransition::kResult,
                                     result.forward());
    } else {
      // Remove the transition edge of the from or result layouts have been
      // collected.
      layout_type_transitions_.atPut(i + LayoutTypeTransition::kFrom,
                                     NoneType::object());
      layout_type_transitions_.atPut(i + LayoutTypeTransition::kTo,
                                     NoneType::object());
      layout_type_transitions_.atPut(i + LayoutTypeTransition::kResult,
                                     NoneType::object());
    }
  }

  compactLayoutTypeTransitions();
  runtime_->setLayoutTypeTransitions(transport(layout_type_transitions_));
}

static inline word getLeftMostNoneObjectIndex(RawTuple layout_type_transitions,
                                              word left, word right) {
  while (left < right &&
         !layout_type_transitions.at(left + LayoutTypeTransition::kFrom)
              .isNoneType()) {
    left += LayoutTypeTransition::kTransitionSize;
  }
  return left;
}

static inline word getRightMostValidObjectIndex(
    RawTuple layout_type_transitions, word left, word right) {
  while (left < right &&
         layout_type_transitions.at(right + LayoutTypeTransition::kFrom)
             .isNoneType()) {
    right -= LayoutTypeTransition::kTransitionSize;
  }
  return right;
}

static inline void swapTriplesInLayoutTypeTransitions(
    RawMutableTuple layout_type_transitions, word left, word right) {
  layout_type_transitions.swap(left + LayoutTypeTransition::kFrom,
                               right + LayoutTypeTransition::kFrom);
  layout_type_transitions.swap(left + LayoutTypeTransition::kTo,
                               right + LayoutTypeTransition::kTo);
  layout_type_transitions.swap(left + LayoutTypeTransition::kResult,
                               right + LayoutTypeTransition::kResult);
}

// applies compaction to layout_type_transitions
// moves all valid triplets to front, and all None-Type triplets to end
void Scavenger::compactLayoutTypeTransitions() {
  word length = layout_type_transitions_.length();
  length = (length / LayoutTypeTransition::kTransitionSize) *
           LayoutTypeTransition::kTransitionSize;

  word left = 0;
  word right = length - LayoutTypeTransition::kTransitionSize;

  do {
    left = getLeftMostNoneObjectIndex(layout_type_transitions_, left, right);
    right = getRightMostValidObjectIndex(layout_type_transitions_, left, right);

    swapTriplesInLayoutTypeTransitions(layout_type_transitions_, left, right);
  } while (left < right);
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
  ListEntry* entry = runtime_->trackedNativeObjects();
  for (ListEntry* next; entry != nullptr; entry = next) {
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

    // TODO(T58548736): Run safe dealloc slots here when possible rather than
    // putting everything on the queue.
    scavengePointer(reinterpret_cast<RawObject*>(&native_instance->reference_));
    RawNativeProxy::enqueue(native_instance->asObject(),
                            runtime_->finalizableReferences());
  }
}

RawObject Scavenger::transport(RawObject old_object) {
  RawHeapObject from_object = HeapObject::cast(old_object);
  DCHECK(from_->contains(from_object.address()),
         "objects must be transported from 'from' space");
  DCHECK(from_object.header().isHeader(),
         "object must have a header and must not forward");
  word size = from_object.size();
  uword address = to_->allocate(size);
  auto dst = reinterpret_cast<void*>(address);
  auto src = reinterpret_cast<void*>(from_object.baseAddress());
  std::memcpy(dst, src, size);
  word offset = from_object.address() - from_object.baseAddress();
  RawHeapObject to_object = HeapObject::fromAddress(address + offset);
  from_object.forwardTo(to_object);

  LayoutId layout_id = to_object.layoutId();
  auto layout_ptr = reinterpret_cast<RawObject*>(
      layouts_.address() + static_cast<word>(layout_id) * kPointerSize);
  scavengePointer(layout_ptr);

  return to_object;
}

RawObject scavenge(Runtime* runtime) { return Scavenger(runtime).scavenge(); }

}  // namespace py
