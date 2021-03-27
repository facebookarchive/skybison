#include "cpython-func.h"

#include "builtins-module.h"
#include "capi-handles.h"
#include "dict-builtins.h"
#include "module-builtins.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "structseq-builtins.h"
#include "type-builtins.h"

char* PyStructSequence_UnnamedField = const_cast<char*>("unnamed field");

namespace py {

PY_EXPORT PyObject* PyStructSequence_GET_ITEM_Func(PyObject* structseq,
                                                   Py_ssize_t pos) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object structseq_obj(&scope, ApiHandle::fromPyObject(structseq)->asObject());
  Object result(&scope, structseqGetItem(thread, structseq_obj, pos));
  return result.isUnbound()
             ? nullptr
             : ApiHandle::newReference(thread->runtime(), *result);
}

PY_EXPORT PyObject* PyStructSequence_GetItem(PyObject* structseq,
                                             Py_ssize_t pos) {
  return PyStructSequence_GET_ITEM_Func(structseq, pos);
}

PY_EXPORT PyObject* PyStructSequence_SET_ITEM_Func(PyObject* structseq,
                                                   Py_ssize_t pos,
                                                   PyObject* value) {
  PyStructSequence_SetItem(structseq, pos, value);
  return value;
}

PY_EXPORT void PyStructSequence_SetItem(PyObject* structseq, Py_ssize_t pos,
                                        PyObject* value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object structseq_obj(&scope, ApiHandle::fromPyObject(structseq)->asObject());
  Object value_obj(&scope, value == nullptr
                               ? Unbound::object()
                               : ApiHandle::fromPyObject(value)->asObject());
  structseqSetItem(thread, structseq_obj, pos, value_obj);
}

PY_EXPORT PyObject* PyStructSequence_New(PyTypeObject* pytype) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, ApiHandle::fromPyTypeObject(pytype)->asObject());
  Object result(&scope, structseqNew(thread, type));
  if (result.isErrorException()) return nullptr;
  return ApiHandle::newReference(thread->runtime(), *result);
}

PY_EXPORT PyTypeObject* PyStructSequence_NewType(PyStructSequence_Desc* desc) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  word num_fields = 0;
  for (PyStructSequence_Field* field = desc->fields; field->name != nullptr;
       field++) {
    num_fields++;
  }
  MutableTuple field_names(&scope, runtime->newMutableTuple(num_fields));
  for (word i = 0; i < num_fields; i++) {
    const char* field_name = desc->fields[i].name;
    field_names.atPut(i, field_name == PyStructSequence_UnnamedField
                             ? NoneType::object()
                             : Runtime::internStrFromCStr(thread, field_name));
  }
  Tuple field_names_tuple(&scope, field_names.becomeImmutable());
  Str name(&scope, runtime->newStrFromCStr(desc->name));
  word flags = Type::Flag::kIsCPythonHeaptype;
  Object result(&scope, structseqNewType(thread, name, field_names_tuple,
                                         desc->n_in_sequence, flags));
  if (result.isErrorException()) return nullptr;
  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::newReference(runtime, *result));
}

}  // namespace py
