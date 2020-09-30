#include "strarray-builtins.h"

#include "builtins.h"
#include "runtime.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kStrArrayAttributes[] = {
    {ID(_str_array__items), RawStrArray::kItemsOffset, AttributeFlags::kHidden},
    {ID(_str_array__num_items), RawStrArray::kNumItemsOffset,
     AttributeFlags::kHidden},
};

void initializeStrArrayType(Thread* thread) {
  addBuiltinType(thread, ID(_str_array), LayoutId::kStrArray,
                 /*superclass_id=*/LayoutId::kObject, kStrArrayAttributes,
                 /*basetype=*/false);
}

RawObject METH(_str_array, __init__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isStrArray()) {
    return thread->raiseRequiresType(self_obj, ID(_str_array));
  }
  StrArray self(&scope, *self_obj);
  self.setNumItems(0);
  Object source(&scope, args.get(1));
  if (source.isUnbound()) {
    return NoneType::object();
  }
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*source)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "_str_array can only be initialized with str");
  }
  Str source_str(&scope, strUnderlying(*source));
  runtime->strArrayAddStr(thread, self, source_str);
  return NoneType::object();
}

RawObject METH(_str_array, __new__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  DCHECK(args.get(0) == runtime->typeAt(LayoutId::kStrArray),
         "_str_array.__new__(X): X is not '_str_array'");
  return runtime->newStrArray();
}

RawObject METH(_str_array, __str__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isStrArray()) {
    return thread->raiseRequiresType(self_obj, ID(_str_array));
  }
  StrArray self(&scope, *self_obj);
  return thread->runtime()->strFromStrArray(self);
}

}  // namespace py
