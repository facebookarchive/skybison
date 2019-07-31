#pragma once

// This exposes some modsupport code to abstract.cpp; it is not cpython API.

#include <cstdarg>

#include "cpython-types.h"

namespace python {

const int kFlagSizeT = 1;

PyObject* makeValueFromFormat(const char** p_format, va_list* p_va, int flags);

}  // namespace python
