#pragma once

#include "globals.h"
#include "objects.h"

namespace python {

class Frame;
class Thread;

// Entry points for interpreted functions
RawObject interpreterTrampoline(Thread* thread, Frame* caller_frame, word argc)
    __attribute__((aligned(16)));
RawObject interpreterTrampolineSlowPath(Thread* thread, RawFunction function,
                                        RawCode code, Frame* caller_frame,
                                        word argc) __attribute__((aligned(16)));
RawObject interpreterTrampolineKw(Thread* thread, Frame* caller_frame,
                                  word argc) __attribute__((aligned(16)));
RawObject interpreterTrampolineEx(Thread* thread, Frame* caller_frame,
                                  word argc) __attribute__((aligned(16)));

// Aborts immediately when called
RawObject unimplementedTrampoline(Thread* thread, Frame* caller_frame,
                                  word argc) __attribute__((aligned(16)));

// Force 16-byte alignment on trampoline addresses to disguise them as
// SmallInts to avoid GC issues
template <RawObject (*Fn)(Thread*, Frame*, word)>
RawObject nativeTrampoline(Thread* thread, Frame* caller_frame, word argc)
    __attribute__((aligned(16)));
template <RawObject (*Fn)(Thread*, Frame*, word)>
RawObject nativeTrampolineKw(Thread* thread, Frame* caller_frame, word argc)
    __attribute__((aligned(16)));

RawObject extensionTrampoline(Thread* thread, Frame* caller_frame, word argc)
    __attribute__((aligned(16)));
RawObject extensionTrampolineKw(Thread* thread, Frame* caller_frame, word argc)
    __attribute__((aligned(16)));
RawObject extensionTrampolineEx(Thread* thread, Frame* caller_frame, word argc)
    __attribute__((aligned(16)));

}  // namespace python
