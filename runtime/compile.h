#pragma once

#include "handles.h"
#include "objects.h"

namespace python {

RawObject bytecodeToCode(Thread* thread, const char* buffer);
RawObject compileFromCStr(const char* buffer, const char* file_name);

}  // namespace python
