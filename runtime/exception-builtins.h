#pragma once

#include "globals.h"
#include "runtime.h"

namespace py {

// Internal equivalent to PyErr_GivenExceptionMatches(): Return whether or not
// given is a subtype of any of the BaseException subtypes in exc, which may
// contain arbitrarily nested tuples.
bool givenExceptionMatches(Thread* thread, const Object& given,
                           const Object& exc);

// Create an exception of the given type, which should derive from
// BaseException. If value is None, no arguments will be passed to the
// constructor, if value is a tuple, it will be unpacked as arguments, and
// otherwise it will be the single argument.
RawObject createException(Thread* thread, const Type& type,
                          const Object& value);

// Return the appropriate OSError subclass from the given errno_value. If a
// corresponding subclass is not found in the mapping, return OSError.
LayoutId errorLayoutFromErrno(int errno_value);

// Internal equivalent to PyErr_NormalizeException(): If exc is a Type subtype,
// ensure that value is an instance of it (or a subtype). If a new exception
// with a traceback is raised during normalization traceback will be set to the
// new traceback.
void normalizeException(Thread* thread, Object* exc, Object* val,
                        Object* traceback);

// Internal equivalents to PyErr_PrintEx(): Print information about the current
// pending exception to sys.stderr, including any chained exceptions, and clear
// the exception. For printPendingExceptionWithSysLastVars(), also set
// sys.last{type,value,traceback} to the type, value, and traceback of the
// exception, respectively.
//
// Any exceptions raised during the printing process are swallowed.
void printPendingException(Thread* thread);
void printPendingExceptionWithSysLastVars(Thread* thread);

// Internal equivalent to PyErr_Display(): Print information about the given
// exception and traceback to sys.stderr, including any chained exceptions.
// Returns None on success or Error on failure.
RawObject displayException(Thread* thread, const Object& value,
                           const Object& traceback);

// Handle an uncaught SystemExit exception. Print information about the
// exception and call std::exit() with a status code extracted from the
// exception.
void handleSystemExit(Thread* thread);

void initializeExceptionTypes(Thread* thread);

}  // namespace py
