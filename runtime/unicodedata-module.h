#pragma once

#include "handles-decl.h"

namespace py {

class Thread;

void initializeUnicodedataModule(Thread* thread, const Module& module);

}  // namespace py
