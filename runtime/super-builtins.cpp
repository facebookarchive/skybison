#include "super-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

const BuiltinMethod SuperBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

RawObject SuperBuiltins::dunderNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newSuper();
}

RawObject SuperBuiltins::dunderInit(Thread* thread, Frame* frame, word nargs) {
  // only support idiomatic usage for now
  // super() -> same as super(__class__, <first argument>)
  // super(type, obj) -> bound super object; requires isinstance(obj, type)
  // super(type, type2) -> bound super object; requires issubclass(type2, type)
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  if (!args.get(0).isSuper()) {
    return thread->raiseTypeErrorWithCStr("requires a super object");
  }
  Super super(&scope, args.get(0));
  Object type_obj(&scope, NoneType::object());
  Object obj(&scope, NoneType::object());
  if (args.get(1).isUnboundValue()) {
    // frame is for __init__, previous frame is __call__
    // this will break if it's not invoked through __call__
    if (frame->previousFrame() == nullptr) {
      return thread->raiseRuntimeErrorWithCStr("super(): no current frame");
    }
    Frame* caller_frame = frame->previousFrame();
    if (caller_frame->previousFrame() == nullptr) {
      return thread->raiseRuntimeErrorWithCStr("super(): no current frame");
    }
    caller_frame = caller_frame->previousFrame();
    if (!caller_frame->code().isCode()) {
      return thread->raiseRuntimeErrorWithCStr("super(): no code object");
    }
    Code code(&scope, caller_frame->code());
    if (code.argcount() == 0) {
      return thread->raiseRuntimeErrorWithCStr("super(): no arguments");
    }
    Tuple free_vars(&scope, code.freevars());
    RawObject cell = Error::object();
    for (word i = 0; i < free_vars.length(); i++) {
      if (RawStr::cast(free_vars.at(i))
              .equals(thread->runtime()->symbols()->DunderClass())) {
        cell = caller_frame->local(code.nlocals() + i);
        break;
      }
    }
    if (cell.isError() || !cell.isValueCell()) {
      return thread->raiseRuntimeErrorWithCStr(
          "super(): __class__ cell not found");
    }
    type_obj = RawValueCell::cast(cell).value();
    // TODO(zekun): handle cell2arg case
    obj = caller_frame->local(0);
  } else {
    if (args.get(2).isUnboundValue()) {
      return thread->raiseTypeErrorWithCStr("super() expected 2 arguments");
    }
    type_obj = args.get(1);
    obj = args.get(2);
  }
  if (!type_obj.isType()) {
    return thread->raiseTypeErrorWithCStr("super() argument 1 must be type");
  }
  super.setType(*type_obj);
  super.setObject(*obj);
  Object obj_type_obj(&scope, NoneType::object());
  Type type(&scope, *type_obj);
  if (obj.isType()) {
    Type obj_type(&scope, *obj);
    if (thread->runtime()->isSubclass(obj_type, type)) {
      obj_type_obj = *obj;
    }
  } else {
    Type obj_type(&scope, thread->runtime()->typeOf(*obj));
    if (thread->runtime()->isSubclass(obj_type, type)) {
      obj_type_obj = *obj_type;
    }
  }
  if (obj_type_obj.isNoneType()) {
    return thread->raiseTypeErrorWithCStr(
        "obj must be an instance or subtype of type");
  }
  super.setObjectType(*obj_type_obj);
  return NoneType::object();
}

}  // namespace python
