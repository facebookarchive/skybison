#include "generator-builtins.h"

#include <string>

#include "builtins.h"
#include "exception-builtins.h"
#include "frame.h"
#include "modules.h"
#include "type-builtins.h"

namespace py {

RawGeneratorBase generatorFromStackFrame(Frame* frame) {
  // For now, we have the invariant that GeneratorBase bodies are only invoked
  // by __next__() or send(), which have the GeneratorBase as their first local.
  return GeneratorBase::cast(frame->previousFrame()->local(0));
}

template <SymbolId name, LayoutId type>
static RawObject sendImpl(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (self.layoutId() != type) return thread->raiseRequiresType(self, name);
  GeneratorBase gen(&scope, *self);
  Object value(&scope, args.get(1));
  return Interpreter::resumeGenerator(thread, gen, value);
}

// Validate the given exception and send it to gen.
static RawObject throwDoRaise(Thread* thread, const GeneratorBase& gen,
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

  return Interpreter::resumeGeneratorWithRaise(thread, gen, exc, value, tb);
}

// Delegate the given exception to yf.throw(). If yf does not have a throw
// attribute, send the exception to gen like normal.
static RawObject throwYieldFrom(Thread* thread, const GeneratorBase& gen,
                                const Object& yf, const Object& exc,
                                const Object& value, const Object& tb) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  // TODO(bsimmers): If exc == GeneratorExit, close the subiterator. See
  // _gen_throw() in CPython.

  Object throw_obj(&scope, runtime->attributeAtById(thread, yf, ID(throw)));
  if (throw_obj.isError()) {
    // If the call failed with an AttributeError, ignore it and proceed with
    // the throw. Otherwise, forward the exception.
    if (throw_obj.isErrorNotFound() ||
        thread->pendingExceptionMatches(LayoutId::kAttributeError)) {
      thread->clearPendingException();
      return throwDoRaise(thread, gen, exc, value, tb);
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
    // don't have to worry about messing with the GeneratorFrame of a generator
    // that's running.
    DCHECK(gen.running() == Bool::falseObj(), "Generator shouldn't be running");
    GeneratorFrame gf(&scope, gen.generatorFrame());
    Object subiter(&scope, gf.popValue());
    DCHECK(*subiter == *yf, "Unexpected subiter on generator stack");
    gf.setVirtualPC(gf.virtualPC() + kCodeUnitSize);

    if (thread->hasPendingStopIteration()) {
      Object subiter_value(&scope, thread->pendingStopIterationValue());
      thread->clearPendingException();
      return Interpreter::resumeGenerator(thread, gen, subiter_value);
    }
    Object exc_type(&scope, thread->pendingExceptionType());
    Object exc_value(&scope, thread->pendingExceptionValue());
    Object exc_traceback(&scope, thread->pendingExceptionTraceback());
    thread->clearPendingException();
    return Interpreter::resumeGeneratorWithRaise(thread, gen, exc_type,
                                                 exc_value, exc_traceback);
  }

  return *result;
}

static RawObject throwImpl(Thread* thread, const GeneratorBase& gen,
                           const Object& exc, const Object& value,
                           const Object& tb) {
  HandleScope scope(thread);
  Object yf(&scope, Interpreter::findYieldFrom(*gen));
  if (!yf.isNoneType()) {
    return throwYieldFrom(thread, gen, yf, exc, value, tb);
  }
  return throwDoRaise(thread, gen, exc, value, tb);
}

template <SymbolId name, LayoutId type>
static RawObject throwImpl(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (self.layoutId() != type) return thread->raiseRequiresType(self, name);
  GeneratorBase gen(&scope, *self);
  Object exc(&scope, args.get(1));
  Object value(&scope, args.get(2));
  Object tb(&scope, args.get(3));

  return throwImpl(thread, gen, exc, value, tb);
}

static RawObject closeImpl(Thread* thread, const GeneratorBase& gen) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object gen_exit_exc(&scope, runtime->typeAt(LayoutId::kGeneratorExit));
  Object none(&scope, NoneType::object());
  Object result(&scope, throwImpl(thread, gen, gen_exit_exc, none, none));
  if (!result.isError()) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "ignored GeneratorExit");
  }
  if (thread->pendingExceptionMatches(LayoutId::kGeneratorExit) ||
      thread->pendingExceptionMatches(LayoutId::kStopIteration)) {
    thread->clearPendingException();
    return NoneType::object();
  }
  return *result;
}

static const BuiltinAttribute kGeneratorAttributes[] = {
    {ID(_generator__frame), RawGenerator::kFrameOffset,
     AttributeFlags::kHidden},
    {ID(_generator__exception_state), RawGenerator::kExceptionStateOffset,
     AttributeFlags::kHidden},
    {ID(_generator__exception_state), RawGenerator::kExceptionStateOffset,
     AttributeFlags::kHidden},
    {ID(gi_running), RawGenerator::kRunningOffset, AttributeFlags::kReadOnly},
    {ID(__name__), RawAsyncGenerator::kNameOffset, AttributeFlags::kReadOnly},
    {ID(__qualname__), RawGenerator::kQualnameOffset},
    {ID(_generator__yield_from), RawGenerator::kYieldFromOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kCoroutineAttributes[] = {
    {ID(_coroutine__frame), RawCoroutine::kFrameOffset,
     AttributeFlags::kHidden},
    {ID(_coroutine__exception_state), RawCoroutine::kExceptionStateOffset,
     AttributeFlags::kHidden},
    {ID(cr_running), RawCoroutine::kRunningOffset, AttributeFlags::kHidden},
    {ID(__name__), RawAsyncGenerator::kNameOffset, AttributeFlags::kReadOnly},
    {ID(__qualname__), RawCoroutine::kQualnameOffset},
    {ID(_coroutine__await), RawCoroutine::kAwaitOffset,
     AttributeFlags::kHidden},
    {ID(_coroutine__origin), RawCoroutine::kOriginOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kCoroutineWrapperAttributes[] = {
    {ID(_coroutine_wrapper__cw_coroutine),
     RawCoroutineWrapper::kCoroutineOffset, AttributeFlags::kHidden},
};

static const BuiltinAttribute kAsyncGeneratorAttributes[] = {
    {ID(_async_generator__frame), RawAsyncGenerator::kFrameOffset,
     AttributeFlags::kHidden},
    {ID(_async_generator__exception_state),
     RawAsyncGenerator::kExceptionStateOffset, AttributeFlags::kHidden},
    // TODO(T70191611) Make __name__ and __qualname__ writable.
    {ID(__name__), RawAsyncGenerator::kNameOffset, AttributeFlags::kReadOnly},
    {ID(__qualname__), RawAsyncGenerator::kQualnameOffset,
     AttributeFlags::kReadOnly},
    {ID(_async_generator__running), RawAsyncGenerator::kRunningOffset,
     AttributeFlags::kHidden},
    {ID(_async_generator__finalizer), RawAsyncGenerator::kFinalizerOffset,
     AttributeFlags::kHidden},
    {ID(_async_generator__hooks_inited), RawAsyncGenerator::kHooksInitedOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kAsyncGeneratorAcloseAttributes[] = {
    {ID(_async_generator_aclose__generator),
     RawAsyncGeneratorAclose::kGeneratorOffset, AttributeFlags::kHidden},
    {ID(_async_generator_aclose__state), RawAsyncGeneratorAclose::kStateOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kAsyncGeneratorAsendAttributes[] = {
    {ID(_async_generator_asend__generator),
     RawAsyncGeneratorAsend::kGeneratorOffset},
    {ID(_async_generator_asend__state), RawAsyncGeneratorAsend::kStateOffset},
    {ID(_async_generator_asend__value), RawAsyncGeneratorAsend::kValueOffset},
};

static const BuiltinAttribute kAsyncGeneratorWrappedValueAttributes[] = {
    {ID(_async_generator_wrapped_value__value),
     RawAsyncGeneratorWrappedValue::kValueOffset},
};

void initializeGeneratorTypes(Thread* thread) {
  addBuiltinType(thread, ID(generator), LayoutId::kGenerator,
                 /*superclass_id=*/LayoutId::kObject, kGeneratorAttributes);

  addBuiltinType(thread, ID(coroutine), LayoutId::kCoroutine,
                 /*superclass_id=*/LayoutId::kObject, kCoroutineAttributes);

  addBuiltinType(thread, ID(coroutine_wrapper), LayoutId::kCoroutineWrapper,
                 /*superclass_id=*/LayoutId::kObject,
                 kCoroutineWrapperAttributes);

  addBuiltinType(thread, ID(async_generator), LayoutId::kAsyncGenerator,
                 /*superclass_id=*/LayoutId::kObject,
                 kAsyncGeneratorAttributes);

  addBuiltinType(
      thread, ID(async_generator_aclose), LayoutId::kAsyncGeneratorAclose,
      /*superclass_id=*/LayoutId::kObject, kAsyncGeneratorAcloseAttributes);

  addBuiltinType(
      thread, ID(async_generator_asend), LayoutId::kAsyncGeneratorAsend,
      /*superclass_id=*/LayoutId::kObject, kAsyncGeneratorAsendAttributes);

  addBuiltinType(thread, ID(async_generator_wrapped_value),
                 LayoutId::kAsyncGeneratorWrappedValue,
                 /*superclass_id=*/LayoutId::kObject,
                 kAsyncGeneratorWrappedValueAttributes);
}

RawObject METH(async_generator, __aiter__)(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isAsyncGenerator()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "__aiter__() must be called with an "
                                "async_generator instance as the first "
                                "argument, not %T",
                                &self);
  }
  return *self;
}

static RawObject initAsyncGenHooksOnInstance(Thread* thread,
                                             const AsyncGenerator& gen) {
  if (gen.hooksInited()) {
    return NoneType::object();
  }
  HandleScope scope(thread);
  gen.setHooksInited(true);
  gen.setFinalizer(thread->asyncgenHooksFinalizer());
  Object first_iter(&scope, thread->asyncgenHooksFirstIter());
  if (!first_iter.isNoneType()) {
    Object first_iter_res(
        &scope, Interpreter::callFunction1(thread, thread->currentFrame(),
                                           first_iter, gen));
    if (first_iter_res.isErrorException()) {
      return *first_iter_res;
    }
  }
  return NoneType::object();
}

static RawObject setupAsyncGenOpIter(HandleScope& scope, Thread* thread,
                                     RawObject raw_self_obj,
                                     LayoutId op_layout) {
  Object self_obj(&scope, raw_self_obj);
  if (!self_obj.isAsyncGenerator()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Must be called with an async_generator "
                                "instance as the first argument, not %T",
                                &self_obj);
  }
  AsyncGenerator self(&scope, *self_obj);
  Object init_res(&scope, initAsyncGenHooksOnInstance(thread, self));
  if (init_res.isErrorException()) {
    return *init_res;
  }
  Runtime* runtime = thread->runtime();
  Layout layout(&scope, runtime->layoutAt(op_layout));
  Object op_iter_obj(&scope, runtime->newInstance(layout));
  AsyncGeneratorOpIterBase op_iter(&scope, *op_iter_obj);
  op_iter.setGenerator(*self);
  op_iter.setState(AsyncGeneratorOpIterBase::State::Init);
  return *op_iter;
}

static RawObject setupAsyncGenASend(Thread* thread, RawObject raw_self_obj,
                                    RawObject initial_send_value) {
  HandleScope scope(thread);
  Object asend_obj(&scope, setupAsyncGenOpIter(scope, thread, raw_self_obj,
                                               LayoutId::kAsyncGeneratorAsend));
  if (asend_obj.isErrorException()) return *asend_obj;
  AsyncGeneratorAsend asend(&scope, *asend_obj);
  asend.setValue(initial_send_value);
  return *asend;
}

RawObject METH(async_generator, __anext__)(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  return setupAsyncGenASend(thread, args.get(0), NoneType::object());
}

RawObject METH(async_generator, aclose)(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  return setupAsyncGenOpIter(scope, thread, args.get(0),
                             LayoutId::kAsyncGeneratorAclose);
}

RawObject METH(async_generator, asend)(Thread* thread, Frame* frame,
                                       word nargs) {
  Arguments args(frame, nargs);
  return setupAsyncGenASend(thread, args.get(0), args.get(1));
}

static RawObject asyncOpIterReturnSelf(Thread* thread, RawObject raw_self_obj,
                                       LayoutId op_layout, SymbolId method,
                                       SymbolId op_type) {
  HandleScope scope(thread);
  Object self(&scope, raw_self_obj);
  if (!self.isHeapObjectWithLayout(op_layout)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "%Y be called with an %Y instance as the first argument, not %T",
        method, op_type, &self);
  }
  return *self;
}

RawObject METH(async_generator_aclose, __await__)(Thread* thread, Frame* frame,
                                                  word nargs) {
  Arguments args(frame, nargs);
  return asyncOpIterReturnSelf(thread, args.get(0),
                               LayoutId::kAsyncGeneratorAclose, ID(__await__),
                               ID(async_generator_aclose));
}

RawObject METH(async_generator_aclose, __iter__)(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  return asyncOpIterReturnSelf(thread, args.get(0),
                               LayoutId::kAsyncGeneratorAclose, ID(__iter__),
                               ID(async_generator_aclose));
}

static RawObject asyncGenAcloseSend(Thread* thread, RawObject raw_self_obj,
                                    RawObject send_value_raw) {
  HandleScope scope(thread);
  Object self_obj(&scope, raw_self_obj);
  if (!self_obj.isAsyncGeneratorAclose()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Must be called with an async_generator_aclose "
                                "instance as the first argument, not %T",
                                &self_obj);
  }
  AsyncGeneratorAclose self(&scope, *self_obj);

  AsyncGeneratorOpIterBase::State state = self.state();
  if (state == AsyncGeneratorOpIterBase::State::Closed) {
    return thread->raiseStopIteration();
  }

  // Depending on whether the close operation has been applied yet either
  // throw GeneratorExit into the generator, or just send into the iterator
  // to make progress through async-like yields.
  Object res(&scope, NoneType::object());
  AsyncGenerator generator(&scope, self.generator());
  Object send_value(&scope, send_value_raw);
  if (state == AsyncGeneratorOpIterBase::State::Init) {
    if (!send_value.isNoneType()) {
      return thread->raiseWithFmt(
          LayoutId::kRuntimeError,
          "Cannot send non-None value to async generator aclose iterator on "
          "first iteration");
    }
    self.setState(AsyncGeneratorOpIterBase::State::Iter);
    Object exception_type(&scope,
                          thread->runtime()->typeAt(LayoutId::kGeneratorExit));
    Object none(&scope, NoneType::object());
    res = throwImpl(thread, generator, exception_type, none, none);
  } else {
    DCHECK(state == AsyncGeneratorOpIterBase::State::Iter, "Unexpected state");
    res = Interpreter::resumeGenerator(thread, generator, send_value);
  }

  if (res.isErrorException()) {
    // If the exceptions are GeneratorExit or StopAsyncIteration, these are
    // correct and expected ways for the overall async generator to stop. So
    // clear the pending exceptions, mark this iterator as closed, and
    // propagate a StopIteration indicating a clean shutdown. As this is a
    // "close" operation the StopIteration value is always None.
    if (thread->pendingExceptionMatches(LayoutId::kGeneratorExit) ||
        thread->pendingExceptionMatches(LayoutId::kStopAsyncIteration)) {
      self.setState(AsyncGeneratorOpIterBase::State::Closed);
      thread->clearPendingException();
      return thread->raiseStopIteration();
    }

    // Propagate all other exceptions/errors.
    return *res;
  }

  // Producing a generator-like yield indcates the generator has ignored the
  // close request.
  if (res.isAsyncGeneratorWrappedValue()) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "async generator ignored GeneratorExit");
  }

  // Pass along async-like yield to caller for propagation  up to the event
  // loop.
  return *res;
}

RawObject METH(async_generator_aclose, __next__)(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  return asyncGenAcloseSend(thread, args.get(0), NoneType::object());
}

RawObject METH(async_generator_aclose, send)(Thread* thread, Frame* frame,
                                             word nargs) {
  Arguments args(frame, nargs);
  return asyncGenAcloseSend(thread, args.get(0), args.get(1));
}

RawObject METH(async_generator_asend, __await__)(Thread* thread, Frame* frame,
                                                 word nargs) {
  Arguments args(frame, nargs);
  return asyncOpIterReturnSelf(thread, args.get(0),
                               LayoutId::kAsyncGeneratorAsend, ID(__await__),
                               ID(async_generator_asend));
}

RawObject METH(async_generator_asend, __iter__)(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  return asyncOpIterReturnSelf(thread, args.get(0),
                               LayoutId::kAsyncGeneratorAsend, ID(__iter__),
                               ID(async_generator_asend));
}

static RawObject asyncGenAsendSend(Thread* thread, RawObject raw_self_obj,
                                   RawObject send_value_raw) {
  HandleScope scope(thread);
  Object self_obj(&scope, raw_self_obj);
  if (!self_obj.isAsyncGeneratorAsend()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Must be called with an async_generator_asend "
                                "instance as the first argument, not %T",
                                &self_obj);
  }
  AsyncGeneratorAsend self(&scope, *self_obj);
  AsyncGeneratorOpIterBase::State state = self.state();
  if (state == AsyncGeneratorOpIterBase::State::Closed) {
    return thread->raiseStopIteration();
  }
  // Only use primed value for initial send, and only if no other specific value
  // is provided.
  Object send_value(&scope, send_value_raw);
  if (state == AsyncGeneratorOpIterBase::State::Init) {
    if (send_value.isNoneType()) {
      send_value = self.value();
    }
  }
  // "Send" value into generator
  AsyncGenerator generator(&scope, self.generator());
  Object send_res(&scope,
                  Interpreter::resumeGenerator(thread, generator, send_value));
  // Send raises: mark this ASend operation as closed and propagate.
  if (send_res.isError()) {
    self.setState(AsyncGeneratorOpIterBase::State::Closed);
    return *send_res;
  }
  // Send produces a generator-like yield: mark this ASend operation as closed
  // and return the value via a StopIteration raise.
  if (send_res.isAsyncGeneratorWrappedValue()) {
    self.setState(AsyncGeneratorOpIterBase::State::Closed);
    AsyncGeneratorWrappedValue res_wrapped(&scope, *send_res);
    Object res_value(&scope, res_wrapped.value());
    return thread->raiseStopIterationWithValue(res_value);
  }
  // Send produces an async-like yield: pass this along to caller to propagate
  // up to the event loop.
  self.setState(AsyncGeneratorOpIterBase::State::Iter);
  return *send_res;
}

RawObject METH(async_generator_asend, __next__)(Thread* thread, Frame* frame,
                                                word nargs) {
  Arguments args(frame, nargs);
  return asyncGenAsendSend(thread, args.get(0), NoneType::object());
}

RawObject METH(async_generator_asend, send)(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  return asyncGenAsendSend(thread, args.get(0), args.get(1));
}

RawObject METH(generator, __iter__)(Thread* thread, Frame* frame, word nargs) {
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

RawObject METH(generator, __next__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isGenerator()) {
    return thread->raiseRequiresType(self, ID(generator));
  }
  Generator gen(&scope, *self);
  Object value(&scope, NoneType::object());
  return Interpreter::resumeGenerator(thread, gen, value);
}

RawObject METH(generator, close)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isGenerator()) {
    return thread->raiseRequiresType(self, ID(generator));
  }
  GeneratorBase gen(&scope, *self);
  return closeImpl(thread, gen);
}

RawObject METH(generator, send)(Thread* thread, Frame* frame, word nargs) {
  return sendImpl<ID(generator), LayoutId::kGenerator>(thread, frame, nargs);
}

RawObject METH(generator, throw)(Thread* thread, Frame* frame, word nargs) {
  return throwImpl<ID(generator), LayoutId::kGenerator>(thread, frame, nargs);
}

RawObject METH(coroutine, __await__)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isCoroutine()) {
    return thread->raiseRequiresType(self_obj, ID(coroutine));
  }
  Coroutine self(&scope, *self_obj);
  Runtime* runtime = thread->runtime();
  Layout coro_wrap_layout(&scope,
                          runtime->layoutAt(LayoutId::kCoroutineWrapper));
  CoroutineWrapper coro_wrap(&scope, runtime->newInstance(coro_wrap_layout));
  coro_wrap.setCoroutine(*self);
  return *coro_wrap;
}

RawObject METH(coroutine, close)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isCoroutine()) {
    return thread->raiseRequiresType(self, ID(coroutine));
  }
  GeneratorBase gen(&scope, *self);
  return closeImpl(thread, gen);
}

RawObject METH(coroutine, send)(Thread* thread, Frame* frame, word nargs) {
  return sendImpl<ID(coroutine), LayoutId::kCoroutine>(thread, frame, nargs);
}

RawObject METH(coroutine, throw)(Thread* thread, Frame* frame, word nargs) {
  return throwImpl<ID(coroutine), LayoutId::kCoroutine>(thread, frame, nargs);
}

RawObject METH(coroutine_wrapper, __iter__)(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  RawObject self = args.get(0);
  if (self.isCoroutineWrapper()) return self;
  HandleScope scope(thread);
  Object self_obj(&scope, self);
  return thread->raiseRequiresType(self_obj, ID(coroutine));
}

RawObject METH(coroutine_wrapper, __next__)(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isCoroutineWrapper()) {
    return thread->raiseRequiresType(self_obj, ID(coroutine));
  }
  CoroutineWrapper self(&scope, *self_obj);
  GeneratorBase gen(&scope, self.coroutine());
  Object none(&scope, NoneType::object());
  return Interpreter::resumeGenerator(thread, gen, none);
}

RawObject METH(coroutine_wrapper, close)(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isCoroutineWrapper()) {
    return thread->raiseRequiresType(self_obj, ID(coroutine));
  }
  CoroutineWrapper self(&scope, *self_obj);
  GeneratorBase gen(&scope, self.coroutine());
  return closeImpl(thread, gen);
}

RawObject METH(coroutine_wrapper, send)(Thread* thread, Frame* frame,
                                        word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isCoroutineWrapper()) {
    return thread->raiseRequiresType(self_obj, ID(coroutine));
  }
  CoroutineWrapper self(&scope, *self_obj);
  GeneratorBase gen(&scope, self.coroutine());
  Object val(&scope, args.get(1));
  return Interpreter::resumeGenerator(thread, gen, val);
}

RawObject METH(coroutine_wrapper, throw)(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isCoroutineWrapper()) {
    return thread->raiseRequiresType(self_obj, ID(coroutine));
  }
  CoroutineWrapper self(&scope, *self_obj);
  GeneratorBase gen(&scope, self.coroutine());
  Object exc(&scope, args.get(1));
  Object value(&scope, args.get(2));
  Object tb(&scope, args.get(3));
  return throwImpl(thread, gen, exc, value, tb);
}

// Intended for tests only
RawObject FUNC(_builtins, _async_generator_finalizer)(Thread* thread,
                                                      Frame* frame,
                                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isAsyncGenerator()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "_async_generator_finalizer() must be called with an "
        "async_generator instance as the first argument, not %T",
        &self_obj);
  }
  AsyncGenerator self(&scope, *self_obj);
  return self.finalizer();
}

// Intended for tests only
RawObject FUNC(_builtins, _async_generator_op_iter_get_state)(Thread* thread,
                                                              Frame* frame,
                                                              word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isAsyncGeneratorOpIterBase()) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError,
        "_async_generator_op_iter_get_state() must be called with an "
        "async_generator_op_iter_base instance as the first argument, not %T",
        &self_obj);
  }
  AsyncGeneratorOpIterBase self(&scope, *self_obj);
  return RawSmallInt::fromWord(static_cast<word>(self.state()));
}

}  // namespace py
