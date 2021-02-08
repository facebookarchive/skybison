#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

void initializeDescriptorTypes(Thread* thread);

RawObject slotDescriptorGet(Thread* thread,
                            const SlotDescriptor& slot_descriptor,
                            const Object& instance_obj,
                            const Object& owner_obj);

RawObject slotDescriptorSet(Thread* thread,
                            const SlotDescriptor& slot_descriptor,
                            const Object& instance_obj, const Object& value);

}  // namespace py
