#include "test-utils.h"

#include <memory>
#include <sstream>

#include "builtins-module.h"
#include "frame.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"
#include "utils.h"

namespace python {
namespace testing {

// helper function to redirect return stdout from running
// a module
std::string compileAndRunToString(Runtime* runtime, const char* src) {
  std::stringstream tmp_stdout;
  std::ostream* saved_stdout = builtInStdout;
  builtInStdout = &tmp_stdout;
  Object* result = runtime->runFromCString(src);
  (void)result;
  CHECK(result == None::object(), "unexpected result");
  builtInStdout = saved_stdout;
  return tmp_stdout.str();
}

Object* findInModule(Runtime* runtime, const Handle<Module>& module,
                     const char* name) {
  HandleScope scope;
  Handle<Object> key(&scope, runtime->newStringFromCString(name));
  return runtime->moduleAt(module, key);
}

std::string callFunctionToString(const Handle<Function>& func,
                                 const Handle<ObjectArray>& args) {
  std::stringstream stream;
  std::ostream* old_stream = builtInStdout;
  builtInStdout = &stream;
  Thread* thread = Thread::currentThread();
  Frame* frame =
      thread->pushNativeFrame(Utils::castFnPtrToVoid(callFunctionToString), 0);
  callFunction(func, args);
  thread->popFrame();
  builtInStdout = old_stream;
  return stream.str();
}

Object* callFunction(const Handle<Function>& func,
                     const Handle<ObjectArray>& args) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Handle<Code> code(&scope, func->code());
  Frame* frame = thread->pushNativeFrame(Utils::castFnPtrToVoid(callFunction),
                                         args->length());
  frame->pushValue(*func);
  for (word i = 0; i < args->length(); i++) {
    frame->pushValue(args->at(i));
  }
  Handle<Object> result(&scope, func->entry()(thread, frame, code->argcount()));
  thread->popFrame();
  return *result;
}

bool objectArrayContains(const Handle<ObjectArray>& object_array,
                         const Handle<Object>& key) {
  for (word i = 0; i < object_array->length(); i++) {
    if (Object::equals(object_array->at(i), *key)) {
      return true;
    }
  }
  return false;
}

Object* findModule(Runtime* runtime, const char* name) {
  HandleScope scope;
  Handle<Object> key(&scope, runtime->newStringFromCString(name));
  return runtime->findModule(key);
}

Object* moduleAt(Runtime* runtime, const Handle<Module>& module,
                 const char* name) {
  HandleScope scope;
  Handle<Object> key(&scope, runtime->newStringFromCString(name));
  Handle<Object> value(&scope, runtime->moduleAt(module, key));
  return *value;
}

}  // namespace testing
}  // namespace python
