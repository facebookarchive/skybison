#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinObjectInit(Thread* thread, Frame* caller, word nargs);
Object* builtinObjectNew(Thread* thread, Frame* caller, word nargs);

}  // namespace python
