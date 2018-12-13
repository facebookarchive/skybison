#pragma once

#include <iostream>

#include "frame.h"
#include "globals.h"
#include "objects.h"

namespace python {

extern std::ostream* builtInStdout;
extern std::ostream* builtinStderr;

RawObject builtinBuildClass(Thread* thread, Frame* frame, word nargs);
RawObject builtinBuildClassKw(Thread* thread, Frame* frame, word nargs);
RawObject builtinCallable(Thread* thread, Frame* frame, word nargs);
RawObject builtinChr(Thread* thread, Frame* frame, word nargs);
RawObject builtinCompile(Thread* thread, Frame* frame, word nargs);
RawObject builtinGetattr(Thread* thread, Frame* frame, word nargs);
RawObject builtinHasattr(Thread* thread, Frame* frame, word nargs);
RawObject builtinIsinstance(Thread* thread, Frame* frame, word nargs);
RawObject builtinIssubclass(Thread* thread, Frame* frame, word nargs);
RawObject builtinLen(Thread* thread, Frame* frame, word nargs);
RawObject builtinOrd(Thread* thread, Frame* frame, word nargs);
RawObject builtinPrint(Thread* thread, Frame* frame, word nargs);
RawObject builtinPrintKw(Thread* thread, Frame* frame, word nargs);
RawObject builtinRange(Thread* thread, Frame* frame, word nargs);
RawObject builtinRepr(Thread* thread, Frame* frame, word nargs);
RawObject builtinSetattr(Thread* thread, Frame* frame, word nargs);

}  // namespace python
