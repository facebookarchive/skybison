#include "frame.h"
#include "objects.h"

namespace python {

int Frame::allocationSize(Object* object) {
  Code* code = Code::cast(object);
  int ncells = ObjectArray::cast(code->cellvars())->length();
  int nfrees = ObjectArray::cast(code->freevars())->length();
  int extras = code->nlocals() - code->argcount() + ncells + nfrees;
  return kSize + extras * kPointerSize;
}

} // namespace python
