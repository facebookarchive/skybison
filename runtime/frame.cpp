#include "frame.h"

#include <cstring>

#include "objects.h"

namespace python {

word Frame::allocationSize(Object* object) {
  Code* code = Code::cast(object);
  word ncells = ObjectArray::cast(code->cellvars())->length();
  word nfrees = ObjectArray::cast(code->freevars())->length();
  word extras =
      code->nlocals() - code->argcount() + ncells + nfrees + code->stacksize();
  return kSize + extras * kPointerSize;
}

void Frame::makeSentinel() {
  std::memset(this, 0, Frame::kSize);
}

} // namespace python
