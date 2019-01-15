#include "list-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject listExtend(Thread* thread, const List& dst, const Object& iterable) {
  HandleScope scope(thread);
  Object elt(&scope, NoneType::object());
  word index = dst->numItems();
  Runtime* runtime = thread->runtime();
  // Special case for lists
  if (runtime->isInstanceOfList(*iterable)) {
    List src(&scope, *iterable);
    if (src->numItems() > 0) {
      word new_capacity = index + src->numItems();
      runtime->listEnsureCapacity(dst, new_capacity);
      dst->setNumItems(new_capacity);
      for (word i = 0; i < src->numItems(); i++) {
        dst->atPut(index++, src->at(i));
      }
    }
    return *dst;
  }
  // Special case for list iterators
  if (iterable->isListIterator()) {
    ListIterator list_iter(&scope, *iterable);
    List src(&scope, list_iter->list());
    word new_capacity = index + src->numItems();
    runtime->listEnsureCapacity(dst, new_capacity);
    dst->setNumItems(new_capacity);
    for (word i = 0; i < src->numItems(); i++) {
      elt = list_iter->next();
      if (elt->isError()) {
        break;
      }
      dst->atPut(index++, src->at(i));
    }
    return *dst;
  }
  // Special case for tuples
  if (iterable->isTuple()) {
    Tuple tuple(&scope, *iterable);
    if (tuple->length() > 0) {
      word new_capacity = index + tuple->length();
      runtime->listEnsureCapacity(dst, new_capacity);
      dst->setNumItems(new_capacity);
      for (word i = 0; i < tuple->length(); i++) {
        dst->atPut(index++, tuple->at(i));
      }
    }
    return *dst;
  }
  // Special case for sets
  if (runtime->isInstanceOfSetBase(*iterable)) {
    SetBase set(&scope, *iterable);
    if (set->numItems() > 0) {
      Tuple data(&scope, set->data());
      word new_capacity = index + set->numItems();
      runtime->listEnsureCapacity(dst, new_capacity);
      dst->setNumItems(new_capacity);
      for (word i = 0; i < data->length(); i += SetBase::Bucket::kNumPointers) {
        if (!SetBase::Bucket::isFilled(data, i)) {
          continue;
        }
        dst->atPut(index++, SetBase::Bucket::key(*data, i));
      }
    }
    return *dst;
  }
  // Special case for dicts
  if (iterable->isDict()) {
    Dict dict(&scope, *iterable);
    if (dict->numItems() > 0) {
      Tuple keys(&scope, runtime->dictKeys(dict));
      word new_capacity = index + dict->numItems();
      runtime->listEnsureCapacity(dst, new_capacity);
      dst->setNumItems(new_capacity);
      for (word i = 0; i < keys->length(); i++) {
        dst->atPut(index++, keys->at(i));
      }
    }
    return *dst;
  }
  // Generic case
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterable, SymbolId::kDunderIter));
  if (iter_method->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, iterable));
  if (iterator->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    return thread->raiseTypeErrorWithCStr("iter() returned a non-iterator");
  }
  Object value(&scope, NoneType::object());
  for (;;) {
    value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                     next_method, iterator);
    if (value->isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *value;
    }
    runtime->listAdd(dst, value);
  }
  return *dst;
}

void listInsert(Thread* thread, const List& list, const Object& value,
                word index) {
  thread->runtime()->listAdd(list, value);
  word last_index = list->numItems() - 1;
  if (index < 0) {
    index = last_index + index;
  }
  index =
      Utils::maximum(static_cast<word>(0), Utils::minimum(last_index, index));
  for (word i = last_index; i > index; i--) {
    list->atPut(i, list->at(i - 1));
  }
  list->atPut(index, *value);
}

RawObject listPop(const List& list, word index) {
  HandleScope scope;
  Object popped(&scope, list->at(index));
  list->atPut(index, NoneType::object());
  word last_index = list->numItems() - 1;
  for (word i = index; i < last_index; i++) {
    list->atPut(i, list->at(i + 1));
  }
  list->setNumItems(list->numItems() - 1);
  return *popped;
}

RawObject listReplicate(Thread* thread, const List& list, word ntimes) {
  HandleScope scope(thread);
  word len = list->numItems();
  Runtime* runtime = thread->runtime();
  Tuple items(&scope, runtime->newTuple(ntimes * len));
  for (word i = 0; i < ntimes; i++) {
    for (word j = 0; j < len; j++) {
      items->atPut((i * len) + j, list->at(j));
    }
  }
  List result(&scope, runtime->newList());
  result->setItems(*items);
  result->setNumItems(items->length());
  return *result;
}

void listReverse(Thread* thread, const List& list) {
  if (list->numItems() == 0) {
    return;
  }
  HandleScope scope(thread);
  Object elt(&scope, NoneType::object());
  for (word low = 0, high = list->numItems() - 1; low < high; ++low, --high) {
    elt = list->at(low);
    list->atPut(low, list->at(high));
    list->atPut(high, *elt);
  }
}

const BuiltinAttribute ListBuiltins::kAttributes[] = {
    {SymbolId::kItems, RawList::kItemsOffset},
    {SymbolId::kAllocated, RawList::kAllocatedOffset},
};

const BuiltinMethod ListBuiltins::kMethods[] = {
    {SymbolId::kAppend, nativeTrampoline<append>},
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderDelItem, nativeTrampoline<dunderDelItem>},
    {SymbolId::kDunderGetItem, nativeTrampoline<dunderGetItem>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderMul, nativeTrampoline<dunderMul>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kDunderSetItem, nativeTrampoline<dunderSetItem>},
    {SymbolId::kExtend, nativeTrampoline<extend>},
    {SymbolId::kInsert, nativeTrampoline<insert>},
    {SymbolId::kPop, nativeTrampoline<pop>},
    {SymbolId::kRemove, nativeTrampoline<remove>}};

void ListBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;

  Type list(&scope,
            runtime->addBuiltinType(SymbolId::kList, LayoutId::kList,
                                    LayoutId::kObject, kAttributes, kMethods));
}

RawObject ListBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type->builtinBase() != LayoutId::kList) {
    return thread->raiseTypeErrorWithCStr("not a subtype of list");
  }
  Layout layout(&scope, type->instanceLayout());
  List result(&scope, thread->runtime()->newInstance(layout));
  result->setNumItems(0);
  result->setItems(thread->runtime()->newTuple(0));
  return *result;
}

RawObject ListBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }

  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__add__() must be called with list instance as first argument");
  }

  Object other(&scope, args.get(1));
  if (other->isList()) {
    word new_capacity =
        RawList::cast(*self)->numItems() + RawList::cast(*other)->numItems();
    List new_list(&scope, runtime->newList());
    runtime->listEnsureCapacity(new_list, new_capacity);
    new_list = listExtend(thread, new_list, self);
    if (!new_list->isError()) {
      new_list = listExtend(thread, new_list, other);
    }
    return *new_list;
  }
  return thread->raiseTypeErrorWithCStr("can only concatenate list to list");
}

RawObject ListBuiltins::append(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "append() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "append() only support list or its subclasses");
  }
  List list(&scope, *self);
  Object value(&scope, args.get(1));
  thread->runtime()->listAdd(list, value);
  return NoneType::object();
}

RawObject ListBuiltins::extend(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "extend() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "extend() only support list or its subclasses");
  }
  List list(&scope, *self);
  Object value(&scope, args.get(1));
  list = listExtend(thread, list, value);
  if (list->isError()) {
    return *list;
  }
  return NoneType::object();
}

RawObject ListBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__len__() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__len__() only support list or its subclasses");
  }
  List list(&scope, *self);
  return SmallInt::fromWord(list->numItems());
}

RawObject ListBuiltins::insert(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr(
        "insert() takes exactly two arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(1)->isInt()) {
    return thread->raiseTypeErrorWithCStr(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'insert' requires a 'list' object");
  }
  List list(&scope, *self);
  word index = RawSmallInt::cast(args.get(1))->value();
  Object value(&scope, args.get(2));
  listInsert(thread, list, value, index);
  return NoneType::object();
}

RawObject ListBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  RawObject other = args.get(1);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__mul__() must be called with list instance as first argument");
  }
  if (other->isSmallInt()) {
    word ntimes = RawSmallInt::cast(other)->value();
    List list(&scope, *self);
    return listReplicate(thread, list, ntimes);
  }
  return thread->raiseTypeErrorWithCStr("can't multiply list by non-int");
}

RawObject ListBuiltins::pop(Thread* thread, Frame* frame, word nargs) {
  if (nargs > 2) {
    return thread->raiseTypeErrorWithCStr("pop() takes at most 1 argument");
  }
  Arguments args(frame, nargs);
  if (nargs == 2 && !args.get(1)->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'pop' requires a 'list' object");
  }
  List list(&scope, *self);
  word index = list->numItems() - 1;
  if (nargs == 2) {
    word last_index = index;
    index = RawSmallInt::cast(args.get(1))->value();
    index = index < 0 ? last_index + index : index;
    // Pop out of bounds
    if (index > last_index) {
      // TODO(T27365047): Throw an IndexError exception
      UNIMPLEMENTED("Throw an RawIndexError for an out of range list index.");
    }
  }
  // Pop empty, or negative out of bounds
  if (index < 0) {
    // TODO(T27365047): Throw an IndexError exception
    UNIMPLEMENTED("Throw an RawIndexError for an out of range list index.");
  }

  return listPop(list, index);
}

RawObject ListBuiltins::remove(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "remove() takes exactly one argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object value(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'remove' requires a 'list' object");
  }
  List list(&scope, *self);
  for (word i = 0; i < list->numItems(); i++) {
    Object item(&scope, list->at(i));
    if (RawBool::cast(Interpreter::compareOperation(thread, frame,
                                                    CompareOp::EQ, item, value))
            ->value()) {
      listPop(list, i);
      return NoneType::object();
    }
  }
  return thread->raiseValueErrorWithCStr("list.remove(x) x not in list");
}

RawObject ListBuiltins::slice(Thread* thread, RawList list, RawSlice slice) {
  word start, stop, step;
  slice->unpack(&start, &stop, &step);
  word length = Slice::adjustIndices(list->numItems(), &start, &stop, step);

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Tuple items(&scope, runtime->newTuple(length));
  for (word i = 0, index = start; i < length; i++, index += step) {
    items->atPut(i, list->at(index));
  }

  List result(&scope, runtime->newList());
  result->setItems(*items);
  result->setNumItems(items->length());
  return *result;
}

RawObject ListBuiltins::dunderGetItem(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "__getitem__() takes exactly one argument (%ld given)", nargs - 1));
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));

  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__getitem__() must be called with a list instance as the first "
        "argument");
  }

  List list(&scope, *self);
  word length = list->numItems();
  RawObject index = args.get(1);
  if (index->isSmallInt()) {
    word idx = RawSmallInt::cast(index)->value();
    if (idx < 0) {
      idx += length;
    }
    if (idx < 0 || idx >= length) {
      return thread->raiseIndexErrorWithCStr("list index out of range");
    }
    return list->at(idx);
  }
  if (index->isSlice()) {
    Slice list_slice(&scope, RawSlice::cast(index));
    return slice(thread, *list, *list_slice);
  }
  return thread->raiseTypeErrorWithCStr(
      "list indices must be integers or slices");
}

RawObject ListBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a list instance as the first argument");
  }
  return thread->runtime()->newListIterator(self);
}

const BuiltinMethod ListIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>}};

void ListIteratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type list_iter(&scope, runtime->addBuiltinTypeWithMethods(
                             SymbolId::kListIterator, LayoutId::kListIterator,
                             LayoutId::kObject, kMethods));
}

RawObject ListIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                           word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isListIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a list iterator instance as the first "
        "argument");
  }
  return *self;
}

RawObject ListIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                           word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isListIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a list iterator instance as the first "
        "argument");
  }
  Object value(&scope, RawListIterator::cast(*self)->next());
  if (value->isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject ListIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                 word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isListIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a list iterator instance as the "
        "first argument");
  }
  ListIterator list_iterator(&scope, *self);
  List list(&scope, list_iterator->list());
  return SmallInt::fromWord(list->numItems() - list_iterator->index());
}

static RawObject setItemSlice(Thread* thread, List& list, Object& slice_index,
                              Object& src) {
  HandleScope scope(thread);
  Slice slice(&scope, Slice::cast(slice_index));
  word start, stop, step;
  slice->unpack(&start, &stop, &step);
  word slice_length =
      Slice::adjustIndices(list->numItems(), &start, &stop, step);
  word suffix_start = Utils::maximum(start + 1, stop);

  // degenerate case of a bad slice; behaves like a single index:
  if ((step >= 0) && (start > stop)) {
    stop = start;
    suffix_start = start;  // do not drop any items
  }

  // Potential optimization:
  // We could check for __length_hint__ or __len__ and attempt to
  // do in-place update or at least have a good estimate of
  // capacity for result_list
  Runtime* runtime = thread->runtime();
  List result_list(&scope, runtime->newList());

  // Note: initial capacity for result array here is an optimistic guess
  // since we don't yet know length iterator
  // Potential optimization: use __length_hint__ of src
  runtime->listEnsureCapacity(result_list, list->numItems());

  // copy prefix into result array:
  word prefix_length = Utils::minimum(start, stop + 1);
  result_list->setNumItems(prefix_length);
  for (word i = 0; i < prefix_length; i++) {
    result_list->atPut(i, list->at(i));
  }
  if (step == 1) {
    Object extend_result(&scope, listExtend(thread, result_list, src));
    if (extend_result->isError()) {
      return thread->raiseTypeErrorWithCStr("can only assign an iterable");
    }
    result_list = *extend_result;
  } else {
    Object iter_method(
        &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), src,
                                          SymbolId::kDunderIter));
    if (iter_method->isError()) {
      return thread->raiseTypeErrorWithCStr("object is not iterable");
    }
    Object iterator(&scope,
                    Interpreter::callMethod1(thread, thread->currentFrame(),
                                             iter_method, src));
    if (iterator->isError()) {
      return thread->raiseTypeErrorWithCStr("object is not iterable");
    }
    Object next_method(
        &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                          iterator, SymbolId::kDunderNext));
    if (next_method->isError()) {
      return thread->raiseTypeErrorWithCStr("iter() returned a non-iterator");
    }

    Object iter_length_val(&scope,
                           runtime->iteratorLengthHint(thread, iterator));
    if (iter_length_val->isError()) {
      return thread->raiseTypeErrorWithCStr(
          "slice assignment: unable to get length of assigned sequence");
    }
    word iter_length = SmallInt::cast(*iter_length_val)->value();
    if (slice_length != iter_length) {
      return thread->raiseValueError(thread->runtime()->newStrFromFormat(
          "attempt to assign sequence of size %ld to extended slice of size "
          "%ld",
          iter_length, slice_length));
    }

    Object value(&scope, NoneType::object());

    result_list->setNumItems(result_list->numItems() + std::abs(stop - start));
    for (word i = start, incr = (step > 0) ? 1 : -1, count = 0; i != stop;
         i += incr, count++) {
      if ((count % step) == 0) {
        value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                         next_method, iterator);
        if (value->isError()) {
          return *value;
        }
      } else {
        value = list->at(i);
      }
      result_list->atPut(i, value);
    }
  }

  // And now copy suffix:
  word suffix_length = list->numItems() - suffix_start;
  word result_length = result_list->numItems() + suffix_length;
  runtime->listEnsureCapacity(result_list, result_length);

  word index = result_list->numItems();
  result_list->setNumItems(result_length);
  for (word i = suffix_start; i < list->numItems(); i++, index++) {
    result_list->atPut(index, list->at(i));
  }

  list->setItems(result_list->items());
  list->setNumItems(result_list->numItems());
  return NoneType::object();
}

RawObject ListBuiltins::dunderSetItem(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "expected 2 arguments, got %ld", nargs - 1));
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));

  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__setitem__() must be called with a list instance as the first "
        "argument");
  }

  List list(&scope, *self);
  Object index(&scope, args.get(1));
  Object src(&scope, args.get(2));

  if (index->isSmallInt()) {
    word idx = RawSmallInt::cast(index)->value();
    word length = list->numItems();
    if (idx < 0) {
      idx += length;
    }
    if (idx < 0 || idx >= length) {
      return thread->raiseIndexErrorWithCStr(
          "list assignment index out of range");
    }
    list->atPut(idx, *src);
    return NoneType::object();
  }
  if (index->isSlice()) {
    return setItemSlice(thread, list, index, src);
  }
  return thread->raiseTypeErrorWithCStr(
      "list indices must be integers or slices");
}

RawObject ListBuiltins::dunderDelItem(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "expected 1 arguments, got %ld", nargs - 1));
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));

  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__delitem__() must be called with a list instance as the first "
        "argument");
  }

  List list(&scope, *self);
  word length = list->numItems();
  RawObject index = args.get(1);
  if (index->isSmallInt()) {
    word idx = RawSmallInt::cast(index)->value();
    if (idx < 0) {
      idx += length;
    }
    if (idx < 0 || idx >= length) {
      return thread->raiseIndexErrorWithCStr(
          "list assignment index out of range");
    }
    listPop(list, idx);
    return NoneType::object();
  }
  // TODO(T31826482): Add support for slices
  return thread->raiseTypeErrorWithCStr(
      "list indices must be integers or slices");
}

}  // namespace python
