#pragma once

#include "globals.h"
#include "handles.h"
#include "objects.h"

namespace python {

class Frame;
class Thread;

using PrepareCallFunc = RawObject (*)(Thread*, const Function&, Frame*, word);
RawObject preparePositionalCall(Thread* thread, const Function& function,
                                Frame* caller, word argc);
RawObject prepareKeywordCall(Thread* thread, const Function& function,
                             Frame* caller, word argc);
RawObject prepareExplodeCall(Thread* thread, const Function& function,
                             Frame* caller, word arg);
void processFreevarsAndCellvars(Thread* thread, const Function& function,
                                Frame* callee_frame);

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

// method trampolines

RawObject methodTrampolineNoArgs(Thread* thread, Frame* caller,
                                 word argc) ALIGN_16;
RawObject methodTrampolineNoArgsKw(Thread* thread, Frame* caller,
                                   word argc) ALIGN_16;
RawObject methodTrampolineNoArgsEx(Thread* thread, Frame* caller,
                                   word argc) ALIGN_16;

RawObject methodTrampolineOneArg(Thread* thread, Frame* caller,
                                 word argc) ALIGN_16;
RawObject methodTrampolineOneArgKw(Thread* thread, Frame* caller,
                                   word argc) ALIGN_16;
RawObject methodTrampolineOneArgEx(Thread* thread, Frame* caller,
                                   word argc) ALIGN_16;

RawObject methodTrampolineVarArgs(Thread* thread, Frame* caller,
                                  word argc) ALIGN_16;
RawObject methodTrampolineVarArgsKw(Thread* thread, Frame* caller,
                                    word argc) ALIGN_16;
RawObject methodTrampolineVarArgsEx(Thread* thread, Frame* caller,
                                    word argc) ALIGN_16;

RawObject methodTrampolineKeywords(Thread* thread, Frame* caller,
                                   word argc) ALIGN_16;
RawObject methodTrampolineKeywordsKw(Thread* thread, Frame* caller,
                                     word argc) ALIGN_16;
RawObject methodTrampolineKeywordsEx(Thread* thread, Frame* caller,
                                     word argc) ALIGN_16;

// module trampolines

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

RawObject moduleTrampolineKeywords(Thread* thread, Frame* caller,
                                   word argc) ALIGN_16;
RawObject moduleTrampolineKeywordsKw(Thread* thread, Frame* caller,
                                     word argc) ALIGN_16;
RawObject moduleTrampolineKeywordsEx(Thread* thread, Frame* caller,
                                     word argc) ALIGN_16;

// Aborts immediately when called
RawObject unimplementedTrampoline(Thread* thread, Frame* caller_frame,
                                  word argc) ALIGN_16;

RawObject builtinTrampoline(Thread* thread, Frame* caller, word argc) ALIGN_16;
RawObject builtinTrampolineKw(Thread* thread, Frame* caller,
                              word argc) ALIGN_16;
RawObject builtinTrampolineEx(Thread* thread, Frame* caller,
                              word argc) ALIGN_16;

RawObject slotTrampoline(Thread* thread, Frame* caller, word argc) ALIGN_16;
RawObject slotTrampolineKw(Thread* thread, Frame*, word argc) ALIGN_16;
RawObject slotTrampolineEx(Thread* thread, Frame* caller, word argc) ALIGN_16;

RawObject varkwSlotTrampoline(Thread* thread, Frame* caller,
                              word argc) ALIGN_16;
RawObject varkwSlotTrampolineKw(Thread* thread, Frame* caller,
                                word argc) ALIGN_16;
RawObject varkwSlotTrampolineEx(Thread* thread, Frame* caller,
                                word flags) ALIGN_16;

}  // namespace python
