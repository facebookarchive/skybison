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

static const BuiltinAttribute kDequeAttributes[] = {
    {ID(_deque__items), RawDeque::kItemsOffset, AttributeFlags::kHidden},
    {ID(_deque__left), RawDeque::kLeftOffset, AttributeFlags::kHidden},
    {ID(_deque__num_items), RawDeque::kNumItemsOffset, AttributeFlags::kHidden},
    {ID(maxlen), RawDeque::kMaxlenOffset, AttributeFlags::kReadOnly},
};

void initializeUnderCollectionsTypes(Thread* thread) {
  addBuiltinType(thread, ID(deque), LayoutId::kDeque,
                 /*superclass_id=*/LayoutId::kObject, kDequeAttributes);
}

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

static void dequeEnsureCapacity(Thread* thread, const Deque& deque,
                                word min_capacity) {
  DCHECK_BOUND(min_capacity, SmallInt::kMaxValue);
  word curr_capacity = deque.capacity();
  if (min_capacity <= curr_capacity) return;
  word new_capacity = Runtime::newCapacity(curr_capacity, min_capacity);
  RawObject maxlen = deque.maxlen();
  if (!maxlen.isNoneType()) {
    new_capacity = Utils::minimum(new_capacity, SmallInt::cast(maxlen).value());
  }

  HandleScope scope(thread);
  MutableTuple new_array(&scope,
                         thread->runtime()->newMutableTuple(new_capacity));
  word length = deque.numItems();
  if (length > 0) {
    Tuple old_array(&scope, deque.items());
    word left = deque.left();
    word right = (left + length) % curr_capacity;
    if (right < left) {
      word count = curr_capacity - left;
      new_array.replaceFromWithStartAt(0, *old_array, count, left);
      new_array.replaceFromWithStartAt(count, *old_array, right, 0);
    } else {
      new_array.replaceFromWithStartAt(0, *old_array, length, left);
    }
  }

  deque.setItems(*new_array);
  deque.setLeft(0);
}

static void dequeAppend(Thread* thread, const Deque& deque,
                        const Object& value) {
  dequeEnsureCapacity(thread, deque, deque.numItems() + 1);

  // TODO(T67099800): make append over maxlen call popleft before adding new
  // element

  word left = deque.left();
  word length = deque.numItems();
  word capacity = deque.capacity();
  DCHECK(length < capacity, "deque should not be full");

  // wrap right around to beginning of tuple if end reached
  word right = (left + length) % capacity;
  deque.setNumItems(length + 1);
  deque.atPut(right, *value);
}

RawObject METH(deque, __len__)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  return SmallInt::fromWord(deque.numItems());
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
  deque.setItems(SmallInt::fromWord(0));
  deque.setLeft(0);
  deque.setNumItems(0);
  return *deque;
}

RawObject METH(deque, append)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  Object value(&scope, args.get(1));
  dequeAppend(thread, deque, value);
  return NoneType::object();
}

}  // namespace py
