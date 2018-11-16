#include "dict-builtins.h"

#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinDictionaryEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isDictionary() && args.get(1)->isDictionary()) {
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();

    Handle<Dictionary> self(&scope, args.get(0));
    Handle<Dictionary> other(&scope, args.get(1));
    if (self->numItems() != other->numItems()) {
      return Boolean::falseObj();
    }
    Handle<ObjectArray> keys(&scope, runtime->dictionaryKeys(self));
    Handle<Object> left_key(&scope, None::object());
    Handle<Object> left(&scope, None::object());
    Handle<Object> right(&scope, None::object());
    word length = keys->length();
    for (word i = 0; i < length; i++) {
      left_key = keys->at(i);
      left = runtime->dictionaryAt(self, left_key);
      right = runtime->dictionaryAt(other, left_key);
      if (right->isError()) {
        return Boolean::falseObj();
      }
      Object* result =
          Interpreter::compareOperation(thread, frame, EQ, left, right);
      if (result == Boolean::falseObj()) {
        return result;
      }
    }
    return Boolean::trueObj();
  }
  // TODO(cshapiro): handle user-defined subtypes of dictionary.
  return thread->runtime()->notImplemented();
}

Object* builtinDictionaryLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("__len__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (self->isDictionary()) {
    return SmallInteger::fromWord(Dictionary::cast(*self)->numItems());
  }
  // TODO(cshapiro): handle user-defined subtypes of dictionary.
  return thread->throwTypeErrorFromCString(
      "'__len__' requires a 'dict' object");
}

}  // namespace python
