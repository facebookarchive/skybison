#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinDoubleAdd(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleEq(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleGe(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleGt(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleLe(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleLt(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleNe(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleNew(Thread* thread, Frame* frame, word nargs);
Object* builtinDoublePow(Thread* thread, Frame* frame, word nargs);
Object* builtinDoubleSub(Thread* thread, Frame* frame, word nargs);

}  // namespace python
