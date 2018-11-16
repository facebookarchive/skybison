#include "set-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod SetBuiltins::kMethods[] = {
    {SymbolId::kAdd, nativeTrampoline<add>},
    {SymbolId::kDunderContains, nativeTrampoline<dunderContains>},
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kPop, nativeTrampoline<pop>}};

void SetBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;

  Handle<Type> set(&scope,
                   runtime->addEmptyBuiltinClass(SymbolId::kSet, LayoutId::kSet,
                                                 LayoutId::kObject));
  set->setFlag(Type::Flag::kSetSubclass);
  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(set, kMethods[i].name,
                                     kMethods[i].address);
  }
}

Object* SetBuiltins::add(Thread* thread, Frame* frame, word nargs) {
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

Object* SetBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
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

Object* SetBuiltins::pop(Thread* thread, Frame* frame, word nargs) {
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

Object* SetBuiltins::dunderContains(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("__contains__ takes 1 arguments.");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Set> self(&scope, args.get(0));
  Handle<Object> value(&scope, args.get(1));
  if (self->isSet()) {
    return Bool::fromBool(thread->runtime()->setIncludes(self, value));
  }
  // TODO(T30253711): handle user-defined subtypes of set.
  return thread->throwTypeErrorFromCString(
      "descriptor 'pop' requires a 'set' object");
}

Object* SetBuiltins::dunderInit(Thread* thread, Frame*, word nargs) {
  if (nargs > 2) {
    return thread->throwTypeErrorFromCString(
        "set expected at most 1 arguments.");
  }
  if (nargs == 2) {
    UNIMPLEMENTED("construct set with iterable");
  }
  return None::object();
}

Object* SetBuiltins::dunderNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newSet();
}

Object* SetBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));

  if (!self->isSet()) {
    return thread->throwTypeErrorFromCString(
        "__iter__() must be called with a set instance as the first argument");
  }
  return thread->runtime()->newSetIterator(self);
}

const BuiltinMethod SetIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>},
};

void SetIteratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> set_iter(&scope, runtime->addEmptyBuiltinClass(
                                    SymbolId::kSetIterator,
                                    LayoutId::kSetIterator, LayoutId::kObject));

  for (uword i = 0; i < ARRAYSIZE(kMethods); i++) {
    runtime->classAddBuiltinFunction(set_iter, kMethods[i].name,
                                     kMethods[i].address);
  }
}

Object* SetIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isSetIterator()) {
    return thread->throwTypeErrorFromCString(
        "__iter__() must be called with a set iterator instance as the first "
        "argument");
  }
  return *self;
}

Object* SetIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isSetIterator()) {
    return thread->throwTypeErrorFromCString(
        "__next__() must be called with a set iterator instance as the first "
        "argument");
  }
  Handle<Object> value(&scope, SetIterator::cast(*self)->next());
  if (value->isError()) {
    UNIMPLEMENTED("throw StopIteration");
  }
  return *value;
}

Object* SetIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                              word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isSetIterator()) {
    return thread->throwTypeErrorFromCString(
        "__length_hint__() must be called with a tuple iterator instance as "
        "the first argument");
  }
  Handle<SetIterator> set_iterator(&scope, *self);
  return SmallInt::fromWord(set_iterator->pendingLength());
}

}  // namespace python
