#pragma once

#include "handles-decl.h"

namespace py {

const int kSlotFlags = -2;
const int kSlotBasicSize = -1;
const int kSlotItemSize = 0;
const int kNumInternalSlots = 3;
const int kSlotOffset = kNumInternalSlots - 1;

class Thread;

bool isValidSlotId(int slot_id);
bool isObjectSlotId(int slot_id);

void typeSlotsAllocate(Thread* thread, const Type& type);

void* typeSlotAt(const Type& type, int slot_id);
RawObject typeSlotObjectAt(const Type& type, int slot_id);
uword typeSlotUWordAt(const Type& type, int slot_id);

void typeSlotAtPut(Thread* thread, const Type& type, int slot_id, void* value);
void typeSlotObjectAtPut(const Type& type, int slot_id, RawObject value);
void typeSlotUWordAtPut(Thread* thread, const Type& type, int slot_id,
                        uword value);

}  // namespace py
