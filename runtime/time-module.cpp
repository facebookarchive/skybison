#include "time-module.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject builtinTime(Thread* thread, Frame*, word) {
  return thread->runtime()->newFloat(OS::currentTime());
}

}  // namespace python
