#include "frame.h"

#include <cstring>

#include "objects.h"

namespace python {

void Frame::makeSentinel() {
  std::memset(this, 0, Frame::kSize);
}

} // namespace python
