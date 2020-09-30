#include "module-proxy-builtins.h"

#include "builtins.h"
#include "module-builtins.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kModuleProxyAttributes[] = {
    {ID(__module_object__), RawModuleProxy::kModuleOffset},
};

void initializeModuleProxyType(Thread* thread) {
  addBuiltinType(thread, ID(module_proxy), LayoutId::kModuleProxy,
                 /*superclass_id=*/LayoutId::kObject, kModuleProxyAttributes,
                 ModuleProxy::kSize, /*basetype=*/true);
}

RawObject METH(module_proxy, __contains__)(Thread* thread, Frame* frame,
                                           word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isModuleProxy()) {
    return thread->raiseRequiresType(self, ID(module_proxy));
  }
  ModuleProxy module_proxy(&scope, *self);
  Object key(&scope, args.get(1));
  key = attributeName(thread, key);
  if (key.isErrorException()) return *key;
  Module module(&scope, module_proxy.module());
  DCHECK(module.moduleProxy() == self, "module.proxy != proxy.module");
  Object result(&scope, moduleAt(thread, module, key));
  if (result.isErrorNotFound()) {
    return Bool::falseObj();
  }
  return Bool::trueObj();
}

RawObject METH(module_proxy, __delitem__)(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isModuleProxy()) {
    return thread->raiseRequiresType(self, ID(module_proxy));
  }
  ModuleProxy module_proxy(&scope, *self);
  Object key(&scope, args.get(1));
  key = attributeName(thread, key);
  Module module(&scope, module_proxy.module());
  DCHECK(module.moduleProxy() == module_proxy, "module.proxy != proxy.module");
  Object hash_obj(&scope, Interpreter::hash(thread, key));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  Object result(&scope, moduleRemove(thread, module, key, hash));
  if (result.isErrorNotFound()) {
    return thread->raiseWithFmt(LayoutId::kKeyError, "'%S'", &key);
  }
  return *result;
}

RawObject METH(module_proxy, __getitem__)(Thread* thread, Frame* frame,
                                          word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isModuleProxy()) {
    return thread->raiseRequiresType(self, ID(module_proxy));
  }
  ModuleProxy module_proxy(&scope, *self);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Module module(&scope, module_proxy.module());
  DCHECK(module.moduleProxy() == module_proxy, "module.proxy != proxy.module");
  Object result(&scope, moduleAt(thread, module, name));
  if (result.isErrorNotFound()) {
    return thread->raiseWithFmt(LayoutId::kKeyError, "'%S'", &name);
  }
  return *result;
}

RawObject METH(module_proxy, __len__)(Thread* thread, Frame* frame,
                                      word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isModuleProxy()) {
    return thread->raiseRequiresType(self, ID(module_proxy));
  }
  ModuleProxy module_proxy(&scope, *self);
  Module module(&scope, module_proxy.module());
  DCHECK(module.moduleProxy() == module_proxy, "module.proxy != proxy.module");
  return moduleLen(thread, module);
}

RawObject METH(module_proxy, get)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isModuleProxy()) {
    return thread->raiseRequiresType(self, ID(module_proxy));
  }
  ModuleProxy module_proxy(&scope, *self);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object default_obj(&scope, args.get(2));
  Module module(&scope, module_proxy.module());
  DCHECK(module.moduleProxy() == module_proxy, "module.proxy != proxy.module");
  Object result(&scope, moduleAt(thread, module, name));
  if (result.isError()) {
    return *default_obj;
  }
  return *result;
}

RawObject METH(module_proxy, pop)(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isModuleProxy()) {
    return thread->raiseRequiresType(self, ID(module_proxy));
  }
  ModuleProxy module_proxy(&scope, *self);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object default_obj(&scope, args.get(2));
  Module module(&scope, module_proxy.module());
  DCHECK(module.moduleProxy() == module_proxy, "module.proxy != proxy.module");
  Object result(&scope, moduleAt(thread, module, name));
  if (result.isError()) {
    if (default_obj.isUnbound()) {
      return thread->raiseWithFmt(LayoutId::kKeyError, "'%S'", &name);
    }
    return *default_obj;
  }
  Object hash_obj(&scope, Interpreter::hash(thread, name));
  if (hash_obj.isErrorException()) return *hash_obj;
  word hash = SmallInt::cast(*hash_obj).value();
  return moduleRemove(thread, module, name, hash);
}

RawObject METH(module_proxy, setdefault)(Thread* thread, Frame* frame,
                                         word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!self.isModuleProxy()) {
    return thread->raiseRequiresType(self, ID(module_proxy));
  }
  ModuleProxy module_proxy(&scope, *self);
  Object name(&scope, args.get(1));
  name = attributeName(thread, name);
  if (name.isErrorException()) return *name;
  Object default_obj(&scope, args.get(2));
  Module module(&scope, module_proxy.module());
  DCHECK(module.moduleProxy() == module_proxy, "module.proxy != proxy.module");
  Object value(&scope, moduleAt(thread, module, name));
  if (value.isErrorNotFound()) {
    value = *default_obj;
    moduleAtPut(thread, module, name, value);
  }
  return *value;
}

}  // namespace py
