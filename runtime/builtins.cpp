#include "builtins.h"

#include <iostream>

#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

std::ostream* builtinPrintStream = &std::cout;

class Arguments {
 public:
  Arguments(Frame* caller, word nargs)
      : tos_(caller->valueStackTop()), nargs_(nargs) {}

  // TODO: Remove this and flesh out the Arguments interface to support
  // keyword argument access.
  Arguments(Object** tos, word nargs) : tos_(tos), nargs_(nargs) {}

  Object* get(word n) const {
    return tos_[nargs_ - 1 - n];
  }

 private:
  Object** tos_;
  word nargs_;
};

// TODO(mpage): isinstance (somewhat unsurprisingly at this point I guess) is
// actually far more complicated than one might expect. This is enough to get
// richards working.
Object* builtinIsinstance(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("isinstance expected 2 arguments");
  }

  Arguments args(caller, nargs);
  if (!args.get(1)->isClass()) {
    // TODO(mpage): This error message is misleading. Ultimately, isinstance()
    // may accept a type or a tuple.
    return thread->throwTypeErrorFromCString("isinstance arg 2 must be a type");
  }

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Handle<Object> obj(&scope, args.get(0));
  Handle<Class> klass(&scope, args.get(1));
  return runtime->isInstance(obj, klass);
}

Object* builtinGenericNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread->handles());
  Handle<Class> klass(&scope, args.get(0));
  Handle<Layout> layout(&scope, klass->instanceLayout());
  return thread->runtime()->newInstance(layout);
}

Object* builtinBuildClass(Thread* thread, Frame* caller, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (nargs < 2) {
    std::abort(); // TODO: throw a TypeError exception.
  }
  Arguments args(caller, nargs);
  if (!args.get(0)->isFunction()) {
    std::abort(); // TODO: throw a TypeError exception.
  }
  if (!args.get(1)->isString()) {
    std::abort(); // TODO: throw a TypeError exception.
  }

  Handle<Function> body(&scope, args.get(0));
  Handle<Object> name(&scope, args.get(1));

  Handle<Dictionary> dictionary(&scope, runtime->newDictionary());
  Handle<Object> key(&scope, runtime->symbols()->DunderName());
  runtime->dictionaryAtPutInValueCell(dictionary, key, name);

  // TODO: might need to do some kind of callback here and we want backtraces to
  // work correctly.  The key to doing that would be to put some state on the
  // stack in between the the incoming arguments from the builtin' caller and
  // the on-stack state for the class body function call.
  thread->runClassFunction(*body, *dictionary);

  Handle<Class> result(&scope, runtime->newClass());
  result->setName(*name);
  result->setDictionary(*dictionary);

  // Compute MRO
  Handle<ObjectArray> parents(&scope, runtime->newObjectArray(nargs - 2));
  for (word i = 0, j = 2; j < nargs; i++, j++) {
    parents->atPut(i, args.get(j));
  }
  Handle<Object> mro(&scope, computeMro(thread, result, parents));
  if (mro->isError()) {
    return *mro;
  }
  result->setMro(*mro);

  // Initialize instance layout
  Handle<Layout> layout(&scope, runtime->computeInitialLayout(thread, result));
  layout->setDescribedClass(*result);
  result->setInstanceLayout(*layout);

  // Initialize builtin base class
  result->setBuiltinBaseClass(runtime->computeBuiltinBaseClass(result));
  Handle<Class> base(&scope, result->builtinBaseClass());
  Handle<Class> list(
      &scope, thread->runtime()->classAt(IntrinsicLayoutId::kList));
  if (Boolean::cast(thread->runtime()->isSubClass(base, list))->value()) {
    result->setFlag(Class::Flag::kListSubclass);
    layout->addDelegateSlot();
  }

  return *result;
}

static void printString(String* str) {
  for (int i = 0; i < str->length(); i++) {
    *builtinPrintStream << str->charAt(i);
  }
}

// NB: The print functions do not represent the final state of builtin functions
// and should not be emulated when creating new builtins. They are minimal
// implementations intended to get the Richards benchmark working.
static Object*
doBuiltinPrint(const Arguments& args, word nargs, const Handle<Object>& end) {
  const char separator = ' ';

  for (word i = 0; i < nargs; i++) {
    Object* arg = args.get(i);
    if (arg->isString()) {
      printString(String::cast(arg));
    } else if (arg->isSmallInteger()) {
      *builtinPrintStream << SmallInteger::cast(arg)->value();
    } else if (arg->isBoolean()) {
      *builtinPrintStream << (Boolean::cast(arg)->value() ? "True" : "False");
    } else if (arg->isDouble()) {
      *builtinPrintStream << Double::cast(arg)->value();
    } else {
      UNIMPLEMENTED("Custom print unsupported.");
    }
    if ((i + 1) != nargs) {
      *builtinPrintStream << separator;
    }
  }
  if (end->isNone()) {
    *builtinPrintStream << "\n";
  } else {
    printString(String::cast(*end));
  }

  return None::object();
}

Object* builtinPrint(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Handle<Object> end(&scope, None::object());
  Arguments args(frame, nargs);
  return doBuiltinPrint(args, nargs, end);
}

Object* builtinPrintKw(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs + 1);

  Object* last_arg = args.get(nargs);
  if (!last_arg->isObjectArray()) {
    thread->throwTypeErrorFromCString("Keyword argument names must be a tuple");
    return Error::object();
  }

  ObjectArray* kw_args = ObjectArray::cast(last_arg);
  if (kw_args->length() != 1) {
    thread->throwRuntimeErrorFromCString(
        "Too many keyword arguments supplied to print");
    return Error::object();
  }

  Object* kw_arg = kw_args->at(0);
  if (!kw_arg->isString()) {
    thread->throwTypeErrorFromCString("Keyword argument names must be strings");
    return Error::object();
  }
  if (!String::cast(kw_arg)->equalsCString("end")) {
    thread->throwRuntimeErrorFromCString(
        "Only the 'end' keyword argument is supported");
    return Error::object();
  }

  HandleScope scope(thread);
  Handle<Object> end(&scope, args.get(nargs - 1));
  if (!(end->isString() || end->isNone())) {
    thread->throwTypeErrorFromCString("'end' must be a string or None");
    return Error::object();
  }

  // Remove kw arg tuple and the value for the end keyword argument
  Arguments rest(frame->valueStackTop() + 2, nargs - 1);
  return doBuiltinPrint(rest, nargs - 1, end);
}

Object* builtinRange(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1 || nargs > 3) {
    thread->throwTypeErrorFromCString(
        "Incorrect number of arguments to range()");
    return Error::object();
  }

  Arguments args(frame, nargs);

  for (word i = 0; i < nargs; i++) {
    if (!args.get(i)->isSmallInteger()) {
      thread->throwTypeErrorFromCString(
          "Arguments to range() must be an integers.");
      return Error::object();
    }
  }

  word start = 0;
  word stop = 0;
  word step = 1;

  if (nargs == 1) {
    stop = SmallInteger::cast(args.get(0))->value();
  } else if (nargs == 2) {
    start = SmallInteger::cast(args.get(0))->value();
    stop = SmallInteger::cast(args.get(1))->value();
  } else if (nargs == 3) {
    start = SmallInteger::cast(args.get(0))->value();
    stop = SmallInteger::cast(args.get(1))->value();
    step = SmallInteger::cast(args.get(2))->value();
  }

  if (step == 0) {
    thread->throwValueErrorFromCString(
        "range() step argument must not be zero");
    return Error::object();
  }

  return thread->runtime()->newRange(start, stop, step);
}

Object* builtinOrd(Thread* thread, Frame* callerFrame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("Unexpected 1 argumment in 'ord'");
  }
  Object* arg = callerFrame->valueStackTop()[0];
  if (!arg->isString()) {
    return thread->throwTypeErrorFromCString(
        "Unsupported type in builtin 'ord'");
  }
  auto* str = String::cast(arg);
  if (str->length() != 1) {
    return thread->throwTypeErrorFromCString(
        "Builtin 'ord' expects string of length 1");
  }
  return SmallInteger::fromWord(str->charAt(0));
}

Object* builtinChr(Thread* thread, Frame* callerFrame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("Unexpected 1 argumment in 'chr'");
  }
  Object* arg = callerFrame->valueStackTop()[0];
  if (!arg->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "Unsupported type in builtin 'chr'");
  }
  word w = SmallInteger::cast(arg)->value();
  const char s[2]{static_cast<char>(w), 0};
  return SmallString::fromCString(s);
}

Object* builtinInt(Thread* thread, Frame* callerFrame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString(
        "int() takes exactly 1 argument"); // TODO(rkng): base (kw/optional)
  }
  HandleScope scope(thread);
  Handle<Object> arg(&scope, *callerFrame->valueStackTop());
  return thread->runtime()->stringToInt(thread, arg);
}

Object* builtinLen(Thread* thread, Frame* callerFrame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString(
        "len() takes exactly one argument");
  }
  HandleScope scope(thread);
  Handle<Object> self(&scope, callerFrame->valueStackTop()[0]);
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    // TODO(T27377670): Support calling __len__
    return thread->throwTypeErrorFromCString(
        "Unsupported type in builtin 'len'");
  }
  return SmallInteger::fromWord(List::cast(*list_or_error)->allocated());
}

// List
Object* listOrDelegate(Thread* thread, const Handle<Object>& instance) {
  if (instance->isList()) {
    return *instance;
  } else {
    HandleScope scope(thread);
    Handle<Class> klass(&scope, thread->runtime()->classOf(*instance));
    if (klass->hasFlag(Class::Flag::kListSubclass)) {
      return thread->runtime()->instanceDelegate(instance);
    }
  }
  return Error::object();
}

Object* builtinListNew(Thread* thread, Frame* caller, word nargs) {
  if (nargs < 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (!args.get(0)->isClass()) {
    thread->throwTypeErrorFromCString("not a type object");
  }
  HandleScope scope(thread->handles());
  Handle<Class> type(&scope, args.get(0));
  Handle<Layout> layout(&scope, type->instanceLayout());
  if (layout->id() == IntrinsicLayoutId::kList) {
    return thread->runtime()->newList();
  }
  CHECK(layout->hasDelegateSlot(), "must have a delegate slot");
  Handle<Object> result(&scope, thread->runtime()->newInstance(layout));
  Handle<Object> delegate(&scope, thread->runtime()->newList());
  thread->runtime()->setInstanceDelegate(result, delegate);
  return *result;
}

Object* builtinListAppend(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "append() takes exactly one argument");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "append() only support list or its subclasses");
  }
  Handle<List> list(&scope, *list_or_error);
  Handle<Object> value(&scope, args.get(1));
  thread->runtime()->listAdd(list, value);
  return None::object();
}

Object* builtinListInsert(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString(
        "insert() takes exactly two arguments");
  }
  Arguments args(frame, nargs);
  if (!args.get(1)->isInteger()) {
    return thread->throwTypeErrorFromCString(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "descriptor 'insert' requires a 'list' object");
  }
  Handle<List> list(&scope, *list_or_error);
  word index = SmallInteger::cast(args.get(1))->value();
  Handle<Object> value(&scope, args.get(2));
  thread->runtime()->listInsert(list, value, index);
  return None::object();
}

Object* builtinListPop(Thread* thread, Frame* frame, word nargs) {
  if (nargs > 2) {
    return thread->throwTypeErrorFromCString("pop() takes at most 1 argument");
  }
  Arguments args(frame, nargs);
  if (nargs == 2 && !args.get(1)->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "index object cannot be interpreted as an integer");
  }

  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "descriptor 'pop' requires a 'list' object");
  }
  Handle<List> list(&scope, *list_or_error);
  word index = list->allocated() - 1;
  if (nargs == 2) {
    word last_index = index;
    index = SmallInteger::cast(args.get(1))->value();
    index = index < 0 ? last_index + index : index;
    // Pop out of bounds
    if (index > last_index) {
      // TODO(T27365047): Throw an IndexError exception
      UNIMPLEMENTED("Throw an IndexError for an out of range list index.");
    }
  }
  // Pop empty, or negative out of bounds
  if (index < 0) {
    // TODO(T27365047): Throw an IndexError exception
    UNIMPLEMENTED("Throw an IndexError for an out of range list index.");
  }

  return thread->runtime()->listPop(list, index);
}

Object* builtinListRemove(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "remove() takes exactly one argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> value(&scope, args.get(1));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "descriptor 'remove' requires a 'list' object");
  }
  Handle<List> list(&scope, *list_or_error);
  for (word i = 0; i < list->allocated(); i++) {
    Handle<Object> item(&scope, list->at(i));
    if (Boolean::cast(Interpreter::richCompare(CompareOp::EQ, item, value))
            ->value()) {
      thread->runtime()->listPop(list, i);
      return None::object();
    }
  }
  return thread->throwValueErrorFromCString("list.remove(x) x not in list");
}

// Descriptor
Object* functionDescriptorGet(
    Thread* thread,
    const Handle<Object>& self,
    const Handle<Object>& instance,
    const Handle<Object>& /* owner */) {
  if (instance->isNone()) {
    return *self;
  }
  return thread->runtime()->newBoundMethod(self, instance);
}

Object* classmethodDescriptorGet(
    Thread* thread,
    const Handle<Object>& self,
    const Handle<Object>& /* instance */,
    const Handle<Object>& type) {
  HandleScope scope(thread->handles());
  Handle<Object> method(&scope, ClassMethod::cast(*self)->function());
  return thread->runtime()->newBoundMethod(method, type);
}

// ClassMethod
Object* builtinClassMethodNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newClassMethod();
}

Object* builtinClassMethodInit(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "classmethod expected 1 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<ClassMethod> classmethod(&scope, args.get(0));
  Handle<Object> arg(&scope, args.get(1));
  classmethod->setFunction(*arg);
  return *classmethod;
}

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

Object* builtinObjectInit(Thread* thread, Frame*, word nargs) {
  if (nargs > 1) {
    thread->throwTypeErrorFromCString("object.__init__() takes no parameters");
  }
  return None::object();
}

// "sys" module

Object* builtinSysExit(Thread* thread, Frame* frame, word nargs) {
  if (nargs > 1) {
    return thread->throwTypeErrorFromCString(
        "exit() accepts at most 1 argument");
  }

  // TODO: PyExc_SystemExit

  int code = 0; // success
  if (nargs == 1) {
    Object* arg = frame->valueStackTop()[0];
    if (!arg->isSmallInteger()) {
      return thread->throwTypeErrorFromCString(
          "exit() expects numeric argument");
    }
    code = SmallInteger::cast(arg)->value();
  }

  std::exit(code);
}

} // namespace python
