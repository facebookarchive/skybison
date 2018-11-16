#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

RawObject builtinFunctionGet(Thread* thread, Frame* frame, word nargs);

}  // namespace python
