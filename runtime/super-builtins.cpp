#include "super-builtins.h"

#include "frame.h"
#include "globals.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "type-builtins.h"

namespace python {

RawObject superGetAttribute(Thread* thread, const Super& super,
                            const Object& name_str) {
  // This must return `super`.
  Runtime* runtime = thread->runtime();
  if (Str::cast(runtime->symbols()->DunderClass()).equals(*name_str)) {
    return runtime->typeOf(*super);
  }

  HandleScope scope(thread);
  Type start_type(&scope, super.objectType());
  Tuple mro(&scope, start_type.mro());
  word i;
  word mro_length = mro.length();
  for (i = 0; i < mro_length; i++) {
    if (super.type() == mro.at(i)) {
      // skip super->type (if any)
      i++;
      break;
    }
  }
  for (; i < mro_length; i++) {
    Type type(&scope, mro.at(i));
    Dict dict(&scope, type.dict());
    Object value(&scope, runtime->typeDictAt(thread, dict, name_str));
    if (value.isError()) {
      continue;
    }
    Type value_type(&scope, runtime->typeOf(*value));
    if (!typeIsNonDataDescriptor(thread, value_type)) {
      return *value;
    }
    Object self(&scope, NoneType::object());
    if (super.object() != *start_type) {
      self = super.object();
    }
    return Interpreter::callDescriptorGet(thread, thread->currentFrame(), value,
                                          self, start_type);
  }

  return objectGetAttribute(thread, super, name_str);
}

const BuiltinMethod SuperBuiltins::kBuiltinMethods[] = {
    {SymbolId::kDunderGetattribute, dunderGetattribute},
    {SymbolId::kDunderInit, dunderInit},
    {SymbolId::kDunderNew, dunderNew},
    {SymbolId::kSentinelId, nullptr},
};

RawObject SuperBuiltins::dunderGetattribute(Thread* thread, Frame* frame,
                                            word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!self_obj.isSuper()) {
    return thread->raiseRequiresType(self_obj, SymbolId::kSuper);
  }
  Super self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseWithFmt(
        LayoutId::kTypeError, "attribute name must be string, not '%T'", &name);
  }
  Object result(&scope, superGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "super object has no attribute '%S'", &name);
  }
  return *result;
}

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
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "requires a super object");
  }
  Super super(&scope, args.get(0));
  Object type_obj(&scope, NoneType::object());
  Object obj(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  if (args.get(1).isUnbound()) {
    // frame is for __init__, previous frame is __call__
    // this will break if it's not invoked through __call__
    if (frame->previousFrame() == nullptr) {
      return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                  "super(): no current frame");
    }
    Frame* caller_frame = frame->previousFrame();
    if (caller_frame->previousFrame() == nullptr) {
      return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                  "super(): no current frame");
    }
    caller_frame = caller_frame->previousFrame();
    if (!caller_frame->code().isCode()) {
      return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                  "super(): no code object");
    }
    Code code(&scope, caller_frame->code());
    if (code.argcount() == 0) {
      return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                  "super(): no arguments");
    }
    Tuple free_vars(&scope, code.freevars());
    RawObject cell = Error::notFound();
    for (word i = 0; i < free_vars.length(); i++) {
      if (Str::cast(free_vars.at(i))
              .equals(runtime->symbols()->DunderClass())) {
        cell = caller_frame->local(code.nlocals() + code.numCellvars() + i);
        break;
      }
    }
    if (cell.isError() || !cell.isValueCell()) {
      return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                  "super(): __class__ cell not found");
    }
    type_obj = ValueCell::cast(cell).value();
    // TODO(zekun): handle cell2arg case
    obj = caller_frame->local(0);
  } else {
    if (args.get(2).isUnbound()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "super() expected 2 arguments");
    }
    type_obj = args.get(1);
    obj = args.get(2);
  }
  if (!runtime->isInstanceOfType(*type_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "super() argument 1 must be type");
  }
  super.setType(*type_obj);
  super.setObject(*obj);
  Object obj_type_obj(&scope, NoneType::object());
  Type type(&scope, *type_obj);
  if (obj.isType()) {
    Type obj_type(&scope, *obj);
    if (runtime->isSubclass(obj_type, type)) {
      obj_type_obj = *obj;
    }
  } else {
    Type obj_type(&scope, runtime->typeOf(*obj));
    if (runtime->isSubclass(obj_type, type)) {
      obj_type_obj = *obj_type;
    }
  }
  if (obj_type_obj.isNoneType()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "obj must be an instance or subtype of type");
  }
  super.setObjectType(*obj_type_obj);
  return NoneType::object();
}

}  // namespace python
