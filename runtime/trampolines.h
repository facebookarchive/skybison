#pragma once

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"

namespace py {

class Frame;
class Thread;

using PrepareCallFunc = RawObject (*)(Thread*, word, RawFunction);
RawObject preparePositionalCall(Thread* thread, word nargs,
                                RawFunction function_raw);
RawObject prepareKeywordCall(Thread* thread, word nargs,
                             RawFunction function_raw);
RawObject prepareExplodeCall(Thread* thread, word flags,
                             RawFunction function_raw);

void processFreevarsAndCellvars(Thread* thread, Frame* frame);

// Entry points for ordinary interpreted functions
RawObject interpreterTrampoline(Thread* thread, word nargs) ALIGN_16;
RawObject interpreterTrampolineKw(Thread* thread, word nargs) ALIGN_16;
RawObject interpreterTrampolineEx(Thread* thread, word flags) ALIGN_16;

// Entry points for interpreted functions with either cell variables or free
// variables.
RawObject interpreterClosureTrampoline(Thread* thread, word nargs) ALIGN_16;
RawObject interpreterClosureTrampolineKw(Thread* thread, word nargs) ALIGN_16;
RawObject interpreterClosureTrampolineEx(Thread* thread, word flags) ALIGN_16;

// Entry points for co-routine and generator functions.
RawObject generatorTrampoline(Thread* thread, word nargs) ALIGN_16;
RawObject generatorTrampolineKw(Thread* thread, word nargs) ALIGN_16;
RawObject generatorTrampolineEx(Thread* thread, word flags) ALIGN_16;

// Aborts immediately when called
RawObject unimplementedTrampoline(Thread* thread, word) ALIGN_16;

RawObject builtinTrampoline(Thread* thread, word nargs) ALIGN_16;
RawObject builtinTrampolineKw(Thread* thread, word nargs) ALIGN_16;
RawObject builtinTrampolineEx(Thread* thread, word flags) ALIGN_16;

RawObject raiseMissingArgumentsError(Thread* thread, word nargs,
                                     RawFunction function);
RawObject addDefaultArguments(Thread* thread, word nargs, RawFunction function,
                              word n_missing_args);
RawObject processDefaultArguments(Thread* thread, word nargs,
                                  RawFunction function);

inline RawObject addDefaultArguments(Thread* thread, word nargs,
                                     RawFunction function,
                                     word n_missing_args) {
  RawObject defaults = function.defaults();
  word n_defaults = defaults.isNoneType() ? 0 : Tuple::cast(defaults).length();
  if (UNLIKELY(n_missing_args > n_defaults)) {
    thread->stackDrop(nargs + 1);
    return raiseMissingArgumentsError(thread, nargs, function);
  }
  // Add default args.
  do {
    thread->stackPush(Tuple::cast(defaults).at(n_defaults - n_missing_args));
    n_missing_args--;
  } while (n_missing_args > 0);
  return function;
}

inline RawObject preparePositionalCall(Thread* thread, word nargs,
                                       RawFunction function) {
  if (!function.hasSimpleCall()) {
    return processDefaultArguments(thread, nargs, function);
  }

  word argcount = function.argcount();
  word n_missing_args = argcount - nargs;
  if (n_missing_args != 0) {
    if (n_missing_args < 0) {
      return processDefaultArguments(thread, nargs, function);
    }
    return addDefaultArguments(thread, nargs, function, n_missing_args);
  }
  return function;
}

}  // namespace py
