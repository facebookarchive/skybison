#include "frame.h"

#include <cstring>

namespace python {

void Frame::makeSentinel() { std::memset(this, 0, Frame::kSize); }

RawObject Frame::function() {
  if (previousFrame() == nullptr) return Error::object();
  RawObject result = *(locals() + 1);
  if (!result.isFunction()) return Error::object();
  return result;
}

}  // namespace python
