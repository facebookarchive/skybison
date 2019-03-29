#include "generator-builtins.h"

#include "frame.h"
#include "trampolines-inl.h"

#include <string>

namespace python {

// Common work between send() and __next__(): Check if the generator has already
// finished, run it, then check if it's newly-finished.
static RawObject sendImpl(Thread* thread, const GeneratorBase& gen,
                          const Object& value) {
  HandleScope scope(thread);
  HeapFrame heap_frame(&scope, gen.heapFrame());
  if (heap_frame.frame()->virtualPC() == Frame::kFinishedGeneratorPC) {
    // The generator has already finished.
    return thread->raiseStopIteration(NoneType::object());
  }
  Object result(&scope, thread->runtime()->genSend(thread, gen, value));
  if (!result.isError() &&
      heap_frame.frame()->virtualPC() == Frame::kFinishedGeneratorPC) {
    // The generator finished normally. Forward its return value in a
    // StopIteration.
    return thread->raiseStopIteration(*result);
  }
  return *result;
}

template <LayoutId target>
RawObject GeneratorBaseBuiltins::send(Thread* thread, Frame* frame,
                                      word nargs) {
  static_assert(
      target == LayoutId::kGenerator || target == LayoutId::kCoroutine,
      "unsupported LayoutId");

  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Str type_name(&scope, target == LayoutId::kGenerator
                            ? runtime->symbols()->Generator()
                            : runtime->symbols()->Coroutine());
  Object self(&scope, args.get(0));
  if (self.layoutId() != target) {
    return thread->raiseAttributeError(thread->runtime()->newStrFromFmt(
        "send() must be called with a %S instance as the first argument",
        &type_name));
  }
  GeneratorBase gen(&scope, *self);
  if (RawHeapFrame::cast(gen.heapFrame()).frame()->virtualPC() == 0 &&
      !args.get(1).isNoneType()) {
    return thread->raiseTypeError(thread->runtime()->newStrFromFmt(
        "can't send non-None value to a just-started %S", &type_name));
  }
  Object value(&scope, args.get(1));
  return sendImpl(thread, gen, value);
}

const BuiltinMethod GeneratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSend, GeneratorBaseBuiltins::send<LayoutId::kGenerator>},
    {SymbolId::kSentinelId, nullptr},
};

RawObject GeneratorBuiltins::dunderIter(Thread* thread, Frame* frame,
                                        word nargs) {
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
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isGenerator()) {
    return thread->raiseRequiresType(self, SymbolId::kGenerator);
  }
  Generator gen(&scope, *self);
  Object value(&scope, NoneType::object());
  return sendImpl(thread, gen, value);
}

const BuiltinMethod CoroutineBuiltins::kBuiltinMethods[] = {
    {SymbolId::kSend, GeneratorBaseBuiltins::send<LayoutId::kCoroutine>},
    {SymbolId::kSentinelId, nullptr},
};

}  // namespace python
