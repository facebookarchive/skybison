#include "builtins.h"
#include "globals.h"
#include "handles.h"
#include "module-builtins.h"
#include "runtime.h"
#include "thread.h"

namespace py {

void FUNC(_thread, __init_module__)(Thread* thread, const Module& module,
                                    View<byte> bytecode) {
  executeFrozenModule(thread, module, bytecode);
}

}  // namespace py
