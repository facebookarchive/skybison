#pragma once

#include "handles.h"

namespace py {

RawObject structseqNew(Thread* thread, const Type& type);

RawObject structseqNewType(Thread* thread, const Str& name,
                           const Tuple& field_names, word num_in_sequence);

}  // namespace py
