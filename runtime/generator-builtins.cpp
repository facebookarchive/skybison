#include "generator-builtins.h"

#include "frame.h"
#include "trampolines-inl.h"

#include <string>

namespace python {

void GeneratorBaseBuiltins::initialize(Runtime* runtime) {
  GeneratorBuiltins::initialize(runtime);
  CoroutineBuiltins::initialize(runtime);
}

template <LayoutId target>
RawObject GeneratorBaseBuiltins::send(Thread* thread, Frame* frame,
                                      word nargs) {
  static_assert(
      target == LayoutId::kGenerator || target == LayoutId::kCoroutine,
      "unsupported LayoutId");
  const char* type_name =
      target == LayoutId::kGenerator ? "generator" : "coroutine";

  if (nargs != 2) {
    return thread->raiseTypeErrorWithCStr("send() takes 1 argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (self.layoutId() != target) {
    return thread->raiseAttributeError(thread->runtime()->newStrFromFormat(
        "send() must be called with a %s instance as the first argument",
        type_name));
  }
  GeneratorBase gen(&scope, *self);
  if (RawHeapFrame::cast(gen.heapFrame())->frame()->virtualPC() == 0 &&
      !args.get(1)->isNoneType()) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFormat(
        "can't send non-None value to a just-started %s", type_name));
  }
  Object value(&scope, args.get(1));
  return thread->runtime()->genSend(thread, gen, value);
}

const BuiltinMethod GeneratorBuiltins::kMethods[] = {
    {SymbolId::kDunderIter, nativeTrampoline<dunderIter>},
    {SymbolId::kDunderNext, nativeTrampoline<dunderNext>},
    {SymbolId::kSend,
     nativeTrampoline<GeneratorBaseBuiltins::send<LayoutId::kGenerator>>},
};

void GeneratorBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type generator(&scope, runtime->addBuiltinTypeWithMethods(
                             SymbolId::kGenerator, LayoutId::kGenerator,
                             LayoutId::kObject, kMethods));
}

RawObject GeneratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__iter__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isGenerator()) {
    return thread->raiseAttributeErrorWithCStr(
        "__iter__() must be called with a generator instance as the first "
        "argument");
  }
  return *self;
}

RawObject GeneratorBuiltins::dunderNext(Thread* thread, Frame* frame,
                                        word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("__next__() takes no arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isGenerator()) {
    return thread->raiseAttributeErrorWithCStr(
        "__next__() must be called with a generator instance as the first "
        "argument");
  }
  Generator gen(&scope, *self);
  Object value(&scope, NoneType::object());
  return thread->runtime()->genSend(thread, gen, value);
}

const BuiltinMethod CoroutineBuiltins::kMethods[] = {
    {SymbolId::kSend,
     nativeTrampoline<GeneratorBaseBuiltins::send<LayoutId::kCoroutine>>},
};

void CoroutineBuiltins::initialize(Runtime* runtime) {
  HandleScope scope;
  Type coroutine(&scope, runtime->addBuiltinTypeWithMethods(
                             SymbolId::kCoroutine, LayoutId::kCoroutine,
                             LayoutId::kObject, kMethods));
}

}  // namespace python
