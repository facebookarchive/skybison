#pragma once

#include "handles.h"
#include "objects.h"

#include <string>
#include "gtest/gtest.h"

// Define EXPECT_DEBUG_ONLY_DEATH, which is like gtest's
// EXPECT_DEBUG_DEATH, except that it does not execute the given statment
// at all in release mode. This is necessary because most of our tests
// which rely on EXPECT_DEBUG_DEATH will cause unsafe operations to
// occur if the statement is executed.

#ifdef NDEBUG

#define EXPECT_DEBUG_ONLY_DEATH(stmt, pattern)

#else

#define EXPECT_DEBUG_ONLY_DEATH(stmt, pattern)                                 \
  EXPECT_DEBUG_DEATH((stmt), (pattern))

#endif

namespace python {
class Runtime;
namespace testing {
// Compile the supplied python snippet, run it, and return stdout or stderr
std::string compileAndRunToString(Runtime* runtime, const char* src);
std::string compileAndRunToStderrString(Runtime* runtime, const char* src);

// A predicate-formatter for checking if a python::Str* has the same contents
// as a std::string
::testing::AssertionResult AssertPyStringEqual(const char* actual_string_expr,
                                               const char* expected_string_expr,
                                               Str* actual_str,
                                               std::string expected_string);

::testing::AssertionResult AssertPyStringEqual(const char* actual_string_expr,
                                               const char* expected_string_expr,
                                               Str* actual_str,
                                               Str* expected_str);

#define EXPECT_PYSTRING_EQ(s1, s2)                                             \
  EXPECT_PRED_FORMAT2(testing::AssertPyStringEqual, s1, s2)

// Calls func using the supplied arguments and captures output.
//
// This opens a new frame linked to the initial frame of the current thread,
// pushes all the arguments onto the stack, and invokes the interpreter.
//
// The caller is responsible for cleaning up any exception state.
std::string callFunctionToString(const Handle<Function>& func,
                                 const Handle<ObjectArray>& args);

// Calls func using the supplied arguments.
//
// This opens a new frame linked to the initial frame of the current thread,
// pushes all the arguments onto the stack, and invokes the interpreter.
//
// The caller is responsible for cleaning up any exception state.
Object* callFunction(const Handle<Function>& func,
                     const Handle<ObjectArray>& args);

bool objectArrayContains(const Handle<ObjectArray>& object_array,
                         const Handle<Object>& key);

// Get the module bound to name in the given runtime. Returns Error::object() if
// not found.
Object* findModule(Runtime* runtime, const char* name);

// Get the value bound to name in the supplied module. Returns Error::object()
// if not found.
Object* moduleAt(Runtime* runtime, const Handle<Module>& module,
                 const char* name);

// Variant of moduleAt() that looks up the module and name at the same time,
// returning Error::object() if either can't be found.
Object* moduleAt(Runtime* runtime, const char* module_name, const char* name);

}  // namespace testing
}  // namespace python
