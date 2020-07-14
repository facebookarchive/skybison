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

RawObject FUNC(_collections, _deque_getitem)(Thread* thread, Frame* frame,
                                             word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDeque(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(deque));
  }
  Object key(&scope, args.get(1));
  if (!runtime->isInstanceOfInt(*key)) {
    return Unbound::object();
  }

  word index = intUnderlying(*key).asWordSaturated();
  if (!SmallInt::isValid(index)) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "cannot fit '%T' into an index-sized integer",
                                &key);
  }
  Deque deque(&scope, *self_obj);
  word deque_index = dequeIndex(deque, index);
  if (deque_index == -1) {
    return thread->raiseWithFmt(LayoutId::kIndexError,
                                "deque index out of range");
  }
  return deque.at(deque_index);
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
  word num_items = deque.numItems();
  if (num_items > 0) {
    Tuple old_array(&scope, deque.items());
    word left = deque.left();
    word right = left + num_items;
    if (right >= curr_capacity) {
      right -= curr_capacity;
    }
    if (right < left) {
      word count = curr_capacity - left;
      new_array.replaceFromWithStartAt(0, *old_array, count, left);
      new_array.replaceFromWithStartAt(count, *old_array, right, 0);
    } else {
      new_array.replaceFromWithStartAt(0, *old_array, num_items, left);
    }
  }

  deque.setItems(*new_array);
  deque.setLeft(0);
}

static void dequeAppend(Thread* thread, const Deque& deque,
                        const Object& value) {
  word num_items = deque.numItems();
  if (deque.maxlen() == SmallInt::fromWord(num_items)) {
    if (num_items == 0) return;
    word left = deque.left();
    word new_left = left + 1;
    word capacity = deque.capacity();
    if (new_left == capacity) {
      new_left = 0;
    }
    deque.atPut(left, *value);
    deque.setLeft(new_left);
    return;
  }
  dequeEnsureCapacity(thread, deque, num_items + 1);
  word left = deque.left();
  word capacity = deque.capacity();
  DCHECK(num_items < capacity, "deque should not be full");
  // wrap right around to beginning of tuple if end reached
  word right = left + num_items;
  if (right >= capacity) {
    right -= capacity;
  }
  deque.setNumItems(num_items + 1);
  deque.atPut(right, *value);
}

static void dequeAppendLeft(Thread* thread, const Deque& deque,
                            const Object& value) {
  word num_items = deque.numItems();
  if (deque.maxlen() == SmallInt::fromWord(num_items)) {
    if (num_items == 0) return;
    word new_left = deque.left() - 1;
    if (new_left < 0) {
      new_left += deque.capacity();
    }
    deque.atPut(new_left, *value);
    deque.setLeft(new_left);
    return;
  }
  dequeEnsureCapacity(thread, deque, num_items + 1);
  word new_left = deque.left();
  new_left -= 1;
  if (new_left < 0) {
    new_left += deque.capacity();
  }
  deque.setNumItems(num_items + 1);
  deque.atPut(new_left, *value);
  deque.setLeft(new_left);
}

word dequeIndex(const Deque& deque, word index) {
  word num_items = deque.numItems();
  if (index >= num_items || index < -num_items) {
    return -1;
  }
  if (index < 0) {
    index += num_items;
  }
  word deque_index = deque.left() + index;
  word capacity = deque.capacity();
  if (deque_index >= capacity) {
    deque_index -= capacity;
  }
  return deque_index;
}

static RawObject dequePop(Thread* thread, const Deque& deque) {
  HandleScope scope(thread);
  word num_items = deque.numItems();
  DCHECK(num_items != 0, "cannot pop from empty deque");
  word tail = (deque.left() + num_items - 1) % deque.capacity();
  Object result(&scope, deque.at(tail));
  deque.atPut(tail, NoneType::object());
  deque.setNumItems(num_items - 1);
  return *result;
}

static RawObject dequePopLeft(Thread* thread, const Deque& deque) {
  HandleScope scope(thread);
  word num_items = deque.numItems();
  DCHECK(num_items != 0, "cannot pop from empty deque");
  word head = deque.left();
  Object result(&scope, deque.at(head));
  deque.atPut(head, NoneType::object());
  deque.setNumItems(deque.numItems() - 1);
  deque.setLeft(head + 1);
  return *result;
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
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  Object value(&scope, args.get(1));
  dequeAppend(thread, deque, value);
  return NoneType::object();
}

RawObject METH(deque, appendleft)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  Object value(&scope, args.get(1));
  dequeAppendLeft(thread, deque, value);
  return NoneType::object();
}

RawObject METH(deque, clear)(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  deque.clear();
  return NoneType::object();
}

RawObject METH(deque, pop)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  if (deque.numItems() == 0) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "pop from empty deque");
  }
  return dequePop(thread, deque);
}

RawObject METH(deque, popleft)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  if (deque.numItems() == 0) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "pop from empty deque");
  }
  return dequePopLeft(thread, deque);
}

}  // namespace py
