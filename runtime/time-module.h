#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"

namespace python {

RawObject builtinTime(Thread* thread, Frame* frame, word nargs);

}  // namespace python
