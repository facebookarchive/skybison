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

#define EXPECT_DEBUG_ONLY_DEATH(stmt, pattern) \
  EXPECT_DEBUG_DEATH((stmt), (pattern))

#endif

namespace python {
class Runtime;
namespace testing {
std::string runToString(Runtime* runtime, const char* buffer);

// Compile the supplied python snippet, run it, and return stdout.
std::string compileAndRunToString(Runtime* runtime, const char* src);

// A predicate-formatter for checking if a python::String* has the same contents
// as a std::string
inline ::testing::AssertionResult AssertPyStringEqual(
    const char* string_expr,
    const char* /*c_string_expr*/,
    String* observed_string,
    std::string expected_string) {
  std::string std_str;
  std_str.reserve(observed_string->length());
  for (word i = 0; i < observed_string->length(); i++) {
    std_str += observed_string->charAt(i);
  }

  if (std_str == expected_string) {
    return ::testing::AssertionSuccess();
  }

  return ::testing::AssertionFailure()
      << "      Expected: " << string_expr << "\n"
      << "      Which is: \"" << std_str << "\"\n"
      << "To be equal to: \"" << expected_string << "\"";
}

#define EXPECT_PYSTRING_EQ(s1, s2) \
  EXPECT_PRED_FORMAT2(AssertPyStringEqual, s1, s2)

// Get the value bound to name in the supplied module. Returns Error::object()
// if not found.
Object*
findInModule(Runtime* runtime, const Handle<Module>& module, const char* name);

// Calls func using the supplied arguments and captures output.
//
// This opens a new frame linked to the initial frame of the current thread,
// pushes all the arguments onto the stack, and invokes the interpreter.
std::string callFunctionToString(
    const Handle<Function>& func,
    const Handle<ObjectArray>& args);

bool objectArrayContains(
    const Handle<ObjectArray>& object_array,
    const Handle<Object>& key);

} // namespace testing
} // namespace python
