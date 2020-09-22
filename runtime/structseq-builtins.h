#pragma once

#include "handles.h"

namespace py {

RawObject structseqGetItem(Thread* thread, const Object& structseq, word index);

RawObject structseqSetItem(Thread* thread, const Object& structseq, word index,
                           const Object& value);

RawObject structseqNew(Thread* thread, const Type& type);

RawObject structseqNewType(Thread* thread, const Str& name,
                           const Tuple& field_names, word num_in_sequence,
                           word flags);

}  // namespace py
