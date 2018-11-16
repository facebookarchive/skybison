#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"

namespace python {

Object* builtinSysExit(Thread* thread, Frame* frame, word nargs);

} // namespace python
