#include "tuple-builtins.h"

#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinTupleEq(Thread* thread, Frame* frame, word nargs) {
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

Object* tupleSlice(Thread* thread, ObjectArray* tuple, Slice* slice) {
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

Object* builtinTupleGetItem(Thread* thread, Frame* frame, word nargs) {
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
      Handle<Slice> slice(&scope, Slice::cast(index));
      return tupleSlice(thread, *tuple, *slice);
    }
    return thread->throwTypeErrorFromCString(
        "tuple indices must be integers or slices");
  }
  // TODO(jeethu): handle user-defined subtypes of tuple.
  return thread->throwTypeErrorFromCString(
      "__getitem__() must be called with a tuple instance as the first "
      "argument");
}

Object* builtinTupleNew(Thread* thread, Frame* frame, word nargs) {
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
  if (!runtime->hasSubClassFlag(*type_obj, Type::Flag::kClassSubclass)) {
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
