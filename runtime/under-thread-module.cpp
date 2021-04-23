#include "builtins.h"
#include "exception-builtins.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "modules.h"
#include "os.h"
#include "runtime.h"
#include "sys-module.h"
#include "thread.h"

namespace py {

RawObject FUNC(_thread, get_ident)(Thread* thread, Arguments) {
  return thread->runtime()->newIntFromCPtr(thread);
}

static void* threadBegin(void* arg) {
  Thread* thread = reinterpret_cast<Thread*>(arg);
  thread->begin();
  {
    HandleScope scope(thread);
    Object func(&scope, thread->stackPeek(2));
    Object result(
        &scope, Interpreter::callEx(thread, CallFunctionExFlag::VAR_KEYWORDS));
    if (result.isErrorException()) {
      if (thread->pendingExceptionMatches(LayoutId::kSystemExit)) {
        thread->clearPendingException();
      } else {
        // TODO(T89490118): call sys.unraisablehook instead
        Object exc(&scope, thread->pendingExceptionType());
        Object val(&scope, thread->pendingExceptionValue());
        Object tb(&scope, thread->pendingExceptionTraceback());
        thread->clearPendingException();

        Object func_str(&scope,
                        thread->invokeFunction1(ID(builtins), ID(str), func));
        if (func_str.isErrorException()) {
          writeStderr(thread, "Exception ignored in thread started by: \n");
        } else {
          Str str(&scope, strUnderlying(*func_str));
          word length = str.length();
          unique_c_ptr<byte[]> buf(
              reinterpret_cast<byte*>(std::malloc(length + 1)));
          str.copyTo(buf.get(), length);
          buf[length] = '\0';
          writeStderr(thread, "Exception ignored in thread started by: %s\n",
                      buf.get());
        }

        thread->setPendingExceptionType(*exc);
        thread->setPendingExceptionValue(*val);
        thread->setPendingExceptionTraceback(*tb);
        printPendingException(thread);
      }
    }
  }
  thread->runtime()->deleteThread(thread);
  return nullptr;
}

static RawObject startNewThread(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  // TODO(T66337218): remove this guard once threading is safe
  if (runtime->lookupNameInModule(thread, ID(_thread), ID(_enable_threads))
          .isErrorNotFound()) {
    UNIMPLEMENTED(
        "PyRO runtime is not thread-safe. "
        "Set `_thread._enable_threads = True` to bypass.");
  }
  Object func(&scope, args.get(0));
  if (!runtime->isCallable(thread, func)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "first arg must be callable");
  }
  Object args_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfTuple(*args_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "2nd arg must be a tuple");
  }
  Object kwargs(&scope, args.get(2));
  if (kwargs.isUnbound()) {
    kwargs = runtime->newDict();
  } else if (!runtime->isInstanceOfDict(*kwargs)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "optional 3rd arg must be a dictionary");
  }
  Thread* new_thread = runtime->newThread();
  new_thread->stackPush(*func);
  new_thread->stackPush(*args_obj);
  new_thread->stackPush(*kwargs);
  OS::createThread(threadBegin, new_thread);
  return runtime->newIntFromCPtr(new_thread);
}

RawObject FUNC(_thread, start_new)(Thread* thread, Arguments args) {
  return startNewThread(thread, args);
}

RawObject FUNC(_thread, start_new_thread)(Thread* thread, Arguments args) {
  return startNewThread(thread, args);
}

}  // namespace py
