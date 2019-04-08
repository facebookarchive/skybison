#include "file.h"

#include <unistd.h>
#include <iostream>

#include "builtins-module.h"
#include "runtime.h"
#include "sys-module.h"
#include "thread.h"
#include "utils.h"

namespace python {

std::ostream* builtinStdout = &std::cout;
std::ostream* builtinStderr = &std::cerr;

static RawObject fileWriteObjectImpl(Thread* thread, const Object& file,
                                     const Object& obj, bool use_str) {
  // TODO(T41323917): Support actual streams.
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  int fileno;
  if (file.isSmallInt()) {
    fileno = RawSmallInt::cast(*file).value();
  } else {
    Object under_fd_name(&scope, runtime->newStrFromCStr("_fd"));
    Object under_fd(&scope, runtime->attributeAt(thread, file, under_fd_name));
    if (!under_fd.isSmallInt()) {
      UNIMPLEMENTED("unexpected stream");
    }
    int fd = RawSmallInt::cast(*under_fd).value();
    if (fd == kStdoutFd) {
      fileno = STDOUT_FILENO;
    } else if (fd == kStderrFd) {
      fileno = STDERR_FILENO;
    } else {
      UNIMPLEMENTED("unknown fd");
    }
  }
  if (fileno != STDOUT_FILENO && fileno != STDERR_FILENO) {
    UNIMPLEMENTED("non stdout/stderr fileno");
  }

  Object obj_converted(&scope, *obj);
  if (!use_str || !obj_converted.isStr()) {
    SymbolId func = use_str ? SymbolId::kStr : SymbolId::kRepr;
    obj_converted = thread->invokeFunction1(SymbolId::kBuiltins, func, obj);
    if (obj_converted.isError()) return *obj_converted;
  }

  Str str(&scope, *obj_converted);
  std::ostream* out = fileno == STDOUT_FILENO ? builtinStdout : builtinStderr;
  *out << unique_c_ptr<char[]>(str.toCStr()).get();

  return NoneType::object();
}

RawObject fileWriteObjectStr(Thread* thread, const Object& file,
                             const Object& obj) {
  return fileWriteObjectImpl(thread, file, obj, true);
}

RawObject fileWriteObjectRepr(Thread* thread, const Object& file,
                              const Object& obj) {
  return fileWriteObjectImpl(thread, file, obj, false);
}

RawObject fileWriteString(Thread* thread, const Object& file, const char* str) {
  HandleScope scope(thread);
  Str str_obj(&scope, thread->runtime()->newStrFromCStr(str));
  return fileWriteObjectStr(thread, file, str_obj);
}

}  // namespace python
