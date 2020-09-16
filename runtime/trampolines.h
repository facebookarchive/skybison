#pragma once

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"

namespace py {

class Frame;
class Thread;

using PrepareCallFunc = RawObject (*)(Thread*, RawFunction, Frame*, word);
RawObject preparePositionalCall(Thread* thread, RawFunction function_raw,
                                Frame* frame, word nargs);
RawObject prepareKeywordCall(Thread* thread, RawFunction function_raw,
                             Frame* frame, word nargs);
RawObject prepareExplodeCall(Thread* thread, RawFunction function_raw,
                             Frame* frame, word flags);

void processFreevarsAndCellvars(Thread* thread, Frame* frame);

// Entry points for ordinary interpreted functions
RawObject interpreterTrampoline(Thread* thread, Frame* frame,
                                word nargs) ALIGN_16;
RawObject interpreterTrampolineKw(Thread* thread, Frame* frame,
                                  word nargs) ALIGN_16;
RawObject interpreterTrampolineEx(Thread* thread, Frame* frame,
                                  word flags) ALIGN_16;

// Entry points for interpreted functions with either cell variables or free
// variables.
RawObject interpreterClosureTrampoline(Thread* thread, Frame* frame,
                                       word nargs) ALIGN_16;
RawObject interpreterClosureTrampolineKw(Thread* thread, Frame* frame,
                                         word nargs) ALIGN_16;
RawObject interpreterClosureTrampolineEx(Thread* thread, Frame* frame,
                                         word flags) ALIGN_16;

// Entry points for co-routine and generator functions.
RawObject generatorTrampoline(Thread* thread, Frame* frame,
                              word nargs) ALIGN_16;
RawObject generatorTrampolineKw(Thread* thread, Frame* frame,
                                word nargs) ALIGN_16;
RawObject generatorTrampolineEx(Thread* thread, Frame* frame,
                                word flags) ALIGN_16;

// Entry points for co-routine and generator functions with either cell
// variables or free variables.
RawObject generatorClosureTrampoline(Thread* thread, Frame* frame,
                                     word nargs) ALIGN_16;
RawObject generatorClosureTrampolineKw(Thread* thread, Frame* frame,
                                       word nargs) ALIGN_16;
RawObject generatorClosureTrampolineEx(Thread* thread, Frame* frame,
                                       word flags) ALIGN_16;

// method trampolines

RawObject methodTrampolineNoArgs(Thread* thread, Frame* frame,
                                 word nargs) ALIGN_16;
RawObject methodTrampolineNoArgsKw(Thread* thread, Frame* frame,
                                   word nargs) ALIGN_16;
RawObject methodTrampolineNoArgsEx(Thread* thread, Frame* frame,
                                   word flags) ALIGN_16;

RawObject methodTrampolineOneArg(Thread* thread, Frame* frame,
                                 word nargs) ALIGN_16;
RawObject methodTrampolineOneArgKw(Thread* thread, Frame* frame,
                                   word nargs) ALIGN_16;
RawObject methodTrampolineOneArgEx(Thread* thread, Frame* frame,
                                   word flags) ALIGN_16;

RawObject methodTrampolineVarArgs(Thread* thread, Frame* frame,
                                  word nargs) ALIGN_16;
RawObject methodTrampolineVarArgsKw(Thread* thread, Frame* frame,
                                    word nargs) ALIGN_16;
RawObject methodTrampolineVarArgsEx(Thread* thread, Frame* frame,
                                    word flags) ALIGN_16;

RawObject methodTrampolineKeywords(Thread* thread, Frame* frame,
                                   word nargs) ALIGN_16;
RawObject methodTrampolineKeywordsKw(Thread* thread, Frame* frame,
                                     word nargs) ALIGN_16;
RawObject methodTrampolineKeywordsEx(Thread* thread, Frame* frame,
                                     word flags) ALIGN_16;

RawObject methodTrampolineFast(Thread* thread, Frame* frame,
                               word nargs) ALIGN_16;
RawObject methodTrampolineFastKw(Thread* thread, Frame* frame,
                                 word nargs) ALIGN_16;
RawObject methodTrampolineFastEx(Thread* thread, Frame* frame,
                                 word flags) ALIGN_16;

RawObject methodTrampolineFastWithKeywords(Thread* thread, Frame* frame,
                                           word nargs) ALIGN_16;
RawObject methodTrampolineFastWithKeywordsKw(Thread* thread, Frame* frame,
                                             word nargs) ALIGN_16;
RawObject methodTrampolineFastWithKeywordsEx(Thread* thread, Frame* frame,
                                             word flags) ALIGN_16;

// Aborts immediately when called
RawObject unimplementedTrampoline(Thread* thread, Frame* frame, word) ALIGN_16;

RawObject builtinTrampoline(Thread* thread, Frame* frame, word nargs) ALIGN_16;
RawObject builtinTrampolineKw(Thread* thread, Frame* frame,
                              word nargs) ALIGN_16;
RawObject builtinTrampolineEx(Thread* thread, Frame* frame,
                              word flags) ALIGN_16;

RawObject raiseMissingArgumentsError(Thread* thread, RawFunction function,
                                     word nargs);
RawObject addDefaultArguments(Thread* thread, RawFunction function,
                              Frame* frame, word nargs, word n_missing_args);
RawObject processDefaultArguments(Thread* thread, RawFunction function,
                                  Frame* frame, const word nargs);

inline RawObject addDefaultArguments(Thread* thread, RawFunction function,
                                     Frame* frame, word nargs,
                                     word n_missing_args) {
  RawObject defaults = function.defaults();
  word n_defaults = defaults.isNoneType() ? 0 : Tuple::cast(defaults).length();
  if (UNLIKELY(n_missing_args > n_defaults)) {
    frame->dropValues(nargs + 1);
    return raiseMissingArgumentsError(thread, function, nargs);
  }
  // Add default args.
  do {
    frame->pushValue(Tuple::cast(defaults).at(n_defaults - n_missing_args));
    n_missing_args--;
  } while (n_missing_args > 0);
  return function;
}

inline RawObject preparePositionalCall(Thread* thread, RawFunction function,
                                       Frame* frame, word nargs) {
  if (!function.hasSimpleCall()) {
    return processDefaultArguments(thread, function, frame, nargs);
  }

  word argcount = function.argcount();
  word n_missing_args = argcount - nargs;
  if (n_missing_args != 0) {
    if (n_missing_args < 0) {
      return processDefaultArguments(thread, function, frame, nargs);
    }
    return addDefaultArguments(thread, function, frame, nargs, n_missing_args);
  }
  return function;
}

}  // namespace py
