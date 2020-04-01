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

PY_EXPORT PyObject* PyStructSequence_GetItem(PyObject* structseq,
                                             Py_ssize_t pos) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  UserTupleBase user_tuple(&scope,
                           ApiHandle::fromPyObject(structseq)->asObject());
  Tuple tuple(&scope, user_tuple.value());
  word num_in_sequence = tuple.length();
  word user_tuple_fields = (UserTupleBase::kSize / kPointerSize);
  word num_fields =
      num_in_sequence + user_tuple.headerCountOrOverflow() - user_tuple_fields;
  CHECK_INDEX(pos, num_fields);
  if (pos < num_in_sequence) {
    return ApiHandle::borrowedReference(thread, tuple.at(pos));
  }
  word offset = (pos - num_in_sequence + user_tuple_fields) * kPointerSize;
  return ApiHandle::borrowedReference(thread,
                                      user_tuple.instanceVariableAt(offset));
}

PY_EXPORT PyObject* PyStructSequence_SET_ITEM_Func(PyObject* structseq,
                                                   Py_ssize_t pos,
                                                   PyObject* value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  UserTupleBase user_tuple(&scope,
                           ApiHandle::fromPyObject(structseq)->asObject());
  Tuple tuple(&scope, user_tuple.value());
  Object value_obj(&scope, value == nullptr
                               ? NoneType::object()
                               : ApiHandle::stealReference(thread, value));
  word num_in_sequence = tuple.length();
  word user_tuple_fields = (RawUserTupleBase::kSize / kPointerSize);
  word num_fields =
      num_in_sequence + user_tuple.headerCountOrOverflow() - user_tuple_fields;
  CHECK_INDEX(pos, num_fields);
  if (pos < num_in_sequence) {
    tuple.atPut(pos, *value_obj);
  } else {
    word offset = (pos - num_in_sequence + user_tuple_fields) * kPointerSize;
    user_tuple.instanceVariableAtPut(offset, *value_obj);
  }
  return value;
}

PY_EXPORT void PyStructSequence_SetItem(PyObject* structseq, Py_ssize_t pos,
                                        PyObject* value) {
  PyStructSequence_SET_ITEM_Func(structseq, pos, value);
}

PY_EXPORT PyObject* PyStructSequence_New(PyTypeObject* pytype) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, ApiHandle::fromPyTypeObject(pytype)->asObject());
  Object result(&scope, structseqNew(thread, type));
  if (result.isErrorException()) return nullptr;
  return ApiHandle::newReference(thread, *result);
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
  Tuple field_names(&scope, runtime->newTuple(num_fields));
  for (word i = 0; i < num_fields; i++) {
    const char* field_name = desc->fields[i].name;
    field_names.atPut(i, field_name == PyStructSequence_UnnamedField
                             ? NoneType::object()
                             : Runtime::internStrFromCStr(thread, field_name));
  }
  Str name(&scope, runtime->newStrFromCStr(desc->name));
  Object result(
      &scope, structseqNewType(thread, name, field_names, desc->n_in_sequence));
  if (result.isErrorException()) return nullptr;
  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::newReference(thread, *result));
}

}  // namespace py
