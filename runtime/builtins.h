#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

// ClassMethod
Object* builtinClassMethodNew(Thread* thread, Frame* frame, word nargs);
Object* builtinClassMethodInit(Thread* thread, Frame* frame, word nargs);

// Object
Object* builtinObjectInit(Thread* thread, Frame* caller, word nargs);
Object* builtinObjectNew(Thread* thread, Frame* caller, word nargs);

// Descriptor
Object* functionDescriptorGet(Thread* thread, Frame* frame, word nargs);
Object* classmethodDescriptorGet(Thread* thread, Frame* frame, word nargs);
Object* staticmethodDescriptorGet(Thread* thread, Frame* frame, word nargs);

// StaticMethod
Object* builtinStaticMethodNew(Thread* thread, Frame* frame, word nargs);
Object* builtinStaticMethodInit(Thread* thread, Frame* frame, word nargs);

// Type
Object* builtinTypeCall(Thread* thread, Frame* frame, word nargs)
    __attribute__((aligned(16)));
Object* builtinTypeNew(Thread* thread, Frame* frame, word nargs);
Object* builtinTypeInit(Thread* thread, Frame* frame, word nargs);

} // namespace python
