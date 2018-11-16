#include "frame.h"

#include <cstring>

namespace python {

void Frame::makeSentinel() {
  std::memset(this, 0, Frame::kSize);
}

} // namespace python
