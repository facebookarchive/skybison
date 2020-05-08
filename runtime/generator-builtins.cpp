#include "generator-builtins.h"

#include <string>

#include "builtins.h"
#include "exception-builtins.h"
#include "frame.h"
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

// If the given GeneratorBase is suspended at a YIELD_FROM instruction, return
// its subiterator. Otherwise, return None.
static RawObject findYieldFrom(Thread* thread, const GeneratorBase& gen) {
  HandleScope scope(thread);
  if (gen.running() == Bool::trueObj()) return NoneType::object();
  GeneratorFrame gf(&scope, gen.generatorFrame());
  word pc = gf.virtualPC();
  if (pc == Frame::kFinishedGeneratorPC) return NoneType::object();
  Function function(&scope, gf.function());
  MutableBytes bytecode(&scope, function.rewrittenBytecode());
  if (bytecode.byteAt(pc) != Bytecode::YIELD_FROM) return NoneType::object();
  return gf.valueStackTop()[0];
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

  return Interpreter::resumeGeneratorWithRaise(thread, gen, exc, value, tb);
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

  Object throw_obj(&scope, runtime->attributeAtById(thread, yf, ID(throw)));
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

template <SymbolId name, LayoutId type>
static RawObject genThrowImpl(Thread* thread, Frame* frame, word nargs) {
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

static const BuiltinAttribute kGeneratorAttributes[] = {
    {ID(_generator__frame), RawGenerator::kFrameOffset,
     AttributeFlags::kHidden},
    {ID(_generator__exception_state), RawGenerator::kExceptionStateOffset,
     AttributeFlags::kHidden},
    {ID(_generator__exception_state), RawGenerator::kExceptionStateOffset,
     AttributeFlags::kHidden},
    {ID(gi_running), RawGenerator::kRunningOffset, AttributeFlags::kReadOnly},
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
    {ID(__qualname__), RawCoroutine::kQualnameOffset},
    {ID(_coroutine__await), RawCoroutine::kAwaitOffset,
     AttributeFlags::kHidden},
    {ID(_coroutine__origin), RawCoroutine::kOriginOffset,
     AttributeFlags::kHidden},
};

static const BuiltinAttribute kAsyncGeneratorAttributes[] = {
    {ID(_async_generator__frame), RawAsyncGenerator::kFrameOffset,
     AttributeFlags::kHidden},
    {ID(_async_generator__exception_state),
     RawAsyncGenerator::kExceptionStateOffset, AttributeFlags::kHidden},
    {ID(_async_generator__running), RawAsyncGenerator::kRunningOffset,
     AttributeFlags::kHidden},
    {ID(__qualname__), RawAsyncGenerator::kQualnameOffset,
     AttributeFlags::kReadOnly},
    {ID(_async_generator__finalizer), RawAsyncGenerator::kFinalizerOffset,
     AttributeFlags::kHidden},
    {ID(_async_generator__hooks_inited), RawAsyncGenerator::kHooksInitedOffset,
     AttributeFlags::kHidden},
    {ID(_async_generator__closed), RawAsyncGenerator::kClosedOffset,
     AttributeFlags::kHidden},
};

void initializeGeneratorTypes(Thread* thread) {
  addBuiltinType(thread, ID(generator), LayoutId::kGenerator,
                 /*superclass_id=*/LayoutId::kObject, kGeneratorAttributes);

  addBuiltinType(thread, ID(coroutine), LayoutId::kCoroutine,
                 /*superclass_id=*/LayoutId::kObject, kCoroutineAttributes);

  addBuiltinType(thread, ID(async_generator), LayoutId::kAsyncGenerator,
                 /*superclass_id=*/LayoutId::kObject,
                 kAsyncGeneratorAttributes);
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

RawObject METH(generator, send)(Thread* thread, Frame* frame, word nargs) {
  return sendImpl<ID(generator), LayoutId::kGenerator>(thread, frame, nargs);
}

RawObject METH(generator, throw)(Thread* thread, Frame* frame, word nargs) {
  return genThrowImpl<ID(generator), LayoutId::kGenerator>(thread, frame,
                                                           nargs);
}

RawObject METH(coroutine, send)(Thread* thread, Frame* frame, word nargs) {
  return sendImpl<ID(coroutine), LayoutId::kCoroutine>(thread, frame, nargs);
}

}  // namespace py
