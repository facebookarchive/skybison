#include "dict-builtins.h"

#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

const BuiltinMethod DictBuiltins::kMethods[] = {
    {SymbolId::kDunderContains, nativeTrampoline<dunderContains>},
    {SymbolId::kDunderDelItem, nativeTrampoline<dunderDelItem>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGetItem, nativeTrampoline<dunderGetItem>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderSetItem, nativeTrampoline<dunderSetItem>},
    {SymbolId::kDunderItems, nativeTrampoline<dunderItems>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderKeys, nativeTrampoline<dunderKeys>},
    {SymbolId::kDunderValues, nativeTrampoline<dunderValues>},
};

void DictBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type dict_type(&scope, runtime->addBuiltinTypeWithMethods(
                             SymbolId::kDict, LayoutId::kDict,
                             LayoutId::kObject, kMethods));
}

RawObject DictBuiltins::dunderContains(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  if (!self->isDict()) {
    return thread->raiseTypeErrorWithCStr(
        "dict.__contains__(self): self must be a dict");
  }
  Dict dict(&scope, *self);
  return Bool::fromBool(thread->runtime()->dictIncludes(dict, key));
}

RawObject DictBuiltins::dunderDelItem(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  if (self->isDict()) {
    Dict dict(&scope, *self);
    // Remove the key. If it doesn't exist, throw a KeyError.
    if (thread->runtime()->dictRemove(dict, key)->isError()) {
      return thread->raiseKeyErrorWithCStr("missing key can't be deleted");
    }
    return NoneType::object();
  }
  // TODO(T32856777): handle user-defined subtypes of dict.
  return thread->raiseTypeErrorWithCStr(
      "__delitem__() must be called with a dict instance as the first "
      "argument");
}

RawObject DictBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isDict() && args.get(1)->isDict()) {
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();

    Dict self(&scope, args.get(0));
    Dict other(&scope, args.get(1));
    if (self->numItems() != other->numItems()) {
      return Bool::falseObj();
    }
    Tuple keys(&scope, runtime->dictKeys(self));
    Object left_key(&scope, NoneType::object());
    Object left(&scope, NoneType::object());
    Object right(&scope, NoneType::object());
    word length = keys->length();
    for (word i = 0; i < length; i++) {
      left_key = keys->at(i);
      left = runtime->dictAt(self, left_key);
      right = runtime->dictAt(other, left_key);
      if (right->isError()) {
        return Bool::falseObj();
      }
      RawObject result =
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

RawObject DictBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__len__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (self->isDict()) {
    return SmallInt::fromWord(RawDict::cast(*self)->numItems());
  }
  // TODO(T32856777): handle user-defined subtypes of dict.
  return thread->raiseTypeErrorWithCStr("'__len__' requires a 'dict' object");
}

RawObject DictBuiltins::dunderGetItem(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  if (self->isDict()) {
    Dict dict(&scope, *self);
    Object dunder_hash(&scope, Interpreter::lookupMethod(
                                   thread, frame, key, SymbolId::kDunderHash));
    Object key_hash(&scope,
                    Interpreter::callMethod1(thread, frame, dunder_hash, key));
    Object value(&scope,
                 thread->runtime()->dictAtWithHash(dict, key, key_hash));
    if (value->isError()) {
      return thread->raiseKeyErrorWithCStr("RawKeyError");
    }
    return *value;
  }
  // TODO(T32856777): handle user-defined subtypes of dict.
  return thread->raiseTypeErrorWithCStr(
      "__getitem__() must be called with a dict instance as the first "
      "argument");
}

RawObject DictBuiltins::dunderSetItem(Thread* thread, Frame* frame,
                                      word nargs) {
  if (nargs != 3) {
    return thread->raiseTypeErrorWithCStr("expected 2 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Object value(&scope, args.get(2));
  if (self->isDict()) {
    Dict dict(&scope, *self);
    Object dunder_hash(&scope, Interpreter::lookupMethod(
                                   thread, frame, key, SymbolId::kDunderHash));
    Object key_hash(&scope,
                    Interpreter::callMethod1(thread, frame, dunder_hash, key));
    if (key_hash->isError()) {
      return *key_hash;
    }
    thread->runtime()->dictAtPutWithHash(dict, key, value, key_hash);
    return NoneType::object();
  }
  return thread->raiseTypeErrorWithCStr(
      "__setitem__() must be called with a dict instance as the first "
      "argument");
}

RawObject DictBuiltins::dunderItems(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__items__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDict()) {
    return thread->raiseTypeErrorWithCStr(
        "__items__() must be called with a dict instance as the first "
        "argument");
  }

  Dict dict(&scope, *self);
  return thread->runtime()->newDictItems(dict);
}

RawObject DictBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDict()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict instance as the first "
        "argument");
  }

  Dict dict(&scope, *self);
  // .iter() on a dict returns a keys iterator
  return thread->runtime()->newDictKeyIterator(dict);
}

RawObject DictBuiltins::dunderKeys(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__keys__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDict()) {
    return thread->raiseTypeErrorWithCStr(
        "__keys__() must be called with a dict instance as the first "
        "argument");
  }

  Dict dict(&scope, *self);
  return thread->runtime()->newDictKeys(dict);
}

RawObject DictBuiltins::dunderValues(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__values__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDict()) {
    return thread->raiseTypeErrorWithCStr(
        "__values__() must be called with a dict instance as the first "
        "argument");
  }

  Dict dict(&scope, *self);
  return thread->runtime()->newDictValues(dict);
}

// TODO(T35787656): Instead of re-writing everything for every class, make a
// helper function that takes a member function (type check) and string for the
// Python symbol name
const BuiltinMethod DictItemIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>}};

void DictItemIteratorBuiltins::initialize(Runtime* runtime) {
  runtime->addBuiltinTypeWithMethods(SymbolId::kDictItemIterator,
                                     LayoutId::kDictItemIterator,
                                     LayoutId::kObject, kMethods);
}

RawObject DictItemIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                               word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictItemIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict_itemiterator iterator instance "
        "as the first argument");
  }
  return *self;
}

RawObject DictItemIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                               word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictItemIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a dict_itemiterator instance as the "
        "first argument");
  }
  DictItemIterator iter(&scope, *self);
  Object value(&scope, thread->runtime()->dictItemIteratorNext(thread, iter));
  if (value->isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject DictItemIteratorBuiltins::dunderLengthHint(Thread* thread,
                                                     Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictItemIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a dict_itemiterator instance as "
        "the first argument");
  }
  DictItemIterator iter(&scope, *self);
  Dict dict(&scope, iter->dict());
  return SmallInt::fromWord(dict->numItems() - iter->numFound());
}

const BuiltinMethod DictItemsBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>}};

void DictItemsBuiltins::initialize(Runtime* runtime) {
  runtime->addBuiltinTypeWithMethods(SymbolId::kDictItems, LayoutId::kDictItems,
                                     LayoutId::kObject, kMethods);
}

RawObject DictItemsBuiltins::dunderIter(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictItems()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict_items instance as the first "
        "argument");
  }

  Dict dict(&scope, DictItems::cast(*self)->dict());
  return thread->runtime()->newDictItemIterator(dict);
}

const BuiltinMethod DictKeyIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>}};

void DictKeyIteratorBuiltins::initialize(Runtime* runtime) {
  runtime->addBuiltinTypeWithMethods(SymbolId::kDictKeyIterator,
                                     LayoutId::kDictKeyIterator,
                                     LayoutId::kObject, kMethods);
}

RawObject DictKeyIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                              word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictKeyIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict_keyiterator iterator instance "
        "as the first argument");
  }
  return *self;
}

RawObject DictKeyIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                              word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictKeyIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a dict_keyiterator instance as the "
        "first argument");
  }
  DictKeyIterator iter(&scope, *self);
  Object value(&scope, thread->runtime()->dictKeyIteratorNext(thread, iter));
  if (value->isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject DictKeyIteratorBuiltins::dunderLengthHint(Thread* thread,
                                                    Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictKeyIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a dict_keyiterator instance as "
        "the first argument");
  }
  DictKeyIterator iter(&scope, *self);
  Dict dict(&scope, iter->dict());
  return SmallInt::fromWord(dict->numItems() - iter->numFound());
}

const BuiltinMethod DictKeysBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>}};

void DictKeysBuiltins::initialize(Runtime* runtime) {
  runtime->addBuiltinTypeWithMethods(SymbolId::kDictKeys, LayoutId::kDictKeys,
                                     LayoutId::kObject, kMethods);
}

RawObject DictKeysBuiltins::dunderIter(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictKeys()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict_keys instance as the first "
        "argument");
  }

  Dict dict(&scope, DictKeys::cast(*self)->dict());
  return thread->runtime()->newDictKeyIterator(dict);
}

const BuiltinMethod DictValueIteratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kDunderLengthHint, nativeTrampoline<dunderLengthHint>}};

void DictValueIteratorBuiltins::initialize(Runtime* runtime) {
  runtime->addBuiltinTypeWithMethods(SymbolId::kDictValueIterator,
                                     LayoutId::kDictValueIterator,
                                     LayoutId::kObject, kMethods);
}

RawObject DictValueIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                                word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictValueIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict_valueiterator iterator instance "
        "as the first argument");
  }
  return *self;
}

RawObject DictValueIteratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                                word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictValueIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a dict_valueiterator instance as the "
        "first argument");
  }
  DictValueIterator iter(&scope, *self);
  Object value(&scope, thread->runtime()->dictValueIteratorNext(thread, iter));
  if (value->isError()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  return *value;
}

RawObject DictValueIteratorBuiltins::dunderLengthHint(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictValueIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a dict_valueiterator instance "
        "as the first argument");
  }
  DictValueIterator iter(&scope, *self);
  Dict dict(&scope, iter->dict());
  return SmallInt::fromWord(dict->numItems() - iter->numFound());
}

const BuiltinMethod DictValuesBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>}};

void DictValuesBuiltins::initialize(Runtime* runtime) {
  runtime->addBuiltinTypeWithMethods(SymbolId::kDictValues,
                                     LayoutId::kDictValues, LayoutId::kObject,
                                     kMethods);
}

RawObject DictValuesBuiltins::dunderIter(Thread* thread, Frame* frame,
                                         word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self->isDictValues()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict_values instance as the first "
        "argument");
  }

  Dict dict(&scope, DictValues::cast(*self)->dict());
  return thread->runtime()->newDictValueIterator(dict);
}

}  // namespace python
