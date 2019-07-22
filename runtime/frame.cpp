#include "frame.h"

#include <cstring>

namespace python {

const char* Frame::isInvalid() {
  if (!at(kPreviousFrameOffset).isSmallInt()) {
    return "bad previousFrame field";
  }
  if (!isSentinel() && !(locals() + 1)->isFunction()) {
    return "bad function";
  }
  return nullptr;
}

}  // namespace python
