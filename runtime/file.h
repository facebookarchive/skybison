#pragma once

#include "handles.h"

namespace python {

class Thread;

extern std::ostream* builtinStdout;
extern std::ostream* builtinStderr;

// Internal equivalents to PyFile_WriteObject(): Write str(obj) or repr(obj) to
// file. They return None on success or Error on failure, and must not be called
// with a pending exception.
RawObject fileWriteObjectStr(Thread* thread, const Object& file,
                             const Object& obj);
RawObject fileWriteObjectRepr(Thread* thread, const Object& file,
                              const Object& obj);

// Internal equivalent to PyFile_WriteString(): Write str to file, returning
// None on success and Error on failure. Must not be called with a pending
// exception.
RawObject fileWriteString(Thread* thread, const Object& file, const char* str);

}  // namespace python
