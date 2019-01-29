#pragma once

#include "objects.h"

namespace python {

class Frame;
class Thread;

RawObject warningsWarn(Thread* thread, Frame* frame, word nargs);

}  // namespace python
