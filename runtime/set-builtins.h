#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinSetAdd(Thread* thread, Frame* frame, word nargs);
Object* builtinSetContains(Thread* thread, Frame* frame, word nargs);
Object* builtinSetInit(Thread* thread, Frame* frame, word nargs);
Object* builtinSetLen(Thread* thread, Frame* frame, word nargs);
Object* builtinSetNew(Thread* thread, Frame* frame, word nargs);
Object* builtinSetPop(Thread* thread, Frame* frame, word nargs);

}  // namespace python
