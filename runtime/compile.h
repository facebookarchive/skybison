#pragma once

#include "handles.h"
#include "objects.h"

namespace python {

RawObject compileFromCStr(const char* buffer, const char* file_name);

}  // namespace python
