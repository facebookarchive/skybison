#pragma once

#include "handles-decl.h"

namespace py {

class Thread;

void initializeUnderWeakrefModule(Thread* thread, const Module& module);

}  // namespace py
