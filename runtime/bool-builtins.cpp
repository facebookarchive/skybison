#include "bool-builtins.h"

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinBooldunderBool(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(frame, nargs);
  if (args.get(0)->isBool()) {
    return args.get(0);
  }
  return thread->throwTypeErrorFromCString("unsupported type for __bool__");
}

Object* builtinBoolNew(Thread* thread, Frame* frame, word nargs) {
  if (nargs == 0) {
    return thread->throwTypeErrorFromCString(
        "bool.__new__(): not enough arguments");
  }
  if (nargs > 2) {
    return thread->throwTypeErrorFromCString(
        "bool() takes at most one argument");
  }

  Runtime* runtime = thread->runtime();
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> type_obj(&scope, args.get(0));
  if (!runtime->hasSubClassFlag(*type_obj, Type::Flag::kClassSubclass)) {
    return thread->throwTypeErrorFromCString(
        "bool.__new__(X): X is not a type object");
  }
  Handle<Type> type(&scope, *type_obj);

  // Since bool can't be subclassed, only need to check if the type is exactly
  // bool.
  Handle<Layout> layout(&scope, type->instanceLayout());
  if (layout->id() != LayoutId::kBool) {
    return thread->throwTypeErrorFromCString("bool.__new__(X): X is not bool");
  }

  // If no arguments are given, return false.
  if (nargs == 1) {
    return Bool::falseObj();
  }

  Handle<Object> arg(&scope, args.get(1));
  // The interpreter reads the value from the frame, so add it on first.
  frame->pushValue(*arg);
  Object* result = Interpreter::isTrue(thread, frame);
  // Pop off the arg to isTrue before returning.
  frame->popValue();
  return result;
}

}  // namespace python
