#pragma once

#include "globals.h"
#include "handles.h"
#include "objects.h"

namespace python {

class Frame;
class Thread;

RawObject preparePositionalCall(Thread* thread, const Function& function,
                                const Code& code, Frame* caller, word argc);

// Entry points for ordinary interpreted functions
RawObject interpreterTrampoline(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject interpreterTrampolineKw(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject interpreterTrampolineEx(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));

// Entry points for interpreted functions with either cell variables or free
// variables.
RawObject interpreterClosureTrampoline(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject interpreterClosureTrampolineKw(Thread* thread, Frame* caller,
                                         word argc)
    __attribute__((aligned(16)));
RawObject interpreterClosureTrampolineEx(Thread* thread, Frame* caller,
                                         word argc)
    __attribute__((aligned(16)));

// Entry points for co-routine and generator functions.
RawObject generatorTrampoline(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject generatorTrampolineKw(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject generatorTrampolineEx(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));

// Entry points for co-routine and generator functions with either cell
// variables or free variables.
RawObject generatorClosureTrampoline(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject generatorClosureTrampolineKw(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject generatorClosureTrampolineEx(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));

// Force 16-byte alignment on trampoline addresses to disguise them as
// SmallInts to avoid GC issues
template <RawObject (*Fn)(Thread*, Frame*, word)>
RawObject nativeTrampoline(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
template <RawObject (*Fn)(Thread*, Frame*, word)>
RawObject nativeTrampolineKw(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));

template <RawObject (*Fn)(Thread*, Frame*, word)>
RawObject builtinTrampolineWrapper(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));

RawObject moduleTrampolineNoArgs(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject moduleTrampolineNoArgsKw(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject moduleTrampolineNoArgsEx(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));

RawObject moduleTrampolineVarArgs(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject moduleTrampolineVarArgsKw(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject moduleTrampolineVarArgsEx(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));

RawObject extensionTrampoline(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject extensionTrampolineKw(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));
RawObject extensionTrampolineEx(Thread* thread, Frame* caller, word argc)
    __attribute__((aligned(16)));

// Aborts immediately when called
RawObject unimplementedTrampoline(Thread* thread, Frame* caller_frame,
                                  word argc) __attribute__((aligned(16)));

RawObject builtinTrampoline(Thread* thread, Frame* caller, word argc,
                            Function::Entry fn);

}  // namespace python
