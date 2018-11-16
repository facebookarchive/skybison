#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"

namespace python {

Object* builtinTime(Thread* thread, Frame* frame, word nargs);

}  // namespace python
