#pragma once

#include "globals.h"
#include "handles-decl.h"
#include "symbols.h"

namespace py {

class Thread;

RawObject delAttribute(Thread* thread, const Object& object,
                       const Object& name);
RawObject getAttribute(Thread* thread, const Object& object,
                       const Object& name);
RawObject hasAttribute(Thread* thread, const Object& object,
                       const Object& name);
RawObject setAttribute(Thread* thread, const Object& object, const Object& name,
                       const Object& value);

RawObject compile(Thread* thread, const Object& source, const Object& filename,
                  SymbolId mode, word flags, int optimize);

}  // namespace py
