#include "typeslots.h"

#include "cpython-data.h"

#include "capi.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"

namespace py {

const int kFirstSlot = Py_bf_getbuffer;
const int kLastSlot = Py_tp_finalize;
static_assert(kSlotFlags < kFirstSlot && kSlotBasicSize < kFirstSlot &&
                  kSlotItemSize < kFirstSlot,
              "slot indexes must not overlap with internal slots");

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
  word length = kNumInternalSlots + kLastSlot + 1;
  MutableTuple slots(&scope, thread->runtime()->newMutableTuple(length));
  slots.fill(SmallInt::fromWord(0));
  type.setSlots(*slots);
}

bool typeHasSlots(const Type& type) { return !type.slots().isNoneType(); }

void* typeSlotAt(const Type& type, int slot_id) {
  DCHECK(isValidSlotId(slot_id) && !isObjectSlotId(slot_id), "invalid slot id");
  return Int::cast(MutableTuple::cast(type.slots()).at(kSlotOffset + slot_id))
      .asCPtr();
}

void typeSlotAtPut(Thread* thread, const Type& type, int slot_id, void* value) {
  DCHECK(isValidSlotId(slot_id) && !isObjectSlotId(slot_id), "invalid slot id");
  MutableTuple::cast(type.slots())
      .atPut(kSlotOffset + slot_id, thread->runtime()->newIntFromCPtr(value));
}

RawObject typeSlotObjectAt(const Type& type, int slot_id) {
  DCHECK(isObjectSlotId(slot_id), "invalid slot id");
  return MutableTuple::cast(type.slots()).at(kSlotOffset + slot_id);
}

void typeSlotObjectAtPut(const Type& type, int slot_id, RawObject value) {
  DCHECK(isObjectSlotId(slot_id), "invalid slot id");
  MutableTuple::cast(type.slots()).atPut(kSlotOffset + slot_id, value);
}

uword typeSlotUWordAt(const Type& type, int slot_id) {
  DCHECK(isInternalSlotId(slot_id), "expected internal slot");
  return Int::cast(MutableTuple::cast(type.slots()).at(kSlotOffset + slot_id))
      .asInt<uword>()
      .value;
}

void typeSlotUWordAtPut(Thread* thread, const Type& type, int slot_id,
                        uword value) {
  DCHECK(isInternalSlotId(slot_id), "expected internal slot");
  MutableTuple::cast(type.slots())
      .atPut(kSlotOffset + slot_id,
             thread->runtime()->newIntFromUnsigned(value));
}

uword typeGetBasicSize(const Type& type) {
  return typeSlotUWordAt(type, kSlotBasicSize);
}

uword typeGetFlags(const Type& type) {
  if (typeHasSlots(type)) {
    return typeSlotUWordAt(type, kSlotFlags);
  }
  uword result = Py_TPFLAGS_READY;
  // TODO(T71637829): Check if the type allows subclassing and set
  // Py_TPFLAGS_BASETYPE appropriately.
  Type::Flag internal_flags = type.flags();
  if (internal_flags & Type::Flag::kHasCycleGC) {
    result |= Py_TPFLAGS_HAVE_GC;
  }
  if (internal_flags & Type::Flag::kIsAbstract) {
    result |= Py_TPFLAGS_IS_ABSTRACT;
  }
  if (internal_flags & Type::Flag::kIsCPythonHeaptype) {
    result |= Py_TPFLAGS_HEAPTYPE;
  }
  if (internal_flags & Type::Flag::kIsBasetype) {
    result |= Py_TPFLAGS_BASETYPE;
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
