#pragma once

// This exposes some modsupport code to abstract.cpp; it is not cpython API.

#include <cstdarg>

#include "cpython-types.h"

namespace py {

const int kFlagSizeT = 1;

PyObject* makeValueFromFormat(const char** p_format, std::va_list* p_va,
                              int flags);
Py_ssize_t countFormat(const char* format, char endchar);

}  // namespace py
