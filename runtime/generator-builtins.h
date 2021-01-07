#pragma once

#include "runtime.h"

namespace py {

// Get the RawGeneratorBase corresponding to the given Frame, assuming it is
// executing in a resumed GeneratorBase.
RawGeneratorBase generatorFromStackFrame(Frame* frame);

RawObject generatorSend(Thread* thread, const Object& self_obj,
                        const Object& value);

RawObject coroutineSend(Thread* thread, const Object& self,
                        const Object& value);

void initializeGeneratorTypes(Thread* thread);

}  // namespace py
