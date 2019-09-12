#pragma once

#include "cpython-types.h"
#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

// Look up the value of ValueCell associated with key in module with
// consideration of placeholders created for caching.
RawObject moduleAt(Thread* thread, const Module& module, const Object& key);
// Same as moduleAt but with SymbolId as key.
RawObject moduleAtById(Thread* thread, const Module& module, SymbolId key);
// Same as moduleAtById but returns the underlying ValueCell.
RawObject moduleValueCellAtById(Thread* thread, const Module& module,
                                SymbolId key);
// Same as moduleAt but with dict type parameter.
RawObject moduleDictAt(Thread* thread, const Dict& module_dict,
                       const Object& key);

// Only callable by Interpreter for manipulating caches. Same as moduleAt but
// returns the ValueCell if found.
RawObject moduleDictValueCellAt(Thread* thread, const Dict& dict,
                                const Object& key);

// Returns `__builtins__` of a module dict. Returns a new dict with a single
// `{"None": None}` entry if `__builtins__` does not exist.
RawDict moduleDictBuiltins(Thread* thread, const Dict& dict);

// Associate key with value in module.
RawObject moduleAtPut(Thread* thread, const Module& module, const Object& key,
                      const Object& value);
// Same as moduleAtPut but with SymbolId as key.
RawObject moduleAtPutById(Thread* thread, const Module& module, SymbolId key,
                          const Object& value);
// Same as moduleAtPut with dict type parameter.
RawObject moduleDictAtPut(Thread* thread, const Dict& module_dict,
                          const Object& key, const Object& value);

// Only callable by Interpreter for manipulating caches. Same as moduleAtPut but
// returns the inserted ValueCell.
RawObject moduleDictValueCellAtPut(Thread* thread, const Dict& module_dict,
                                   const Object& key, const Object& value);

// Remove the ValueCell associcated with key in module_dict.
RawObject moduleDictRemove(Thread* thread, const Dict& module_dict,
                           const Object& key);

// Returns keys associated with non-placeholder ValueCells in module_dict.
RawObject moduleDictKeys(Thread* thread, const Dict& module_dict);

RawObject moduleInit(Thread* thread, const Module& module, const Object& name);

// Returns the number of keys associated with non-placeholder ValueCells.
RawObject moduleLen(Thread* thread, const Module& module);

// Returns the list of values contained in non-placeholder ValueCells.
RawObject moduleValues(Thread* thread, const Module& module);

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
  static RawObject dunderInit(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderNew(Thread* thread, Frame* frame, word nargs);
  static RawObject dunderSetattr(Thread* thread, Frame* frame, word nargs);

  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ModuleBuiltins);
};

class ModuleProxyBuiltins
    : public Builtins<ModuleProxyBuiltins, SymbolId::kModuleProxy,
                      LayoutId::kModuleProxy> {
 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(ModuleProxyBuiltins);
};

}  // namespace python
