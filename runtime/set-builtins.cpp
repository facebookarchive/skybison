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
    {SymbolId::kIsDisjoint, nativeTrampoline<isDisjoint>},
    {SymbolId::kPop, nativeTrampoline<pop>}};

void SetBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;

  Handle<Type> set(&scope,
                   runtime->addBuiltinClass(SymbolId::kSet, LayoutId::kSet,
                                            LayoutId::kObject, kMethods));
  set->setFlag(Type::Flag::kSetSubclass);
}

Object* SetBuiltins::add(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("add() takes exactly one argument");
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
  return thread->raiseTypeErrorWithCStr("'add' requires a 'set' object");
}

Object* SetBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__len__() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  if (self->isSet()) {
    return SmallInt::fromWord(Set::cast(*self)->numItems());
  }
  // TODO(cshapiro): handle user-defined subtypes of set.
  return thread->raiseTypeErrorWithCStr("'__len__' requires a 'set' object");
}

Object* SetBuiltins::pop(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("pop() takes no arguments");
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
    return thread->raiseKeyErrorWithCStr("pop from an empty set");
  }
  // TODO(T30253711): handle user-defined subtypes of set.
  return thread->raiseTypeErrorWithCStr(
      "descriptor 'pop' requires a 'set' object");
}

Object* SetBuiltins::dunderContains(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("__contains__ takes 1 arguments.");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Set> self(&scope, args.get(0));
  Handle<Object> value(&scope, args.get(1));
  if (self->isSet()) {
    return Bool::fromBool(thread->runtime()->setIncludes(self, value));
  }
  // TODO(T30253711): handle user-defined subtypes of set.
  return thread->raiseTypeErrorWithCStr(
      "descriptor 'pop' requires a 'set' object");
}

Object* SetBuiltins::dunderInit(Thread* thread, Frame*, word nargs) {
  if (nargs > 2) {
    return thread->raiseTypeErrorWithCStr("set expected at most 1 arguments.");
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
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));

  if (!self->isSet()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a set instance as the first argument");
  }
  return thread->runtime()->newSetIterator(self);
}

Object* SetBuiltins::isDisjoint(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "isdisjoint() takes exactly one argument");
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> other(&scope, args.get(1));
  Handle<Object> value(&scope, None::object());
  if (self->isSet()) {
    Handle<Set> a(&scope, *self);
    if (a->numItems() == 0) {
      return Bool::trueObj();
    }
    if (other->isSet()) {
      Handle<Set> b(&scope, *other);
      if (b->numItems() == 0) {
        return Bool::trueObj();
      }
      // Iterate over the smaller set
      if (a->numItems() > b->numItems()) {
        a = *other;
        b = *self;
      }
      Handle<SetIterator> set_iter(&scope, runtime->newSetIterator(a));
      for (;;) {
        value = set_iter->next();
        if (value->isError()) {
          break;
        }
        if (runtime->setIncludes(b, value)) {
          return Bool::falseObj();
        }
      }
      return Bool::trueObj();
    }
    // Generic iterator case
    Handle<Object> iter_method(
        &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), other,
                                          SymbolId::kDunderIter));
    if (iter_method->isError()) {
      return thread->raiseTypeErrorWithCStr("object is not iterable");
    }
    Handle<Object> iterator(
        &scope, Interpreter::callMethod1(thread, thread->currentFrame(),
                                         iter_method, other));
    if (iterator->isError()) {
      return thread->raiseTypeErrorWithCStr("object is not iterable");
    }
    Handle<Object> next_method(
        &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                          iterator, SymbolId::kDunderNext));
    if (next_method->isError()) {
      return thread->raiseTypeErrorWithCStr("iter() returned a non-iterator");
    }
    while (!runtime->isIteratorExhausted(thread, iterator)) {
      value = Interpreter::callMethod1(thread, thread->currentFrame(),
                                       next_method, iterator);
      if (value->isError()) {
        return *value;
      }
      if (runtime->setIncludes(a, value)) {
        return Bool::falseObj();
      }
    }
    return Bool::trueObj();
  }
  // TODO(jeethu): handle user-defined subtypes of set.
  return thread->raiseTypeErrorWithCStr(
      "descriptor 'is_disjoint' requires a 'set' object");
}

const BuiltinMethod SetIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>},
};

void SetIteratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> set_iter(
      &scope,
      runtime->addBuiltinClass(SymbolId::kSetIterator, LayoutId::kSetIterator,
                               LayoutId::kObject, kMethods));
}

Object* SetIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isSetIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a set iterator instance as the first "
        "argument");
  }
  return *self;
}

Object* SetIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isSetIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a set iterator instance as the first "
        "argument");
  }
  Handle<Object> value(&scope, SetIterator::cast(*self)->next());
  if (value->isError()) {
    return thread->raiseStopIteration(None::object());
  }
  return *value;
}

Object* SetIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
                                              word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (!self->isSetIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a tuple iterator instance as "
        "the first argument");
  }
  Handle<SetIterator> set_iterator(&scope, *self);
  return SmallInt::fromWord(set_iterator->pendingLength());
}

}  // namespace python
