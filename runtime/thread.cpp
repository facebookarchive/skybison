#include "thread.h"

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include "builtins-module.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "tuple-builtins.h"
#include "type-builtins.h"
#include "utils.h"
#include "visitor.h"

namespace python {

Handles::Handles() { scopes_.reserve(kInitialSize); }

void Handles::visitPointers(PointerVisitor* visitor) {
  for (auto const& scope : scopes_) {
    Handle<RawObject>* handle = scope->list();
    while (handle != nullptr) {
      visitor->visitPointer(handle->pointer());
      handle = handle->nextHandle();
    }
  }
}

thread_local Thread* Thread::current_thread_ = nullptr;

Thread::Thread(word size)
    : size_(Utils::roundUp(size, kPointerSize)),
      initialFrame_(nullptr),
      next_(nullptr),
      runtime_(nullptr),
      pending_exc_type_(NoneType::object()),
      pending_exc_value_(NoneType::object()),
      pending_exc_traceback_(NoneType::object()),
      caught_exc_stack_(NoneType::object()),
      api_repr_list_(NoneType::object()) {
  start_ = new byte[size];
  // Stack growns down in order to match machine convention
  end_ = start_ + size;
  pushInitialFrame();
}

Thread::~Thread() { delete[] start_; }

void Thread::visitRoots(PointerVisitor* visitor) {
  visitStackRoots(visitor);
  handles()->visitPointers(visitor);
  visitor->visitPointer(&pending_exc_type_);
  visitor->visitPointer(&pending_exc_value_);
  visitor->visitPointer(&pending_exc_traceback_);
  visitor->visitPointer(&caught_exc_stack_);
}

void Thread::visitStackRoots(PointerVisitor* visitor) {
  auto address = reinterpret_cast<uword>(stackPtr());
  auto end = reinterpret_cast<uword>(end_);
  for (; address < end; address += kPointerSize) {
    visitor->visitPointer(reinterpret_cast<RawObject*>(address));
  }
}

Thread* Thread::current() { return Thread::current_thread_; }

void Thread::setCurrentThread(Thread* thread) {
  Thread::current_thread_ = thread;
}

byte* Thread::stackPtr() {
  return reinterpret_cast<byte*>(currentFrame_->valueStackTop());
}

Frame* Thread::openAndLinkFrame(word num_args, word num_vars,
                                word stack_depth) {
  DCHECK(num_args >= 0, "must have 0 or more arguments");
  DCHECK(num_vars >= 0, "must have 0 or more locals");
  DCHECK(stack_depth >= 0, "stack depth cannot be negative");
  // HACK: Reserve one extra stack slot for the case where we need to unwrap a
  // bound method
  stack_depth += 1;

  checkStackOverflow(Frame::kSize + (num_vars + stack_depth) * kPointerSize);

  // Initialize the frame.
  byte* sp = stackPtr();
  word size = Frame::kSize + num_vars * kPointerSize;
  sp -= size;
  std::memset(sp, 0, size);
  auto frame = reinterpret_cast<Frame*>(sp);
  frame->setValueStackTop(reinterpret_cast<RawObject*>(frame));
  frame->setNumLocals(num_args + num_vars);

  // return a pointer to the base of the frame
  return linkFrame(frame);
}

Frame* Thread::linkFrame(Frame* frame) {
  frame->setPreviousFrame(currentFrame_);
  return currentFrame_ = frame;
}

void Thread::checkStackOverflow(word max_size) {
  // Check that there is sufficient space on the stack
  // TODO(T36407214): Grow stack
  byte* sp = stackPtr();
  CHECK(sp - max_size >= start_, "stack overflow");
}

Frame* Thread::pushNativeFrame(void* fn, word nargs) {
  // TODO(T36407290): native frames push arguments onto the stack when calling
  // back into the interpreter, but we can't statically know how much stack
  // space they will need. We may want to extend the api for such native calls
  // to include a declaration of how much space is needed. However, that's of
  // limited use right now since we can't detect an "overflow" of a frame
  // anyway.
  Frame* frame = openAndLinkFrame(nargs, 0, 0);
  frame->makeNativeFrame(runtime()->newIntFromCPtr(fn));
  return frame;
}

Frame* Thread::pushFrame(const Code& code, const Dict& globals,
                         const Dict& builtins) {
  Frame* frame =
      openAndLinkFrame(code.totalArgs(), code.totalVars(), code.stacksize());
  frame->setCode(*code);
  frame->setGlobals(*globals);
  frame->setBuiltins(*builtins);
  return frame;
}

Frame* Thread::pushCallFrame(const Function& function) {
  HandleScope scope(this);
  Dict globals(&scope, function.globals());
  Frame* current_frame = currentFrame();
  // If we stay in the same global namespace, use the same builtins.
  Object builtins(&scope, NoneType::object());
  if (globals == current_frame->globals()) {
    builtins = current_frame->builtins();
  } else {
    // Otherwise get the `__builtins__` module from the global namespace.
    // TODO(T36407403): Set builtins appropriately.
    Object dunder_builtins_name(&scope, runtime()->symbols()->DunderBuiltins());
    Object builtins_module(
        &scope, runtime()->moduleDictAt(this, globals, dunder_builtins_name));
    if (builtins_module.isModule()) {
      builtins = Module::cast(*builtins_module).dict();
    } else {
      // Create a minimal builtins dictionary with just `{'None': None}`.
      builtins = runtime()->newDict();
      Object none_name(&scope, runtime()->symbols()->None());
      Object none(&scope, NoneType::object());
      Dict builtins_dict(&scope, *builtins);
      runtime()->moduleDictAtPut(this, builtins_dict, none_name, none);
    }
  }
  Code code(&scope, function.code());
  Dict builtins_dict(&scope, *builtins);
  Frame* result = pushFrame(code, globals, builtins_dict);
  result->setVirtualPC(0);
  return result;
}

Frame* Thread::pushExecFrame(const Code& code, const Dict& globals,
                             const Object& locals) {
  HandleScope scope(this);
  Str dunder_builtins_name(&scope, runtime()->symbols()->DunderBuiltins());
  Object builtins_obj(
      &scope, runtime()->typeDictAt(this, globals, dunder_builtins_name));
  if (builtins_obj.isError()) {  // couldn't find __builtins__ in globals
    builtins_obj = runtime()->findModuleById(SymbolId::kBuiltins);
    runtime()->typeDictAtPut(this, globals, dunder_builtins_name, builtins_obj);
  }
  Module builtins_module(&scope, *builtins_obj);
  Dict builtins(&scope, builtins_module.dict());
  Frame* result = pushFrame(code, globals, builtins);
  result->setFastGlobals(
      runtime()->computeFastGlobals(code, globals, builtins));
  result->setImplicitGlobals(*locals);
  return result;
}

Frame* Thread::pushClassFunctionFrame(const Function& function,
                                      const Dict& dict) {
  HandleScope scope(this);
  Frame* result = pushCallFrame(function);
  Code code(&scope, function.code());
  Dict globals(&scope, result->globals());
  Dict builtins(&scope, result->builtins());
  result->setFastGlobals(
      runtime()->computeFastGlobals(code, globals, builtins));
  result->setImplicitGlobals(*dict);

  word num_locals = code.nlocals();
  word num_cellvars = code.numCellvars();
  DCHECK(code.cell2arg().isNoneType(), "class body cannot have cell2arg.");
  for (word i = 0; i < code.numCellvars(); i++) {
    result->setLocal(num_locals + i, runtime()->newValueCell());
  }

  // initialize free vars
  DCHECK(code.numFreevars() == 0 ||
             code.numFreevars() ==
                 Tuple::cast(Function::cast(*function).closure()).length(),
         "Number of freevars is different than the closure.");
  for (word i = 0; i < code.numFreevars(); i++) {
    result->setLocal(num_locals + num_cellvars + i,
                     Tuple::cast(Function::cast(*function).closure()).at(i));
  }
  return result;
}

void Thread::pushInitialFrame() {
  DCHECK(initialFrame_ == nullptr, "initial frame already pushed");

  byte* sp = end_ - Frame::kSize;
  CHECK(sp > start_, "no space for initial frame");
  initialFrame_ = reinterpret_cast<Frame*>(sp);
  initialFrame_->makeSentinel();
  initialFrame_->setValueStackTop(reinterpret_cast<RawObject*>(sp));

  currentFrame_ = initialFrame_;
}

void Thread::popFrame() {
  Frame* frame = currentFrame_;
  DCHECK(!frame->isSentinelFrame(), "cannot pop initial frame");
  currentFrame_ = frame->previousFrame();
}

RawObject Thread::run(const Code& code) {
  DCHECK(currentFrame_ == initialFrame_, "thread must be inactive");
  HandleScope scope(this);
  Dict globals(&scope, runtime()->newDict());
  Dict builtins(&scope, runtime()->newDict());
  Frame* frame = pushFrame(code, globals, builtins);
  return Interpreter::execute(this, frame);
}

RawObject Thread::exec(const Code& code, const Dict& globals,
                       const Object& locals) {
  Frame* frame = pushExecFrame(code, globals, locals);
  return Interpreter::execute(this, frame);
}

RawObject Thread::runClassFunction(const Function& function, const Dict& dict) {
  Frame* frame = pushClassFunctionFrame(function, dict);
  return Interpreter::execute(this, frame);
}

RawObject Thread::invokeMethod1(const Object& receiver, SymbolId selector) {
  HandleScope scope(this);
  Object method(&scope, Interpreter::lookupMethod(this, currentFrame_, receiver,
                                                  selector));
  if (method.isError()) return *method;
  return Interpreter::callMethod1(this, currentFrame_, method, receiver);
}

RawObject Thread::invokeMethod2(const Object& receiver, SymbolId selector,
                                const Object& arg1) {
  HandleScope scope(this);
  Object method(&scope, Interpreter::lookupMethod(this, currentFrame_, receiver,
                                                  selector));
  if (method.isError()) return *method;
  return Interpreter::callMethod2(this, currentFrame_, method, receiver, arg1);
}

RawObject Thread::invokeMethod3(const Object& receiver, SymbolId selector,
                                const Object& arg1, const Object& arg2) {
  HandleScope scope(this);
  Object method(&scope, Interpreter::lookupMethod(this, currentFrame_, receiver,
                                                  selector));
  if (method.isError()) return *method;
  return Interpreter::callMethod3(this, currentFrame_, method, receiver, arg1,
                                  arg2);
}

RawObject Thread::invokeMethodStatic2(LayoutId type, SymbolId method_name,
                                      const Object& arg1, const Object& arg2) {
  HandleScope scope(this);
  Object type_obj(&scope, runtime()->typeAt(type));
  if (type_obj.isError()) return *type_obj;
  Type type_handle(&scope, *type_obj);
  Object method(&scope, typeLookupSymbolInMro(this, type_handle, method_name));
  if (method.isError()) return *method;
  return Interpreter::callMethod2(this, currentFrame_, method, arg1, arg2);
}

RawObject Thread::invokeMethodStatic3(LayoutId type, SymbolId method_name,
                                      const Object& arg1, const Object& arg2,
                                      const Object& arg3) {
  HandleScope scope(this);
  Object type_obj(&scope, runtime()->typeAt(type));
  if (type_obj.isError()) return *type_obj;
  Type type_handle(&scope, *type_obj);
  Object method(&scope, typeLookupSymbolInMro(this, type_handle, method_name));
  if (method.isError()) return *method;
  return Interpreter::callMethod3(this, currentFrame_, method, arg1, arg2,
                                  arg3);
}

RawObject Thread::invokeMethodStatic4(LayoutId type, SymbolId method_name,
                                      const Object& receiver,
                                      const Object& arg1, const Object& arg2,
                                      const Object& arg3) {
  HandleScope scope(this);
  Object type_obj(&scope, runtime()->typeAt(type));
  if (type_obj.isError()) return *type_obj;
  Type type_handle(&scope, *type_obj);
  Object method(&scope, typeLookupSymbolInMro(this, type_handle, method_name));
  if (method.isError()) return *method;
  return Interpreter::callMethod4(this, currentFrame_, method, receiver, arg1,
                                  arg2, arg3);
}

RawObject Thread::invokeFunction1(SymbolId module, SymbolId name,
                                  const Object& arg1) {
  HandleScope scope(this);
  Object func(&scope, runtime()->lookupNameInModule(this, module, name));
  if (func.isError()) return *func;
  return Interpreter::callFunction1(this, currentFrame_, func, arg1);
}

RawObject Thread::invokeFunction2(SymbolId module, SymbolId name,
                                  const Object& arg1, const Object& arg2) {
  HandleScope scope(this);
  Object func(&scope, runtime()->lookupNameInModule(this, module, name));
  if (func.isError()) return *func;
  return Interpreter::callFunction2(this, currentFrame_, func, arg1, arg2);
}

RawObject Thread::invokeFunction3(SymbolId module, SymbolId name,
                                  const Object& arg1, const Object& arg2,
                                  const Object& arg3) {
  HandleScope scope(this);
  Object func(&scope, runtime()->lookupNameInModule(this, module, name));
  if (func.isError()) return *func;
  return Interpreter::callFunction3(this, currentFrame_, func, arg1, arg2,
                                    arg3);
}

RawObject Thread::invokeFunction4(SymbolId module, SymbolId name,
                                  const Object& arg1, const Object& arg2,
                                  const Object& arg3, const Object& arg4) {
  HandleScope scope(this);
  Object func(&scope, runtime()->lookupNameInModule(this, module, name));
  if (func.isError()) return *func;
  return Interpreter::callFunction4(this, currentFrame_, func, arg1, arg2, arg3,
                                    arg4);
}

RawObject Thread::invokeFunction5(SymbolId module, SymbolId name,
                                  const Object& arg1, const Object& arg2,
                                  const Object& arg3, const Object& arg4,
                                  const Object& arg5) {
  HandleScope scope(this);
  Object func(&scope, runtime()->lookupNameInModule(this, module, name));
  if (func.isError()) return *func;
  return Interpreter::callFunction5(this, currentFrame_, func, arg1, arg2, arg3,
                                    arg4, arg5);
}

RawObject Thread::raise(LayoutId type, RawObject value) {
  DCHECK(!hasPendingException(), "unhandled exception lingering");
  setPendingExceptionType(runtime()->typeAt(type));
  setPendingExceptionValue(value);
  setPendingExceptionTraceback(NoneType::object());
  return Error::exception();
}

RawObject Thread::raiseWithFmt(LayoutId type, const char* fmt, ...) {
  va_list args;
  va_start(args, fmt);
  HandleScope scope(this);
  Object message(&scope, runtime()->newStrFromFmtV(this, fmt, args));
  va_end(args);
  return raise(type, *message);
}

RawObject Thread::raiseWithId(LayoutId type, SymbolId msg) {
  return raise(type, runtime()->symbols()->at(msg));
}

// Convenience method for throwing a binary-operation-specific TypeError
// exception with an error message.
RawObject Thread::raiseUnsupportedBinaryOperation(
    const Handle<RawObject>& left, const Handle<RawObject>& right,
    const Handle<RawStr>& op_name) {
  return raiseWithFmt(LayoutId::kTypeError, "%T.%S(%T) is not supported", &left,
                      &op_name, &right);
}

void Thread::raiseBadArgument() {
  raiseWithId(LayoutId::kTypeError,
              SymbolId::kBadArgumentTypeForBuiltinOperation);
}

void Thread::raiseBadInternalCall() {
  raiseWithId(LayoutId::kSystemError, SymbolId::kBadArgumentToInternalFunction);
}

RawObject Thread::raiseMemoryError() {
  return raise(LayoutId::kMemoryError, NoneType::object());
}

RawObject Thread::raiseRequiresType(const Object& obj, SymbolId expected_type) {
  HandleScope scope(this);
  Function function(&scope, currentFrame()->function());
  Str function_name(&scope, function.name());
  return raiseWithFmt(LayoutId::kTypeError,
                      "'%S' requires a '%Y' object but got '%T'",
                      &function_name, expected_type, &obj);
}

bool Thread::hasPendingException() { return !pending_exc_type_.isNoneType(); }

bool Thread::hasPendingStopIteration() {
  return pending_exc_type_.isType() &&
         Type::cast(pending_exc_type_).builtinBase() ==
             LayoutId::kStopIteration;
}

bool Thread::clearPendingStopIteration() {
  if (hasPendingStopIteration()) {
    clearPendingException();
    return true;
  }
  return false;
}

RawObject Thread::pendingStopIterationValue() {
  DCHECK(hasPendingStopIteration(),
         "Shouldn't be called without a pending StopIteration");

  HandleScope scope(this);
  Object exc_value(&scope, pendingExceptionValue());
  if (runtime()->isInstanceOfStopIteration(*exc_value)) {
    StopIteration si(&scope, *exc_value);
    return si.value();
  }
  if (runtime()->isInstanceOfTuple(*exc_value)) {
    Tuple tuple(&scope, tupleUnderlying(this, exc_value));
    return tuple.at(0);
  }
  return *exc_value;
}

void Thread::ignorePendingException() {
  if (!hasPendingException()) {
    return;
  }
  fprintf(stderr, "ignore pending exception");
  if (pendingExceptionValue().isStr()) {
    RawStr message = Str::cast(pendingExceptionValue());
    word len = message.length();
    byte* buffer = new byte[len + 1];
    message.copyTo(buffer, len);
    buffer[len] = 0;
    fprintf(stderr, ": %s", buffer);
    delete[] buffer;
  }
  fprintf(stderr, "\n");
  clearPendingException();
  Utils::printTraceback(stderr);
}

void Thread::clearPendingException() {
  setPendingExceptionType(NoneType::object());
  setPendingExceptionValue(NoneType::object());
  setPendingExceptionTraceback(NoneType::object());
}

bool Thread::pendingExceptionMatches(LayoutId type) {
  HandleScope scope(this);
  Type exc(&scope, pendingExceptionType());
  Type parent(&scope, runtime()->typeAt(type));
  return runtime()->isSubclass(exc, parent);
}

bool Thread::hasCaughtException() {
  return !caughtExceptionType().isNoneType();
}

RawObject Thread::caughtExceptionType() {
  return ExceptionState::cast(caught_exc_stack_).type();
}

RawObject Thread::caughtExceptionValue() {
  return ExceptionState::cast(caught_exc_stack_).value();
}

RawObject Thread::caughtExceptionTraceback() {
  return ExceptionState::cast(caught_exc_stack_).traceback();
}

void Thread::setCaughtExceptionType(RawObject type) {
  ExceptionState::cast(caught_exc_stack_).setType(type);
}

void Thread::setCaughtExceptionValue(RawObject value) {
  ExceptionState::cast(caught_exc_stack_).setValue(value);
}

void Thread::setCaughtExceptionTraceback(RawObject tb) {
  ExceptionState::cast(caught_exc_stack_).setTraceback(tb);
}

RawObject Thread::caughtExceptionState() { return caught_exc_stack_; }

void Thread::setCaughtExceptionState(RawObject state) {
  caught_exc_stack_ = state;
}

bool Thread::isErrorValueOk(RawObject obj) {
  return (!obj.isError() && !hasPendingException()) ||
         (obj.isErrorException() && hasPendingException());
}

void Thread::visitFrames(FrameVisitor* visitor) {
  Frame* frame = currentFrame();
  while (!frame->isSentinelFrame()) {
    if (!visitor->visit(frame)) {
      break;
    }
    frame = frame->previousFrame();
  }
}

RawObject Thread::reprEnter(const Object& obj) {
  HandleScope scope(this);
  if (api_repr_list_.isNoneType()) {
    api_repr_list_ = runtime_->newList();
  }
  List list(&scope, api_repr_list_);
  for (word i = list.numItems() - 1; i >= 0; i--) {
    if (list.at(i).raw() == obj.raw()) {
      return RawBool::trueObj();
    }
  }
  // TODO(emacs): When there is better error handling, raise an exception.
  runtime_->listAdd(list, obj);
  return RawBool::falseObj();
}

void Thread::reprLeave(const Object& obj) {
  HandleScope scope(this);
  List list(&scope, api_repr_list_);
  for (word i = list.numItems() - 1; i >= 0; i--) {
    if (list.at(i).raw() == obj.raw()) {
      list.atPut(i, Unbound::object());
      break;
    }
  }
}

int Thread::recursionLimit() { return recursion_limit_; }

void Thread::setRecursionLimit(int /* limit */) {
  UNIMPLEMENTED("Thread::setRecursionLimit");
}

}  // namespace python
