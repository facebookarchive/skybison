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
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderMul, nativeTrampoline<dunderMul>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>}};

void TupleBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> type(
      &scope, runtime->addEmptyBuiltinClass(
                  SymbolId::kTuple, LayoutId::kObjectArray, LayoutId::kObject));
  type->setFlag(Type::Flag::kTupleSubclass);
  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(type, kMethods[i].name,
                                     kMethods[i].address);
  }
}

Object* TupleBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isObjectArray() && args.get(1)->isObjectArray()) {
    HandleScope scope(thread);
    Handle<ObjectArray> self(&scope, args.get(0));
    Handle<ObjectArray> other(&scope, args.get(1));
    if (self->length() != other->length()) {
      return Bool::falseObj();
    }
    Handle<Object> left(&scope, None::object());
    Handle<Object> right(&scope, None::object());
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
    return thread->throwTypeErrorFromCString("expected 1 argument");
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
        return thread->throwIndexErrorFromCString("tuple index out of range");
      }
      return tuple->at(idx);
    } else if (index->isSlice()) {
      Handle<Slice> tuple_slice(&scope, Slice::cast(index));
      return slice(thread, *tuple, *tuple_slice);
    }
    return thread->throwTypeErrorFromCString(
        "tuple indices must be integers or slices");
  }
  // TODO(jeethu): handle user-defined subtypes of tuple.
  return thread->throwTypeErrorFromCString(
      "__getitem__() must be called with a tuple instance as the first "
      "argument");
}

Object* TupleBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString(
        "tuple.__len__ takes exactly 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> obj(&scope, args.get(0));
  if (!obj->isObjectArray()) {
    return thread->throwTypeErrorFromCString(
        "tuple.__len__(self): self is not a tuple");
  }
  Handle<ObjectArray> self(&scope, *obj);
  return thread->runtime()->newInt(self->length());
}

Object* TupleBuiltins::dunderMul(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->throwTypeErrorFromCString(
        "descriptor '__mul__' of 'tuple' object needs an argument");
  }
  if (nargs != 2) {
    return thread->throwTypeError(thread->runtime()->newStringFromFormat(
        "expected 1 argument, got %ld", nargs - 1));
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<ObjectArray> self(&scope, args.get(0));
  Handle<Object> rhs(&scope, args.get(1));
  if (!rhs->isInt()) {
    return thread->throwTypeErrorFromCString(
        "can't multiply sequence by non-int");
  }
  if (!rhs->isSmallInt()) {
    return thread->throwOverflowErrorFromCString(
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
    return thread->throwOverflowErrorFromCString(
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
    return thread->throwTypeErrorFromCString(
        "tuple.__new__(): not enough arguments");
  }
  if (nargs > 2) {
    return thread->throwTypeError(thread->runtime()->newStringFromFormat(
        "tuple() takes at most 1 argument (%ld given)", nargs - 1));
  }

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> type_obj(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*type_obj, Type::Flag::kTypeSubclass)) {
    return thread->throwTypeErrorFromCString(
        "tuple.__new__(X): X is not a type object");
  }

  Handle<Type> type(&scope, *type_obj);
  if (!type->hasFlag(Type::Flag::kTupleSubclass)) {
    return thread->throwTypeErrorFromCString(
        "tuple.__new__(X): X is not a subclass of tuple");
  }

  // If no iterable is given as an argument, return an empty zero tuple.
  if (nargs == 1) {
    return runtime->newObjectArray(0);
  }

  // Construct a new tuple from the iterable.
  UNIMPLEMENTED("Can't construct tuple from iterable");
}

}  // namespace python
