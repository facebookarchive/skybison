#pragma once

#include "frame.h"
#include "symbols.h"
#include "thread.h"

namespace py {

// Executes the function at the given symbol without pushing a new frame.
// If the call succeeds, pops the arguments off of the caller's frame, sets the
// top value to the result, and returns true. If the call fails, leaves the
// stack unchanged and returns false.
bool doIntrinsic(Thread* thread, SymbolId name);

}  // namespace py
