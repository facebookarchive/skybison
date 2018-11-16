#pragma once

#include "runtime.h"

namespace python {
namespace testing {

int exceptionMessageMatches(const char* message);
PyObject* moduleGet(const char* module, const char* name);
int isBorrowed(PyObject*);

}  // namespace testing
}  // namespace python
