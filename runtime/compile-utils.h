#pragma once

#include "handles-decl.h"
#include "symbols.h"

namespace py {

class Thread;

RawObject compile(Thread* thread, const Object& source, const Object& filename,
                  SymbolId mode, word flags, int optimize);

RawObject mangle(Thread* thread, const Object& privateobj, const Str& ident);

}  // namespace py
