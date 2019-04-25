#include "generator-builtins.h"

#include "frame.h"
#include "trampolines-inl.h"

#include <string>

namespace python {

// Check that the generator is in an OK state to resume, run it, then check if
// it's newly-finished.
static RawObject sendImpl(Thread* thread, const GeneratorBase& gen,
                          const Object& value) {
  HandleScope scope(thread);
  HeapFrame heap_frame(&scope, gen.heapFrame());
  // Don't resume an already-running generator.
  if (gen.running() == Bool::trueObj()) {
    return thread->raiseWithFmt(LayoutId::kValueError, "%T already executing",
                                &gen);
  }
  // If the generator has finished and we're not already sending an exception,
  // raise StopIteration.
  if (heap_frame.frame()->virtualPC() == Frame::kFinishedGeneratorPC &&
      !thread->hasPendingException()) {
    return thread->raiseStopIteration(NoneType::object());
  }
  // Don't allow sending non-None values before the generator is primed.
  if (heap_frame.frame()->virtualPC() == 0 && !value.isNoneType()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "can't send non-None value to a just-started %T",
        &gen);
  }

  gen.setRunning(Bool::trueObj());
  Object result(&scope, thread->runtime()->genSend(thread, gen, value));
  gen.setRunning(Bool::falseObj());

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
  Object value(&scope, args.get(1));
  return sendImpl(thread, gen, value);
}

const BuiltinMethod GeneratorBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSend, GeneratorBaseBuiltins::send<LayoutId::kGenerator>},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinAttribute GeneratorBuiltins::kAttributes[] = {
    {SymbolId::kDunderQualname, RawGeneratorBase::kQualnameOffset},
    {SymbolId::kGiRunning, RawGenerator::kRunningOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
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

const BuiltinAttribute CoroutineBuiltins::kAttributes[] = {
    {SymbolId::kDunderQualname, RawGeneratorBase::kQualnameOffset},
    {SymbolId::kCrRunning, RawCoroutine::kRunningOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
};

}  // namespace python
