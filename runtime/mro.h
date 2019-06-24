#pragma once

#include "handles.h"
#include "objects.h"
#include "thread.h"

namespace python {

// Computes the complete MRO for the type's ancestors via C3 linearization,
// given the list of its immediate parents.
RawObject computeMro(Thread* thread, const Type& type, const Tuple& parents);

}  // namespace python
