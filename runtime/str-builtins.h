#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinStringEq(Thread* thread, Frame* frame, word nargs);
Object* builtinStringGe(Thread* thread, Frame* frame, word nargs);
Object* builtinStringGt(Thread* thread, Frame* frame, word nargs);
Object* builtinStringLe(Thread* thread, Frame* frame, word nargs);
Object* builtinStringLt(Thread* thread, Frame* frame, word nargs);
Object* builtinStringNe(Thread* thread, Frame* frame, word nargs);

}  // namespace python
