#include "capi-typeslots.h"

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

}  // namespace py
