#pragma once
#include <string>

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
}
} // namespace python
