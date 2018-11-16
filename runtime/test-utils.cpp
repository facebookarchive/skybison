#include "test-utils.h"

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
} // namespace testing
} // namespace python
