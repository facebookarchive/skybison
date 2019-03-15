#include "thread.h"

#include <cstdio>
#include <cstring>

#include "builtins-module.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
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

Thread* Thread::currentThread() { return Thread::current_thread_; }

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

Frame* Thread::pushFrame(const Code& code) {
  Frame* frame =
      openAndLinkFrame(code.totalArgs(), code.totalVars(), code.stacksize());
  frame->setCode(*code);
  return frame;
}

Frame* Thread::pushExecFrame(const Code& code, const Dict& globals,
                             const Object& locals) {
  HandleScope scope(this);
  Str dunder_builtins_name(&scope, runtime()->symbols()->DunderBuiltins());
  Object builtins_obj(&scope,
                      runtime()->typeDictAt(globals, dunder_builtins_name));
  // TODO(T36914735): __builtins__ should always be in globals.
  if (builtins_obj.isError()) {  // couldn't find __builtins__ in globals
    Str builtins_name(&scope, runtime()->symbols()->Builtins());
    builtins_obj = runtime()->findModule(builtins_name);
  }
  Module builtins(&scope, *builtins_obj);
  Dict builtins_dict(&scope, builtins.dict());
  Frame* result = pushFrame(code);
  result->setGlobals(*globals);
  result->setBuiltins(*builtins_dict);
  result->setFastGlobals(
      runtime()->computeFastGlobals(code, globals, builtins_dict));
  result->setImplicitGlobals(*locals);
  return result;
}

Frame* Thread::pushModuleFunctionFrame(const Module& module, const Code& code) {
  HandleScope scope(this);
  Dict globals(&scope, module.dict());
  return pushExecFrame(code, globals, globals);
}

Frame* Thread::pushClassFunctionFrame(const Function& function,
                                      const Dict& dict) {
  HandleScope scope(this);
  Code code(&scope, RawFunction::cast(*function).code());
  Dict globals(&scope, RawFunction::cast(*function).globals());
  Frame* result = pushExecFrame(code, globals, dict);

  word num_locals = code.nlocals();
  word num_cellvars = code.numCellvars();
  DCHECK(code.cell2arg().isNoneType(), "class body cannot have cell2arg.");
  for (word i = 0; i < code.numCellvars(); i++) {
    result->setLocal(num_locals + i, runtime()->newValueCell());
  }

  // initialize free vars
  DCHECK(
      code.numFreevars() == 0 ||
          code.numFreevars() ==
              RawTuple::cast(RawFunction::cast(*function).closure()).length(),
      "Number of freevars is different than the closure.");
  for (word i = 0; i < code.numFreevars(); i++) {
    result->setLocal(
        num_locals + num_cellvars + i,
        RawTuple::cast(RawFunction::cast(*function).closure()).at(i));
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
  Frame* frame = pushFrame(code);
  return Interpreter::execute(this, frame);
}

RawObject Thread::exec(const Code& code, const Dict& globals,
                       const Object& locals) {
  Frame* frame = pushExecFrame(code, globals, locals);
  return Interpreter::execute(this, frame);
}

RawObject Thread::runModuleFunction(const Module& module, const Code& code) {
  Frame* frame = pushModuleFunctionFrame(module, code);
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
  Object method(&scope,
                runtime()->lookupSymbolInMro(this, type_handle, method_name));
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
  Object method(&scope,
                runtime()->lookupSymbolInMro(this, type_handle, method_name));
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
  Object method(&scope,
                runtime()->lookupSymbolInMro(this, type_handle, method_name));
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
  setPendingExceptionType(runtime()->typeAt(type));
  setPendingExceptionValue(value);
  setPendingExceptionTraceback(NoneType::object());
  return Error::object();
}

RawObject Thread::raiseWithCStr(LayoutId type, const char* message) {
  return raise(type, runtime()->newStrFromCStr(message));
}

// Convenience method for throwing a RuntimeError exception with an error
// message.
RawObject Thread::raiseRuntimeError(RawObject value) {
  return raise(LayoutId::kRuntimeError, value);
}

RawObject Thread::raiseRuntimeErrorWithCStr(const char* message) {
  return raiseRuntimeError(runtime()->newStrFromCStr(message));
}

// Convenience method for throwing a SystemError exception with an error
// message.
RawObject Thread::raiseSystemError(RawObject value) {
  return raise(LayoutId::kSystemError, value);
}

RawObject Thread::raiseSystemErrorWithCStr(const char* message) {
  return raiseSystemError(runtime()->newStrFromCStr(message));
}

// Convenience method for throwing a TypeError exception with an error message.
RawObject Thread::raiseTypeError(RawObject value) {
  return raise(LayoutId::kTypeError, value);
}

RawObject Thread::raiseTypeErrorWithCStr(const char* message) {
  return raiseTypeError(runtime()->newStrFromCStr(message));
}

// Convenience method for throwing a binary-operation-specific TypeError
// exception with an error message.
RawObject Thread::raiseUnsupportedBinaryOperation(RawObject /* left */,
                                                  RawObject /* right */,
                                                  RawStr op_name) {
  unique_c_ptr<char> op_name_str(op_name.toCStr());
  // TODO(T39041197): Pull out the types of left, right, and format them into
  // the message
  return raiseTypeError(
      runtime()->newStrFromFormat("'%s' is not supported", op_name_str.get()));
}

// Convenience method for throwing a ValueError exception with an error message.
RawObject Thread::raiseValueError(RawObject value) {
  return raise(LayoutId::kValueError, value);
}

RawObject Thread::raiseValueErrorWithCStr(const char* message) {
  return raiseValueError(runtime()->newStrFromCStr(message));
}

RawObject Thread::raiseAttributeError(RawObject value) {
  return raise(LayoutId::kAttributeError, value);
}

RawObject Thread::raiseAttributeErrorWithCStr(const char* message) {
  return raiseAttributeError(runtime()->newStrFromCStr(message));
}

void Thread::raiseBadArgument() {
  raiseTypeErrorWithCStr("bad argument type for built-in operation");
}

void Thread::raiseBadInternalCall() {
  raiseSystemErrorWithCStr("bad argument to internal function");
}

RawObject Thread::raiseBufferError(RawObject value) {
  return raise(LayoutId::kBufferError, value);
}

RawObject Thread::raiseBufferErrorWithCStr(const char* message) {
  return raiseBufferError(runtime()->newStrFromCStr(message));
}

RawObject Thread::raiseKeyError(RawObject value) {
  return raise(LayoutId::kKeyError, value);
}

RawObject Thread::raiseKeyErrorWithCStr(const char* message) {
  return raiseKeyError(runtime()->newStrFromCStr(message));
}

RawObject Thread::raiseMemoryError() {
  return raise(LayoutId::kMemoryError, NoneType::object());
}

RawObject Thread::raiseOverflowError(RawObject value) {
  return raise(LayoutId::kOverflowError, value);
}

RawObject Thread::raiseOverflowErrorWithCStr(const char* message) {
  return raiseOverflowError(runtime()->newStrFromCStr(message));
}

RawObject Thread::raiseIndexError(RawObject value) {
  return raise(LayoutId::kIndexError, value);
}

RawObject Thread::raiseIndexErrorWithCStr(const char* message) {
  return raiseIndexError(runtime()->newStrFromCStr(message));
}

RawObject Thread::raiseStopIteration(RawObject value) {
  return raise(LayoutId::kStopIteration, value);
}

RawObject Thread::raiseZeroDivisionError(RawObject value) {
  return raise(LayoutId::kZeroDivisionError, value);
}

RawObject Thread::raiseZeroDivisionErrorWithCStr(const char* message) {
  return raiseZeroDivisionError(runtime()->newStrFromCStr(message));
}

bool Thread::hasPendingException() { return !pending_exc_type_.isNoneType(); }

bool Thread::hasPendingStopIteration() {
  return pending_exc_type_.isType() &&
         RawType::cast(pending_exc_type_).builtinBase() ==
             LayoutId::kStopIteration;
}

bool Thread::clearPendingStopIteration() {
  if (hasPendingStopIteration()) {
    clearPendingException();
    return true;
  }
  return false;
}

void Thread::ignorePendingException() {
  if (!hasPendingException()) {
    return;
  }
  fprintf(stderr, "ignore pending exception");
  if (pendingExceptionValue().isStr()) {
    RawStr message = RawStr::cast(pendingExceptionValue());
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

void Thread::printPendingException() {
  DCHECK(hasPendingException(), "No pending exception");
  HandleScope scope(this);
  Type exc_type(&scope, pendingExceptionType());
  Object exc_obj(&scope, pendingExceptionValue());
  clearPendingException();
  Str type_name(&scope, exc_type.name());
  std::cerr << unique_c_ptr<char[]>(type_name.toCStr()).get() << ": ";

  if (runtime()->isInstanceOfStr(*exc_obj)) {
    Str str(&scope, *exc_obj);
    std::cerr << unique_c_ptr<char[]>(str.toCStr()).get() << "\n";
    return;
  }

  if (runtime()->isInstanceOfBaseException(*exc_obj)) {
    Object dunder_str(&scope,
                      Interpreter::lookupMethod(this, currentFrame(), exc_obj,
                                                SymbolId::kDunderStr));
    Object result(&scope, Interpreter::callMethod1(this, currentFrame(),
                                                   dunder_str, exc_obj));
    if (result.isError() || !runtime()->isInstanceOfStr(*result)) {
      std::cerr << "<exception str() failed>\n";
    } else {
      Str result_str(&scope, *result);
      std::cerr << unique_c_ptr<char[]>(result_str.toCStr()).get() << "\n";
    }
    return;
  }

  UNIMPLEMENTED("unknown exception state");
}

void Thread::clearPendingException() {
  setPendingExceptionType(NoneType::object());
  setPendingExceptionValue(NoneType::object());
  setPendingExceptionTraceback(NoneType::object());
}

bool Thread::hasCaughtException() {
  return !caughtExceptionType().isNoneType();
}

RawObject Thread::caughtExceptionType() {
  return RawExceptionState::cast(caught_exc_stack_).type();
}

RawObject Thread::caughtExceptionValue() {
  return RawExceptionState::cast(caught_exc_stack_).value();
}

RawObject Thread::caughtExceptionTraceback() {
  return RawExceptionState::cast(caught_exc_stack_).traceback();
}

void Thread::setCaughtExceptionType(RawObject type) {
  RawExceptionState::cast(caught_exc_stack_).setType(type);
}

void Thread::setCaughtExceptionValue(RawObject value) {
  RawExceptionState::cast(caught_exc_stack_).setValue(value);
}

void Thread::setCaughtExceptionTraceback(RawObject tb) {
  RawExceptionState::cast(caught_exc_stack_).setTraceback(tb);
}

RawObject Thread::caughtExceptionState() { return caught_exc_stack_; }

void Thread::setCaughtExceptionState(RawObject state) {
  caught_exc_stack_ = state;
}

void Thread::visitFrames(FrameVisitor* visitor) {
  Frame* frame = currentFrame();
  while (!frame->isSentinelFrame()) {
    visitor->visit(frame);
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
      list.atPut(i, runtime_->unboundValue());
      break;
    }
  }
}

int Thread::recursionLimit() { return recursion_limit_; }

void Thread::setRecursionLimit(int /* limit */) {
  UNIMPLEMENTED("Thread::setRecursionLimit");
}

}  // namespace python
