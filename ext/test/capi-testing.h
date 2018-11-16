#pragma once

#include "runtime.h"

namespace python {
namespace testing {

bool exceptionMessageMatches(const char* message);
PyObject* moduleGet(const char* module, const char* name);
bool isBorrowed(PyObject* pyobj);

}  // namespace testing
}  // namespace python
