#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinDictEq(Thread* thread, Frame* frame, word nargs);
Object* builtinDictGetItem(Thread* thread, Frame* frame, word nargs);
Object* builtinDictLen(Thread* thread, Frame* frame, word nargs);

}  // namespace python
