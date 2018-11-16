#pragma once

#include <iostream>

#include "frame.h"
#include "globals.h"
#include "objects.h"

namespace python {

extern std::ostream* builtInStdout;
extern std::ostream* builtinStderr;

Object* builtinBuildClass(Thread* thread, Frame* frame, word nargs);
Object* builtinBuildClassKw(Thread* thread, Frame* frame, word nargs);
Object* builtinChr(Thread* thread, Frame* frame, word nargs);
Object* builtinGetattr(Thread* thread, Frame* frame, word nargs);
Object* builtinHasattr(Thread* thread, Frame* frame, word nargs);
Object* builtinIsinstance(Thread* thread, Frame* frame, word nargs);
Object* builtinLen(Thread* thread, Frame* frame, word nargs);
Object* builtinOrd(Thread* thread, Frame* frame, word nargs);
Object* builtinPrint(Thread* thread, Frame* frame, word nargs);
Object* builtinPrintKw(Thread* thread, Frame* frame, word nargs);
Object* builtinRange(Thread* thread, Frame* frame, word nargs);
Object* builtinRepr(Thread* thread, Frame* frame, word nargs);
Object* builtinSetattr(Thread* thread, Frame* frame, word nargs);

}  // namespace python
