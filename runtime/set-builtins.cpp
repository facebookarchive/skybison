#include "set-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinAttribute SetBuiltins::kAttributes[] = {
    {SymbolId::kItems, Set::kDataOffset},
    {SymbolId::kAllocated, Set::kNumItemsOffset},
};

const BuiltinMethod SetBuiltins::kMethods[] = {
    {SymbolId::kAdd, nativeTrampoline<add>},
    {SymbolId::kDunderAnd, nativeTrampoline<dunderAnd>},
    {SymbolId::kDunderContains, nativeTrampoline<dunderContains>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGe, nativeTrampoline<dunderGe>},
    {SymbolId::kDunderGt, nativeTrampoline<dunderGt>},
    {SymbolId::kDunderIand, nativeTrampoline<dunderIand>},
    {SymbolId::kDunderInit, nativeTrampoline<dunderInit>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLe, nativeTrampoline<dunderLe>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderLt, nativeTrampoline<dunderLt>},
    {SymbolId::kDunderNe, nativeTrampoline<dunderNe>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kIsDisjoint, nativeTrampoline<isDisjoint>},
    {SymbolId::kIntersection, nativeTrampoline<intersection>},
    {SymbolId::kPop, nativeTrampoline<pop>}};

void SetBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;

  Type set(&scope,
           runtime->addBuiltinClass(SymbolId::kSet, LayoutId::kSet,
                                    LayoutId::kObject, kAttributes, kMethods));
  set->setFlag(Type::Flag::kSetSubclass);
}

RawObject SetBuiltins::add(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("add() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  if (self->isSet()) {
    Set set(&scope, *self);
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
  Object self(&scope, args.get(0));
  if (self->isSet()) {
    return SmallInt::fromWord(RawSet::cast(*self)->numItems());
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
  Set self(&scope, args.get(0));
  if (self->isSet()) {
    ObjectArray data(&scope, self->data());
    word num_items = self->numItems();
    if (num_items == 0) {
      return thread->raiseKeyErrorWithCStr("pop from an empty set");
    }
    for (word i = 0, length = data->length(); i < length;
         i += Set::Bucket::kNumPointers) {
      if (Set::Bucket::isFilled(*data, i)) {
        Object value(&scope, Set::Bucket::key(*data, i));
        Set::Bucket::setTombstone(*data, i);
        self->setNumItems(num_items - 1);
        return *value;
      }
    }
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
  Set self(&scope, args.get(0));
  Object value(&scope, args.get(1));
  if (self->isSet()) {
    return Bool::fromBool(thread->runtime()->setIncludes(self, value));
  }
  // TODO(T30253711): handle user-defined subtypes of set.
  return thread->raiseTypeErrorWithCStr(
      "descriptor 'pop' requires a 'set' object");
}

RawObject SetBuiltins::dunderInit(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__init__' of 'set' object needs an argument");
  }
  if (nargs > 2) {
    return thread->raiseTypeError(runtime->newStrFromFormat(
        "set expected at most 1 arguments, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__init__' requires a 'set' object");
  }
  if (nargs == 2) {
    Set set(&scope, self);
    Object iterable(&scope, args.get(1));
    Object result(&scope, runtime->setUpdate(thread, set, iterable));
    if (result->isError()) {
      return *result;
    }
  }
  return NoneType::object();
}

RawObject SetBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (!type->hasFlag(Type::Flag::kSetSubclass)) {
    return thread->raiseTypeErrorWithCStr("not a subtype of set");
  }
  Layout layout(&scope, type->instanceLayout());
  Set result(&scope, thread->runtime()->newInstance(layout));
  result->setNumItems(0);
  result->setData(thread->runtime()->newObjectArray(0));
  return *result;
}

RawObject SetBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));

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
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Object value(&scope, NoneType::object());
  if (self->isSet()) {
    Set a(&scope, *self);
    if (a->numItems() == 0) {
      return Bool::trueObj();
    }
    if (other->isSet()) {
      Set b(&scope, *other);
      if (b->numItems() == 0) {
        return Bool::trueObj();
      }
      // Iterate over the smaller set
      if (a->numItems() > b->numItems()) {
        a = *other;
        b = *self;
      }
      SetIterator set_iter(&scope, runtime->newSetIterator(a));
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
    Object iter_method(
        &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), other,
                                          SymbolId::kDunderIter));
    if (iter_method->isError()) {
      return thread->raiseTypeErrorWithCStr("object is not iterable");
    }
    Object iterator(&scope,
                    Interpreter::callMethod1(thread, thread->currentFrame(),
                                             iter_method, other));
    if (iterator->isError()) {
      return thread->raiseTypeErrorWithCStr("object is not iterable");
    }
    Object next_method(
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
  Set self(&scope, args.get(0));
  Object other(&scope, args.get(1));
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
  Set self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (self->isSet()) {
    if (!other->isSet()) {
      return thread->runtime()->notImplemented();
    }
    Object intersection(
        &scope, thread->runtime()->setIntersection(thread, self, other));
    if (intersection->isError()) {
      return *intersection;
    }
    RawSet intersection_set = RawSet::cast(*intersection);
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
  Object self(&scope, args.get(0));
  if (self->isSet()) {
    Set set(&scope, *self);
    if (nargs == 1) {
      // Return a copy of the set
      return thread->runtime()->setCopy(set);
    }
    // nargs is at least 2
    Object other(&scope, args.get(1));
    Object result(&scope,
                  thread->runtime()->setIntersection(thread, set, other));
    if (result->isError() || nargs == 2) {
      return *result;
    }
    if (nargs > 2) {
      Set base(&scope, *result);
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

RawObject SetBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__eq__' of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 arguments, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__eq__' requires a 'set' object");
  }
  if (!runtime->isInstanceOfSet(*other)) {
    return runtime->notImplemented();
  }
  Set set(&scope, *self);
  Set other_set(&scope, *other);
  return Bool::fromBool(runtime->setEquals(thread, set, other_set));
}

RawObject SetBuiltins::dunderNe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__ne__' of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 arguments, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__ne__' requires a 'set' object");
  }
  if (!runtime->isInstanceOfSet(*other)) {
    return runtime->notImplemented();
  }
  Set set(&scope, *self);
  Set other_set(&scope, *other);
  return Bool::fromBool(!runtime->setEquals(thread, set, other_set));
}

RawObject SetBuiltins::dunderLe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__le__' of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 arguments, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__le__' requires a 'set' object");
  }
  if (!runtime->isInstanceOfSet(*other)) {
    return runtime->notImplemented();
  }
  Set set(&scope, *self);
  Set other_set(&scope, *other);
  return Bool::fromBool(runtime->setIsSubset(thread, set, other_set));
}

RawObject SetBuiltins::dunderLt(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__lt__' of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 arguments, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__lt__' requires a 'set' object");
  }
  if (!runtime->isInstanceOfSet(*other)) {
    return runtime->notImplemented();
  }
  Set set(&scope, *self);
  Set other_set(&scope, *other);
  return Bool::fromBool(runtime->setIsProperSubset(thread, set, other_set));
}

RawObject SetBuiltins::dunderGe(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__ge__' of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 arguments, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__ge__' requires a 'set' object");
  }
  if (!runtime->isInstanceOfSet(*other)) {
    return runtime->notImplemented();
  }
  Set set(&scope, *self);
  Set other_set(&scope, *other);
  return Bool::fromBool(runtime->setIsSubset(thread, other_set, set));
}

RawObject SetBuiltins::dunderGt(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  if (nargs == 0) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__gt__' of 'set' object needs an argument");
  }
  if (nargs != 2) {
    return thread->raiseTypeError(
        runtime->newStrFromFormat("expected 1 arguments, got %ld", nargs - 1));
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  if (!runtime->isInstanceOfSet(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "descriptor '__gt__' requires a 'set' object");
  }
  if (!runtime->isInstanceOfSet(*other)) {
    return runtime->notImplemented();
  }
  Set set(&scope, *self);
  Set other_set(&scope, *other);
  return Bool::fromBool(runtime->setIsProperSubset(thread, other_set, set));
}

const BuiltinMethod SetIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>},
};

void SetIteratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type set_iter(&scope, runtime->addBuiltinClassWithMethods(
                            SymbolId::kSetIterator, LayoutId::kSetIterator,
                            LayoutId::kObject, kMethods));
}

RawObject SetIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                          word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
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
  Object self(&scope, args.get(0));
  if (!self->isSetIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a set iterator instance as the first "
        "argument");
  }
  Object value(&scope, RawSetIterator::cast(*self)->next());
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
  Object self(&scope, args.get(0));
  if (!self->isSetIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a set iterator instance as "
        "the first argument");
  }
  SetIterator set_iterator(&scope, *self);
  return SmallInt::fromWord(set_iterator->pendingLength());
}

}  // namespace python
