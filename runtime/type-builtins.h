#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinTypeCall(Thread* thread, Frame* caller, word nargs);
Object* builtinTypeCallKw(Thread* thread, Frame* caller, word nargs);
Object* builtinTypeNew(Thread* thread, Frame* frame, word nargs);
Object* builtinTypeInit(Thread* thread, Frame* frame, word nargs);
Object* builtinTypeRepr(Thread* thread, Frame* frame, word nargs);

}  // namespace python
