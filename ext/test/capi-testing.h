#pragma once

#include "cpython-types.h"
#include "gtest/gtest.h"

namespace python {
namespace testing {

::testing::AssertionResult exceptionValueMatches(const char* message);
PyObject* moduleGet(const char* module, const char* name);
int moduleSet(const char* module, const char* name, PyObject*);

}  // namespace testing
}  // namespace python
