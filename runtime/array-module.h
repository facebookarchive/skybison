#pragma once

#include "handles-decl.h"

namespace py {

class Thread;

void initializeArrayModule(Thread* thread, const Module& module);

void initializeArrayType(Thread* thread);

}  // namespace py
