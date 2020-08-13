#include "typeslots.h"

#include "cpython-data.h"

#include "capi.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"

namespace py {

const int kFirstSlot = Py_mp_ass_subscript;
const int kLastSlot = Py_tp_finalize;
static_assert(kSlotFlags < kFirstSlot && kSlotBasicSize < kFirstSlot &&
                  kSlotItemSize < kFirstSlot,
              "slot indexes must not overlap");

bool isValidSlotId(int slot_id) {
  return kFirstSlot <= slot_id && slot_id <= kLastSlot;
}

bool isObjectSlotId(int slot_id) {
  switch (slot_id) {
    case Py_tp_base:
    case Py_tp_bases:
      return true;
  }
  return false;
}

static bool isInternalSlotId(int slot_id) {
  switch (slot_id) {
    case kSlotFlags:
    case kSlotBasicSize:
    case kSlotItemSize:
      return true;
  }
  return false;
}

void typeSlotsAllocate(Thread* thread, const Type& type) {
  DCHECK(!typeHasSlots(type), "type must not have slots yet");
  HandleScope scope(thread);
  word length = kLastSlot + 1;
  MutableTuple slots(&scope, thread->runtime()->newMutableTuple(length));
  slots.fill(SmallInt::fromWord(0));
  type.setSlots(*slots);
}

bool typeHasSlots(const Type& type) { return !type.slots().isNoneType(); }

void* typeSlotAt(const Type& type, int slot_id) {
  DCHECK(isValidSlotId(slot_id) && !isObjectSlotId(slot_id), "invalid slot id");
  return Int::cast(MutableTuple::cast(type.slots()).at(slot_id)).asCPtr();
}

void typeSlotAtPut(Thread* thread, const Type& type, int slot_id, void* value) {
  DCHECK(isValidSlotId(slot_id) && !isObjectSlotId(slot_id), "invalid slot id");
  MutableTuple::cast(type.slots())
      .atPut(slot_id, thread->runtime()->newIntFromCPtr(value));
}

RawObject typeSlotObjectAt(const Type& type, int slot_id) {
  DCHECK(isObjectSlotId(slot_id), "invalid slot id");
  return MutableTuple::cast(type.slots()).at(slot_id);
}

void typeSlotObjectAtPut(const Type& type, int slot_id, RawObject value) {
  DCHECK(isObjectSlotId(slot_id), "invalid slot id");
  MutableTuple::cast(type.slots()).atPut(slot_id, value);
}

uword typeSlotUWordAt(const Type& type, int slot_id) {
  DCHECK(isInternalSlotId(slot_id), "expected internal slot");
  return Int::cast(MutableTuple::cast(type.slots()).at(slot_id))
      .asInt<uword>()
      .value;
}

void typeSlotUWordAtPut(Thread* thread, const Type& type, int slot_id,
                        uword value) {
  DCHECK(isInternalSlotId(slot_id), "expected internal slot");
  MutableTuple::cast(type.slots())
      .atPut(slot_id, thread->runtime()->newIntFromUnsigned(value));
}

uword typeGetBasicSize(const Type& type) {
  return typeSlotUWordAt(type, kSlotBasicSize);
}

uword typeGetFlags(const Type& type) {
  if (typeHasSlots(type)) {
    return typeSlotUWordAt(type, kSlotFlags);
  }
  // It is not yet clear whether presenting all Pyro types as heap types is the
  // best way to go about things. While it is true that all of our types are
  // allocated on the heap (as opposed to being static), we have some similar
  // features to CPython static types such as preventing modifications to type
  // dictionaries.
  // Also, our default is different from CPython's default and that's okay.
  static const uword default_flags = Py_TPFLAGS_HEAPTYPE | Py_TPFLAGS_READY;
  uword result = default_flags;
  // TODO(T71637829): Check if the type allows subclassing and set
  // Py_TPFLAGS_BASETYPE appropriately.
  Type::Flag internal_flags = type.flags();
  if (internal_flags & Type::Flag::kHasCycleGC) {
    result |= Py_TPFLAGS_HAVE_GC;
  }
  if (internal_flags & Type::Flag::kIsAbstract) {
    result |= Py_TPFLAGS_IS_ABSTRACT;
  }
  switch (type.builtinBase()) {
    case LayoutId::kInt:
      result |= Py_TPFLAGS_LONG_SUBCLASS;
      break;
    case LayoutId::kList:
      result |= Py_TPFLAGS_LIST_SUBCLASS;
      break;
    case LayoutId::kTuple:
      result |= Py_TPFLAGS_TUPLE_SUBCLASS;
      break;
    case LayoutId::kBytes:
      result |= Py_TPFLAGS_BYTES_SUBCLASS;
      break;
    case LayoutId::kStr:
      result |= Py_TPFLAGS_UNICODE_SUBCLASS;
      break;
    case LayoutId::kDict:
      result |= Py_TPFLAGS_DICT_SUBCLASS;
      break;
    // BaseException is handled separately down below
    case LayoutId::kType:
      result |= Py_TPFLAGS_TYPE_SUBCLASS;
      break;
    default:
      if (type.isBaseExceptionSubclass()) {
        result |= Py_TPFLAGS_BASE_EXC_SUBCLASS;
      }
      break;
  }
  return result;
}

}  // namespace py
