#pragma once

#include "cpython-data.h"

#include "globals.h"
#include "handles.h"
#include "modules.h"
#include "runtime.h"
#include "symbols.h"
#include "thread.h"
#include "view.h"

namespace py {

// Get a function pointer out of the current frame's consts.
inline void* getNativeFunc(Thread* thread) {
  HandleScope scope(thread);
  Code code(&scope, thread->currentFrame()->code());
  Tuple consts(&scope, code.consts());
  DCHECK(consts.length() == 1, "Unexpected tuple length");
  Int raw_fn(&scope, consts.at(0));
  return raw_fn.asCPtr();
}

RawObject newExtCode(Thread* thread, const Object& name,
                     View<SymbolId> parameters, word flags,
                     BuiltinFunction function, void* slot_value);

RawObject newGetSet(Thread* thread, const Object& name, PyGetSetDef* def);

}  // namespace py
