#include "dict-builtins.h"

#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinDictDelItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> key(&scope, args.get(1));
  if (self->isDict()) {
    Handle<Dict> dict(&scope, *self);
    // Remove the key. If it doesn't exist, throw a KeyError.
    if (!thread->runtime()->dictRemove(dict, key, nullptr)) {
      return thread->throwKeyErrorFromCString("missing key can't be deleted");
    }
    return None::object();
  }
  // TODO(T32856777): handle user-defined subtypes of dict.
  return thread->throwTypeErrorFromCString(
      "__delitem__() must be called with a dict instance as the first "
      "argument");
}

Object* builtinDictEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isDict() && args.get(1)->isDict()) {
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();

    Handle<Dict> self(&scope, args.get(0));
    Handle<Dict> other(&scope, args.get(1));
    if (self->numItems() != other->numItems()) {
      return Bool::falseObj();
    }
    Handle<ObjectArray> keys(&scope, runtime->dictKeys(self));
    Handle<Object> left_key(&scope, None::object());
    Handle<Object> left(&scope, None::object());
    Handle<Object> right(&scope, None::object());
    word length = keys->length();
    for (word i = 0; i < length; i++) {
      left_key = keys->at(i);
      left = runtime->dictAt(self, left_key);
      right = runtime->dictAt(other, left_key);
      if (right->isError()) {
        return Bool::falseObj();
      }
      Object* result =
          Interpreter::compareOperation(thread, frame, EQ, left, right);
      if (result == Bool::falseObj()) {
        return result;
      }
    }
    return Bool::trueObj();
  }
  // TODO(T32856777): handle user-defined subtypes of dict.
  return thread->runtime()->notImplemented();
}

Object* builtinDictLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("__len__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (self->isDict()) {
    return SmallInt::fromWord(Dict::cast(*self)->numItems());
  }
  // TODO(T32856777): handle user-defined subtypes of dict.
  return thread->throwTypeErrorFromCString(
      "'__len__' requires a 'dict' object");
}

Object* builtinDictGetItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> key(&scope, args.get(1));
  if (self->isDict()) {
    Handle<Dict> dict(&scope, *self);
    Handle<Object> dunder_hash(
        &scope,
        Interpreter::lookupMethod(thread, frame, key, SymbolId::kDunderHash));
    Handle<Object> key_hash(
        &scope, Interpreter::callMethod1(thread, frame, dunder_hash, key));
    Handle<Object> value(
        &scope, thread->runtime()->dictAtWithHash(dict, key, key_hash));
    if (value->isError()) {
      return thread->throwKeyErrorFromCString("KeyError");
    }
    return *value;
  }
  // TODO(T32856777): handle user-defined subtypes of dict.
  return thread->throwTypeErrorFromCString(
      "__getitem__() must be called with a dict instance as the first "
      "argument");
}

}  // namespace python
