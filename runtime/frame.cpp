#include "frame.h"

#include <cstring>

namespace python {

void Frame::makeSentinel() { std::memset(this, 0, Frame::kSize); }

RawObject Frame::function() { return *(locals() + 1); }

}  // namespace python
