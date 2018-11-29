#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"

namespace python {

RawObject ioReadFile(Thread* thread, Frame* frame, word nargs);
RawObject ioReadBytes(Thread* thread, Frame* frame, word nargs);

}  // namespace python
