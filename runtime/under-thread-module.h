#pragma once

#include "handles-decl.h"

namespace py {

class Thread;

void initializeUnderThreadModule(Thread* thread, const Module& module);

}  // namespace py
