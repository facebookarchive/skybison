#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"

namespace python {

extern word marshalVersion;

class MarshalModule {
 public:
  static RawObject loads(Thread* thread, Frame* frame, word nargs);
};

}  // namespace python
