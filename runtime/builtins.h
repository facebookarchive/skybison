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

Object* builtinPrint(Thread* thread, Frame* frame, word argc);

} // namespace python
