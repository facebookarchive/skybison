#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "modules.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject FUNC(_thread, get_ident)(Thread* thread, Frame*, word) {
  return thread->runtime()->newIntFromUnsigned(thread->id());
}

}  // namespace py
