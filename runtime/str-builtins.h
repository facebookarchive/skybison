#pragma once

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinStrAdd(Thread* thread, Frame* frame, word nargs);
Object* builtinStrEq(Thread* thread, Frame* frame, word nargs);
Object* builtinStrGe(Thread* thread, Frame* frame, word nargs);
Object* builtinStrGetItem(Thread* thread, Frame* frame, word nargs);
Object* builtinStrGt(Thread* thread, Frame* frame, word nargs);
Object* builtinStrJoin(Thread* thread, Frame* frame, word nargs);
Object* builtinStrLe(Thread* thread, Frame* frame, word nargs);
Object* builtinStrLen(Thread* thread, Frame* frame, word nargs);
Object* builtinStrLower(Thread* thread, Frame* frame, word nargs);
Object* builtinStrLt(Thread* thread, Frame* frame, word nargs);
Object* builtinStrMod(Thread* thread, Frame* caller, word nargs);
Object* builtinStrNe(Thread* thread, Frame* frame, word nargs);
Object* builtinStrNew(Thread* thread, Frame* frame, word nargs);
Object* builtinStrRepr(Thread* thread, Frame* frame, word nargs);

}  // namespace python
