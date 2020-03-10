#include "unicodedata-module.h"

#include "frozen-modules.h"
#include "handles-decl.h"
#include "layout.h"
#include "module-builtins.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"

namespace py {

void UnicodedataModule::initialize(Thread* thread, const Module& module) {
  executeFrozenModule(thread, &kUnicodedataModuleData, module);

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type ucd_type(&scope, moduleAtById(thread, module, ID(UCD)));
  Layout ucd_layout(&scope, ucd_type.instanceLayout());
  Object old_ucd(&scope, runtime->newInstance(ucd_layout));
  moduleAtPutById(thread, module, ID(ucd_3_2_0), old_ucd);
}

}  // namespace py
