#include "scavenger.h"

#include <cstring>
#include <memory>

#include "capi.h"
#include "runtime.h"

namespace py {

class Scavenger : public PointerVisitor {
 public:
  explicit Scavenger(Runtime* runtime);

  bool isWhiteObject(RawHeapObject object);

  RawObject scavenge();

  void visitPointer(RawObject* pointer, PointerKind kind) override;

 private:
  void scavengePointer(RawObject* pointer);

  RawObject transport(RawObject old_object);

  void processImmortalRoots(Space*);

  void processDelayedReferences();

  void processGrayObjects();

  void processLayouts();

  void compactLayoutTypeTransitions();

  Runtime* runtime_;
  Heap* heap_;
  Space* from_;
  std::unique_ptr<Space> to_;
  uword scan_;
  RawMutableTuple layouts_;
  RawMutableTuple layout_type_transitions_;
  RawObject delayed_references_;
  RawObject delayed_callbacks_;
};

Scavenger::Scavenger(Runtime* runtime)
    : runtime_(runtime),
      heap_(runtime->heap()),
      from_(heap_->space()),
      to_(nullptr),
      layouts_(MutableTuple::cast(runtime->layouts())),
      layout_type_transitions_(
          MutableTuple::cast(runtime->layoutTypeTransitions())),
      delayed_references_(NoneType::object()),
      delayed_callbacks_(NoneType::object()) {}

RawObject Scavenger::scavenge() {
  DCHECK(heap_->verify(), "Heap failed to verify before GC");
  to_.reset(new Space(from_->size()));
  scan_ = to_->start();
  // Nothing should be allocating during a GC.
  heap_->setSpace(nullptr);
  if (Space* immortal = heap_->immortal()) {
    processImmortalRoots(immortal);
  }
  runtime_->visitRootsWithoutApiHandles(this);
  visitIncrementedApiHandles(runtime_, this);
  processGrayObjects();
  visitExtensionObjects(runtime_, this, this);
  processGrayObjects();
  visitNotIncrementedBorrowedApiHandles(runtime_, this, this);
  processGrayObjects();
  processDelayedReferences();
  processLayouts();
  heap_->setSpace(to_.release());
  DCHECK(heap_->verify(), "Heap failed to verify after GC");
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
    DCHECK(
        to_->contains(object.address()) || heap_->isImmortal(object.address()),
        "object must be in 'from' or 'to' or 'immortal'space");
  } else if (object.isForwarding()) {
    DCHECK(to_->contains(HeapObject::cast(object.forward()).address()),
           "transported object must be located in 'to' space");
    *pointer = object.forward();
  } else {
    *pointer = transport(object);
  }
}

void Scavenger::processImmortalRoots(Space* immortal) {
  uword scan = immortal->start();
  while (scan < immortal->fill()) {
    if (!(*reinterpret_cast<RawObject*>(scan)).isHeader()) {
      // Skip immediate values for alignment padding or header overflow.
      scan += kPointerSize;
    } else {
      RawHeapObject object = HeapObject::fromAddress(scan + RawHeader::kSize);
      uword end = object.baseAddress() + object.size();
      if (!object.isRoot()) {
        scan = end;
      } else {
        for (scan += RawHeader::kSize; scan < end; scan += kPointerSize) {
          auto pointer = reinterpret_cast<RawObject*>(scan);
          scavengePointer(pointer);
        }
      }
    }
  }
}

bool Scavenger::isWhiteObject(RawHeapObject object) {
  DCHECK(!to_->contains(object.address()),
         "must not test objects that have already been visited");
  return !object.isForwarding();
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
      if (object.isWeakRef()) {
        RawWeakRef weakref = WeakRef::cast(object);
        RawObject referent = weakref.referent();
        if (!referent.isNoneType() &&
            isWhiteObject(HeapObject::cast(referent))) {
          // Delay the reference object for later processing.
          WeakRef::enqueue(object, &delayed_references_);
          // Skip over the referent field and continue scavenging.
          scan += kPointerSize;
        }
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
    if (layout == SmallInt::fromWord(0)) continue;
    RawHeapObject heap_obj = HeapObject::cast(layout);
    if (to_->contains(heap_obj.address())) continue;
    if (heap_->isImmortal(heap_obj.address())) continue;

    if (heap_obj.isForwarding()) {
      DCHECK(heap_obj.forward().isLayout(), "Bad Layout forwarded value");
      layouts_.atPut(i, heap_obj.forward());
    } else {
      layouts_.atPut(i, SmallInt::fromWord(0));
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
    if (from_obj == SmallInt::fromWord(0)) continue;

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
                                     SmallInt::fromWord(0));
      layout_type_transitions_.atPut(i + LayoutTypeTransition::kTo,
                                     SmallInt::fromWord(0));
      layout_type_transitions_.atPut(i + LayoutTypeTransition::kResult,
                                     SmallInt::fromWord(0));
    }
  }

  compactLayoutTypeTransitions();
  runtime_->setLayoutTypeTransitions(transport(layout_type_transitions_));
}

static inline word getLeftMostNoneObjectIndex(RawTuple layout_type_transitions,
                                              word left, word right) {
  while (left < right &&
         layout_type_transitions.at(left + LayoutTypeTransition::kFrom) !=
             SmallInt::fromWord(0)) {
    left += LayoutTypeTransition::kTransitionSize;
  }
  return left;
}

static inline word getRightMostValidObjectIndex(
    RawTuple layout_type_transitions, word left, word right) {
  while (left < right &&
         layout_type_transitions.at(right + LayoutTypeTransition::kFrom) ==
             SmallInt::fromWord(0)) {
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

RawObject Scavenger::transport(RawObject old_object) {
  RawHeapObject from_object = HeapObject::cast(old_object);
  if (heap_->isImmortal(from_object.address())) {
    return old_object;
  }
  DCHECK(from_->contains(from_object.address()),
         "objects must be transported from 'from' space");
  DCHECK(from_object.header().isHeader(),
         "object must have a header and must not forward");
  word size = from_object.size();
  uword address = 0;
  to_->allocate(size, &address);
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

bool isWhiteObject(Scavenger* scavenger, RawHeapObject object) {
  return scavenger->isWhiteObject(object);
}

RawObject scavenge(Runtime* runtime) { return Scavenger(runtime).scavenge(); }

void scavengeImmortalize(Runtime* runtime) {
  Heap* heap = runtime->heap();
  heap->makeImmortal();
  DCHECK(heap->verify(), "Heap OK");
}

}  // namespace py
