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
RawObject moduleAt(Thread* thread, const Module& module, const Object& name);
// Same as moduleAt but with SymbolId as key.
RawObject moduleAtById(Thread* thread, const Module& module, SymbolId id);
// Same as moduleAtById but returns the underlying ValueCell.
RawObject moduleValueCellAtById(Thread* thread, const Module& module,
                                SymbolId id);
// Same as moduleAt but returns the underlying ValueCell.
RawObject moduleValueCellAt(Thread* thread, const Module& module,
                            const Object& name);

// Associate name with value in module.
RawObject moduleAtPut(Thread* thread, const Module& module, const Object& name,
                      const Object& value);
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
                             const Object& name);

RawObject moduleGetAttributeSetLocation(Thread* thread, const Module& module,
                                        const Object& name,
                                        Object* location_out);

RawObject moduleRaiseAttributeError(Thread* thread, const Module& module,
                                    const Object& name);

RawObject moduleSetAttr(Thread* thread, const Module& module,
                        const Object& name, const Object& value);

// A version of Dict::Bucket::nextItem for module dict to filter out
// placeholders.
bool nextModuleDictItem(RawTuple data, word* idx);

// Runs the executable functions found in the PyModuleDef
int execDef(Thread* thread, const Module& module, PyModuleDef* def);

class ModuleBuiltins
    : public Builtins<ModuleBuiltins, ID(module), LayoutId::kModule> {
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

}  // namespace py
