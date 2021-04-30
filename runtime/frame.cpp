#include "frame.h"

#include <cstring>

#include "dict-builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"

namespace py {

const char* Frame::isInvalid() {
  if (!at(kPreviousFrameOffset).isSmallInt()) {
    return "bad previousFrame field";
  }
  if (!isSentinel() && !(locals() + kFunctionOffsetFromLocals)->isFunction()) {
    return "bad function";
  }
  return nullptr;
}

RawObject frameLocals(Thread* thread, Frame* frame) {
  HandleScope scope(thread);
  Function function(&scope, frame->function());
  if (!function.hasOptimizedOrNewlocals()) {
    Object implicit_globals(&scope, frame->implicitGlobals());
    if (implicit_globals.isNoneType()) {
      Module module(&scope, function.moduleObject());
      return module.moduleProxy();
    }
    return *implicit_globals;
  }

  Code code(&scope, function.code());
  Runtime* runtime = thread->runtime();
  Tuple empty_tuple(&scope, runtime->emptyTuple());
  Tuple var_names(&scope,
                  code.varnames().isTuple() ? code.varnames() : *empty_tuple);
  Tuple freevar_names(
      &scope, code.freevars().isTuple() ? code.freevars() : *empty_tuple);
  Tuple cellvar_names(
      &scope, code.cellvars().isTuple() ? code.cellvars() : *empty_tuple);

  word var_names_length = var_names.length();
  word freevar_names_length = freevar_names.length();
  word cellvar_names_length = cellvar_names.length();

  DCHECK(function.totalLocals() ==
             var_names_length + freevar_names_length + cellvar_names_length,
         "numbers of local variables do not match");

  Dict result(&scope, runtime->newDict());
  Str name(&scope, Str::empty());
  Object value(&scope, NoneType::object());
  for (word i = 0; i < var_names_length; ++i) {
    name = var_names.at(i);
    value = frame->local(i);
    // TODO(T89882231) Remove check when we can verify locals have been
    // initialized
    if (value.isInternal()) continue;
    dictAtPutByStr(thread, result, name, value);
  }
  for (word i = 0, j = var_names_length; i < freevar_names_length; ++i, ++j) {
    name = freevar_names.at(i);
    DCHECK(frame->local(j).isCell(), "freevar must be Cell");
    value = Cell::cast(frame->local(j)).value();
    // TODO(T89882231) Remove check when we can verify locals have been
    // initialized
    if (value.isInternal()) continue;
    dictAtPutByStr(thread, result, name, value);
  }
  for (word i = 0, j = var_names_length + freevar_names_length;
       i < cellvar_names_length; ++i, ++j) {
    name = cellvar_names.at(i);
    DCHECK(frame->local(j).isCell(), "cellvar must be Cell");
    value = Cell::cast(frame->local(j)).value();
    // TODO(T89882231) Remove check when we can verify locals have been
    // initialized
    if (value.isInternal()) continue;
    dictAtPutByStr(thread, result, name, value);
  }
  return *result;
}

}  // namespace py
