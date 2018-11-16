#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinSmallIntegerBool(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerEq(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerGe(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerGt(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerInvert(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerLe(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerLt(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerNe(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerNeg(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerPos(Thread* thread, Frame* frame, word nargs);
Object* builtinSmallIntegerSub(Thread* thread, Frame* frame, word nargs);

} // namespace python
