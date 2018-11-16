#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinBooldunderBool(Thread* thread, Frame* frame, word nargs);
Object* builtinBoolNew(Thread* thread, Frame* frame, word nargs);

}  // namespace python
