#pragma once

#include "runtime.h"

#include "gtest/gtest.h"

namespace python {
namespace testing {

::testing::AssertionResult exceptionValueMatches(const char* message);
PyObject* moduleGet(const char* module, const char* name);
bool isBorrowed(PyObject* pyobj);

}  // namespace testing
}  // namespace python
