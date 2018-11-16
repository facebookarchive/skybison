#pragma once

#include "cpython-types.h"
#include "gtest/gtest.h"

namespace python {
namespace testing {

::testing::AssertionResult exceptionValueMatches(const char* message);
PyObject* moduleGet(const char* module, const char* name);

}  // namespace testing
}  // namespace python
