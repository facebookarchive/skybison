#include "sys-module.h"

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
  if (obj->isNoneType()) {
    return NoneType::object();
  }
  UNIMPLEMENTED("sys.displayhook()");
}

RawObject builtinSysExit(Thread* thread, Frame* frame, word nargs) {
  if (nargs > 1) {
    return thread->raiseTypeErrorWithCStr("exit() accepts at most 1 argument");
  }
  // TODO(T36407058): raise SystemExit instead of calling std::exit
  word code = 0;  // success
  if (nargs == 1) {
    Arguments args(frame, nargs);
    RawObject arg = args.get(0);
    if (!arg->isSmallInt()) {
      return thread->raiseTypeErrorWithCStr("exit() expects numeric argument");
    }
    code = RawSmallInt::cast(arg)->value();
  }

  std::exit(code);
}

RawObject initialSysPath(Thread* thread) {
  // TODO(T39319124) move this function into sys-module.cpp.
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  List result(&scope, runtime->newList());

  const char* python_path = getenv("PYTHONPATH");
  if (!python_path || python_path[0] == '\0') return *result;

  // TODO(T39226962): We should rewrite this in python so we have path
  // manipulation helpers available. Current limitations:
  // - Does not transform relative paths to absolute ones.
  // - Does not normalize paths.
  // - Does not filter out duplicate paths.
  Object path(&scope, NoneType::object());
  for (const char *c = python_path, *segment_begin = c;; ++c) {
    if (*c != '\0' && *c != ':') continue;
    const char* segment_end = c;

    if (segment_begin[0] != '/') {
      UNIMPLEMENTED("pathStringToList: Relative paths not supported yet\n");
    }

    View<byte> path_bytes(reinterpret_cast<const byte*>(segment_begin),
                          segment_end - segment_begin);
    path = runtime->newStrWithAll(path_bytes);
    runtime->listAdd(result, path);

    if (*c == '\0') break;
    segment_begin = c + 1;
  }
  return *result;
}

}  // namespace python
