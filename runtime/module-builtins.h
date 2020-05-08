#pragma once

#include "cpython-types.h"

#include "globals.h"
#include "handles-decl.h"
#include "objects.h"
#include "symbols.h"

namespace py {

class Thread;

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
// Associate name with value in module.
RawObject moduleAtPutByCStr(Thread* thread, const Module& module,
                            const char* name_cstr, const Object& value);
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

// Runs the executable functions found in the PyModuleDef
int execDef(Thread* thread, const Module& module, PyModuleDef* def);

void initializeModuleType(Thread* thread);

}  // namespace py
