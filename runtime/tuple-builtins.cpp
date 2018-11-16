#include "tuple-builtins.h"

#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod TupleBuiltins::kMethods[] = {
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGetItem, nativeTrampoline<dunderGetItem>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderMul, nativeTrampoline<dunderMul>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>}};

void TupleBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(
      &scope, runtime->addBuiltinClass(SymbolId::kTuple, LayoutId::kObjectArray,
                                       LayoutId::kObject, kMethods));
  type->setFlag(Type::Flag::kTupleSubclass);
}

Object* TupleBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isObjectArray() && args.get(1)->isObjectArray()) {
    HandleScope scope(thread);
    Handle<ObjectArray> self(&scope, args.get(0));
    Handle<ObjectArray> other(&scope, args.get(1));
    if (self->length() != other->length()) {
      return Bool::falseObj();
    }
    Handle<Object> left(&scope, NoneType::object());
    Handle<Object> right(&scope, NoneType::object());
    word length = self->length();
    for (word i = 0; i < length; i++) {
      left = self->at(i);
      right = other->at(i);
      Object* result =
          Interpreter::compareOperation(thread, frame, EQ, left, right);
      if (result == Bool::falseObj()) {
        return result;
      }
    }
    return Bool::trueObj();
  }
  // TODO(cshapiro): handle user-defined subtypes of tuple.
  return thread->runtime()->notImplemented();
}

Object* TupleBuiltins::slice(Thread* thread, ObjectArray* tuple, Slice* slice) {
  word start, stop, step;
  slice->unpack(&start, &stop, &step);
  word length = Slice::adjustIndices(tuple->length(), &start, &stop, step);
  if (start == 0 && stop >= tuple->length() && step == 1) {
    return tuple;
  }

  HandleScope scope(thread);
  Handle<ObjectArray> items(&scope, thread->runtime()->newObjectArray(length));
  for (word i = 0, index = start; i < length; i++, index += step) {
    items->atPut(i, tuple->at(index));
  }
  return *items;
}

Object* TupleBuiltins::dunderGetItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (self->isObjectArray()) {
    Handle<ObjectArray> tuple(&scope, *self);
    Object* index = args.get(1);
    if (index->isSmallInt()) {
      word idx = SmallInt::cast(index)->value();
      if (idx < 0) {
        idx = tuple->length() - idx;
      }
      if (idx < 0 || idx >= tuple->length()) {
        return thread->raiseIndexErrorWithCStr("tuple index out of range");
      }
      return tuple->at(idx);
    } else if (index->isSlice()) {
      Handle<Slice> tuple_slice(&scope, Slice::cast(index));
      return slice(thread, *tuple, *tuple_slice);
    }
    return thread->raiseTypeErrorWithCStr(
        "tuple indices must be integers or slices");
  }
  // TODO(jeethu): handle user-defined subtypes of tuple.
  return thread->raiseTypeErrorWithCStr(
      "__getitem__() must be called with a tuple instance as the first "
      "argument");
}

Object* TupleBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__len__ takes exactly 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> obj(&scope, args.get(0));
  if (!obj->isObjectArray()) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__len__(self): self is not a tuple");
  }
  Handle<ObjectArray> self(&scope, *obj);
  return thread->runtime()->newInt(self->length());
}

Object* TupleBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__mul__' of 'tuple' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "expected 1 argument, got %ld", nargs - 1));
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<ObjectArray> self(&scope, args.get(0));
  Handle<Object> rhs(&scope, args.get(1));
  if (!rhs->isInt()) {
    return thread->raiseTypeErrorWithCStr("can't multiply sequence by non-int");
  }
  if (!rhs->isSmallInt()) {
    return thread->raiseOverflowErrorWithCStr(
        "cannot fit 'int' into an index-sized integer");
  }
  Handle<SmallInt> right(&scope, *rhs);
  word length = self->length();
  word times = right->value();
  if (length == 0 || times <= 0) {
    return thread->runtime()->newObjectArray(0);
  }
  if (length == 1 || times == 1) {
    return *self;
  }

  word new_length = length * times;
  // If the new length overflows, raise an OverflowError.
  if ((new_length / length) != times) {
    return thread->raiseOverflowErrorWithCStr(
        "cannot fit 'int' into an index-sized integer");
  }

  Handle<ObjectArray> new_tuple(&scope,
                                thread->runtime()->newObjectArray(new_length));
  for (word i = 0; i < times; i++) {
    for (word j = 0; j < length; j++) {
      new_tuple->atPut(i * length + j, self->at(j));
    }
  }
  return *new_tuple;
}

Object* TupleBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__new__(): not enough arguments");
  }
  if (nargs > 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "tuple() takes at most 1 argument (%ld given)", nargs - 1));
  }

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> type_obj(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*type_obj, Type::Flag::kTypeSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__new__(X): X is not a type object");
  }

  Handle<Type> type(&scope, *type_obj);
  if (!type->hasFlag(Type::Flag::kTupleSubclass)) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__new__(X): X is not a subclass of tuple");
  }

  // If no iterable is given as an argument, return an empty zero tuple.
  if (nargs == 1) {
    return runtime->newObjectArray(0);
  }

  // Construct a new tuple from the iterable.
  Handle<Object> iterable(&scope, args.get(1));
  Handle<Object> dunder_iter(&scope,
                             Interpreter::lookupMethod(thread, frame, iterable,
                                                       SymbolId::kDunderIter));
  if (dunder_iter->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Handle<Object> iterator(
      &scope, Interpreter::callMethod1(thread, frame, dunder_iter, iterable));
  Handle<Object> dunder_next(&scope,
                             Interpreter::lookupMethod(thread, frame, iterator,
                                                       SymbolId::kDunderNext));
  word max_len = 10;
  // If the iterator has a __length_hint__, use that as max_len to avoid
  // resizes.
  Handle<Type> iter_type(&scope, runtime->typeOf(*iterator));
  Handle<Object> length_hint(
      &scope, runtime->lookupSymbolInMro(thread, iter_type,
                                         SymbolId::kDunderLengthHint));
  if (length_hint->isSmallInt()) {
    max_len = SmallInt::cast(*length_hint)->value();
  }

  word curr = 0;
  Handle<ObjectArray> result(&scope, runtime->newObjectArray(max_len));
  // Iterate through the iterable, copying elements into the tuple.
  while (!runtime->isIteratorExhausted(thread, iterator)) {
    Handle<Object> elem(
        &scope, Interpreter::callMethod1(thread, frame, dunder_next, iterator));
    DCHECK(!elem->isError(), "__next__ raised exception");
    // If the capacity of the current result is reached, create a new larger
    // tuple and copy over the contents.
    if (curr == max_len) {
      max_len *= 2;
      Handle<ObjectArray> new_tuple(&scope, runtime->newObjectArray(max_len));
      for (word i = 0; i < curr; i++) {
        new_tuple->atPut(i, result->at(i));
      }
      result = *new_tuple;
    }
    result->atPut(curr++, *elem);
  }

  // If the result is perfectly sized, return it.
  if (curr == max_len) {
    return *result;
  }

  // The result was over-allocated, shrink it.
  Handle<ObjectArray> new_tuple(&scope, runtime->newObjectArray(curr));
  for (word i = 0; i < curr; i++) {
    new_tuple->atPut(i, result->at(i));
  }
  return *new_tuple;
}

Object* TupleBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));

  if (!self->isObjectArray()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a tuple instance as the first "
        "argument");
  }
  return thread->runtime()->newTupleIterator(self);
}

const BuiltinMethod TupleIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>}};

void TupleIteratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> tuple_iter(
      &scope, runtime->addBuiltinClass(SymbolId::kTupleIterator,
                                       LayoutId::kTupleIterator,
                                       LayoutId::kObject, kMethods));
}

Object* TupleIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                          word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isTupleIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a tuple iterator instance as the first "
        "argument");
  }
  return *self;
}

Object* TupleIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                          word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isTupleIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a tuple iterator instance as the first "
        "argument");
  }
  Handle<Object> value(&scope, TupleIterator::cast(*self)->next());
  if (value->isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

Object* TupleIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isTupleIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a tuple iterator instance as "
        "the "
        "first argument");
  }
  Handle<TupleIterator> tuple_iterator(&scope, *self);
  Handle<ObjectArray> tuple(&scope, tuple_iterator->tuple());
  return SmallInt::fromWord(tuple->length() - tuple_iterator->index());
}

}  // namespace python
