#include "dict-builtins.h"

#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

RawObject dictCopy(Thread* thread, const Dict& dict) {
  HandleScope scope(thread);
  Dict copy(&scope, thread->runtime()->newDict());
  Object result(&scope, dictMergeError(thread, copy, dict));
  if (result->isError()) {
    return *result;
  }
  return *copy;
}

namespace {
enum class Override {
  kIgnore,
  kOverride,
  kError,
};

RawObject dictMergeDict(Thread* thread, const Dict& dict, const Object& mapping,
                        Override do_override) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  if (*mapping == *dict) return NoneType::object();

  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  Object hash(&scope, NoneType::object());
  Dict other(&scope, *mapping);
  Tuple other_data(&scope, other->data());
  for (word i = 0; i < other_data->length(); i += Dict::Bucket::kNumPointers) {
    if (Dict::Bucket::isFilled(*other_data, i)) {
      key = Dict::Bucket::key(*other_data, i);
      value = Dict::Bucket::value(*other_data, i);
      hash = Dict::Bucket::hash(*other_data, i);
      if (do_override == Override::kOverride ||
          !runtime->dictIncludesWithHash(dict, key, hash)) {
        runtime->dictAtPutWithHash(dict, key, value, hash);
      } else if (do_override == Override::kError) {
        return thread->raiseKeyError(*key);
      }
    }
  }
  return NoneType::object();
}

RawObject dictMergeImpl(Thread* thread, const Dict& dict, const Object& mapping,
                        Override do_override) {
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfDict(*mapping)) {
    return dictMergeDict(thread, dict, mapping, do_override);
  }

  HandleScope scope;
  Object key(&scope, NoneType::object());
  Object value(&scope, NoneType::object());
  Frame* frame = thread->currentFrame();
  Object keys_method(&scope, Interpreter::lookupMethod(thread, frame, mapping,
                                                       SymbolId::kKeys));
  if (keys_method->isError()) {
    return thread->raiseAttributeErrorWithCStr(
        "object has no 'keys' attribute");
  }

  // Generic mapping, use keys() and __getitem__()
  Object subscr_method(&scope,
                       Interpreter::lookupMethod(thread, frame, mapping,
                                                 SymbolId::kDunderGetItem));
  if (subscr_method->isError()) {
    return thread->raiseTypeErrorWithCStr("object is not subscriptable");
  }
  Object keys(&scope,
              Interpreter::callMethod1(thread, frame, keys_method, mapping));
  if (keys->isError()) return *keys;

  if (keys->isList()) {
    List keys_list(&scope, *keys);
    for (word i = 0; i < keys_list->numItems(); ++i) {
      key = keys_list->at(i);
      if (do_override == Override::kOverride ||
          !runtime->dictIncludes(dict, key)) {
        value = Interpreter::callMethod2(thread, frame, subscr_method, mapping,
                                         key);
        if (value->isError()) return *value;
        runtime->dictAtPut(dict, key, value);
      } else if (do_override == Override::kError) {
        return thread->raiseKeyError(*key);
      }
    }
    return NoneType::object();
  }

  if (keys->isTuple()) {
    Tuple keys_tuple(&scope, *keys);
    for (word i = 0; i < keys_tuple->length(); ++i) {
      key = keys_tuple->at(i);
      if (do_override == Override::kOverride ||
          !runtime->dictIncludes(dict, key)) {
        value = Interpreter::callMethod2(thread, frame, subscr_method, mapping,
                                         key);
        if (value->isError()) return *value;
        runtime->dictAtPut(dict, key, value);
      } else if (do_override == Override::kError) {
        return thread->raiseKeyError(*key);
      }
    }
    return NoneType::object();
  }

  // keys is probably an iterator
  Object iter_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(), keys,
                                        SymbolId::kDunderIter));
  if (iter_method->isError()) {
    return thread->raiseTypeErrorWithCStr("keys() is not iterable");
  }

  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, keys));
  if (iterator->isError()) {
    return thread->raiseTypeErrorWithCStr("keys() is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method->isError()) {
    return thread->raiseTypeErrorWithCStr("keys() is not iterable");
  }
  for (;;) {
    key = Interpreter::callMethod1(thread, thread->currentFrame(), next_method,
                                   iterator);
    if (key->isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *key;
    }
    if (do_override == Override::kOverride ||
        !runtime->dictIncludes(dict, key)) {
      value =
          Interpreter::callMethod2(thread, frame, subscr_method, mapping, key);
      if (value->isError()) return *value;
      runtime->dictAtPut(dict, key, value);
    } else if (do_override == Override::kError) {
      return thread->raiseKeyError(*key);
    }
  }
  return NoneType::object();
}
}  // namespace

RawObject dictMergeOverride(Thread* thread, const Dict& dict,
                            const Object& mapping) {
  return dictMergeImpl(thread, dict, mapping, Override::kOverride);
}

RawObject dictMergeError(Thread* thread, const Dict& dict,
                         const Object& mapping) {
  return dictMergeImpl(thread, dict, mapping, Override::kError);
}

RawObject dictMergeIgnore(Thread* thread, const Dict& dict,
                          const Object& mapping) {
  return dictMergeImpl(thread, dict, mapping, Override::kIgnore);
}

RawObject dictItemIteratorNext(Thread* thread, DictItemIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.dict());
  Tuple buckets(&scope, dict.data());
  word jump = Dict::Bucket::kNumPointers;

  word i = iter.index();
  for (; i < buckets->length() && Dict::Bucket::isEmpty(*buckets, i);
       i += jump) {
  }

  if (i < buckets->length()) {
    // At this point, we have found a valid index in the buckets.
    Object key(&scope, Dict::Bucket::key(*buckets, i));
    Object value(&scope, Dict::Bucket::value(*buckets, i));
    Tuple kv_pair(&scope, thread->runtime()->newTuple(2));
    kv_pair->atPut(0, *key);
    kv_pair->atPut(1, *value);
    iter.setIndex(i + jump);
    iter.setNumFound(iter.numFound() + 1);
    return *kv_pair;
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::object();
}

RawObject dictKeyIteratorNext(Thread* thread, DictKeyIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.dict());
  Tuple buckets(&scope, dict.data());
  word jump = Dict::Bucket::kNumPointers;

  word i = iter.index();
  for (; i < buckets->length() && Dict::Bucket::isEmpty(*buckets, i);
       i += jump) {
  }

  if (i < buckets->length()) {
    // At this point, we have found a valid index in the buckets.
    iter.setIndex(i + jump);
    iter.setNumFound(iter.numFound() + 1);
    return Dict::Bucket::key(*buckets, i);
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::object();
}

RawObject dictValueIteratorNext(Thread* thread, DictValueIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.dict());
  Tuple buckets(&scope, dict.data());
  word jump = Dict::Bucket::kNumPointers;

  word i = iter.index();
  for (; i < buckets->length() && Dict::Bucket::isEmpty(*buckets, i);
       i += jump) {
  }

  if (i < buckets->length()) {
    // At this point, we have found a valid index in the buckets.
    iter.setIndex(i + jump);
    iter.setNumFound(iter.numFound() + 1);
    return Dict::Bucket::value(*buckets, i);
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::object();
}

const BuiltinAttribute DictBuiltins::kAttributes[] = {
    {SymbolId::kInvalid, RawDict::kNumItemsOffset},
    {SymbolId::kInvalid, RawDict::kDataOffset},
};

const BuiltinMethod DictBuiltins::kMethods[] = {
    {SymbolId::kDunderContains, nativeTrampoline<dunderContains>},
    {SymbolId::kDunderDelItem, nativeTrampoline<dunderDelItem>},
    {SymbolId::kDunderEq, nativeTrampoline<dunderEq>},
    {SymbolId::kDunderGetItem, nativeTrampoline<dunderGetItem>},
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderLen, nativeTrampoline<dunderLen>},
    {SymbolId::kDunderNew, nativeTrampoline<dunderNew>},
    {SymbolId::kDunderSetItem, nativeTrampoline<dunderSetItem>},
    {SymbolId::kGet, nativeTrampoline<get>},
    {SymbolId::kItems, nativeTrampoline<items>},
    {SymbolId::kKeys, nativeTrampoline<keys>},
    {SymbolId::kUpdate, nativeTrampoline<update>},
    {SymbolId::kValues, nativeTrampoline<values>},
};

void DictBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type dict_type(&scope, runtime->addBuiltinType(
                             SymbolId::kDict, LayoutId::kDict,
                             LayoutId::kObject, kAttributes, kMethods));
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
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "dict.__contains__(self): self must be a dict");
  }
  Dict dict(&scope, *self);
  Object dunder_hash(&scope, Interpreter::lookupMethod(thread, frame, key,
                                                       SymbolId::kDunderHash));
  if (dunder_hash->isNoneType()) {
    return thread->raiseTypeErrorWithCStr("unhashable type");
  }
  Object key_hash(&scope,
                  Interpreter::callMethod1(thread, frame, dunder_hash, key));
  if (key_hash->isError()) {
    return *key_hash;
  }
  if (!runtime->isInstanceOfInt(key_hash)) {
    return thread->raiseTypeErrorWithCStr("__hash__ must return 'int'");
  }
  return Bool::fromBool(runtime->dictIncludesWithHash(dict, key, key_hash));
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
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__delitem__() must be called with a dict instance as the first "
        "argument");
  }
  Dict dict(&scope, *self);
  // Remove the key. If it doesn't exist, throw a KeyError.
  if (runtime->dictRemove(dict, key)->isError()) {
    return thread->raiseKeyErrorWithCStr("missing key can't be deleted");
  }
  return NoneType::object();
}

RawObject DictBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(args.get(0))) {
    return thread->raiseTypeErrorWithCStr(
        "__eq__() must be called with a dict instance as the first "
        "argument");
  }
  if (runtime->isInstanceOfDict(args.get(1))) {
    HandleScope scope(thread);
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
  return thread->runtime()->notImplemented();
}

RawObject DictBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__len__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDict(*self)) {
    return thread->raiseTypeErrorWithCStr("'__len__' requires a 'dict' object");
  }

  Dict dict(&scope, *self);
  return SmallInt::fromWord(dict->numItems());
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
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__getitem__() must be called with a dict instance as the first "
        "argument");
  }
  Dict dict(&scope, *self);
  Object dunder_hash(&scope, Interpreter::lookupMethod(thread, frame, key,
                                                       SymbolId::kDunderHash));
  Object key_hash(&scope,
                  Interpreter::callMethod1(thread, frame, dunder_hash, key));
  Object value(&scope, runtime->dictAtWithHash(dict, key, key_hash));
  if (value->isError()) {
    return thread->raiseKeyErrorWithCStr("RawKeyError");
  }
  return *value;
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
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__setitem__() must be called with a dict instance as the first "
        "argument");
  }
  Dict dict(&scope, *self);
  Object dunder_hash(&scope, Interpreter::lookupMethod(thread, frame, key,
                                                       SymbolId::kDunderHash));
  Object key_hash(&scope,
                  Interpreter::callMethod1(thread, frame, dunder_hash, key));
  if (key_hash->isError()) {
    return *key_hash;
  }
  runtime->dictAtPutWithHash(dict, key, value, key_hash);
  return NoneType::object();
}

RawObject DictBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict instance as the first "
        "argument");
  }

  Dict dict(&scope, *self);
  // .iter() on a dict returns a keys iterator
  return runtime->newDictKeyIterator(dict);
}

RawObject DictBuiltins::items(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("items() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "items() must be called with a dict instance as the first "
        "argument");
  }

  Dict dict(&scope, *self);
  return runtime->newDictItems(dict);
}

RawObject DictBuiltins::keys(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("keys() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "keys() must be called with a dict instance as the first "
        "argument");
  }

  Dict dict(&scope, *self);
  return runtime->newDictKeys(dict);
}

RawObject DictBuiltins::values(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("values() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "values() must be called with a dict instance as the first "
        "argument");
  }

  Dict dict(&scope, *self);
  return runtime->newDictValues(dict);
}

RawObject DictBuiltins::get(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 2) {
    return thread->raiseTypeErrorWithCStr("get expected at least one argument");
  }
  if (nargs > 3) {
    return thread->raiseTypeErrorWithCStr("get expected at most 2 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseTypeErrorWithCStr(
        "get() must be called with a dict instance as the first "
        "argument");
  }
  Dict dict(&scope, *self);

  // Check key hash
  Object key(&scope, args.get(1));
  Object dunder_hash(&scope, Interpreter::lookupMethod(thread, frame, key,
                                                       SymbolId::kDunderHash));
  if (dunder_hash->isNoneType()) {
    return thread->raiseTypeErrorWithCStr("unhashable type");
  }
  Object key_hash(&scope,
                  Interpreter::callMethod1(thread, frame, dunder_hash, key));
  if (key_hash->isError()) {
    return *key_hash;
  }
  if (!runtime->isInstanceOfInt(key_hash)) {
    return thread->raiseTypeErrorWithCStr("__hash__ must return 'int'");
  }

  // Return results
  Object result(&scope, runtime->dictAtWithHash(dict, key, key_hash));
  if (!result->isError()) return *result;
  if (nargs == 3) return args.get(2);
  return NoneType::object();
}

RawObject DictBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type->builtinBase() != LayoutId::kDict) {
    return thread->raiseTypeErrorWithCStr("not a subtype of dict");
  }
  Layout layout(&scope, type->instanceLayout());
  Dict result(&scope, thread->runtime()->newInstance(layout));
  result->setNumItems(0);
  result->setData(thread->runtime()->newTuple(0));
  return *result;
}

RawObject DictBuiltins::update(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1) {
    return thread->raiseTypeErrorWithCStr("dict.update: not enough arguments");
  }
  if (nargs > 2) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "update expected at most 1 arguments, got %ld", nargs));
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(self)) {
    return thread->raiseTypeErrorWithCStr("'update' requires a 'dict' object");
  }
  Dict dict(&scope, *self);
  Object other(&scope, args.get(1));
  Type other_type(&scope, runtime->typeOf(*other));
  if (!runtime->lookupSymbolInMro(thread, other_type, SymbolId::kKeys)
           .isError()) {
    return dictMergeOverride(thread, dict, other);
  }

  // TODO(bsimmers): Support iterables of pairs, like PyDict_MergeFromSeq2().
  return thread->raiseTypeErrorWithCStr("object is not a mapping");
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
  Object value(&scope, dictItemIteratorNext(thread, iter));
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
  Object value(&scope, dictKeyIteratorNext(thread, iter));
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
  Object value(&scope, dictValueIteratorNext(thread, iter));
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
