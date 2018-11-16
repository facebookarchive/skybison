#pragma once

#include "globals.h"

namespace python {

class Frame;
class Object;
class Thread;
class Function;
class Code;

// Entry points for interpreted functions
Object* interpreterTrampoline(Thread* thread, Frame* callerFrame, word argc)
    __attribute__((aligned(16)));
Object* interpreterTrampolineSlowPath(Thread* thread, Function* function,
                                      Code* code, Frame* callerFrame, word argc)
    __attribute__((aligned(16)));
Object* interpreterTrampolineKw(Thread* thread, Frame* callerFrame, word argc)
    __attribute__((aligned(16)));
Object* interpreterTrampolineEx(Thread* thread, Frame* callerFrame, word argc)
    __attribute__((aligned(16)));

// Aborts immediately when called
Object* unimplementedTrampoline(Thread* thread, Frame* callerFrame, word argc)
    __attribute__((aligned(16)));

// Force 16-byte alignment on trampoline addresses to disguise them as
// smallIntegers to avoid GC issues
template <Object* (*Fn)(Thread*, Frame*, word)>
Object* nativeTrampoline(Thread* thread, Frame* callerFrame, word argc)
    __attribute__((aligned(16)));
template <Object* (*Fn)(Thread*, Frame*, word)>
Object* nativeTrampolineKw(Thread* thread, Frame* callerFrame, word argc)
    __attribute__((aligned(16)));

Object* extensionTrampoline(Thread* thread, Frame* previousFrame, word argc)
    __attribute__((aligned(16)));
Object* extensionTrampolineKw(Thread* thread, Frame* previousFrame, word argc)
    __attribute__((aligned(16)));
Object* extensionTrampolineEx(Thread* thread, Frame* previousFrame, word argc)
    __attribute__((aligned(16)));

}  // namespace python
