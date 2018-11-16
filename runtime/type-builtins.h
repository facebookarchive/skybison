#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

RawObject builtinTypeCall(Thread* thread, Frame* caller, word nargs);
RawObject builtinTypeCallKw(Thread* thread, Frame* caller, word nargs);
RawObject builtinTypeNew(Thread* thread, Frame* frame, word nargs);
RawObject builtinTypeInit(Thread* thread, Frame* frame, word nargs);
RawObject builtinTypeRepr(Thread* thread, Frame* frame, word nargs);

}  // namespace python
