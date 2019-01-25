#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"

namespace python {

RawObject builtinSysDisplayhook(Thread* thread, Frame* frame, word nargs);
RawObject builtinSysExit(Thread* thread, Frame* frame, word nargs);
RawObject initialSysPath(Thread* thread);

}  // namespace python
