#pragma once

#include "globals.h"

namespace python {

class Frame;
class Object;
class Thread;

// Entry point for an interpreted function with no defaults invoked via
// CALL_FUNCTION
Object* interpreterTrampoline(Thread* thread, Frame* previousFrame, word argc)
    __attribute__((aligned(16)));

// Entry point for an interpreted function with no defaults invoked via
// CALL_FUNCTION_KW
Object* interpreterTrampolineKw(Thread* thread, Frame* previousFrame, word argc)
    __attribute__((aligned(16)));

// Aborts immediately when called
Object* unimplementedTrampoline(Thread* thread, Frame* previousFrame, word argc)
    __attribute__((aligned(16)));

} // namespace python
