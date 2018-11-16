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
  // super(type, obj) -> bound super object; requires isinstance(obj, type)
  // super(type, type2) -> bound super object; requires issubclass(type2, type)
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("super() expected 2 arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(1)->isClass()) {
    return thread->throwTypeErrorFromCString("super() argument 1 must be type");
  }
  HandleScope scope(thread);
  Handle<Super> super(&scope, args.get(0));
  Handle<Class> klass(&scope, args.get(1));
  Handle<Object> obj(&scope, args.get(2));
  super->setType(*klass);
  super->setObject(*obj);
  Handle<Object> obj_type(&scope, None::object());
  if (obj->isClass()) {
    Handle<Class> obj_klass(&scope, *obj);
    if (Boolean::cast(thread->runtime()->isSubClass(obj_klass, klass))
            ->value()) {
      obj_type = *obj;
    }
  } else {
    Handle<Class> obj_klass(&scope, thread->runtime()->classOf(*obj));
    if (Boolean::cast(thread->runtime()->isSubClass(obj_klass, klass))
            ->value()) {
      obj_type = *obj_klass;
    }
    // Fill in the __class__ case
  }
  if (obj_type->isNone()) {
    return thread->throwTypeErrorFromCString(
        "obj must be an instance or subtype of type");
  }
  super->setObjectType(*obj_type);
  return *super;
}

} // namespace python
