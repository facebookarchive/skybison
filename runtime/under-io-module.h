#pragma once

#include "handles-decl.h"

namespace py {

class Thread;

void initializeUnderIOModule(Thread* thread, const Module& module);

void initializeUnderIOTypes(Thread* thread);

}  // namespace py
