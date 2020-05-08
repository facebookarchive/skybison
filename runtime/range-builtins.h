#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

// Returns the same value as Slice::length for SmallInt, but allows LargeInt.
RawObject rangeLen(Thread* thread, const Object& start_obj,
                   const Object& stop_obj, const Object& step_obj);

void initializeRangeTypes(Thread* thread);

}  // namespace py
