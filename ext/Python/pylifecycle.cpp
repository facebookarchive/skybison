#include "runtime.h"

namespace python {

extern "C" void Py_Initialize(void) { new Runtime; }

extern "C" int Py_FinalizeEx(void) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  delete runtime;
  return 0;
}

}  // namespace python
