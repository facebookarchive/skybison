#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "thread.h"

namespace python {

Object* builtinDictionaryEq(Thread* thread, Frame* frame, word nargs);
Object* builtinDictionaryLen(Thread* thread, Frame* frame, word nargs);

}  // namespace python
