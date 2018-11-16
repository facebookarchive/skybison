#include "test-utils.h"

#include <memory>
#include <sstream>
#include "builtins.h"
#include "runtime.h"

namespace python {
namespace testing {

// helper function to redirect return stdout from running
// a moodule
std::string runToString(Runtime* runtime, const char* buffer) {
  std::stringstream stream;
  std::ostream* oldStream = builtinPrintStream;
  builtinPrintStream = &stream;
  Object* result = runtime->run(buffer);
  (void)result;
  assert(result == None::object());
  builtinPrintStream = oldStream;
  return stream.str();
}

std::string compileAndRunToString(Runtime* runtime, const char* src) {
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  return runToString(runtime, buffer.get());
}

} // namespace testing
} // namespace python
