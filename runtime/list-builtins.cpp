#include "list-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinAttribute ListBuiltins::kAttributes[] = {
    {SymbolId::kItems, List::kItemsOffset},
    {SymbolId::kAllocated, List::kAllocatedOffset},
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

  Handle<Type> list(&scope, runtime->addBuiltinClass(
                                SymbolId::kList, LayoutId::kList,
                                LayoutId::kObject, kAttributes, kMethods));
  list->setFlag(Type::Flag::kListSubclass);
}

Object* ListBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Handle<Type> type(&scope, args.get(0));
  if (!type->hasFlag(Type::Flag::kListSubclass)) {
    return thread->raiseTypeErrorWithCStr("not a subtype of list");
  }
  Handle<Layout> layout(&scope, type->instanceLayout());
  Handle<List> result(&scope, thread->runtime()->newInstance(layout));
  result->setAllocated(0);
  result->setItems(thread->runtime()->newObjectArray(0));
  return *result;
}

Object* ListBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }

  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__add__() must be called with list instance as first argument");
  }

  Handle<Object> other(&scope, args.get(1));
  if (other->isList()) {
    word new_capacity =
        List::cast(*self)->allocated() + List::cast(*other)->allocated();
    Handle<List> new_list(&scope, runtime->newList());
    runtime->listEnsureCapacity(new_list, new_capacity);
    new_list = runtime->listExtend(thread, new_list, self);
    if (!new_list->isError()) {
      new_list = runtime->listExtend(thread, new_list, other);
    }
    return *new_list;
  }
  return thread->raiseTypeErrorWithCStr("can only concatenate list to list");
}

Object* ListBuiltins::append(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "append() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "append() only support list or its subclasses");
  }
  Handle<List> list(&scope, *self);
  Handle<Object> value(&scope, args.get(1));
  thread->runtime()->listAdd(list, value);
  return None::object();
}

Object* ListBuiltins::extend(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "extend() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "extend() only support list or its subclasses");
  }
  Handle<List> list(&scope, *self);
  Handle<Object> value(&scope, args.get(1));
  list = thread->runtime()->listExtend(thread, list, value);
  if (list->isError()) {
    return *list;
  }
  return None::object();
}

Object* ListBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__len__() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__len__() only support list or its subclasses");
  }
  Handle<List> list(&scope, *self);
  return SmallInt::fromWord(list->allocated());
}

Object* ListBuiltins::insert(Thread* thread, Frame* frame, word nargs) {
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
  Handle<Object> self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'insert' requires a 'list' object");
  }
  Handle<List> list(&scope, *self);
  word index = SmallInt::cast(args.get(1))->value();
  Handle<Object> value(&scope, args.get(2));
  thread->runtime()->listInsert(list, value, index);
  return None::object();
}

Object* ListBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Object* other = args.get(1);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__mul__() must be called with list instance as first argument");
  }
  if (other->isSmallInt()) {
    word ntimes = SmallInt::cast(other)->value();
    Handle<List> list(&scope, *self);
    return thread->runtime()->listReplicate(thread, list, ntimes);
  }
  return thread->raiseTypeErrorWithCStr("can't multiply list by non-int");
}

Object* ListBuiltins::pop(Thread* thread, Frame* frame, word nargs) {
  if (nargs > 2) {
    return thread->raiseTypeErrorWithCStr("pop() takes at most 1 argument");
  }
  Arguments args(frame, nargs);
  if (nargs == 2 && !args.get(1)->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'pop' requires a 'list' object");
  }
  Handle<List> list(&scope, *self);
  word index = list->allocated() - 1;
  if (nargs == 2) {
    word last_index = index;
    index = SmallInt::cast(args.get(1))->value();
    index = index < 0 ? last_index + index : index;
    // Pop out of bounds
    if (index > last_index) {
      // TODO(T27365047): Throw an IndexError exception
      UNIMPLEMENTED("Throw an IndexError for an out of range list index.");
    }
  }
  // Pop empty, or negative out of bounds
  if (index < 0) {
    // TODO(T27365047): Throw an IndexError exception
    UNIMPLEMENTED("Throw an IndexError for an out of range list index.");
  }

  return thread->runtime()->listPop(list, index);
}

Object* ListBuiltins::remove(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "remove() takes exactly one argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> value(&scope, args.get(1));
  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'remove' requires a 'list' object");
  }
  Handle<List> list(&scope, *self);
  for (word i = 0; i < list->allocated(); i++) {
    Handle<Object> item(&scope, list->at(i));
    if (Bool::cast(Interpreter::compareOperation(thread, frame, CompareOp::EQ,
                                                 item, value))
            ->value()) {
      thread->runtime()->listPop(list, i);
      return None::object();
    }
  }
  return thread->raiseValueErrorWithCStr("list.remove(x) x not in list");
}

Object* ListBuiltins::slice(Thread* thread, List* list, Slice* slice) {
  word start, stop, step;
  slice->unpack(&start, &stop, &step);
  word length = Slice::adjustIndices(list->allocated(), &start, &stop, step);

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<ObjectArray> items(&scope, runtime->newObjectArray(length));
  for (word i = 0, index = start; i < length; i++, index += step) {
    items->atPut(i, list->at(index));
  }

  Handle<List> result(&scope, runtime->newList());
  result->setItems(*items);
  result->setAllocated(items->length());
  return *result;
}

Object* ListBuiltins::dunderGetItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));

  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__getitem__() must be called with a list instance as the first "
        "argument");
  }

  Handle<List> list(&scope, *self);
  Object* index = args.get(1);
  if (index->isSmallInt()) {
    word idx = SmallInt::cast(index)->value();
    if (idx < 0) {
      idx = list->allocated() - idx;
    }
    if (idx < 0 || idx >= list->allocated()) {
      return thread->raiseIndexErrorWithCStr("list index out of range");
    }
    return list->at(idx);
  } else if (index->isSlice()) {
    Handle<Slice> list_slice(&scope, Slice::cast(index));
    return slice(thread, *list, *list_slice);
  } else {
    return thread->raiseTypeErrorWithCStr(
        "list indices must be integers or slices");
  }
}

Object* ListBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
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
  Handle<Type> list_iter(
      &scope,
      runtime->addBuiltinClass(SymbolId::kListIterator, LayoutId::kListIterator,
                               LayoutId::kObject, kMethods));
}

Object* ListIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                         word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isListIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a list iterator instance as the first "
        "argument");
  }
  return *self;
}

Object* ListIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                         word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isListIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a list iterator instance as the first "
        "argument");
  }
  Handle<Object> value(&scope, ListIterator::cast(*self)->next());
  if (value->isError()) {
    return thread->raiseStopIteration(None::object());
  }
  return *value;
}

Object* ListIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                               word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isListIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a list iterator instance as the "
        "first argument");
  }
  Handle<ListIterator> list_iterator(&scope, *self);
  Handle<List> list(&scope, list_iterator->list());
  return SmallInt::fromWord(list->allocated() - list_iterator->index());
}

Object* ListBuiltins::dunderSetItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr("expected 3 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));

  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__setitem__() must be called with a list instance as the first "
        "argument");
  }

  Handle<List> list(&scope, *self);
  Object* index = args.get(1);
  if (index->isSmallInt()) {
    word idx = SmallInt::cast(index)->value();
    if (idx < 0) {
      idx = list->allocated() + idx;
    }
    if (idx < 0 || idx >= list->allocated()) {
      return thread->raiseIndexErrorWithCStr(
          "list assignment index out of range");
    }
    Handle<Object> value(&scope, args.get(2));
    list->atPut(idx, *value);
    return None::object();
  }
  // TODO(T31826482): Add support for slices
  return thread->raiseTypeErrorWithCStr(
      "list indices must be integers or slices");
}

Object* ListBuiltins::dunderDelItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 2 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));

  if (!thread->runtime()->isInstanceOfList(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__delitem__() must be called with a list instance as the first "
        "argument");
  }

  Handle<List> list(&scope, *self);
  Object* index = args.get(1);
  if (index->isSmallInt()) {
    word idx = SmallInt::cast(index)->value();
    idx = idx < 0 ? list->allocated() + idx : idx;
    if (idx < 0 || idx >= list->allocated()) {
      return thread->raiseIndexErrorWithCStr(
          "list assignment index out of range");
    }
    return thread->runtime()->listPop(list, idx);
  }
  // TODO(T31826482): Add support for slices
  return thread->raiseTypeErrorWithCStr(
      "list indices must be integers or slices");
}

}  // namespace python
