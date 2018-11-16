/* longobject.c implementation */

#include "Python.h"

#include "runtime/handles.h"
#include "runtime/objects.h"
#include "runtime/runtime.h"

namespace python {

extern "C" PyObject* PyLong_FromLong(long ival) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

  word val = reinterpret_cast<word>(ival);
  Handle<Object> value(&scope, runtime->newInteger(val));
  return runtime->asApiHandle(*value)->asPyObject();
}

} // namespace python
