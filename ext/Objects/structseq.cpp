#include "runtime.h"

#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "type-builtins.h"

char* PyStructSequence_UnnamedField = const_cast<char*>("unnamed field");

namespace python {

PY_EXPORT int PyStructSequence_InitType2(PyTypeObject*,
                                         PyStructSequence_Desc*) {
  UNIMPLEMENTED("PyStructSequence_InitType2 is not supported");
}

PY_EXPORT PyObject* PyStructSequence_GetItem(PyObject* /* p */,
                                             Py_ssize_t /* i */) {
  UNIMPLEMENTED("PyStructSequence_GetItem");
}

PY_EXPORT PyObject* PyStructSequence_New(PyTypeObject* /* e */) {
  UNIMPLEMENTED("PyStructSequence_New");
}

PY_EXPORT PyTypeObject* PyStructSequence_NewType(PyStructSequence_Desc* desc) {
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  // Create the class name
  const char* class_name = strrchr(desc->name, '.');
  if (class_name == nullptr) {
    class_name = desc->name;
  } else {
    class_name++;
  }
  Str name(&scope, runtime->newStrFromCStr(class_name));

  // Create bases
  Tuple bases(&scope, runtime->newTuple(1));
  bases.atPut(0, runtime->typeAt(LayoutId::kTuple));

  // Create the structsequence dictionary
  word num_fields = 0;
  word num_unnamed_fields = 0;
  while (desc->fields[num_fields].name != nullptr) {
    if (desc->fields[num_fields].name == PyStructSequence_UnnamedField) {
      num_unnamed_fields++;
    }
    num_fields++;
  }
  Dict dict(&scope, runtime->newDict());
  Object n_sequence_key(&scope, runtime->symbols()->NSequenceFields());
  Object n_sequence(&scope, runtime->newInt(desc->n_in_sequence));
  runtime->typeDictAtPut(dict, n_sequence_key, n_sequence);
  Object n_fields_key(&scope, runtime->symbols()->NFields());
  Object n_fields(&scope, runtime->newInt(num_fields));
  runtime->typeDictAtPut(dict, n_fields_key, n_fields);
  Object unnamed_fields_key(&scope, runtime->symbols()->NUnnamedFields());
  Object unnamed_fields(&scope, runtime->newInt(num_unnamed_fields));
  runtime->typeDictAtPut(dict, unnamed_fields_key, unnamed_fields);

  Type type(&scope, typeNew(thread, LayoutId::kType, name, bases, dict));
  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::newReference(thread, *type));
}

PY_EXPORT void PyStructSequence_SetItem(PyObject* /* p */, Py_ssize_t /* i */,
                                        PyObject* /* v */) {
  UNIMPLEMENTED("PyStructSequence_SetItem");
}

}  // namespace python
