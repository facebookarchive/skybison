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
  if (result.isError()) {
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
  Tuple other_data(&scope, other.data());
  for (word i = Dict::Bucket::kFirst;
       Dict::Bucket::nextItem(*other_data, &i);) {
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
  if (keys_method.isError()) {
    return thread->raiseAttributeErrorWithCStr(
        "object has no 'keys' attribute");
  }

  // Generic mapping, use keys() and __getitem__()
  Object subscr_method(&scope,
                       Interpreter::lookupMethod(thread, frame, mapping,
                                                 SymbolId::kDunderGetitem));
  if (subscr_method.isError()) {
    return thread->raiseTypeErrorWithCStr("object is not subscriptable");
  }
  Object keys(&scope,
              Interpreter::callMethod1(thread, frame, keys_method, mapping));
  if (keys.isError()) return *keys;

  if (keys.isList()) {
    List keys_list(&scope, *keys);
    for (word i = 0; i < keys_list.numItems(); ++i) {
      key = keys_list.at(i);
      if (do_override == Override::kOverride ||
          !runtime->dictIncludes(dict, key)) {
        value = Interpreter::callMethod2(thread, frame, subscr_method, mapping,
                                         key);
        if (value.isError()) return *value;
        runtime->dictAtPut(dict, key, value);
      } else if (do_override == Override::kError) {
        return thread->raiseKeyError(*key);
      }
    }
    return NoneType::object();
  }

  if (keys.isTuple()) {
    Tuple keys_tuple(&scope, *keys);
    for (word i = 0; i < keys_tuple.length(); ++i) {
      key = keys_tuple.at(i);
      if (do_override == Override::kOverride ||
          !runtime->dictIncludes(dict, key)) {
        value = Interpreter::callMethod2(thread, frame, subscr_method, mapping,
                                         key);
        if (value.isError()) return *value;
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
  if (iter_method.isError()) {
    return thread->raiseTypeErrorWithCStr("keys() is not iterable");
  }

  Object iterator(&scope,
                  Interpreter::callMethod1(thread, thread->currentFrame(),
                                           iter_method, keys));
  if (iterator.isError()) {
    return thread->raiseTypeErrorWithCStr("keys() is not iterable");
  }
  Object next_method(
      &scope, Interpreter::lookupMethod(thread, thread->currentFrame(),
                                        iterator, SymbolId::kDunderNext));
  if (next_method.isError()) {
    return thread->raiseTypeErrorWithCStr("keys() is not iterable");
  }
  for (;;) {
    key = Interpreter::callMethod1(thread, thread->currentFrame(), next_method,
                                   iterator);
    if (key.isError()) {
      if (thread->clearPendingStopIteration()) break;
      return *key;
    }
    if (do_override == Override::kOverride ||
        !runtime->dictIncludes(dict, key)) {
      value =
          Interpreter::callMethod2(thread, frame, subscr_method, mapping, key);
      if (value.isError()) return *value;
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

RawObject dictItemIteratorNext(Thread* thread, const DictItemIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.iterable());
  Tuple buckets(&scope, dict.data());

  word i = iter.index();
  if (Dict::Bucket::nextItem(*buckets, &i)) {
    // At this point, we have found a valid index in the buckets.
    Object key(&scope, Dict::Bucket::key(*buckets, i));
    Object value(&scope, Dict::Bucket::value(*buckets, i));
    Tuple kv_pair(&scope, thread->runtime()->newTuple(2));
    kv_pair.atPut(0, *key);
    kv_pair.atPut(1, *value);
    iter.setIndex(i);
    iter.setNumFound(iter.numFound() + 1);
    return *kv_pair;
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::object();
}

RawObject dictKeyIteratorNext(Thread* thread, const DictKeyIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.iterable());
  Tuple buckets(&scope, dict.data());

  word i = iter.index();
  if (Dict::Bucket::nextItem(*buckets, &i)) {
    // At this point, we have found a valid index in the buckets.
    iter.setIndex(i);
    iter.setNumFound(iter.numFound() + 1);
    return Dict::Bucket::key(*buckets, i);
  }

  // We hit the end.
  iter.setIndex(i);
  return Error::object();
}

RawObject dictValueIteratorNext(Thread* thread, const DictValueIterator& iter) {
  HandleScope scope(thread);
  Dict dict(&scope, iter.iterable());
  Tuple buckets(&scope, dict.data());

  word i = iter.index();
  if (Dict::Bucket::nextItem(*buckets, &i)) {
    // At this point, we have found a valid index in the buckets.
    iter.setIndex(i);
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
    {SymbolId::kSentinelId, -1},
};

const BuiltinMethod DictBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderDelitem, dunderDelItem},
    {SymbolId::kDunderEq, dunderEq},
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLen, dunderLen},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kDunderSetitem, dunderSetItem},
    {SymbolId::kGet, get},
    {SymbolId::kItems, items},
    {SymbolId::kKeys, keys},
    {SymbolId::kUpdate, update},
    {SymbolId::kValues, values},
    {SymbolId::kSentinelId, nullptr},
};

RawObject DictBuiltins::dunderDelItem(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);
  // Remove the key. If it doesn't exist, throw a KeyError.
  if (runtime->dictRemove(dict, key).isError()) {
    return thread->raiseKeyError(*key);
  }
  return NoneType::object();
}

RawObject DictBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfDict(*self_obj)) {
    return thread->raiseRequiresType(self_obj, SymbolId::kDict);
  }
  if (runtime->isInstanceOfDict(args.get(1))) {
    Dict self(&scope, *self_obj);
    Dict other(&scope, args.get(1));
    if (self.numItems() != other.numItems()) {
      return Bool::falseObj();
    }
    Tuple keys(&scope, runtime->dictKeys(self));
    Object left_key(&scope, NoneType::object());
    Object left(&scope, NoneType::object());
    Object right(&scope, NoneType::object());
    word length = keys.length();
    for (word i = 0; i < length; i++) {
      left_key = keys.at(i);
      left = runtime->dictAt(self, left_key);
      right = runtime->dictAt(other, left_key);
      if (right.isError()) {
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
  return NotImplementedType::object();
}

RawObject DictBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);
  return SmallInt::fromWord(dict.numItems());
}

RawObject DictBuiltins::dunderSetItem(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Object value(&scope, args.get(2));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);
  Object dunder_hash(&scope, Interpreter::lookupMethod(thread, frame, key,
                                                       SymbolId::kDunderHash));
  Object key_hash(&scope,
                  Interpreter::callMethod1(thread, frame, dunder_hash, key));
  if (key_hash.isError()) {
    return *key_hash;
  }
  runtime->dictAtPutWithHash(dict, key, value, key_hash);
  return NoneType::object();
}

RawObject DictBuiltins::dunderIter(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);
  // .iter() on a dict returns a keys iterator
  return runtime->newDictKeyIterator(dict);
}

RawObject DictBuiltins::items(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);
  return runtime->newDictItems(dict);
}

RawObject DictBuiltins::keys(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);
  return runtime->newDictKeys(dict);
}

RawObject DictBuiltins::values(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);
  return runtime->newDictValues(dict);
}

RawObject DictBuiltins::get(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object key(&scope, args.get(1));
  Object default_obj(&scope, args.get(2));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);

  // Check key hash
  Object dunder_hash(&scope, Interpreter::lookupMethod(thread, frame, key,
                                                       SymbolId::kDunderHash));
  if (dunder_hash.isNoneType()) {
    return thread->raiseTypeErrorWithCStr("unhashable type");
  }
  Object key_hash(&scope,
                  Interpreter::callMethod1(thread, frame, dunder_hash, key));
  if (key_hash.isError()) {
    return *key_hash;
  }
  // TODO(T38780562): Handle Int subclasses
  if (!runtime->isInstanceOfInt(*key_hash)) {
    return thread->raiseTypeErrorWithCStr("__hash__ must return 'int'");
  }
  if (!key_hash.isInt()) {
    UNIMPLEMENTED("int subclassing");
  }

  // Return results
  Object result(&scope, runtime->dictAtWithHash(dict, key, key_hash));
  if (!result.isError()) return *result;
  return *default_obj;
}

RawObject DictBuiltins::dunderNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (!args.get(0).isType()) {
    return thread->raiseTypeErrorWithCStr("not a type object");
  }
  HandleScope scope(thread);
  Type type(&scope, args.get(0));
  if (type.builtinBase() != LayoutId::kDict) {
    return thread->raiseTypeErrorWithCStr("not a subtype of dict");
  }
  Layout layout(&scope, type.instanceLayout());
  Dict result(&scope, thread->runtime()->newInstance(layout));
  result.setNumItems(0);
  result.setData(thread->runtime()->newTuple(0));
  return *result;
}

RawObject DictBuiltins::update(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object other(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfDict(*self)) {
    return thread->raiseRequiresType(self, SymbolId::kDict);
  }
  Dict dict(&scope, *self);
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
const BuiltinMethod DictItemIteratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLengthHint, dunderLengthHint},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSentinelId, nullptr},
};

RawObject DictItemIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                               word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItemIterator()) {
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
  if (!self.isDictItemIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a dict_itemiterator instance as the "
        "first argument");
  }
  DictItemIterator iter(&scope, *self);
  Object value(&scope, dictItemIteratorNext(thread, iter));
  if (value.isError()) {
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
  if (!self.isDictItemIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a dict_itemiterator instance as "
        "the first argument");
  }
  DictItemIterator iter(&scope, *self);
  Dict dict(&scope, iter.iterable());
  return SmallInt::fromWord(dict.numItems() - iter.numFound());
}

const BuiltinMethod DictItemsBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kSentinelId, nullptr},
};

RawObject DictItemsBuiltins::dunderIter(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictItems()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict_items instance as the first "
        "argument");
  }

  Dict dict(&scope, DictItems::cast(*self).dict());
  return thread->runtime()->newDictItemIterator(dict);
}

const BuiltinMethod DictKeyIteratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLengthHint, dunderLengthHint},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSentinelId, nullptr},
};

RawObject DictKeyIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                              word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeyIterator()) {
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
  if (!self.isDictKeyIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a dict_keyiterator instance as the "
        "first argument");
  }
  DictKeyIterator iter(&scope, *self);
  Object value(&scope, dictKeyIteratorNext(thread, iter));
  if (value.isError()) {
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
  if (!self.isDictKeyIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a dict_keyiterator instance as "
        "the first argument");
  }
  DictKeyIterator iter(&scope, *self);
  Dict dict(&scope, iter.iterable());
  return SmallInt::fromWord(dict.numItems() - iter.numFound());
}

const BuiltinMethod DictKeysBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kSentinelId, nullptr},
};

RawObject DictKeysBuiltins::dunderIter(Thread* thread, Frame* frame,
                                       word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictKeys()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict_keys instance as the first "
        "argument");
  }

  Dict dict(&scope, DictKeys::cast(*self).dict());
  return thread->runtime()->newDictKeyIterator(dict);
}

const BuiltinMethod DictValueIteratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderLengthHint, dunderLengthHint},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSentinelId, nullptr},
};

RawObject DictValueIteratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                                word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValueIterator()) {
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
  if (!self.isDictValueIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__next__() must be called with a dict_valueiterator instance as the "
        "first argument");
  }
  DictValueIterator iter(&scope, *self);
  Object value(&scope, dictValueIteratorNext(thread, iter));
  if (value.isError()) {
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
  if (!self.isDictValueIterator()) {
    return thread->raiseTypeErrorWithCStr(
        "__length_hint__() must be called with a dict_valueiterator instance "
        "as the first argument");
  }
  DictValueIterator iter(&scope, *self);
  Dict dict(&scope, iter.iterable());
  return SmallInt::fromWord(dict.numItems() - iter.numFound());
}

const BuiltinMethod DictValuesBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kSentinelId, nullptr},
};

RawObject DictValuesBuiltins::dunderIter(Thread* thread, Frame* frame,
                                         word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isDictValues()) {
    return thread->raiseTypeErrorWithCStr(
        "__iter__() must be called with a dict_values instance as the first "
        "argument");
  }

  Dict dict(&scope, DictValues::cast(*self).dict());
  return thread->runtime()->newDictValueIterator(dict);
}

}  // namespace python
