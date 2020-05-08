#pragma once

#include "handles-decl.h"

namespace py {

class Thread;

void initializeUnderBuiltinsModule(Thread* thread, const Module& module);

}  // namespace py
