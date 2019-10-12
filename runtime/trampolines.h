#pragma once

#include "globals.h"
#include "handles.h"
#include "objects.h"

namespace python {

class Frame;
class Thread;

using PrepareCallFunc = RawObject (*)(Thread*, RawFunction, Frame*, word);
RawObject preparePositionalCall(Thread* thread, RawFunction function_raw,
                                Frame* frame, word argc);
RawObject prepareKeywordCall(Thread* thread, RawFunction function_raw,
                             Frame* frame, word argc);
RawObject prepareExplodeCall(Thread* thread, RawFunction function_raw,
                             Frame* frame, word flags);

void processFreevarsAndCellvars(Thread* thread, Frame* frame);

// Entry points for ordinary interpreted functions
RawObject interpreterTrampoline(Thread* thread, Frame* frame,
                                word argc) ALIGN_16;
RawObject interpreterTrampolineKw(Thread* thread, Frame* frame,
                                  word argc) ALIGN_16;
RawObject interpreterTrampolineEx(Thread* thread, Frame* frame,
                                  word flags) ALIGN_16;

// Entry points for interpreted functions with either cell variables or free
// variables.
RawObject interpreterClosureTrampoline(Thread* thread, Frame* frame,
                                       word argc) ALIGN_16;
RawObject interpreterClosureTrampolineKw(Thread* thread, Frame* frame,
                                         word argc) ALIGN_16;
RawObject interpreterClosureTrampolineEx(Thread* thread, Frame* frame,
                                         word flags) ALIGN_16;

// Entry points for co-routine and generator functions.
RawObject generatorTrampoline(Thread* thread, Frame* frame, word argc) ALIGN_16;
RawObject generatorTrampolineKw(Thread* thread, Frame* frame,
                                word argc) ALIGN_16;
RawObject generatorTrampolineEx(Thread* thread, Frame* frame,
                                word flags) ALIGN_16;

// Entry points for co-routine and generator functions with either cell
// variables or free variables.
RawObject generatorClosureTrampoline(Thread* thread, Frame* frame,
                                     word argc) ALIGN_16;
RawObject generatorClosureTrampolineKw(Thread* thread, Frame* frame,
                                       word argc) ALIGN_16;
RawObject generatorClosureTrampolineEx(Thread* thread, Frame* frame,
                                       word flags) ALIGN_16;

// method trampolines

RawObject methodTrampolineNoArgs(Thread* thread, Frame* frame,
                                 word argc) ALIGN_16;
RawObject methodTrampolineNoArgsKw(Thread* thread, Frame* frame,
                                   word argc) ALIGN_16;
RawObject methodTrampolineNoArgsEx(Thread* thread, Frame* frame,
                                   word flags) ALIGN_16;

RawObject methodTrampolineOneArg(Thread* thread, Frame* frame,
                                 word argc) ALIGN_16;
RawObject methodTrampolineOneArgKw(Thread* thread, Frame* frame,
                                   word argc) ALIGN_16;
RawObject methodTrampolineOneArgEx(Thread* thread, Frame* frame,
                                   word flags) ALIGN_16;

RawObject methodTrampolineVarArgs(Thread* thread, Frame* frame,
                                  word argc) ALIGN_16;
RawObject methodTrampolineVarArgsKw(Thread* thread, Frame* frame,
                                    word argc) ALIGN_16;
RawObject methodTrampolineVarArgsEx(Thread* thread, Frame* frame,
                                    word flags) ALIGN_16;

RawObject methodTrampolineKeywords(Thread* thread, Frame* frame,
                                   word argc) ALIGN_16;
RawObject methodTrampolineKeywordsKw(Thread* thread, Frame* frame,
                                     word argc) ALIGN_16;
RawObject methodTrampolineKeywordsEx(Thread* thread, Frame* frame,
                                     word flags) ALIGN_16;

RawObject methodTrampolineFastCall(Thread* thread, Frame* frame,
                                   word argc) ALIGN_16;
RawObject methodTrampolineFastCallKw(Thread* thread, Frame* frame,
                                     word argc) ALIGN_16;
RawObject methodTrampolineFastCallEx(Thread* thread, Frame* frame,
                                     word flags) ALIGN_16;

// module trampolines

RawObject moduleTrampolineNoArgs(Thread* thread, Frame* frame,
                                 word argc) ALIGN_16;
RawObject moduleTrampolineNoArgsKw(Thread* thread, Frame* frame,
                                   word argc) ALIGN_16;
RawObject moduleTrampolineNoArgsEx(Thread* thread, Frame* frame,
                                   word flags) ALIGN_16;

RawObject moduleTrampolineOneArg(Thread* thread, Frame* frame,
                                 word argc) ALIGN_16;
RawObject moduleTrampolineOneArgKw(Thread* thread, Frame* frame,
                                   word argc) ALIGN_16;
RawObject moduleTrampolineOneArgEx(Thread* thread, Frame* frame,
                                   word flags) ALIGN_16;

RawObject moduleTrampolineVarArgs(Thread* thread, Frame* frame,
                                  word argc) ALIGN_16;
RawObject moduleTrampolineVarArgsKw(Thread* thread, Frame* frame,
                                    word argc) ALIGN_16;
RawObject moduleTrampolineVarArgsEx(Thread* thread, Frame* frame,
                                    word flags) ALIGN_16;

RawObject moduleTrampolineKeywords(Thread* thread, Frame* frame,
                                   word argc) ALIGN_16;
RawObject moduleTrampolineKeywordsKw(Thread* thread, Frame* frame,
                                     word argc) ALIGN_16;
RawObject moduleTrampolineKeywordsEx(Thread* thread, Frame* frame,
                                     word flags) ALIGN_16;

RawObject moduleTrampolineFastCall(Thread* thread, Frame* frame,
                                   word argc) ALIGN_16;
RawObject moduleTrampolineFastCallKw(Thread* thread, Frame* frame,
                                     word argc) ALIGN_16;
RawObject moduleTrampolineFastCallEx(Thread* thread, Frame* frame,
                                     word flags) ALIGN_16;

// Aborts immediately when called
RawObject unimplementedTrampoline(Thread* thread, Frame* frame, word) ALIGN_16;

RawObject builtinTrampoline(Thread* thread, Frame* frame, word argc) ALIGN_16;
RawObject builtinTrampolineKw(Thread* thread, Frame* frame, word argc) ALIGN_16;
RawObject builtinTrampolineEx(Thread* thread, Frame* frame,
                              word flags) ALIGN_16;

RawObject processDefaultArguments(Thread* thread, RawFunction function_raw,
                                  Frame* frame, const word argc);

inline RawObject preparePositionalCall(Thread* thread, RawFunction function_raw,
                                       Frame* frame, word argc) {
  // Are we one of the less common cases?
  if (argc != function_raw.argcount() || !function_raw.hasSimpleCall()) {
    return processDefaultArguments(thread, function_raw, frame, argc);
  }
  return function_raw;
}

}  // namespace python
