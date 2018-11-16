#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinClassMethodGet(Thread* thread, Frame* frame, word nargs);
Object* builtinClassMethodInit(Thread* thread, Frame* frame, word nargs);
Object* builtinClassMethodNew(Thread* thread, Frame* frame, word nargs);

Object* builtinStaticMethodGet(Thread* thread, Frame* frame, word nargs);
Object* builtinStaticMethodInit(Thread* thread, Frame* frame, word nargs);
Object* builtinStaticMethodNew(Thread* thread, Frame* frame, word nargs);

Object* builtinPropertyDeleter(Thread* thread, Frame* frame, word nargs);
Object* builtinPropertyDunderGet(Thread* thread, Frame* frame, word nargs);
Object* builtinPropertyGetter(Thread* thread, Frame* frame, word nargs);
Object* builtinPropertyInit(Thread* thread, Frame* frame, word nargs);
Object* builtinPropertyNew(Thread* thread, Frame* frame, word nargs);
Object* builtinPropertySetter(Thread* thread, Frame* frame, word nargs);

} // namespace python
