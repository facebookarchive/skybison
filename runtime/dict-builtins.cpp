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
};

void DictBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type dict_type(&scope,
                 runtime->addBuiltinClass(SymbolId::kDict, LayoutId::kDict,
                                          LayoutId::kObject, kMethods));
  dict_type->setFlag(Type::Flag::kDictSubclass);
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
    ObjectArray keys(&scope, runtime->dictKeys(self));
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

}  // namespace python
