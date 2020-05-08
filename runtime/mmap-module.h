#pragma once

#include "handles-decl.h"

namespace py {

class Thread;

void initializeMmapModule(Thread* thread, const Module& module);

void initializeMmapType(Thread* thread);

}  // namespace py
