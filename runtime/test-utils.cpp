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
  Handle<Dictionary> dict(&scope, module->dictionary());
  Handle<Object> key(&scope, runtime->newStringFromCString(name));
  Handle<Object> value_cell(&scope, None::object());
  // TODO(T25495827) - Make returning an Error the default.
  if (!runtime->dictionaryAt(dict, key, value_cell.pointer())) {
    return Error::object();
  }
  return ValueCell::cast(*value_cell)->value();
}

std::string callFunctionToString(
    const Handle<Function>& func,
    const Handle<ObjectArray>& args) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread->handles());
  Handle<Code> code(&scope, func->code());
  Frame* frame = thread->openAndLinkFrame(
      0, 0, code->argcount() + 1, thread->initialFrame());
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

} // namespace testing
} // namespace python
