#pragma once

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinStringAdd(Thread* thread, Frame* frame, word nargs);
Object* builtinStringEq(Thread* thread, Frame* frame, word nargs);
Object* builtinStringGe(Thread* thread, Frame* frame, word nargs);
Object* builtinStringGetItem(Thread* thread, Frame* frame, word nargs);
Object* builtinStringGt(Thread* thread, Frame* frame, word nargs);
Object* builtinStringLe(Thread* thread, Frame* frame, word nargs);
Object* builtinStringLt(Thread* thread, Frame* frame, word nargs);
Object* builtinStringMod(Thread* thread, Frame* caller, word nargs);
Object* builtinStringNe(Thread* thread, Frame* frame, word nargs);
Object* builtinStringNew(Thread* thread, Frame* frame, word nargs);

// old-style formatting, helper for __mod__
Object* stringFormat(Thread* thread, const Handle<String>& fmt,
                     const Handle<ObjectArray>& args);

}  // namespace python
