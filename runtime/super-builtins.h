#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinSuperInit(Thread* thread, Frame* frame, word nargs);
Object* builtinSuperNew(Thread* thread, Frame* frame, word nargs);

} // namespace python
