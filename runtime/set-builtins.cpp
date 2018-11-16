#include "set-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinSetAdd(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "add() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> key(&scope, args.get(1));
  if (self->isSet()) {
    Handle<Set> set(&scope, *self);
    thread->runtime()->setAdd(set, key);
    return None::object();
  }
  // TODO(zekun): handle subclass of set
  return thread->throwTypeErrorFromCString("'add' requires a 'set' object");
}

Object* builtinSetLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("__len__() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  if (self->isSet()) {
    return SmallInt::fromWord(Set::cast(*self)->numItems());
  }
  // TODO(cshapiro): handle user-defined subtypes of set.
  return thread->throwTypeErrorFromCString("'__len__' requires a 'set' object");
}

Object* builtinSetPop(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("pop() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Set> self(&scope, args.get(0));
  if (self->isSet()) {
    Handle<ObjectArray> data(&scope, self->data());
    word num_items = self->numItems();
    if (num_items > 0) {
      for (word i = 0; i < data->length(); i += Set::Bucket::kNumPointers) {
        if (Set::Bucket::isTombstone(*data, i) ||
            Set::Bucket::isEmpty(*data, i)) {
          continue;
        }
        Handle<Object> value(&scope, Set::Bucket::key(*data, i));
        Set::Bucket::setTombstone(*data, i);
        self->setNumItems(num_items - 1);
        return *value;
      }
    }
    return thread->throwKeyErrorFromCString("pop from an empty set");
  }
  // TODO(T30253711): handle user-defined subtypes of set.
  return thread->throwTypeErrorFromCString(
      "descriptor 'pop' requires a 'set' object");
}

Object* builtinSetContains(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("__contains__ takes 1 arguments.");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Set> self(&scope, args.get(0));
  Handle<Object> value(&scope, args.get(1));
  if (self->isSet()) {
    return Boolean::fromBool(thread->runtime()->setIncludes(self, value));
  }
  // TODO(T30253711): handle user-defined subtypes of set.
  return thread->throwTypeErrorFromCString(
      "descriptor 'pop' requires a 'set' object");
}

Object* builtinSetInit(Thread* thread, Frame*, word nargs) {
  if (nargs > 2) {
    return thread->throwTypeErrorFromCString(
        "set expected at most 1 arguments.");
  }
  if (nargs == 2) {
    UNIMPLEMENTED("construct set with iterable");
  }
  return None::object();
}

Object* builtinSetNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newSet();
}

}  // namespace python
