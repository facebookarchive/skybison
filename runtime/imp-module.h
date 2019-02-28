#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"

namespace python {

RawObject builtinImpAcquireLock(Thread* thread, Frame* frame, word nargs);
RawObject builtinImpCreateBuiltin(Thread* thread, Frame* frame, word nargs);
RawObject builtinImpExecBuiltin(Thread* thread, Frame* frame, word nargs);
RawObject builtinImpExecDynamic(Thread* thread, Frame* frame, word nargs);
RawObject builtinImpExtensionSuffixes(Thread* thread, Frame* frame, word nargs);
RawObject builtinImpFixCoFilename(Thread* thread, Frame* frame, word nargs);
RawObject builtinImpGetFrozenObject(Thread* thread, Frame* frame, word nargs);
RawObject builtinImpIsBuiltin(Thread* thread, Frame* frame, word nargs);
RawObject builtinImpIsFrozen(Thread* thread, Frame* frame, word nargs);
RawObject builtinImpIsFrozenPackage(Thread* thread, Frame* frame, word nargs);
RawObject builtinImpReleaseLock(Thread* thread, Frame* frame, word nargs);

void importAcquireLock(Thread* thread);
bool importReleaseLock(Thread* thread);

}  // namespace python
