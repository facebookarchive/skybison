#pragma once

#include "globals.h"
#include "objects.h"

namespace py {

class Thread;

word arrayByteLength(RawArray array);

void initializeArrayType(Thread* thread);

}  // namespace py
