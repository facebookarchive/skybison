#include "super-builtins.h"

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace python {

Object* builtinSuperNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newSuper();
}

Object* builtinSuperInit(Thread* thread, Frame* frame, word nargs) {
  // only support idiomatic usage for now
  // super() -> same as super(__class__, <first argument>)
  // super(type, obj) -> bound super object; requires isinstance(obj, type)
  // super(type, type2) -> bound super object; requires issubclass(type2, type)
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  if (!args.get(0)->isSuper()) {
    return thread->raiseTypeErrorWithCStr("requires a super object");
  }
  Handle<Super> super(&scope, args.get(0));
  Handle<Object> klass_obj(&scope, NoneType::object());
  Handle<Object> obj(&scope, NoneType::object());
  if (nargs == 1) {
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
    if (!caller_frame->code()->isCode()) {
      return thread->raiseRuntimeErrorWithCStr("super(): no code object");
    }
    Handle<Code> code(&scope, caller_frame->code());
    if (code->argcount() == 0) {
      return thread->raiseRuntimeErrorWithCStr("super(): no arguments");
    }
    Handle<ObjectArray> free_vars(&scope, code->freevars());
    Object* cell = Error::object();
    for (word i = 0; i < free_vars->length(); i++) {
      if (Str::cast(free_vars->at(i))
              ->equals(thread->runtime()->symbols()->DunderClass())) {
        cell = caller_frame->getLocal(code->nlocals() + i);
        break;
      }
    }
    if (cell->isError() || !cell->isValueCell()) {
      return thread->raiseRuntimeErrorWithCStr(
          "super(): __class__ cell not found");
    }
    klass_obj = ValueCell::cast(cell)->value();
    // TODO(zekun): handle cell2arg case
    obj = caller_frame->getLocal(0);
  } else {
    if (nargs != 3) {
      return thread->raiseTypeErrorWithCStr("super() expected 2 arguments");
    }
    klass_obj = args.get(1);
    obj = args.get(2);
  }
  if (!klass_obj->isType()) {
    return thread->raiseTypeErrorWithCStr("super() argument 1 must be type");
  }
  super->setType(*klass_obj);
  super->setObject(*obj);
  Handle<Object> obj_type(&scope, NoneType::object());
  Handle<Type> klass(&scope, *klass_obj);
  if (obj->isType()) {
    Handle<Type> obj_klass(&scope, *obj);
    if (thread->runtime()->isSubClass(obj_klass, klass) == Bool::trueObj()) {
      obj_type = *obj;
    }
  } else {
    Handle<Type> obj_klass(&scope, thread->runtime()->typeOf(*obj));
    if (thread->runtime()->isSubClass(obj_klass, klass) == Bool::trueObj()) {
      obj_type = *obj_klass;
    }
  }
  if (obj_type->isNoneType()) {
    return thread->raiseTypeErrorWithCStr(
        "obj must be an instance or subtype of type");
  }
  super->setObjectType(*obj_type);
  return *super;
}

}  // namespace python
