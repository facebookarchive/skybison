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
};

void DictBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Handle<Type> dict_type(
      &scope, runtime->addBuiltinClass(SymbolId::kDict, LayoutId::kDict,
                                       LayoutId::kObject, kMethods));
  dict_type->setFlag(Type::Flag::kDictSubclass);
}

Object* DictBuiltins::dunderContains(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> key(&scope, args.get(1));
  if (!self->isDict()) {
    return thread->throwTypeErrorFromCStr(
        "dict.__contains__(self): self must be a dict");
  }
  Handle<Dict> dict(&scope, *self);
  return Bool::fromBool(thread->runtime()->dictIncludes(dict, key));
}

Object* DictBuiltins::dunderDelItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> key(&scope, args.get(1));
  if (self->isDict()) {
    Handle<Dict> dict(&scope, *self);
    // Remove the key. If it doesn't exist, throw a KeyError.
    if (!thread->runtime()->dictRemove(dict, key, nullptr)) {
      return thread->throwKeyErrorFromCStr("missing key can't be deleted");
    }
    return None::object();
  }
  // TODO(T32856777): handle user-defined subtypes of dict.
  return thread->throwTypeErrorFromCStr(
      "__delitem__() must be called with a dict instance as the first "
      "argument");
}

Object* DictBuiltins::dunderEq(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isDict() && args.get(1)->isDict()) {
    HandleScope scope(thread);
    Runtime* runtime = thread->runtime();

    Handle<Dict> self(&scope, args.get(0));
    Handle<Dict> other(&scope, args.get(1));
    if (self->numItems() != other->numItems()) {
      return Bool::falseObj();
    }
    Handle<ObjectArray> keys(&scope, runtime->dictKeys(self));
    Handle<Object> left_key(&scope, None::object());
    Handle<Object> left(&scope, None::object());
    Handle<Object> right(&scope, None::object());
    word length = keys->length();
    for (word i = 0; i < length; i++) {
      left_key = keys->at(i);
      left = runtime->dictAt(self, left_key);
      right = runtime->dictAt(other, left_key);
      if (right->isError()) {
        return Bool::falseObj();
      }
      Object* result =
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

Object* DictBuiltins::dunderLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCStr("__len__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  if (self->isDict()) {
    return SmallInt::fromWord(Dict::cast(*self)->numItems());
  }
  // TODO(T32856777): handle user-defined subtypes of dict.
  return thread->throwTypeErrorFromCStr("'__len__' requires a 'dict' object");
}

Object* DictBuiltins::dunderGetItem(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCStr("expected 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> key(&scope, args.get(1));
  if (self->isDict()) {
    Handle<Dict> dict(&scope, *self);
    Handle<Object> dunder_hash(
        &scope,
        Interpreter::lookupMethod(thread, frame, key, SymbolId::kDunderHash));
    Handle<Object> key_hash(
        &scope, Interpreter::callMethod1(thread, frame, dunder_hash, key));
    Handle<Object> value(
        &scope, thread->runtime()->dictAtWithHash(dict, key, key_hash));
    if (value->isError()) {
      return thread->throwKeyErrorFromCStr("KeyError");
    }
    return *value;
  }
  // TODO(T32856777): handle user-defined subtypes of dict.
  return thread->throwTypeErrorFromCStr(
      "__getitem__() must be called with a dict instance as the first "
      "argument");
}

}  // namespace python
