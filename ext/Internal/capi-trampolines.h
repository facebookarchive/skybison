#pragma once

#include "globals.h"
#include "objects.h"

namespace py {

class Thread;

RawObject methodTrampolineNoArgs(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineNoArgsKw(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineNoArgsEx(Thread* thread, word flags) ALIGN_16;

RawObject methodTrampolineOneArg(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineOneArgKw(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineOneArgEx(Thread* thread, word flags) ALIGN_16;

RawObject methodTrampolineVarArgs(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineVarArgsKw(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineVarArgsEx(Thread* thread, word flags) ALIGN_16;

RawObject methodTrampolineKeywords(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineKeywordsKw(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineKeywordsEx(Thread* thread, word flags) ALIGN_16;

RawObject methodTrampolineFast(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineFastKw(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineFastEx(Thread* thread, word flags) ALIGN_16;

RawObject methodTrampolineFastWithKeywords(Thread* thread, word nargs) ALIGN_16;
RawObject methodTrampolineFastWithKeywordsKw(Thread* thread,
                                             word nargs) ALIGN_16;
RawObject methodTrampolineFastWithKeywordsEx(Thread* thread,
                                             word flags) ALIGN_16;

}  // namespace py
