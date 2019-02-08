#include "sys-module.h"

#include "builtins-module.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

RawObject builtinSysDisplayhook(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr(
        "displayhook() takes exactly one argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  if (obj.isNoneType()) {
    return NoneType::object();
  }
  UNIMPLEMENTED("sys.displayhook()");
}

RawObject initialSysPath(Thread* thread) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  List result(&scope, runtime->newList());
  Object empty_string(&scope, runtime->newStrWithAll(View<byte>(nullptr, 0)));
  runtime->listAdd(result, empty_string);

  const char* python_path = getenv("PYTHONPATH");
  if (!python_path || python_path[0] == '\0') return *result;

  // TODO(T39226962): We should rewrite this in python so we have path
  // manipulation helpers available. Current limitations:
  // - Does not transform relative paths to absolute ones.
  // - Does not normalize paths.
  // - Does not filter out duplicate paths.
  Object path(&scope, NoneType::object());
  const char* c = python_path;
  do {
    const char* segment_begin = c;
    // Advance to the next delimiter or end of string.
    while (*c != ':' && *c != '\0') {
      c++;
    }

    View<byte> path_bytes(reinterpret_cast<const byte*>(segment_begin),
                          c - segment_begin);
    CHECK(path_bytes.data()[0] == '/',
          "relative paths in PYTHONPATH not supported yet");
    path = runtime->newStrWithAll(path_bytes);
    runtime->listAdd(result, path);
  } while (*c++ != '\0');
  return *result;
}

}  // namespace python
