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
    {SymbolId::kDunderAnd, nativeTrampoline<dunderAnd>},
    {SymbolId::kDunderContains, nativeTrampoline<dunderContains>},
    {SymbolId::kDunderIand, nativeTrampoline<dunderIand>},
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kIsDisjoint, nativeTrampoline<isDisjoint>},
    {SymbolId::kIntersection, nativeTrampoline<intersection>},
    {SymbolId::kPop, nativeTrampoline<pop>}};

void SetBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;

  Handle<Type> set(&scope,
                   runtime->addBuiltinClass(SymbolId::kSet, LayoutId::kSet,
                                            LayoutId::kObject, kMethods));
  set->setFlag(Type::Flag::kSetSubclass);
}

RawObject SetBuiltins::add(Thread* thread, Frame* frame, word nargs) {
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
    return NoneType::object();
  }
  // TODO(zekun): handle subclass of set
  return thread->raiseTypeErrorWithCStr("'add' requires a 'set' object");
}

RawObject SetBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
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

RawObject SetBuiltins::pop(Thread* thread, Frame* frame, word nargs) {
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

RawObject SetBuiltins::dunderContains(Thread* thread, Frame* frame,
                                      word nargs) {
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

RawObject SetBuiltins::dunderInit(Thread* thread, Frame*, word nargs) {
  if (nargs > 2) {
    return thread->raiseTypeErrorWithCStr("set expected at most 1 arguments.");
  }
  if (nargs == 2) {
    UNIMPLEMENTED("construct set with iterable");
  }
  return NoneType::object();
}

RawObject SetBuiltins::dunderNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newSet();
}

RawObject SetBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
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

RawObject SetBuiltins::isDisjoint(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr(
        "isdisjoint() takes exactly one argument");
  }
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> other(&scope, args.get(1));
  Handle<Object> value(&scope, NoneType::object());
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

RawObject SetBuiltins::dunderAnd(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("__and__ takes 1 arguments.");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Set> self(&scope, args.get(0));
  Handle<Object> other(&scope, args.get(1));
  if (self->isSet()) {
    if (!other->isSet()) {
      return thread->runtime()->notImplemented();
    }
    return thread->runtime()->setIntersection(thread, self, other);
  }
  // TODO(jeethu): handle user-defined subtypes of set.
  return thread->raiseTypeErrorWithCStr(
      "descriptor '__and__' requires a 'set' object");
}

RawObject SetBuiltins::dunderIand(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("__iand__ takes 1 arguments.");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Set> self(&scope, args.get(0));
  Handle<Object> other(&scope, args.get(1));
  if (self->isSet()) {
    if (!other->isSet()) {
      return thread->runtime()->notImplemented();
    }
    Handle<Object> intersection(
        &scope, thread->runtime()->setIntersection(thread, self, other));
    if (intersection->isError()) {
      return *intersection;
    }
    RawSet intersection_set = Set::cast(*intersection);
    self->setData(intersection_set->data());
    self->setNumItems(intersection_set->numItems());
    return *self;
  }
  // TODO(jeethu): handle user-defined subtypes of set.
  return thread->raiseTypeErrorWithCStr(
      "descriptor '__iand__' requires a 'set' object");
}

RawObject SetBuiltins::intersection(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor 'intersection' of 'set' object needs an argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  if (self->isSet()) {
    Handle<Set> set(&scope, *self);
    if (nargs == 1) {
      // Return a copy of the set
      return thread->runtime()->setCopy(set);
    }
    // nargs is at least 2
    Handle<Object> other(&scope, args.get(1));
    Handle<Object> result(
        &scope, thread->runtime()->setIntersection(thread, set, other));
    if (result->isError() || nargs == 2) {
      return *result;
    }
    if (nargs > 2) {
      Handle<Set> base(&scope, *result);
      for (word i = 2; i < nargs; i++) {
        other = args.get(i);
        result = thread->runtime()->setIntersection(thread, base, other);
        if (result->isError()) {
          return *result;
        }
        base = *result;
        // Early exit
        if (base->numItems() == 0) {
          break;
        }
      }
      return *result;
    }
  }
  // TODO(jeethu): handle user-defined subtypes of set.
  return thread->raiseTypeErrorWithCStr(
      "descriptor 'intersection' requires a 'set' object");
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

RawObject SetIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
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

RawObject SetIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
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
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject SetIteratorBuiltins::dunderLengthHint(Thread* thread, Frame* frame,
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
        "__length_hint__() must be called with a set iterator instance as "
        "the first argument");
  }
  Handle<SetIterator> set_iterator(&scope, *self);
  return SmallInt::fromWord(set_iterator->pendingLength());
}

}  // namespace python
