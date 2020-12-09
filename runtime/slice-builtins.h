#pragma once

#include "runtime.h"

namespace py {

void initializeSliceType(Thread* thread);

// Attempts to unpack a possibly-slice key. Returns true and sets start, stop if
// key is a slice with None step and None/SmallInt start and stop. The start and
// stop values must still be adjusted for the container's length. Returns false
// if key is not a slice or if the slice bounds are not the common types.
bool tryUnpackSlice(const RawObject& key, word* start, word* stop);

}  // namespace py
