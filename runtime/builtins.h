#pragma once

#include <iostream>

#include "globals.h"

namespace python {

class Frame;
class Object;
class Thread;

// TODO: Remove this along with the iostream include once we have file-like
// objects. This is a side channel that allows us to override print's stdout
// during tests.
extern std::ostream* builtinPrintStream;

Object* builtinBuildClass(Thread* thread, Frame* frame, word nargs)
    __attribute__((aligned(16)));

// CALL_FUNCTION entry point for print()
Object* builtinPrint(Thread* thread, Frame* frame, word nargs)
    __attribute__((aligned(16)));

// CALL_FUNCTION_KW entry pointer for print()
Object* builtinPrintKw(Thread* thread, Frame* frame, word nargs)
    __attribute__((aligned(16)));

} // namespace python
