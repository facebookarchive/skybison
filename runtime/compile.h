#pragma once

#include "handles.h"
#include "objects.h"

namespace py {

RawObject bytecodeToCode(Thread* thread, const char* buffer);
RawObject compileFromCStr(const char* buffer, const char* file_name);
int runInteractive(FILE* fp);

}  // namespace py
