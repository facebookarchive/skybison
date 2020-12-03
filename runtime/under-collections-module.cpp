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
    {ID(_deque__state), RawDeque::kStateOffset, AttributeFlags::kHidden},
};

static const BuiltinAttribute kDequeIteratorAttributes[] = {
    {ID(_deque_iterator__iterable), RawDequeIterator::kIterableOffset,
     AttributeFlags::kHidden},
    {ID(_deque_iterator__index), RawDequeIterator::kIndexOffset,
     AttributeFlags::kHidden},
    {ID(_deque_iterator__state), RawDequeIterator::kStateOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kDequeReverseIteratorAttributes[] = {
    {ID(_deque_reverse_iterator__iterable),
     RawDequeReverseIterator::kIterableOffset, AttributeFlags::kHidden},
    {ID(_deque_reverse_iterator__index), RawDequeReverseIterator::kIndexOffset,
     AttributeFlags::kHidden},
    {ID(_deque_reverse_iterator__state), RawDequeReverseIterator::kStateOffset,
     AttributeFlags::kHidden},
};

void initializeUnderCollectionsTypes(Thread* thread) {
  addBuiltinType(thread, ID(deque), LayoutId::kDeque,
                 /*superclass_id=*/LayoutId::kObject, kDequeAttributes,
                 Deque::kSize, /*basetype=*/true);
  addBuiltinType(thread, ID(_deque_iterator), LayoutId::kDequeIterator,
                 /*superclass_id=*/LayoutId::kObject, kDequeIteratorAttributes,
                 DequeIterator::kSize, /*basetype=*/false);
  addBuiltinType(
      thread, ID(_deque_reverse_iterator), LayoutId::kDequeReverseIterator,
      /*superclass_id=*/LayoutId::kObject, kDequeReverseIteratorAttributes,
      DequeReverseIterator::kSize, /*basetype=*/false);
}

RawObject FUNC(_collections, _deque_delitem)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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

  MutableTuple items(&scope, deque.items());
  word left = deque.left();
  word num_items = deque.numItems();
  if (deque_index < left) {
    // shift (deque_index, right] one to the left
    word right = left + num_items - items.length() - 1;
    items.replaceFromWithStartAt(deque_index, *items, right - deque_index,
                                 deque_index + 1);
    items.atPut(right, NoneType::object());
  } else {
    // shift [left, deque_index) one to the right
    items.replaceFromWithStartAt(left + 1, *items, deque_index - left, left);
    items.atPut(left, NoneType::object());
    word new_left = left + 1;
    deque.setLeft(new_left < items.length() ? new_left : 0);
  }
  deque.setNumItems(num_items - 1);
  return NoneType::object();
}

RawObject FUNC(_collections, _deque_getitem)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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

RawObject FUNC(_collections, _deque_set_maxlen)(Thread* thread,
                                                Arguments args) {
  HandleScope scope(thread);
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

RawObject FUNC(_collections, _deque_setitem)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
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
  deque.atPut(deque_index, args.get(2));
  return NoneType::object();
}

RawObject METH(_deque_iterator, __length_hint__)(Thread* thread,
                                                 Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isDequeIterator()) {
    return thread->raiseRequiresType(self_obj, ID(_deque_iterator));
  }
  DequeIterator self(&scope, *self_obj);
  Deque deque(&scope, self.iterable());
  return SmallInt::fromWord(deque.numItems() - self.index());
}

RawObject METH(_deque_iterator, __new__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object cls(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*cls)) {
    return thread->raiseRequiresType(cls, ID(type));
  }
  if (cls != runtime->typeAt(LayoutId::kDequeIterator)) {
    Type type(&scope, *cls);
    Str name(&scope, type.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "_collections._deque_iterator.__new__(%S): "
        "%S is not a subtype of _collections._deque_iterator",
        &name, &name);
  }

  Object iterable(&scope, args.get(1));
  if (!runtime->isInstanceOfDeque(*iterable)) {
    return thread->raiseRequiresType(iterable, ID(deque));
  }
  Deque deque(&scope, *iterable);

  Object index_obj(&scope, args.get(2));
  index_obj = intFromIndex(thread, index_obj);
  if (index_obj.isErrorException()) {
    return *index_obj;
  }
  Int index_int(&scope, intUnderlying(*index_obj));
  if (index_int.numDigits() > 1) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C ssize_t");
  }
  word index = index_int.asWord();
  if (index < 0) {
    index = 0;
  } else {
    word length = deque.numItems();
    if (index > length) {
      index = length;
    }
  }
  return runtime->newDequeIterator(deque, index);
}

RawObject METH(_deque_iterator, __next__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isDequeIterator()) {
    return thread->raiseRequiresType(self_obj, ID(_deque_iterator));
  }

  DequeIterator self(&scope, *self_obj);
  Deque deque(&scope, self.iterable());
  if (deque.state() != self.state()) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "deque mutated during iteration");
  }

  word index = self.index();
  word length = deque.numItems();
  DCHECK_BOUND(index, length);
  if (index == length) {
    return thread->raiseStopIteration();
  }

  self.setIndex(index + 1);
  index += deque.left();
  word capacity = deque.capacity();
  if (index < capacity) {
    return deque.at(index);
  }
  return deque.at(index - capacity);
}

RawObject METH(_deque_iterator, __reduce__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isDequeIterator()) {
    return thread->raiseRequiresType(self_obj, ID(_deque_iterator));
  }
  Runtime* runtime = thread->runtime();
  DequeIterator self(&scope, *self_obj);
  Object type(&scope, runtime->typeAt(LayoutId::kDequeIterator));
  Object deque(&scope, self.iterable());
  Object index(&scope, SmallInt::fromWord(self.index()));
  Object tuple(&scope, runtime->newTupleWith2(deque, index));
  return runtime->newTupleWith2(type, tuple);
}

RawObject METH(_deque_reverse_iterator, __length_hint__)(Thread* thread,
                                                         Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isDequeReverseIterator()) {
    return thread->raiseRequiresType(self_obj, ID(_deque_reverse_iterator));
  }
  DequeReverseIterator self(&scope, *self_obj);
  Deque deque(&scope, self.iterable());
  return SmallInt::fromWord(deque.numItems() - self.index());
}

RawObject METH(_deque_reverse_iterator, __new__)(Thread* thread,
                                                 Arguments args) {
  HandleScope scope(thread);
  Object cls(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfType(*cls)) {
    return thread->raiseRequiresType(cls, ID(type));
  }
  if (cls != runtime->typeAt(LayoutId::kDequeReverseIterator)) {
    Type type(&scope, *cls);
    Str name(&scope, type.name());
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "_collections._deque_reverse_iterator.__new__(%S): "
        "%S is not a subtype of _collections._deque_reverse_iterator",
        &name, &name);
  }

  Object iterable(&scope, args.get(1));
  if (!runtime->isInstanceOfDeque(*iterable)) {
    return thread->raiseRequiresType(iterable, ID(deque));
  }
  Deque deque(&scope, *iterable);

  Object index_obj(&scope, args.get(2));
  index_obj = intFromIndex(thread, index_obj);
  if (index_obj.isErrorException()) {
    return *index_obj;
  }
  Int index_int(&scope, intUnderlying(*index_obj));
  if (index_int.numDigits() > 1) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "Python int too large to convert to C ssize_t");
  }
  word index = index_int.asWord();
  if (index < 0) {
    index = 0;
  } else {
    word length = deque.numItems();
    if (index > length) {
      index = length;
    }
  }
  return runtime->newDequeReverseIterator(deque, index);
}

RawObject METH(_deque_reverse_iterator, __next__)(Thread* thread,
                                                  Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isDequeReverseIterator()) {
    return thread->raiseRequiresType(self_obj, ID(_deque_reverse_iterator));
  }

  DequeReverseIterator self(&scope, *self_obj);
  Deque deque(&scope, self.iterable());
  if (deque.state() != self.state()) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "deque mutated during iteration");
  }

  word index = self.index();
  word length = deque.numItems();
  DCHECK_BOUND(index, length);
  if (index == length) {
    return thread->raiseStopIteration();
  }

  index += 1;
  self.setIndex(index);
  index = deque.left() + deque.numItems() - index;
  word capacity = deque.capacity();
  if (index < capacity) {
    return deque.at(index);
  }
  return deque.at(index - capacity);
}

RawObject METH(_deque_reverse_iterator, __reduce__)(Thread* thread,
                                                    Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isDequeIterator()) {
    return thread->raiseRequiresType(self_obj, ID(_deque_iterator));
  }
  Runtime* runtime = thread->runtime();
  DequeReverseIterator self(&scope, *self_obj);
  Object type(&scope, runtime->typeAt(LayoutId::kDequeReverseIterator));
  Object deque(&scope, self.iterable());
  Object index(&scope, SmallInt::fromWord(self.index()));
  Object tuple(&scope, runtime->newTupleWith2(deque, index));
  return runtime->newTupleWith2(type, tuple);
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
  word new_length = num_items - 1;
  word tail = deque.left() + new_length;
  word capacity = deque.capacity();
  if (tail >= capacity) {
    tail -= capacity;
  }
  Object result(&scope, deque.at(tail));
  deque.atPut(tail, NoneType::object());
  deque.setNumItems(new_length);
  return *result;
}

static RawObject dequePopLeft(Thread* thread, const Deque& deque) {
  HandleScope scope(thread);
  word num_items = deque.numItems();
  DCHECK(num_items != 0, "cannot pop from empty deque");
  word head = deque.left();
  word new_head = head + 1;
  word capacity = deque.capacity();
  if (new_head >= capacity) {
    new_head -= capacity;
  }
  Object result(&scope, deque.at(head));
  deque.atPut(head, NoneType::object());
  deque.setNumItems(num_items - 1);
  deque.setLeft(new_head);
  return *result;
}

RawObject METH(deque, __iter__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  return runtime->newDequeIterator(deque, 0);
}

RawObject METH(deque, __len__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  return SmallInt::fromWord(deque.numItems());
}

RawObject METH(deque, __new__)(Thread* thread, Arguments args) {
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
  deque.setState(0);
  return *deque;
}

RawObject METH(deque, __reversed__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  return runtime->newDequeReverseIterator(deque, 0);
}

RawObject METH(deque, append)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  Object value(&scope, args.get(1));
  dequeAppend(thread, deque, value);
  deque.setState(deque.state() + 1);
  return NoneType::object();
}

RawObject METH(deque, appendleft)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  deque.setState(deque.state() + 1);
  Object value(&scope, args.get(1));
  dequeAppendLeft(thread, deque, value);
  return NoneType::object();
}

RawObject METH(deque, clear)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  deque.setState(deque.state() + 1);
  deque.clear();
  return NoneType::object();
}

RawObject METH(deque, pop)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  if (deque.numItems() == 0) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "pop from empty deque");
  }
  deque.setState(deque.state() + 1);
  return dequePop(thread, deque);
}

RawObject METH(deque, popleft)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }
  Deque deque(&scope, *self);
  if (deque.numItems() == 0) {
    return thread->raiseWithFmt(LayoutId::kIndexError, "pop from empty deque");
  }
  deque.setState(deque.state() + 1);
  return dequePopLeft(thread, deque);
}

RawObject METH(deque, reverse)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDeque(*self)) {
    return thread->raiseRequiresType(self, ID(deque));
  }

  Deque deque(&scope, *self);
  word length = deque.numItems();
  if (length == 0) {
    return NoneType::object();
  }

  MutableTuple items(&scope, deque.items());
  word left = deque.left();
  word capacity = items.length();
  word right = left + length;

  word prefix = right - capacity;
  word suffix = capacity - left;
  if (prefix > suffix) {
    for (right = prefix - 1; left < capacity; left++, right--) {
      items.swap(left, right);
    }
    left = 0;
  } else if (prefix > 0) {
    for (right = prefix - 1; right >= 0; left++, right--) {
      items.swap(left, right);
    }
    right += capacity;
  } else {
    right--;
  }

  for (; left < right; left++, right--) {
    items.swap(left, right);
  }

  return NoneType::object();
}

}  // namespace py
