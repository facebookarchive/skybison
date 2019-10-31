#pragma once

#include "handles.h"
#include "objects.h"
#include "thread.h"

namespace py {

// Computes the complete MRO for the type's ancestors via C3 linearization
// based on the types bases.
RawObject computeMro(Thread* thread, const Type& type);

}  // namespace py
