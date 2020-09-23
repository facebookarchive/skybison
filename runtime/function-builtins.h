#pragma once

#include "runtime.h"

namespace py {

void initializeFunctionTypes(Thread* thread);

// Fetches the type associated with the given C-API slot wrapper function. This
// is used in type checking arguments in slot functions.
RawObject slotWrapperFunctionType(const Function& function);

// Sets the type associated with the given C-API slot wrapper function. The
// type will be used in tyope checking arguments in slot functions.
void slotWrapperFunctionSetType(const Function& function, const Type& type);

}  // namespace py
