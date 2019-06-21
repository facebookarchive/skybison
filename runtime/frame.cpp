#include "frame.h"

#include <cstring>

namespace python {

void Frame::makeSentinel() { std::memset(this, 0, Frame::kSize); }

const char* Frame::isInvalid() {
  if (!at(kPreviousFrameOffset).isSmallInt()) {
    return "bad previousFrame field";
  }
  if (!at(kNumLocalsOffset).isSmallInt()) {
    return "bad numLocals field";
  }
  if (!isSentinel() && !(locals() + 1)->isFunction()) {
    return "bad function";
  }
  return nullptr;
}

}  // namespace python
