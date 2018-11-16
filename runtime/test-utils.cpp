#include "test-utils.h"

#include <memory>
#include <sstream>
#include "builtins.h"
#include "frame.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"

namespace python {
namespace testing {

// helper function to redirect return stdout from running
// a moodule
std::string runToString(Runtime* runtime, const char* buffer) {
  std::stringstream stream;
  std::ostream* oldStream = builtinPrintStream;
  builtinPrintStream = &stream;
  Object* result = runtime->run(buffer);
  (void)result;
  assert(result == None::object());
  builtinPrintStream = oldStream;
  return stream.str();
}

std::string compileAndRunToString(Runtime* runtime, const char* src) {
  std::unique_ptr<char[]> buffer(Runtime::compile(src));
  return runToString(runtime, buffer.get());
}

Object*
findInModule(Runtime* runtime, const Handle<Module>& module, const char* name) {
  HandleScope scope;
  Handle<Object> key(&scope, runtime->newStringFromCString(name));
  return runtime->moduleAt(module, key);
}

std::string callFunctionToString(
    const Handle<Function>& func,
    const Handle<ObjectArray>& args) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread->handles());
  Handle<Code> code(&scope, func->code());
  Frame* frame = thread->openAndLinkFrame(0, 0, code->argcount() + 1);
  Object** sp = frame->valueStackTop();
  *--sp = *func;
  for (word i = 0; i < args->length(); i++) {
    *--sp = args->at(i);
  }
  frame->setValueStackTop(sp);
  std::stringstream stream;
  std::ostream* oldStream = builtinPrintStream;
  builtinPrintStream = &stream;
  func->entry()(thread, frame, code->argcount());
  builtinPrintStream = oldStream;
  return stream.str();
}

bool objectArrayContains(
    const Handle<ObjectArray>& object_array,
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

} // namespace testing
} // namespace python
