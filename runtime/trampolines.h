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
RawObject interpreterTrampoline(Thread* thread, Frame* caller,
                                word argc) ALIGN_16;
RawObject interpreterTrampolineKw(Thread* thread, Frame* caller,
                                  word argc) ALIGN_16;
RawObject interpreterTrampolineEx(Thread* thread, Frame* caller,
                                  word argc) ALIGN_16;

// Entry points for interpreted functions with either cell variables or free
// variables.
RawObject interpreterClosureTrampoline(Thread* thread, Frame* caller,
                                       word argc) ALIGN_16;
RawObject interpreterClosureTrampolineKw(Thread* thread, Frame* caller,
                                         word argc) ALIGN_16;
RawObject interpreterClosureTrampolineEx(Thread* thread, Frame* caller,
                                         word argc) ALIGN_16;

// Entry points for co-routine and generator functions.
RawObject generatorTrampoline(Thread* thread, Frame* caller,
                              word argc) ALIGN_16;
RawObject generatorTrampolineKw(Thread* thread, Frame* caller,
                                word argc) ALIGN_16;
RawObject generatorTrampolineEx(Thread* thread, Frame* caller,
                                word argc) ALIGN_16;

// Entry points for co-routine and generator functions with either cell
// variables or free variables.
RawObject generatorClosureTrampoline(Thread* thread, Frame* caller,
                                     word argc) ALIGN_16;
RawObject generatorClosureTrampolineKw(Thread* thread, Frame* caller,
                                       word argc) ALIGN_16;
RawObject generatorClosureTrampolineEx(Thread* thread, Frame* caller,
                                       word argc) ALIGN_16;

// Force 16-byte alignment on trampoline addresses to disguise them as
// SmallInts to avoid GC issues
template <RawObject (*Fn)(Thread*, Frame*, word)>
RawObject nativeTrampoline(Thread* thread, Frame* caller, word argc) ALIGN_16;
template <RawObject (*Fn)(Thread*, Frame*, word)>
RawObject nativeTrampolineKw(Thread* thread, Frame* caller, word argc) ALIGN_16;

RawObject moduleTrampolineNoArgs(Thread* thread, Frame* caller,
                                 word argc) ALIGN_16;
RawObject moduleTrampolineNoArgsKw(Thread* thread, Frame* caller,
                                   word argc) ALIGN_16;
RawObject moduleTrampolineNoArgsEx(Thread* thread, Frame* caller,
                                   word argc) ALIGN_16;

RawObject moduleTrampolineOneArg(Thread* thread, Frame* caller,
                                 word argc) ALIGN_16;
RawObject moduleTrampolineOneArgKw(Thread* thread, Frame* caller,
                                   word argc) ALIGN_16;
RawObject moduleTrampolineOneArgEx(Thread* thread, Frame* caller,
                                   word argc) ALIGN_16;

RawObject moduleTrampolineVarArgs(Thread* thread, Frame* caller,
                                  word argc) ALIGN_16;
RawObject moduleTrampolineVarArgsKw(Thread* thread, Frame* caller,
                                    word argc) ALIGN_16;
RawObject moduleTrampolineVarArgsEx(Thread* thread, Frame* caller,
                                    word argc) ALIGN_16;

RawObject moduleTrampolineKeywordArgs(Thread* thread, Frame* caller,
                                      word argc) ALIGN_16;
RawObject moduleTrampolineKeywordArgsKw(Thread* thread, Frame* caller,
                                        word argc) ALIGN_16;
RawObject moduleTrampolineKeywordArgsEx(Thread* thread, Frame* caller,
                                        word argc) ALIGN_16;

RawObject extensionTrampoline(Thread* thread, Frame* caller,
                              word argc) ALIGN_16;
RawObject extensionTrampolineKw(Thread* thread, Frame* caller,
                                word argc) ALIGN_16;
RawObject extensionTrampolineEx(Thread* thread, Frame* caller,
                                word argc) ALIGN_16;

// Aborts immediately when called
RawObject unimplementedTrampoline(Thread* thread, Frame* caller_frame,
                                  word argc) ALIGN_16;

RawObject builtinTrampoline(Thread* thread, Frame* caller, word argc) ALIGN_16;
RawObject builtinTrampolineKw(Thread* thread, Frame* caller,
                              word argc) ALIGN_16;
RawObject builtinTrampolineEx(Thread* thread, Frame* caller,
                              word argc) ALIGN_16;

}  // namespace python
