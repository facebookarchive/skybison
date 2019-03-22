#include "operator-module.h"

#include "frame.h"
#include "frozen-modules.h"
#include "globals.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace python {
const char* const OperatorModule::kFrozenData = kOperatorModuleData;
}  // namespace python
