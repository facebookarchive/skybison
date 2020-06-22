#include "under-collections-module.h"

#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "slice-builtins.h"
#include "thread.h"
#include "tuple-builtins.h"
#include "type-builtins.h"

namespace py {

static const BuiltinType kCollectionsBuiltinTypes[] = {
    {ID(deque), LayoutId::kDeque},
};

void FUNC(_collections, __init_module__)(Thread* thread, const Module& module,
                                         View<byte> bytecode) {
  moduleAddBuiltinTypes(thread, module, kCollectionsBuiltinTypes);
  executeFrozenModule(thread, module, bytecode);
}

RawObject FUNC(_collections, _deque_set_maxlen)(Thread* thread, Frame* frame,
                                                word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Deque deque(&scope, args.get(0));
  Object maxlen_obj(&scope, args.get(1));
  if (maxlen_obj.isNoneType()) {
    deque.setMaxlen(NoneType::object());
    return NoneType::object();
  }
  if (!thread->runtime()->isInstanceOfInt(*maxlen_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "an integer is required");
  }
  word maxlen = intUnderlying(*maxlen_obj).asWordSaturated();
  if (!SmallInt::isValid(maxlen)) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C ssize_t");
  }
  if (maxlen < 0) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "maxlen must be non-negative");
  }
  deque.setMaxlen(SmallInt::fromWord(maxlen));
  return NoneType::object();
}

static const BuiltinAttribute kDequeAttributes[] = {
    {ID(maxlen), RawDeque::kMaxlenOffset, AttributeFlags::kReadOnly},
};

void initializeUnderCollectionsTypes(Thread* thread) {
  addBuiltinType(thread, ID(deque), LayoutId::kDeque,
                 /*superclass_id=*/LayoutId::kObject, kDequeAttributes);
}

RawObject METH(deque, __new__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object type_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a type object");
  }
  Type type(&scope, *type_obj);
  if (type.builtinBase() != LayoutId::kDeque) {
    return thread->raiseWithFmt(LayoutId::kTypeError, "not a subtype of deque");
  }
  Layout layout(&scope, type.instanceLayout());
  Deque deque(&scope, runtime->newInstance(layout));
  return *deque;
}

}  // namespace py
