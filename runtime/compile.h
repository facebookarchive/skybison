#pragma once

#include "handles.h"
#include "objects.h"

namespace py {

RawObject compileFromCStr(const char* buffer, const char* file_name);
int runInteractive(FILE* fp);

}  // namespace py
