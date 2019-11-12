#include "builtins-module.h"
#include "capi-handles.h"
#include "cpython-func.h"
#include "dict-builtins.h"
#include "module-builtins.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "type-builtins.h"

char* PyStructSequence_UnnamedField = const_cast<char*>("unnamed field");

namespace py {

PY_EXPORT PyObject* PyStructSequence_GetItem(PyObject* structseq,
                                             Py_ssize_t pos) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object structseq_obj(&scope, ApiHandle::fromPyObject(structseq)->asObject());
  Object pos_obj(&scope, thread->runtime()->newInt(pos));

  // _structseq_getitem(structseq, pos)
  Object result(&scope,
                thread->invokeFunction2(SymbolId::kBuiltins,
                                        SymbolId::kUnderStructseqGetItem,
                                        structseq_obj, pos_obj));
  if (result.isError()) return nullptr;
  return ApiHandle::borrowedReference(thread, *result);
}

PY_EXPORT PyObject* PyStructSequence_SET_ITEM_Func(PyObject* structseq,
                                                   Py_ssize_t pos,
                                                   PyObject* value) {
  // This function can't be implemented in Python since it's an immutable type
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Object structseq_obj(&scope, ApiHandle::fromPyObject(structseq)->asObject());

  // Bound check
  Str n_fields_key(&scope, runtime->symbols()->NFields());
  Int n_fields(&scope,
               runtime->attributeAt(thread, structseq_obj, n_fields_key));
  Object value_obj(&scope, value == nullptr
                               ? NoneType::object()
                               : ApiHandle::stealReference(thread, value));
  if (pos < 0 || pos >= n_fields.asWord()) {
    thread->raiseWithFmt(LayoutId::kIndexError,
                         "tuple assignment index out of range");
    return value;
  }
  // Try to set to the tuple first
  Str n_sequence_key(&scope, runtime->symbols()->NSequenceFields());
  Int n_sequence(&scope,
                 runtime->attributeAt(thread, structseq_obj, n_sequence_key));
  if (pos < n_sequence.asWord()) {
    UserTupleBase user_tuple(&scope, *structseq_obj);
    Tuple tuple(&scope, user_tuple.value());
    tuple.atPut(pos, *value_obj);
    return value;
  }

  // Fall back to the hidden fields.
  Str field_names_key(&scope, runtime->symbols()->UnderStructseqFieldNames());
  Tuple field_names(
      &scope, runtime->attributeAt(thread, structseq_obj, field_names_key));
  Str field_name(&scope, field_names.at(pos));

  // Bypass the immutability of structseq_field.__set__
  Instance instance(&scope, *structseq_obj);
  instanceSetAttr(thread, instance, field_name, value_obj);
  return value;
}

PY_EXPORT void PyStructSequence_SetItem(PyObject* structseq, Py_ssize_t pos,
                                        PyObject* value) {
  PyStructSequence_SET_ITEM_Func(structseq, pos, value);
}

PY_EXPORT PyObject* PyStructSequence_New(PyTypeObject* pytype) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(&scope, ApiHandle::fromPyTypeObject(pytype)->asObject());
  Str n_fields_key(&scope, runtime->symbols()->NFields());
  Int n_fields(&scope, runtime->attributeAt(thread, type, n_fields_key));
  Tuple tuple(&scope, runtime->newTuple(n_fields.asWord()));

  // _structseq_new(type, tuple)
  Object structseq(&scope, thread->invokeFunction2(SymbolId::kBuiltins,
                                                   SymbolId::kUnderStructseqNew,
                                                   type, tuple));
  return ApiHandle::newReference(thread, *structseq);
}

PY_EXPORT PyTypeObject* PyStructSequence_NewType(PyStructSequence_Desc* desc) {
  Thread* thread = Thread::current();
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

  // Add n_sequence_fields
  Dict dict(&scope, runtime->newDict());
  Object n_sequence(&scope, runtime->newInt(desc->n_in_sequence));
  dictAtPutById(thread, dict, SymbolId::kNSequenceFields, n_sequence);

  // Add n_fields
  word num_fields = 0;
  while (desc->fields[num_fields].name != nullptr) {
    CHECK(desc->fields[num_fields].name != PyStructSequence_UnnamedField,
          "The use of unnamed fields is not allowed");
    num_fields++;
  }
  Object n_fields(&scope, runtime->newInt(num_fields));
  dictAtPutById(thread, dict, SymbolId::kNFields, n_fields);

  // unnamed fields are banned. This is done to support _structseq_getitem(),
  // which accesses hidden fields by name.
  Object unnamed_fields(&scope, runtime->newInt(0));
  dictAtPutById(thread, dict, SymbolId::kNUnnamedFields, unnamed_fields);

  // Add __new__
  Module builtins(&scope, runtime->findModuleById(SymbolId::kBuiltins));
  Function structseq_new(
      &scope, moduleAtById(thread, builtins, SymbolId::kUnderStructseqNew));
  dictAtPutById(thread, dict, SymbolId::kDunderNew, structseq_new);

  // Add __repr__
  Function structseq_repr(
      &scope, moduleAtById(thread, builtins, SymbolId::kUnderStructseqRepr));
  dictAtPutById(thread, dict, SymbolId::kDunderRepr, structseq_repr);

  Tuple field_names(&scope, runtime->newTuple(num_fields));
  for (word i = 0; i < num_fields; i++) {
    field_names.atPut(i,
                      runtime->internStrFromCStr(thread, desc->fields[i].name));
  }
  dictAtPutById(thread, dict, SymbolId::kUnderStructseqFieldNames, field_names);

  // Create type
  Tuple bases(&scope, runtime->newTuple(1));
  bases.atPut(0, runtime->typeAt(LayoutId::kTuple));
  Type type(&scope, typeNew(thread, LayoutId::kType, name, bases, dict,
                            static_cast<Type::Flag>(0)));

  // Add struct sequence fields
  Str field_name(&scope, Str::empty());
  Object index(&scope, NoneType::object());
  Object field(&scope, NoneType::object());
  for (word i = 0; i < num_fields; i++) {
    field_name = field_names.at(i);
    index = i < desc->n_in_sequence ? runtime->newInt(i) : NoneType::object();
    field = thread->invokeFunction2(
        SymbolId::kBuiltins, SymbolId::kUnderStructseqField, field_name, index);
    typeSetAttr(thread, type, field_name, field);
  }

  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::newReference(thread, *type));
}

}  // namespace py
