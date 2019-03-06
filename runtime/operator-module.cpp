#include "operator-module.h"

#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace python {

void OperatorModule::postInitialize(Thread*, Runtime* runtime,
                                    const Module& module) {
  CHECK(!runtime->executeModule(kOperatorModuleData, module).isError(),
        "Failed to initialize operator module");
}

}  // namespace python
