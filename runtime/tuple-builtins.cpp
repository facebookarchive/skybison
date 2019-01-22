#include "tuple-builtins.h"

#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject sequenceAsTuple(Thread* thread, const Object& seq) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (seq->isTuple()) return *seq;
  if (runtime->isInstanceOfList(*seq)) {
    List list(&scope, *seq);
    word len = list->numItems();
    Tuple ret(&scope, runtime->newTuple(len));
    for (word i = 0; i < len; i++) {
      ret->atPut(i, list->at(i));
    }
    return *ret;
  }

  // TODO(bsimmers): Support arbitrary iterables.
  return thread->raiseWithCStr(
      LayoutId::kNotImplementedError,
      "Iterables not yet supported in sequenceAsTuple()");
}

const BuiltinAttribute TupleBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, RawUserTupleBase::kTupleOffset},
};

const BuiltinMethod TupleBuiltins::kMethods[] = {
    {SymbolId::kDunderAdd, nativeTrampoline<dunderAdd>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGetItem, nativeTrampoline<dunderGetItem>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderMul, nativeTrampoline<dunderMul>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>}};

void TupleBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type type(&scope,
            runtime->addBuiltinType(SymbolId::kTuple, LayoutId::kTuple,
                                    LayoutId::kObject, kAttributes, kMethods));
}

RawObject TupleBuiltins::dunderAdd(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr("'__add__' of 'tuple' missing self");
  }
  Runtime* runtime = thread->runtime();
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 argument, got %ld", nargs - 1));
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object lhs(&scope, args.get(0));
  if (!runtime->isInstanceOfTuple(lhs)) {
    return thread->raiseTypeErrorWithCStr("'__add__' requires a tuple");
  }
  if (!lhs->isTuple()) {
    UserTupleBase lhs_user(&scope, *lhs);
    lhs = lhs_user->tupleValue();
  }
  Tuple left(&scope, *lhs);
  Object rhs(&scope, args.get(1));
  if (!runtime->isInstanceOfTuple(rhs)) {
    return thread->raiseTypeErrorWithCStr(
        "can only concatenate tuple to tuple");
  }
  if (!rhs->isTuple()) {
    UserTupleBase rhs_user(&scope, *rhs);
    rhs = rhs_user->tupleValue();
  }
  Tuple right(&scope, *rhs);
  word llength = left->length();
  word rlength = right->length();

  word new_length = llength + rlength;
  if (new_length > kMaxWord) {
    return thread->raiseOverflowErrorWithCStr(
        "cannot fit 'int' into an index-sized integer");
  }

  Tuple new_tuple(&scope, thread->runtime()->newTuple(new_length));
  for (word i = 0; i < llength; ++i) {
    new_tuple->atPut(i, left->at(i));
  }
  for (word j = 0; j < rlength; ++j) {
    new_tuple->atPut(llength + j, right->at(j));
  }
  return *new_tuple;
}

RawObject TupleBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Object other_obj(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(self_obj)) {
    return thread->raiseTypeErrorWithCStr("'__eq__' requires a 'tuple' object");
  }
  if (!runtime->isInstanceOfTuple(other_obj)) {
    return runtime->notImplemented();
  }

  if (!self_obj->isTuple()) {
    UserTupleBase user_self(&scope, *self_obj);
    self_obj = user_self->tupleValue();
  }
  if (!other_obj->isTuple()) {
    UserTupleBase user_other(&scope, *other_obj);
    other_obj = user_other->tupleValue();
  }

  Tuple self(&scope, *self_obj);
  Tuple other(&scope, *other_obj);
  if (self->length() != other->length()) {
    return Bool::falseObj();
  }
  Object left(&scope, NoneType::object());
  Object right(&scope, NoneType::object());
  word length = self->length();
  for (word i = 0; i < length; i++) {
    left = self->at(i);
    right = other->at(i);
    RawObject result =
        Interpreter::compareOperation(thread, frame, EQ, left, right);
    if (result == Bool::falseObj()) {
      return result;
    }
  }
  return Bool::trueObj();
}

RawObject TupleBuiltins::slice(Thread* thread, const Tuple& tuple,
                               const Slice& slice) {
  word start, stop, step;
  slice->unpack(&start, &stop, &step);
  return sliceWithWords(thread, tuple, start, stop, step);
}

RawObject TupleBuiltins::sliceWithWords(Thread* thread, const Tuple& tuple,
                                        word start, word stop, word step) {
  word length = Slice::adjustIndices(tuple->length(), &start, &stop, step);
  if (start == 0 && stop >= tuple->length() && step == 1) {
    return tuple;
  }

  HandleScope scope(thread);
  Tuple items(&scope, thread->runtime()->newTuple(length));
  for (word i = 0, index = start; i < length; i++, index += step) {
    items->atPut(i, tuple->at(index));
  }
  return *items;
}

RawObject TupleBuiltins::dunderGetItem(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(self)) {
    return thread->raiseTypeErrorWithCStr(
        "__getitem__() must be called with a tuple instance as the first "
        "argument");
  }

  if (!self->isTuple()) {
    UserTupleBase user_tuple(&scope, *self);
    self = user_tuple->tupleValue();
  }

  Tuple tuple(&scope, *self);
  Object index(&scope, args.get(1));
  if (index->isSmallInt()) {
    word idx = RawSmallInt::cast(*index)->value();
    if (idx < 0) {
      idx = tuple->length() - idx;
    }
    if (idx < 0 || idx >= tuple->length()) {
      return thread->raiseIndexErrorWithCStr("tuple index out of range");
    }
    return tuple->at(idx);
  }
  if (index->isSlice()) {
    Slice tuple_slice(&scope, *index);
    return slice(thread, tuple, tuple_slice);
  }
  return thread->raiseTypeErrorWithCStr(
      "tuple indices must be integers or slices");
}

RawObject TupleBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__len__ takes exactly 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(obj)) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__len__(self): self is not a tuple");
  }
  if (!obj->isTuple()) {
    UserTupleBase user_tuple(&scope, *obj);
    obj = user_tuple->tupleValue();
  }
  Tuple self(&scope, *obj);
  return runtime->newInt(self->length());
}

RawObject TupleBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
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
  Object self_obj(&scope, args.get(0));
  if (!self_obj->isTuple()) {
    UserTupleBase user_tuple(&scope, *self_obj);
    self_obj = user_tuple->tupleValue();
  }
  Tuple self(&scope, *self_obj);
  Object rhs(&scope, args.get(1));
  if (!rhs->isInt()) {
    return thread->raiseTypeErrorWithCStr("can't multiply sequence by non-int");
  }
  if (!rhs->isSmallInt()) {
    return thread->raiseOverflowErrorWithCStr(
        "cannot fit 'int' into an index-sized integer");
  }
  SmallInt right(&scope, *rhs);
  word length = self->length();
  word times = right->value();
  if (length == 0 || times <= 0) {
    return thread->runtime()->newTuple(0);
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

  Tuple new_tuple(&scope, thread->runtime()->newTuple(new_length));
  for (word i = 0; i < times; i++) {
    for (word j = 0; j < length; j++) {
      new_tuple->atPut(i * length + j, self->at(j));
    }
  }
  return *new_tuple;
}

static RawObject newTupleOrUserSubclass(Thread* thread, const Tuple& tuple,
                                        const Type& type) {
  if (type->isBuiltin()) return *tuple;
  HandleScope scope(thread);
  Layout layout(&scope, type->instanceLayout());
  UserTupleBase instance(&scope, thread->runtime()->newInstance(layout));
  instance->setTupleValue(*tuple);
  return *instance;
}

RawObject TupleBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
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
  Object type_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__new__(X): X is not a type object");
  }

  Type type(&scope, *type_obj);
  if (type->builtinBase() != LayoutId::kTuple) {
    return thread->raiseTypeErrorWithCStr(
        "tuple.__new__(X): X is not a subclass of tuple");
  }

  // If no iterable is given as an argument, return an empty zero tuple.
  if (nargs == 1) {
    Tuple tuple(&scope, runtime->newTuple(0));
    return newTupleOrUserSubclass(thread, tuple, type);
  }

  // Construct a new tuple from the iterable.
  Object iterable(&scope, args.get(1));
  Object dunder_iter(&scope, Interpreter::lookupMethod(thread, frame, iterable,
                                                       SymbolId::kDunderIter));
  if (dunder_iter->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not iterable");
  }
  Object iterator(
      &scope, Interpreter::callMethod1(thread, frame, dunder_iter, iterable));
  Object dunder_next(&scope, Interpreter::lookupMethod(thread, frame, iterator,
                                                       SymbolId::kDunderNext));
  word max_len = 10;
  // If the iterator has a __length_hint__, use that as max_len to avoid
  // resizes.
  Type iter_type(&scope, runtime->typeOf(*iterator));
  Object length_hint(&scope,
                     runtime->lookupSymbolInMro(thread, iter_type,
                                                SymbolId::kDunderLengthHint));
  if (length_hint->isSmallInt()) {
    max_len = RawSmallInt::cast(*length_hint)->value();
  }

  word curr = 0;
  Tuple result(&scope, runtime->newTuple(max_len));
  // Iterate through the iterable, copying elements into the tuple.
  for (;;) {
    Object elem(&scope,
                Interpreter::callMethod1(thread, frame, dunder_next, iterator));
    if (elem->isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *elem;
    }
    // If the capacity of the current result is reached, create a new larger
    // tuple and copy over the contents.
    if (curr == max_len) {
      max_len *= 2;
      Tuple new_tuple(&scope, runtime->newTuple(max_len));
      for (word i = 0; i < curr; i++) {
        new_tuple->atPut(i, result->at(i));
      }
      result = *new_tuple;
    }
    result->atPut(curr++, *elem);
  }

  // If the result is perfectly sized, return it.
  if (curr == max_len) {
    return newTupleOrUserSubclass(thread, result, type);
  }

  // The result was over-allocated, shrink it.
  Tuple new_tuple(&scope, runtime->newTuple(curr));
  for (word i = 0; i < curr; i++) {
    new_tuple->atPut(i, result->at(i));
  }
  return newTupleOrUserSubclass(thread, new_tuple, type);
}

RawObject TupleBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));

  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTuple(self)) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a tuple instance as the first "
        "argument");
  }
  if (!self->isTuple()) {
    UserTupleBase user_tuple(&scope, *self);
    self = user_tuple->tupleValue();
  }
  return runtime->newTupleIterator(self);
}

const BuiltinMethod TupleIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>}};

void TupleIteratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type tuple_iter(
      &scope, runtime->addBuiltinTypeWithMethods(SymbolId::kTupleIterator,
                                                 LayoutId::kTupleIterator,
                                                 LayoutId::kObject, kMethods));
}

RawObject TupleIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                            word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isTupleIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a tuple iterator instance as the first "
        "argument");
  }
  return *self;
}

RawObject TupleIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                            word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isTupleIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a tuple iterator instance as the first "
        "argument");
  }
  Object value(&scope, RawTupleIterator::cast(*self)->next());
  if (value->isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject TupleIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                                  word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isTupleIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a tuple iterator instance as "
        "the "
        "first argument");
  }
  TupleIterator tuple_iterator(&scope, *self);
  Tuple tuple(&scope, tuple_iterator->tuple());
  return SmallInt::fromWord(tuple->length() - tuple_iterator->index());
}

}  // namespace python
