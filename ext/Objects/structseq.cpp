#include "runtime.h"

#include "builtins-module.h"
#include "cpython-func.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "trampolines-inl.h"
#include "type-builtins.h"

char* PyStructSequence_UnnamedField = const_cast<char*>("unnamed field");

namespace python {

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

PY_EXPORT void PyStructSequence_SetItem(PyObject* structseq, Py_ssize_t pos,
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
  if (pos < 0 || pos >= n_fields.asWord()) {
    thread->raiseIndexErrorWithCStr("tuple assignment index out of range");
    return;
  }
  ApiHandle* value_handle = ApiHandle::fromPyObject(value);
  Object value_obj(&scope, value_handle->asObject());

  // Try to set to the tuple first
  Str n_sequence_key(&scope, runtime->symbols()->NSequenceFields());
  Int n_sequence(&scope,
                 runtime->attributeAt(thread, structseq_obj, n_sequence_key));
  if (pos < n_sequence.asWord()) {
    UserTupleBase user_tuple(&scope, *structseq_obj);
    Tuple tuple(&scope, user_tuple.tupleValue());
    tuple.atPut(pos, *value_obj);
    value_handle->decref();  // steal reference
    return;
  }

  // Fall back to the hidden fields.
  Str field_names_key(&scope, runtime->symbols()->UnderStructseqFieldNames());
  Tuple field_names(
      &scope, runtime->attributeAt(thread, structseq_obj, field_names_key));
  Str field_name(&scope, field_names.at(pos));

  // Bypass the immutability of structseq_field.__set__
  HeapObject structseq_heap(&scope, *structseq_obj);
  instanceSetAttr(thread, structseq_heap, field_name, value_obj);
  value_handle->decref();  // steal reference
}

PY_EXPORT PyObject* PyStructSequence_New(PyTypeObject* pytype) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Type type(
      &scope,
      ApiHandle::fromPyObject(reinterpret_cast<PyObject*>(pytype))->asObject());
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
  Object n_sequence_key(&scope, runtime->symbols()->NSequenceFields());
  Object n_sequence(&scope, runtime->newInt(desc->n_in_sequence));
  runtime->typeDictAtPut(thread, dict, n_sequence_key, n_sequence);

  // Add n_fields
  word num_fields = 0;
  while (desc->fields[num_fields].name != nullptr) {
    CHECK(desc->fields[num_fields].name != PyStructSequence_UnnamedField,
          "The use of unnamed fields is not allowed");
    num_fields++;
  }
  Object n_fields_key(&scope, runtime->symbols()->NFields());
  Object n_fields(&scope, runtime->newInt(num_fields));
  runtime->typeDictAtPut(thread, dict, n_fields_key, n_fields);

  // unnamed fields are banned. This is done to support _structseq_getitem(),
  // which accesses hidden fields by name.
  Object unnamed_fields_key(&scope, runtime->symbols()->NUnnamedFields());
  Object unnamed_fields(&scope, runtime->newInt(0));
  runtime->typeDictAtPut(thread, dict, unnamed_fields_key, unnamed_fields);

  // Add __new__
  Module builtins(&scope, runtime->findModuleById(SymbolId::kBuiltins));
  Function structseq_new(
      &scope, runtime->moduleAtById(builtins, SymbolId::kUnderStructseqNew));
  Str dunder_new_name(&scope, runtime->symbols()->DunderNew());
  runtime->typeDictAtPut(thread, dict, dunder_new_name, structseq_new);

  // Add __repr__
  Function structseq_repr(
      &scope, runtime->moduleAtById(builtins, SymbolId::kUnderStructseqRepr));
  Str dunder_repr_name(&scope, runtime->symbols()->DunderRepr());
  runtime->typeDictAtPut(thread, dict, dunder_repr_name, structseq_repr);

  // Create type
  Tuple bases(&scope, runtime->newTuple(1));
  bases.atPut(0, runtime->typeAt(LayoutId::kTuple));
  Type type(&scope, typeNew(thread, LayoutId::kType, name, bases, dict));

  // Add struct sequence fields
  Tuple field_names(&scope, runtime->newTuple(num_fields));
  for (int i = 0; i < num_fields; i++) {
    Object member_name(&scope,
                       runtime->internStrFromCStr(desc->fields[i].name));
    Object index(&scope, i < desc->n_in_sequence ? runtime->newInt(i)
                                                 : NoneType::object());
    Object structseq_field(
        &scope, thread->invokeFunction2(SymbolId::kBuiltins,
                                        SymbolId::kUnderStructseqField,
                                        member_name, index));
    typeSetAttr(thread, type, member_name, structseq_field);
    field_names.atPut(i, *member_name);
  }
  Str field_names_key(&scope, runtime->symbols()->UnderStructseqFieldNames());
  runtime->typeDictAtPut(thread, dict, field_names_key, field_names);

  return reinterpret_cast<PyTypeObject*>(
      ApiHandle::newReference(thread, *type));
}

}  // namespace python
