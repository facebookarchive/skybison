#include "generator-builtins.h"

#include <string>

#include "builtins.h"
#include "exception-builtins.h"
#include "frame.h"
#include "modules.h"
#include "objects.h"
#include "type-builtins.h"

namespace py {

RawGeneratorBase generatorFromStackFrame(Frame* frame) {
  // For now, we have the invariant that GeneratorBase bodies are only invoked
  // by __next__() or send(), which have the GeneratorBase as their first local.
  return GeneratorBase::cast(frame->previousFrame()->local(0));
}

template <SymbolId name, LayoutId type>
static RawObject sendImpl(Thread* thread, RawObject raw_self,
                          RawObject raw_value) {
  HandleScope scope(thread);
  Object self(&scope, raw_self);
  Object value(&scope, raw_value);
  if (self.layoutId() != type) return thread->raiseRequiresType(self, name);
  GeneratorBase gen(&scope, *self);
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

  if (!tb.isNoneType() && !tb.isTraceback()) {
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
    result = Interpreter::call1(thread, throw_obj, exc);
  } else if (tb.isUnbound()) {
    result = Interpreter::call2(thread, throw_obj, exc, value);
  } else {
    result = Interpreter::call3(thread, throw_obj, exc, value, tb);
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
static RawObject throwImpl(Thread* thread, RawObject raw_self,
                           RawObject raw_exc, RawObject raw_value,
                           RawObject raw_tb) {
  HandleScope scope(thread);
  Object self(&scope, raw_self);
  Object exc(&scope, raw_exc);
  Object value(&scope, raw_value);
  Object tb(&scope, raw_tb);
  if (self.layoutId() != type) return thread->raiseRequiresType(self, name);
  GeneratorBase gen(&scope, *self);

  return throwImpl(thread, gen, exc, value, tb);
}

static RawObject closeImpl(Thread* thread, const GeneratorBase& gen) {
  HandleScope scope(thread);
  GeneratorFrame generator_frame(&scope, gen.generatorFrame());
  if (generator_frame.virtualPC() == Frame::kFinishedGeneratorPC) {
    return NoneType::object();
  }
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
    {ID(__name__), RawAsyncGenerator::kNameOffset, AttributeFlags::kReadOnly},
    {ID(__qualname__), RawGenerator::kQualnameOffset},
    {ID(gi_running), RawGenerator::kRunningOffset, AttributeFlags::kReadOnly},
    {ID(_generator__yield_from), RawGenerator::kYieldFromOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kCoroutineAttributes[] = {
    {ID(_coroutine__frame), RawCoroutine::kFrameOffset,
     AttributeFlags::kHidden},
    {ID(_coroutine__exception_state), RawCoroutine::kExceptionStateOffset,
     AttributeFlags::kHidden},
    {ID(__name__), RawAsyncGenerator::kNameOffset, AttributeFlags::kReadOnly},
    {ID(__qualname__), RawCoroutine::kQualnameOffset},
    {ID(cr_running), RawCoroutine::kRunningOffset, AttributeFlags::kReadOnly},
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
     RawAsyncGeneratorAsend::kGeneratorOffset, AttributeFlags::kHidden},
    {ID(_async_generator_asend__state), RawAsyncGeneratorAsend::kStateOffset,
     AttributeFlags::kHidden},
    {ID(_async_generator_asend__value), RawAsyncGeneratorAsend::kValueOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kAsyncGeneratorAthrowAttributes[] = {
    {ID(_async_generator_athrow__generator),
     RawAsyncGeneratorAthrow::kGeneratorOffset, AttributeFlags::kHidden},
    {ID(_async_generator_athrow__state), RawAsyncGeneratorAthrow::kStateOffset,
     AttributeFlags::kHidden},
    {ID(_async_generator_athrow__exception_traceback),
     RawAsyncGeneratorAthrow::kExceptionTracebackOffset,
     AttributeFlags::kHidden},
    {ID(_async_generator_athrow__exception_type),
     RawAsyncGeneratorAthrow::kExceptionTypeOffset, AttributeFlags::kHidden},
    {ID(_async_generator_athrow__exception_value),
     RawAsyncGeneratorAthrow::kExceptionValueOffset, AttributeFlags::kHidden},
};

static const BuiltinAttribute kAsyncGeneratorWrappedValueAttributes[] = {
    {ID(_async_generator_wrapped_value__value),
     RawAsyncGeneratorWrappedValue::kValueOffset},
};

void initializeGeneratorTypes(Thread* thread) {
  addBuiltinType(thread, ID(generator), LayoutId::kGenerator,
                 /*superclass_id=*/LayoutId::kObject, kGeneratorAttributes,
                 Generator::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(frame), LayoutId::kGeneratorFrame,
                 LayoutId::kObject, kNoAttributes, GeneratorFrame::kSize,
                 /*basetype=*/false);

  addBuiltinType(thread, ID(coroutine), LayoutId::kCoroutine,
                 /*superclass_id=*/LayoutId::kObject, kCoroutineAttributes,
                 Coroutine::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(coroutine_wrapper), LayoutId::kCoroutineWrapper,
                 /*superclass_id=*/LayoutId::kObject,
                 kCoroutineWrapperAttributes, CoroutineWrapper::kSize,
                 /*basetype=*/false);

  addBuiltinType(thread, ID(async_generator), LayoutId::kAsyncGenerator,
                 /*superclass_id=*/LayoutId::kObject, kAsyncGeneratorAttributes,
                 AsyncGenerator::kSize, /*basetype=*/false);

  addBuiltinType(
      thread, ID(async_generator_aclose), LayoutId::kAsyncGeneratorAclose,
      /*superclass_id=*/LayoutId::kObject, kAsyncGeneratorAcloseAttributes,
      RawAsyncGeneratorAclose::kSize, /*basetype=*/false);

  addBuiltinType(
      thread, ID(async_generator_asend), LayoutId::kAsyncGeneratorAsend,
      /*superclass_id=*/LayoutId::kObject, kAsyncGeneratorAsendAttributes,
      RawAsyncGeneratorAsend::kSize, /*basetype=*/false);

  addBuiltinType(
      thread, ID(async_generator_athrow), LayoutId::kAsyncGeneratorAthrow,
      /*superclass_id=*/LayoutId::kObject, kAsyncGeneratorAthrowAttributes,
      RawAsyncGeneratorAthrow::kSize, /*basetype=*/false);

  addBuiltinType(thread, ID(async_generator_wrapped_value),
                 LayoutId::kAsyncGeneratorWrappedValue,
                 /*superclass_id=*/LayoutId::kObject,
                 kAsyncGeneratorWrappedValueAttributes,
                 AsyncGeneratorWrappedValue::kSize,
                 /*basetype=*/false);
}

RawObject METH(async_generator, __aiter__)(Thread* thread, Arguments args) {
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
    Object first_iter_res(&scope, Interpreter::call1(thread, first_iter, gen));
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

RawObject METH(async_generator, __anext__)(Thread* thread, Arguments args) {
  return setupAsyncGenASend(thread, args.get(0), NoneType::object());
}

RawObject METH(async_generator, aclose)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  return setupAsyncGenOpIter(scope, thread, args.get(0),
                             LayoutId::kAsyncGeneratorAclose);
}

RawObject METH(async_generator, asend)(Thread* thread, Arguments args) {
  return setupAsyncGenASend(thread, args.get(0), args.get(1));
}

RawObject METH(async_generator, athrow)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object athrow_obj(&scope,
                    setupAsyncGenOpIter(scope, thread, args.get(0),
                                        LayoutId::kAsyncGeneratorAthrow));
  if (athrow_obj.isErrorException()) return *athrow_obj;
  AsyncGeneratorAthrow athrow(&scope, *athrow_obj);
  athrow.setExceptionType(args.get(1));
  athrow.setExceptionValue(args.get(2));
  athrow.setExceptionTraceback(args.get(3));
  return *athrow;
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

RawObject METH(async_generator_aclose, __await__)(Thread* thread,
                                                  Arguments args) {
  return asyncOpIterReturnSelf(thread, args.get(0),
                               LayoutId::kAsyncGeneratorAclose, ID(__await__),
                               ID(async_generator_aclose));
}

RawObject METH(async_generator_aclose, __iter__)(Thread* thread,
                                                 Arguments args) {
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
    return thread->raiseWithFmt(
        LayoutId::kRuntimeError,
        "cannot reuse already awaited aclose()/athrow()");
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

RawObject METH(async_generator_aclose, __next__)(Thread* thread,
                                                 Arguments args) {
  return asyncGenAcloseSend(thread, args.get(0), NoneType::object());
}

static RawObject closeAsyncGenOpIter(Thread* thread, RawObject raw_self_obj,
                                     LayoutId op_layout, SymbolId op_type) {
  HandleScope scope(thread);
  Object self_obj(&scope, raw_self_obj);
  if (!self_obj.isHeapObjectWithLayout(op_layout)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "close() must be called with an %Y instance as "
                                "the first argument, not %T",
                                op_type, &self_obj);
  }
  AsyncGeneratorOpIterBase self(&scope, *self_obj);
  self.setState(AsyncGeneratorOpIterBase::State::Closed);
  return NoneType::object();
}

RawObject METH(async_generator_aclose, close)(Thread* thread, Arguments args) {
  return closeAsyncGenOpIter(thread, args.get(0),
                             LayoutId::kAsyncGeneratorAclose,
                             ID(async_generator_aclose));
}

RawObject METH(async_generator_aclose, send)(Thread* thread, Arguments args) {
  return asyncGenAcloseSend(thread, args.get(0), args.get(1));
}

RawObject METH(async_generator_aclose, throw)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isAsyncGeneratorAclose()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Must be called with an async_generator_aclose "
                                "instance as the first argument, not %T",
                                &self_obj);
  }
  AsyncGeneratorAclose self(&scope, *self_obj);

  AsyncGeneratorOpIterBase::State state = self.state();
  if (state == AsyncGeneratorOpIterBase::State::Closed) {
    return thread->raiseWithFmt(
        LayoutId::kRuntimeError,
        "cannot reuse already awaited aclose()/athrow()");
  }
  if (state == AsyncGeneratorOpIterBase::State::Init) {
    return thread->raiseWithFmt(
        LayoutId::kRuntimeError,
        "cannot throw into async generator via aclose iterator before send");
  }

  // Throw into generator
  AsyncGenerator generator(&scope, self.generator());
  Object exception_type(&scope, args.get(1));
  Object exception_value(&scope, args.get(2));
  Object exception_traceback(&scope, args.get(3));
  Object res(&scope, throwImpl(thread, generator, exception_type,
                               exception_value, exception_traceback));

  // Getting a generator-like yield means the generator ignored the close
  // operation.
  if (res.isAsyncGeneratorWrappedValue()) {
    return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                "async generator ignored GeneratorExit");
  }

  // Propagate async-like yield.
  return *res;
}

RawObject METH(async_generator_asend, __await__)(Thread* thread,
                                                 Arguments args) {
  return asyncOpIterReturnSelf(thread, args.get(0),
                               LayoutId::kAsyncGeneratorAsend, ID(__await__),
                               ID(async_generator_asend));
}

RawObject METH(async_generator_asend, __iter__)(Thread* thread,
                                                Arguments args) {
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
    return thread->raiseWithFmt(
        LayoutId::kRuntimeError,
        "cannot reuse already awaited __anext__()/asend()");
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
  if (send_res.isErrorException()) {
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

RawObject METH(async_generator_asend, __next__)(Thread* thread,
                                                Arguments args) {
  return asyncGenAsendSend(thread, args.get(0), NoneType::object());
}

RawObject METH(async_generator_asend, close)(Thread* thread, Arguments args) {
  return closeAsyncGenOpIter(thread, args.get(0),
                             LayoutId::kAsyncGeneratorAsend,
                             ID(async_generator_asend));
}

RawObject METH(async_generator_asend, send)(Thread* thread, Arguments args) {
  return asyncGenAsendSend(thread, args.get(0), args.get(1));
}

RawObject METH(async_generator_asend, throw)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isAsyncGeneratorAsend()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Must be called with an async_generator_asend "
                                "instance as the first argument, not %T",
                                &self_obj);
  }
  AsyncGeneratorAsend self(&scope, *self_obj);

  AsyncGeneratorOpIterBase::State state = self.state();
  if (state == AsyncGeneratorOpIterBase::State::Closed) {
    return thread->raiseWithFmt(
        LayoutId::kRuntimeError,
        "cannot reuse already awaited __anext__()/asend()");
  }

  // Throw into generator
  AsyncGenerator generator(&scope, self.generator());
  Object exception_type(&scope, args.get(1));
  Object exception_value(&scope, args.get(2));
  Object exception_traceback(&scope, args.get(3));
  Object res(&scope, throwImpl(thread, generator, exception_type,
                               exception_value, exception_traceback));

  // Propagate any uncaught exceptions and mark this send operation as closed.
  if (res.isError()) {
    self.setState(AsyncGeneratorOpIterBase::State::Closed);
    return *res;
  }

  // Generator-like yield: raise this in a StopIteration and mark this iterator
  // as closed.
  if (res.isAsyncGeneratorWrappedValue()) {
    self.setState(AsyncGeneratorOpIterBase::State::Closed);
    AsyncGeneratorWrappedValue wrapped_value(&scope, *res);
    Object value(&scope, wrapped_value.value());
    return thread->raiseStopIterationWithValue(value);
  }

  // Async-like yield: mark this iterator as being in the iteration state and
  // pass the result to the caller for propagation to the event-loop.
  self.setState(AsyncGeneratorOpIterBase::State::Iter);
  return *res;
}

RawObject METH(async_generator_athrow, __await__)(Thread* thread,
                                                  Arguments args) {
  return asyncOpIterReturnSelf(thread, args.get(0),
                               LayoutId::kAsyncGeneratorAthrow, ID(__await__),
                               ID(async_generator_athrow));
}

RawObject METH(async_generator_athrow, __iter__)(Thread* thread,
                                                 Arguments args) {
  return asyncOpIterReturnSelf(thread, args.get(0),
                               LayoutId::kAsyncGeneratorAthrow, ID(__iter__),
                               ID(async_generator_athrow));
}

static RawObject asyncGenAthrowSend(Thread* thread, RawObject raw_self_obj,
                                    RawObject send_value_raw) {
  HandleScope scope(thread);
  Object self_obj(&scope, raw_self_obj);
  if (!self_obj.isAsyncGeneratorAthrow()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Must be called with an async_generator_athrow "
                                "instance as the first argument, not %T",
                                &self_obj);
  }
  AsyncGeneratorAthrow self(&scope, *self_obj);

  AsyncGeneratorOpIterBase::State state = self.state();
  if (state == AsyncGeneratorOpIterBase::State::Closed) {
    return thread->raiseWithFmt(
        LayoutId::kRuntimeError,
        "cannot reuse already awaited aclose()/athrow()");
  }

  // Depending on whether the throw operation has been applied yet either
  // implement the throw, or just send into the iterator to make progress
  // through async-like yields.
  Object res(&scope, NoneType::object());
  AsyncGenerator generator(&scope, self.generator());
  Object send_value(&scope, send_value_raw);
  if (state == AsyncGeneratorOpIterBase::State::Init) {
    if (!send_value.isNoneType()) {
      return thread->raiseWithFmt(
          LayoutId::kRuntimeError,
          "Cannot send non-None value to async generator athrow iterator on "
          "first iteration");
    }
    self.setState(AsyncGeneratorOpIterBase::State::Iter);
    Object exception_type(&scope, self.exceptionType());
    Object exception_value(&scope, self.exceptionValue());
    Object exception_traceback(&scope, self.exceptionTraceback());
    res = throwImpl(thread, generator, exception_type, exception_value,
                    exception_traceback);
    // Handle StopAsyncIteration and GeneratorExit exceptions raised here.
    // Other exceptions are handled further down.
    if (res.isErrorException()) {
      if (thread->pendingExceptionMatches(LayoutId::kStopAsyncIteration)) {
        self.setState(AsyncGeneratorOpIterBase::State::Closed);
        return *res;
      }
      if (thread->pendingExceptionMatches(LayoutId::kGeneratorExit)) {
        self.setState(AsyncGeneratorOpIterBase::State::Closed);
        thread->clearPendingException();
        return thread->raiseStopIteration();
      }
    }
  } else {
    DCHECK(state == AsyncGeneratorOpIterBase::State::Iter, "Unexpected state");
    res = Interpreter::resumeGenerator(thread, generator, send_value);
  }

  // Propagate all unhandled exceptions from send or throw operation.
  if (res.isErrorException()) return *res;

  // Generator-like yield: raise this in a StopIteration and mark this iterator
  // as closed.
  if (res.isAsyncGeneratorWrappedValue()) {
    // Note we don't move into the "closed" state here as we would in an asend
    // iterator. I'm not sure why, but this is the CPython behavior.
    AsyncGeneratorWrappedValue wrapped_value(&scope, *res);
    Object value(&scope, wrapped_value.value());
    return thread->raiseStopIterationWithValue(value);
  }

  // Async-like yield: pass the result to the caller for propagation to the
  // event-loop.
  return *res;
}

RawObject METH(async_generator_athrow, __next__)(Thread* thread,
                                                 Arguments args) {
  return asyncGenAthrowSend(thread, args.get(0), NoneType::object());
}

RawObject METH(async_generator_athrow, close)(Thread* thread, Arguments args) {
  return closeAsyncGenOpIter(thread, args.get(0),
                             LayoutId::kAsyncGeneratorAthrow,
                             ID(async_generator_athrow));
}

RawObject METH(async_generator_athrow, send)(Thread* thread, Arguments args) {
  return asyncGenAthrowSend(thread, args.get(0), args.get(1));
}

RawObject METH(async_generator_athrow, throw)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isAsyncGeneratorAthrow()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Must be called with an async_generator_athrow "
                                "instance as the first argument, not %T",
                                &self_obj);
  }
  AsyncGeneratorAthrow self(&scope, *self_obj);

  AsyncGeneratorOpIterBase::State state = self.state();
  if (state == AsyncGeneratorOpIterBase::State::Closed) {
    return thread->raiseWithFmt(
        LayoutId::kRuntimeError,
        "cannot reuse already awaited aclose()/athrow()");
  }

  if (state == AsyncGeneratorOpIterBase::State::Init) {
    return thread->raiseWithFmt(
        LayoutId::kRuntimeError,
        "cannot throw into async generator via athrow iterator before send");
  }

  // Throw into generator
  AsyncGenerator generator(&scope, self.generator());
  Object exception_type(&scope, args.get(1));
  Object exception_value(&scope, args.get(2));
  Object exception_traceback(&scope, args.get(3));
  Object res(&scope, throwImpl(thread, generator, exception_type,
                               exception_value, exception_traceback));

  // Propagate any uncaught exceptions.
  if (res.isError()) return *res;

  // Generator-like yield: raise this in a StopIteration
  if (res.isAsyncGeneratorWrappedValue()) {
    AsyncGeneratorWrappedValue wrapped_value(&scope, *res);
    Object value(&scope, wrapped_value.value());
    return thread->raiseStopIterationWithValue(value);
  }

  // Async-like yield: pass result to the caller for propagation to the
  // event-loop.
  return *res;
}

RawObject METH(generator, __iter__)(Thread* thread, Arguments args) {
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

RawObject METH(generator, __next__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isGenerator()) {
    return thread->raiseRequiresType(self, ID(generator));
  }
  Generator gen(&scope, *self);
  Object value(&scope, NoneType::object());
  return Interpreter::resumeGenerator(thread, gen, value);
}

RawObject METH(generator, close)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isGenerator()) {
    return thread->raiseRequiresType(self, ID(generator));
  }
  GeneratorBase gen(&scope, *self);
  return closeImpl(thread, gen);
}

RawObject generatorSend(Thread* thread, const Object& self_obj,
                        const Object& value) {
  return sendImpl<ID(generator), LayoutId::kGenerator>(thread, *self_obj,
                                                       *value);
}
RawObject METH(generator, send)(Thread* thread, Arguments args) {
  return sendImpl<ID(generator), LayoutId::kGenerator>(thread, args.get(0),
                                                       args.get(1));
}

RawObject METH(generator, throw)(Thread* thread, Arguments args) {
  return throwImpl<ID(generator), LayoutId::kGenerator>(
      thread, args.get(0), args.get(1), args.get(2), args.get(3));
}

RawObject METH(coroutine, __await__)(Thread* thread, Arguments args) {
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

RawObject METH(coroutine, close)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isCoroutine()) {
    return thread->raiseRequiresType(self, ID(coroutine));
  }
  GeneratorBase gen(&scope, *self);
  return closeImpl(thread, gen);
}

RawObject coroutineSend(Thread* thread, const Object& self,
                        const Object& value) {
  return sendImpl<ID(coroutine), LayoutId::kCoroutine>(thread, *self, *value);
}

RawObject METH(coroutine, send)(Thread* thread, Arguments args) {
  return sendImpl<ID(coroutine), LayoutId::kCoroutine>(thread, args.get(0),
                                                       args.get(1));
}

RawObject METH(coroutine, throw)(Thread* thread, Arguments args) {
  return throwImpl<ID(coroutine), LayoutId::kCoroutine>(
      thread, args.get(0), args.get(1), args.get(2), args.get(3));
}

RawObject METH(coroutine_wrapper, __iter__)(Thread* thread, Arguments args) {
  RawObject self = args.get(0);
  if (self.isCoroutineWrapper()) return self;
  HandleScope scope(thread);
  Object self_obj(&scope, self);
  return thread->raiseRequiresType(self_obj, ID(coroutine));
}

RawObject METH(coroutine_wrapper, __next__)(Thread* thread, Arguments args) {
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

RawObject METH(coroutine_wrapper, close)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isCoroutineWrapper()) {
    return thread->raiseRequiresType(self_obj, ID(coroutine));
  }
  CoroutineWrapper self(&scope, *self_obj);
  GeneratorBase gen(&scope, self.coroutine());
  return closeImpl(thread, gen);
}

RawObject METH(coroutine_wrapper, send)(Thread* thread, Arguments args) {
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

RawObject METH(coroutine_wrapper, throw)(Thread* thread, Arguments args) {
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
                                                      Arguments args) {
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
                                                              Arguments args) {
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
