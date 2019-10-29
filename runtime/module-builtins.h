#pragma once

#include "cpython-types.h"
#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

// Look up the value of ValueCell associated with key in module with
// consideration of placeholders created for caching.
RawObject moduleAt(Thread* thread, const Module& module, const Object& key,
                   word hash);
// Same as moduleAt but with the Str `name` as key.
RawObject moduleAtByStr(Thread* thread, const Module& module, const Str& name);
// Same as moduleAt but with SymbolId as key.
RawObject moduleAtById(Thread* thread, const Module& module, SymbolId id);
// Same as moduleAtById but returns the underlying ValueCell.
RawObject moduleValueCellAtById(Thread* thread, const Module& module,
                                SymbolId id);
// Same as moduleAtByStr but returns the underlying ValueCell.
RawObject moduleValueCellAtByStr(Thread* thread, const Module& module,
                                 const Str& name);

// Associate key with value in module.
RawObject moduleAtPut(Thread* thread, const Module& module, const Object& key,
                      word hash, const Object& value);
// Associate name with value in module.
RawObject moduleAtPutByStr(Thread* thread, const Module& module,
                           const Str& name, const Object& value);
// Associate id with value in module.
RawObject moduleAtPutById(Thread* thread, const Module& module, SymbolId id,
                          const Object& value);

RawObject moduleInit(Thread* thread, const Module& module, const Object& name);

// Returns keys associated with non-placeholder ValueCells in `module`.
RawObject moduleKeys(Thread* thread, const Module& module);

// Returns the number of keys associated with non-placeholder ValueCells.
RawObject moduleLen(Thread* thread, const Module& module);

// Remove the ValueCell associcated with key in module_dict.
RawObject moduleRemove(Thread* thread, const Module& module, const Object& key,
                       word hash);

// Returns the list of values contained in non-placeholder ValueCells.
RawObject moduleValues(Thread* thread, const Module& module);

RawObject moduleGetAttribute(Thread* thread, const Module& module,
                             const Object& name_str, word hash);

RawObject moduleSetAttr(Thread* thread, const Module& module,
                        const Object& name_str, word hash, const Object& value);

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

}  // namespace py
