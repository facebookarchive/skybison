#include "super-builtins.h"

#include "attributedict.h"
#include "builtins.h"
#include "frame.h"
#include "globals.h"
#include "object-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "type-builtins.h"

namespace py {

RawObject superGetAttribute(Thread* thread, const Super& super,
                            const Object& name) {
  // This must return `super`.
  Runtime* runtime = thread->runtime();
  if (name == runtime->symbols()->at(ID(__class__))) {
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
    Object value(&scope, typeAt(type, name));
    if (value.isError()) {
      continue;
    }
    Type value_type(&scope, runtime->typeOf(*value));
    if (!typeIsNonDataDescriptor(thread, *value_type)) {
      return *value;
    }
    Object self(&scope, NoneType::object());
    if (super.object() != *start_type) {
      self = super.object();
    }
    // TODO(T56507184): Remove this once super.__getattribute__ gets cached.
    if (value.isFunction()) {
      // See METH(function, __get__) for details.
      if (self.isNoneType() &&
          start_type.builtinBase() != LayoutId::kNoneType) {
        return *value;
      }
      return runtime->newBoundMethod(value, self);
    }
    return Interpreter::callDescriptorGet(thread, value, self, start_type);
  }

  return objectGetAttribute(thread, super, name);
}

static const BuiltinAttribute kSuperAttributes[] = {
    {ID(__thisclass__), RawSuper::kTypeOffset, AttributeFlags::kReadOnly},
    {ID(__self__), RawSuper::kObjectOffset, AttributeFlags::kReadOnly},
    {ID(__self_class__), RawSuper::kObjectTypeOffset,
     AttributeFlags::kReadOnly},
};

void initializeSuperType(Thread* thread) {
  addBuiltinType(thread, ID(super), LayoutId::kSuper,
                 /*superclass_id=*/LayoutId::kObject, kSuperAttributes,
                 Super::kSize, /*basetype=*/true);
}

RawObject METH(super, __getattribute__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isSuper()) {
    return thread->raiseRequiresType(self_obj, ID(super));
  }
  Super self(&scope, *self_obj);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object result(&scope, superGetAttribute(thread, self, name));
  if (result.isErrorNotFound()) {
    return thread->raiseWithFmt(LayoutId::kAttributeError,
                                "super object has no attribute '%S'", &name);
  }
  return *result;
}

RawObject METH(super, __new__)(Thread* thread, Arguments) {
  return thread->runtime()->newSuper();
}

RawObject METH(super, __init__)(Thread* thread, Arguments args) {
  // only support idiomatic usage for now
  // super() -> same as super(__class__, <first argument>)
  // super(type, obj) -> bound super object; requires isinstance(obj, type)
  // super(type, type2) -> bound super object; requires issubclass(type2, type)
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!self_obj.isSuper()) {
    return thread->raiseRequiresType(self_obj, ID(super));
  }
  Super super(&scope, *self_obj);
  Object type(&scope, NoneType::object());
  Object obj(&scope, NoneType::object());
  Runtime* runtime = thread->runtime();
  if (args.get(1).isUnbound()) {
    Frame* frame = thread->currentFrame();
    // frame is for __init__, previous frame is __call__
    // this will break if it's not invoked through __call__
    if (frame->isSentinel()) {
      return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                  "super(): no current frame");
    }
    Frame* caller_frame = frame->previousFrame();
    if (caller_frame->isSentinel()) {
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
    for (word i = 0, length = free_vars.length(); i < length; i++) {
      if (free_vars.at(i) == runtime->symbols()->at(ID(__class__))) {
        cell = caller_frame->local(code.nlocals() + code.numCellvars() + i);
        break;
      }
    }
    if (cell.isErrorNotFound() || !cell.isCell()) {
      return thread->raiseWithFmt(LayoutId::kRuntimeError,
                                  "super(): __class__ cell not found");
    }
    type = Cell::cast(cell).value();
    obj = caller_frame->local(0);
    // The parameter value may have been moved into a value cell.
    if (obj.isNoneType() && !code.cell2arg().isNoneType()) {
      Tuple cell2arg(&scope, code.cell2arg());
      for (word i = 0, length = cell2arg.length(); i < length; i++) {
        if (cell2arg.at(i) == SmallInt::fromWord(0)) {
          obj = Cell::cast(caller_frame->local(code.nlocals() + i)).value();
          break;
        }
      }
    }
  } else {
    if (args.get(2).isUnbound()) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "super() expected 2 arguments");
    }
    type = args.get(1);
    obj = args.get(2);
  }
  if (!runtime->isInstanceOfType(*type)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "super() argument 1 must be type");
  }
  super.setType(*type);
  super.setObject(*obj);
  Object obj_type_obj(&scope, NoneType::object());
  if (runtime->isInstanceOfType(*obj) && typeIsSubclass(*obj, *type)) {
    obj_type_obj = *obj;
  } else {
    Type obj_type(&scope, runtime->typeOf(*obj));
    if (typeIsSubclass(*obj_type, *type)) {
      obj_type_obj = *obj_type;
    } else {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "obj must be an instance or subtype of type");
    }
  }
  super.setObjectType(*obj_type_obj);
  return NoneType::object();
}

}  // namespace py
