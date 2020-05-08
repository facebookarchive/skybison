#pragma once

#include "handles-decl.h"

namespace py {

class Thread;

void initializeUnderCtypesModule(Thread* thread, const Module& module);

}  // namespace py
