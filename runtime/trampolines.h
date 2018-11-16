#pragma once

#include "globals.h"

namespace python {

class Frame;
class Object;
class Thread;

// Entry points for interpreted functions
Object* interpreterTrampoline(Thread* thread, Frame* previousFrame, word argc)
    __attribute__((aligned(16)));
Object* interpreterTrampolineKw(Thread* thread, Frame* previousFrame, word argc)
    __attribute__((aligned(16)));

// Aborts immediately when called
Object* unimplementedTrampoline(Thread* thread, Frame* previousFrame, word argc)
    __attribute__((aligned(16)));

template <Object* (*Fn)(Thread*, Frame*, word)>
Object* nativeTrampoline(Thread* thread, Frame* previousFrame, word argc)
    __attribute__((aligned(16)));

} // namespace python
