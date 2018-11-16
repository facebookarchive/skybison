#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

RawObject builtinClassMethodGet(Thread* thread, Frame* frame, word nargs);
RawObject builtinClassMethodInit(Thread* thread, Frame* frame, word nargs);
RawObject builtinClassMethodNew(Thread* thread, Frame* frame, word nargs);

RawObject builtinStaticMethodGet(Thread* thread, Frame* frame, word nargs);
RawObject builtinStaticMethodInit(Thread* thread, Frame* frame, word nargs);
RawObject builtinStaticMethodNew(Thread* thread, Frame* frame, word nargs);

RawObject builtinPropertyDeleter(Thread* thread, Frame* frame, word nargs);
RawObject builtinPropertyDunderGet(Thread* thread, Frame* frame, word nargs);
RawObject builtinPropertyDunderSet(Thread* thread, Frame* frame, word nargs);
RawObject builtinPropertyGetter(Thread* thread, Frame* frame, word nargs);
RawObject builtinPropertyInit(Thread* thread, Frame* frame, word nargs);
RawObject builtinPropertyNew(Thread* thread, Frame* frame, word nargs);
RawObject builtinPropertySetter(Thread* thread, Frame* frame, word nargs);

}  // namespace python
