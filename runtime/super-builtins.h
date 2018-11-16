#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

RawObject builtinSuperInit(Thread* thread, Frame* frame, word nargs);
RawObject builtinSuperNew(Thread* thread, Frame* frame, word nargs);

}  // namespace python
