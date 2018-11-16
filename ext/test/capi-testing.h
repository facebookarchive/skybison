#pragma once

#include "cpython-types.h"
#include "gtest/gtest.h"

namespace python {
namespace testing {

::testing::AssertionResult exceptionValueMatches(const char* message);
PyObject* moduleGet(const char* module, const char* name);
int moduleSet(const char* module, const char* name, PyObject*);

// Create an object that is guaranteed to have reference count one. Used when
// the type and value does not matter but reference count does.
PyObject* createUniqueObject();

}  // namespace testing
}  // namespace python
