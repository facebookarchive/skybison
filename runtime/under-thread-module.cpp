#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "modules.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject FUNC(_thread, get_ident)(Thread* thread, Arguments) {
  return thread->runtime()->newIntFromUnsigned(OS::threadID());
}

}  // namespace py
