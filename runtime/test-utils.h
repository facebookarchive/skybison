#pragma once
#include <string>

namespace python {
class Runtime;
namespace testing {
std::string runToString(Runtime* runtime, const char* buffer);
}
} // namespace python
