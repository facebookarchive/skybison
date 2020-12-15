#include "structseq-builtins.h"

#include "dict-builtins.h"
#include "module-builtins.h"
#include "runtime.h"
#include "str-builtins.h"
#include "type-builtins.h"

namespace py {

static const word kUserTupleFields = UserTupleBase::kSize / kPointerSize;

RawObject structseqGetItem(Thread* thread, const Object& structseq,
                           word index) {
  HandleScope scope(thread);
  UserTupleBase user_tuple(&scope, *structseq);
  Tuple tuple(&scope, user_tuple.value());
  word num_in_sequence = tuple.length();
  if (index < num_in_sequence) {
    return tuple.at(index);
  }
  word attribute_index = index - num_in_sequence + kUserTupleFields;
  CHECK_INDEX(attribute_index, user_tuple.headerCountOrOverflow());
  return user_tuple.instanceVariableAt(attribute_index * kPointerSize);
}

RawObject structseqSetItem(Thread* thread, const Object& structseq, word index,
                           const Object& value) {
  HandleScope scope(thread);
  UserTupleBase user_tuple(&scope, *structseq);
  Tuple tuple(&scope, user_tuple.value());
  word num_in_sequence = tuple.length();
  if (index < num_in_sequence) {
    tuple.atPut(index, *value);
    return NoneType::object();
  }
  word attribute_index = index - num_in_sequence + kUserTupleFields;
  CHECK_INDEX(attribute_index, user_tuple.headerCountOrOverflow());
  user_tuple.instanceVariableAtPut(attribute_index * kPointerSize, *value);
  return NoneType::object();
}

RawObject structseqNew(Thread* thread, const Type& type) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, type.instanceLayout());
  UserTupleBase result(&scope, runtime->newInstance(layout));
  Int n_sequence_fields(&scope,
                        typeAtById(thread, type, ID(n_sequence_fields)));
  result.setValue(runtime->newTuple(n_sequence_fields.asWord()));
  return *result;
}

RawObject structseqNewType(Thread* thread, const Str& name,
                           const Tuple& field_names, word num_in_sequence,
                           word flags) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  word num_fields = field_names.length();
  DCHECK(num_in_sequence <= num_fields, "n_in_sequence too big");

  Str type_name(&scope, *name);
  Object module_name(&scope, NoneType::object());
  word dot = strRFindAsciiChar(name, '.');
  if (dot >= 0) {
    module_name = strSubstr(thread, name, 0, dot);
    type_name = strSubstr(thread, name, dot + 1, name.length() - (dot + 1));
  } else {
    module_name = runtime->symbols()->at(ID(builtins));
  }

  Dict dict(&scope, runtime->newDict());
  dictAtPutById(thread, dict, ID(__qualname__), type_name);
  dictAtPutById(thread, dict, ID(__module__), module_name);

  // Create type
  Tuple bases(&scope, runtime->newTuple(1));
  bases.atPut(0, runtime->typeAt(LayoutId::kTuple));
  Type metaclass(&scope, runtime->typeAt(LayoutId::kType));
  Type type(&scope, typeNew(thread, metaclass, type_name, bases, dict, flags,
                            /*inherit_slots=*/true,
                            /*add_instance_dict=*/false));

  // Add hidden fields as in-object attributes in the instance layout.
  Layout layout(&scope, type.instanceLayout());
  DCHECK(layout.numInObjectAttributes() == kUserTupleFields,
         "unexpected number of attributes");
  if (num_fields > num_in_sequence) {
    Str field_name(&scope, Str::empty());
    for (word i = num_in_sequence, offset = RawUserTupleBase::kSize;
         i < num_fields; i++, offset += kPointerSize) {
      AttributeInfo info(offset, AttributeFlags::kInObject);
      Tuple entries(&scope, layout.inObjectAttributes());
      field_name = field_names.at(i);
      layout.setNumInObjectAttributes(layout.numInObjectAttributes() + 1);
      layout.setInObjectAttributes(
          runtime->layoutAddAttributeEntry(thread, entries, field_name, info));
    }
  }
  layout.seal();

  word num_unnamed_fields = 0;
  Object field_name(&scope, Str::empty());
  Object descriptor(&scope, NoneType::object());
  SmallInt index(&scope, SmallInt::fromWord(0));
  for (word i = 0; i < num_fields; i++) {
    field_name = field_names.at(i);
    if (field_name.isNoneType()) {
      DCHECK(i < num_in_sequence, "unnamed fields must be in-sequence");
      num_unnamed_fields++;
      continue;
    }
    DCHECK(Runtime::isInternedStr(thread, field_name),
           "field_names must contain interned strings or None");
    index = SmallInt::fromWord(i);
    descriptor = thread->invokeFunction2(ID(builtins), ID(_structseq_field),
                                         type, index);
    if (descriptor.isErrorException()) return *descriptor;
    typeAtPut(thread, type, field_name, descriptor);
  }

  typeAtPutById(thread, type, ID(_structseq_field_names), field_names);
  Object value(&scope, SmallInt::fromWord(num_fields));
  typeAtPutById(thread, type, ID(n_fields), value);
  value = SmallInt::fromWord(num_in_sequence);
  typeAtPutById(thread, type, ID(n_sequence_fields), value);
  value = SmallInt::fromWord(num_unnamed_fields);
  typeAtPutById(thread, type, ID(n_unnamed_fields), value);

  Module builtins(&scope, runtime->findModuleById(ID(builtins)));
  value = moduleAtById(thread, builtins, ID(_structseq_new));
  typeAtPutById(thread, type, ID(__new__), value);
  CHECK(typeAtById(thread, type, ID(__new__)) == value, "");
  Object n(&scope, runtime->symbols()->at(ID(__new__)));
  CHECK(typeAt(type, n) == value, "");
  CHECK(typeGetAttribute(thread, type, n) == value, "");
  value = moduleAtById(thread, builtins, ID(_structseq_repr));
  typeAtPutById(thread, type, ID(__repr__), value);

  return *type;
}

}  // namespace py
