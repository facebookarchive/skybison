#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// Returns keys associated with non-placeholder ValueCells in module_dict.
RawObject moduleDictKeys(Thread* thread, const Dict& module_dict);

RawObject moduleGetAttribute(Thread* thread, const Module& module,
                             const Object& name_str);

RawObject moduleSetAttr(Thread* thread, const Module& module,
                        const Object& name_str, const Object& value);

// A version of Dict::Bucket::nextItem for module dict to filter out
// placeholders.
bool nextModuleDictItem(RawTuple data, word* idx);

// Runs the executable functions found in the PyModuleDef
int execDef(Thread* thread, const Module& module, PyModuleDef* def);

class ModuleBuiltins
    : public Builtins<ModuleBuiltins, SymbolId::kModule, LayoutId::kModule> {
 public:
  static RawObject dunderGetattribute(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSetattr(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ModuleBuiltins);
};

}  // namespace python
