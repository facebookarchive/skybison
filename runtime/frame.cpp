#include "frame.h"

#include <cstring>

namespace python {

void Frame::makeSentinel() { std::memset(this, 0, Frame::kSize); }

RawObject Frame::function() {
  if (previousFrame() == nullptr) return Error::notFound();
  RawObject result = *(locals() + 1);
  if (!result.isFunction()) return Error::notFound();
  return result;
}

}  // namespace python
