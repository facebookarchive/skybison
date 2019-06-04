#include "generator-builtins.h"

#include "exception-builtins.h"
#include "frame.h"

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
  word pc = heap_frame.virtualPC();
  if (pc == Frame::kFinishedGeneratorPC && !thread->hasPendingException()) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  // Don't allow sending non-None values before the generator is primed.
  if (pc == 0 && !value.isNoneType()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "can't send non-None value to a just-started %T",
        &gen);
  }

  gen.setRunning(Bool::trueObj());
  Object result(&scope, thread->runtime()->genSend(thread, gen, value));
  gen.setRunning(Bool::falseObj());

  if (!result.isError() &&
      heap_frame.virtualPC() == Frame::kFinishedGeneratorPC) {
    // The generator finished normally. Forward its return value in a
    // StopIteration.
    return thread->raise(LayoutId::kStopIteration, *result);
  }
  return *result;
}

template <typename T, SymbolId name, LayoutId type>
RawObject GeneratorBaseBuiltins<T, name, type>::send(Thread* thread,
                                                     Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (self.layoutId() != type) return thread->raiseRequiresType(self, name);
  GeneratorBase gen(&scope, *self);
  Object value(&scope, args.get(1));
  return sendImpl(thread, gen, value);
}

// If the given GeneratorBase is suspended at a YIELD_FROM instruction, return
// its subiterator. Otherwise, return None.
static RawObject findYieldFrom(Thread* thread, const GeneratorBase& gen) {
  HandleScope scope(thread);
  if (gen.running() == Bool::trueObj()) return NoneType::object();
  HeapFrame hf(&scope, gen.heapFrame());
  word pc = hf.virtualPC();
  if (pc == Frame::kFinishedGeneratorPC) return NoneType::object();
  Function function(&scope, hf.function());
  MutableBytes bytecode(&scope, function.rewrittenBytecode());
  if (bytecode.byteAt(pc) != Bytecode::YIELD_FROM) return NoneType::object();
  return hf.valueStackTop()[0];
}

// Validate the given exception and send it to gen.
static RawObject genThrowDoRaise(Thread* thread, const GeneratorBase& gen,
                                 const Object& exc_in, const Object& value_in,
                                 const Object& tb_in) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object exc(&scope, *exc_in);
  Object value(&scope, value_in.isUnbound() ? NoneType::object() : *value_in);
  Object tb(&scope, tb_in.isUnbound() ? NoneType::object() : *tb_in);

  // TODO(T39919701): Until we have proper traceback support, we sometimes pass
  // around a string as an exception's traceback.
  if (!tb.isNoneType() && !tb.isTraceback() && !tb.isStr()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "throw() third argument must be a traceback object");
  }
  if (runtime->isInstanceOfType(*exc) &&
      Type(&scope, *exc).isBaseExceptionSubclass()) {
    normalizeException(thread, &exc, &value, &tb);
  } else if (runtime->isInstanceOfBaseException(*exc)) {
    if (!value.isNoneType()) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError,
          "instance exception may not have a separate value");
    }
    value = *exc;
    exc = runtime->typeOf(*exc);
    if (tb.isNoneType()) {
      BaseException base_exc(&scope, *value);
      tb = base_exc.traceback();
    }
  } else {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "exceptions must be classes or instances "
                                "deriving from BaseException, not %T",
                                &exc);
  }

  thread->setPendingExceptionType(*exc);
  thread->setPendingExceptionValue(*value);
  thread->setPendingExceptionTraceback(*tb);
  Object none(&scope, NoneType::object());
  return sendImpl(thread, gen, none);
}

// Delegate the given exception to yf.throw(). If yf does not have a throw
// attribute, send the exception to gen like normal.
static RawObject genThrowYieldFrom(Thread* thread, const GeneratorBase& gen,
                                   const Object& yf, const Object& exc,
                                   const Object& value, const Object& tb) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  // TODO(bsimmers): If exc == GeneratorExit, close the subiterator. See
  // _gen_throw() in CPython.

  Object throw_obj(&scope,
                   runtime->attributeAtId(thread, yf, SymbolId::kThrow));
  if (throw_obj.isError()) {
    // If the call failed with an AttributeError, ignore it and proceed with
    // the throw. Otherwise, forward the exception.
    if (throw_obj.isErrorNotFound() ||
        thread->pendingExceptionMatches(LayoutId::kAttributeError)) {
      thread->clearPendingException();
      return genThrowDoRaise(thread, gen, exc, value, tb);
    }
    return *throw_obj;
  }

  Object result(&scope, NoneType::object());
  gen.setRunning(Bool::trueObj());
  // This is awkward but necessary to maintain compatibility with how CPython
  // calls yf.throw(): it forwards exaclty as many arguments as it was given.
  if (value.isUnbound()) {
    result = Interpreter::callFunction1(thread, thread->currentFrame(),
                                        throw_obj, exc);
  } else if (tb.isUnbound()) {
    result = Interpreter::callFunction2(thread, thread->currentFrame(),
                                        throw_obj, exc, value);
  } else {
    result = Interpreter::callFunction3(thread, thread->currentFrame(),
                                        throw_obj, exc, value, tb);
  }
  gen.setRunning(Bool::falseObj());

  if (result.isError()) {
    // The subiterator raised, so finish the YIELD_FROM in the parent. If the
    // exception is a StopIteration, continue in the parent like usually;
    // otherwise, propagate the exception at the YIELD_FROM.

    // findYieldFrom() returns None when gen is currently executing, so we
    // don't have to worry about messing with the HeapFrame of a generator
    // that's running.
    DCHECK(gen.running() == Bool::falseObj(), "Generator shouldn't be running");
    HeapFrame hf(&scope, gen.heapFrame());
    Object subiter(&scope, hf.popValue());
    DCHECK(*subiter == *yf, "Unexpected subiter on generator stack");
    hf.setVirtualPC(hf.virtualPC() + Frame::kCodeUnitSize);

    Object subiter_value(&scope, NoneType::object());
    if (thread->hasPendingStopIteration()) {
      subiter_value = thread->pendingStopIterationValue();
      thread->clearPendingException();
    }
    return sendImpl(thread, gen, subiter_value);
  }

  return *result;
}

static RawObject genThrowImpl(Thread* thread, const GeneratorBase& gen,
                              const Object& exc, const Object& value,
                              const Object& tb) {
  HandleScope scope(thread);
  Object yf(&scope, findYieldFrom(thread, gen));
  if (!yf.isNoneType()) {
    return genThrowYieldFrom(thread, gen, yf, exc, value, tb);
  }
  return genThrowDoRaise(thread, gen, exc, value, tb);
}

template <typename T, SymbolId name, LayoutId type>
RawObject GeneratorBaseBuiltins<T, name, type>::genThrow(Thread* thread,
                                                         Frame* frame,
                                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (self.layoutId() != type) return thread->raiseRequiresType(self, name);
  GeneratorBase gen(&scope, *self);
  Object exc(&scope, args.get(1));
  Object value(&scope, args.get(2));
  Object tb(&scope, args.get(3));

  return genThrowImpl(thread, gen, exc, value, tb);
}

const BuiltinMethod GeneratorBuiltins::kBuiltinMethods[] = {
    // clang-format off
    {SymbolId::kDunderIter, dunderIter},
    {SymbolId::kDunderNext, dunderNext},
    {SymbolId::kSend, Base::send},
    {SymbolId::kThrow, Base::genThrow},
    {SymbolId::kSentinelId, nullptr},
    // clang-format on
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
    return thread->raiseWithFmt(
        LayoutId::kAttributeError,
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
    {SymbolId::kSend, Base::send},
    {SymbolId::kSentinelId, nullptr},
};

const BuiltinAttribute CoroutineBuiltins::kAttributes[] = {
    {SymbolId::kDunderQualname, RawGeneratorBase::kQualnameOffset},
    {SymbolId::kCrRunning, RawCoroutine::kRunningOffset,
     AttributeFlags::kReadOnly},
    {SymbolId::kSentinelId, -1},
};

}  // namespace python
