#include "builtins.h"

#include <unistd.h>
#include <iostream>

#include "bytecode.h"
#include "frame.h"
#include "globals.h"
#include "interpreter.h"
#include "mro.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"
#include "trampolines-inl.h"

namespace python {

std::ostream* builtInStdout = &std::cout;
std::ostream* builtinStderr = &std::cerr;

class Arguments {
 public:
  Arguments(Frame* caller, word nargs)
      : Arguments(caller->valueStackTop(), nargs) {}

  Arguments(Object** tos, word nargs) {
    bos_ = tos + nargs - 1;
    num_args_ = nargs;
  }

  Object* get(word n) const {
    CHECK(n < num_args_, "index out of range");
    return *(bos_ - n);
  }

  word numArgs() const {
    return num_args_;
  }

 protected:
  Object** bos_;
  word num_args_;
};

class KwArguments : public Arguments {
 public:
  KwArguments(Frame* caller, word nargs)
      : KwArguments(caller->valueStackTop(), nargs) {}

  // +1 for the keywords names tuple
  KwArguments(Object** tos, word nargs) : Arguments(tos, nargs + 1) {
    kwnames_ = ObjectArray::cast(*tos);
    num_keywords_ = kwnames_->length();
    num_args_ = nargs - num_keywords_;
  }

  Object* getKw(Object* name) const {
    for (word i = 0; i < num_keywords_; i++) {
      if (String::cast(name)->equals(kwnames_->at(i))) {
        return *(bos_ - num_args_ - i);
      }
    }
    CHECK(false, "keyword argument not found");
  }

  word numKeywords() const {
    return num_keywords_;
  }

 private:
  word num_keywords_;
  ObjectArray* kwnames_;
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

Object* builtinTypeCall(Thread* thread, Frame* caller, word nargs) {
  HandleScope scope(thread->handles());

  // Create a frame big enough to hold all of the outgoing arguments and the
  // function object for the __new__ and __init__ calls.
  Frame* frame = thread->openAndLinkFrame(nargs, 0, nargs + 1);

  Arguments args(caller, nargs);

  Runtime* runtime = thread->runtime();
  Handle<Object> name(&scope, runtime->symbols()->DunderNew());

  // First, call __new__ to allocate a new instance.

  Handle<Class> type(&scope, args.get(0));
  Handle<Function> dunder_new(
      &scope, runtime->lookupNameInMro(thread, type, name));

  Object** sp = frame->valueStackTop();
  *--sp = *dunder_new;
  for (word i = 0; i < nargs; i++) {
    *--sp = args.get(i);
  }
  frame->setValueStackTop(sp);

  Handle<Object> result(&scope, dunder_new->entry()(thread, frame, nargs));

  // Pop all of the arguments we pushed for the __new__ call.  While we will
  // push the same number of arguments again for the __init__ call below,
  // starting over from scratch keeps the addressing arithmetic simple.
  frame->setValueStackTop(sp + nargs + 1);

  // Second, call __init__ to initialize the instance.

  // top of the stack should be the new instance
  Handle<Object> init(&scope, runtime->symbols()->DunderInit());
  Handle<Function> dunder_init(
      &scope, runtime->lookupNameInMro(thread, type, init));

  sp = frame->valueStackTop();
  *--sp = *dunder_init;
  *--sp = *result;
  for (word i = 1; i < nargs; i++) {
    *--sp = args.get(i);
  }
  frame->setValueStackTop(sp);

  dunder_init->entry()(thread, frame, nargs);

  // TODO: throw a type error if the __init__ method does not return None.

  thread->popFrame();

  return *result;
}

Object* builtinTypeNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Handle<Class> metatype(&scope, args.get(0));
  Handle<Object> name(&scope, args.get(1));
  Handle<Class> result(&scope, runtime->newClass());
  result->setName(*name);

  // Compute MRO
  Handle<ObjectArray> parents(&scope, args.get(2));
  Handle<Object> mro(&scope, computeMro(thread, result, parents));
  if (mro->isError()) {
    return *mro;
  }
  result->setMro(*mro);

  Handle<Dictionary> dictionary(&scope, args.get(3));
  result->setDictionary(*dictionary);

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

Object* builtinTypeInit(Thread*, Frame*, word) {
  return None::object();
}

Object* builtinObjectInit(Thread* thread, Frame*, word nargs) {
  if (nargs != 1) {
    thread->throwTypeErrorFromCString("object.__init__() takes no arguments");
    return Error::object();
  }
  return None::object();
}

Object* builtinObjectNew(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  if (nargs < 1) {
    thread->throwTypeErrorFromCString("object.__new__() takes no arguments");
    return Error::object();
  }
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
  Handle<ObjectArray> bases(&scope, runtime->newObjectArray(nargs - 2));
  for (word i = 0, j = 2; j < nargs; i++, j++) {
    bases->atPut(i, args.get(j));
  }

  Handle<Dictionary> dictionary(&scope, runtime->newDictionary());
  Handle<Object> key(&scope, runtime->symbols()->DunderName());
  runtime->dictionaryAtPutInValueCell(dictionary, key, name);
  // TODO: might need to do some kind of callback here and we want backtraces to
  // work correctly.  The key to doing that would be to put some state on the
  // stack in between the the incoming arguments from the builtin' caller and
  // the on-stack state for the class body function call.
  thread->runClassFunction(*body, *dictionary);

  Object** sp = caller->valueStackTop();
  *--sp = runtime->classAt(IntrinsicLayoutId::kType);
  *--sp = *name;
  *--sp = *bases;
  *--sp = *dictionary;
  caller->setValueStackTop(sp);
  Handle<Class> result(&scope, builtinTypeCall(thread, caller, 4));
  caller->setValueStackTop(sp + 4);

  return *result;
}

Object* builtinBuildClassKw(Thread* thread, Frame* caller, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  KwArguments args(caller, nargs);
  if (args.numArgs() < 2) {
    return thread->throwTypeErrorFromCString(
        "not enough args for build class.");
  }
  if (!args.get(0)->isFunction()) {
    return thread->throwTypeErrorFromCString("class body is not function.");
  }
  if (!args.get(1)->isString()) {
    return thread->throwTypeErrorFromCString("class name is not string.");
  }

  Handle<Function> body(&scope, args.get(0));
  Handle<Object> name(&scope, args.get(1));
  Handle<Class> metaclass(&scope, args.getKw(runtime->symbols()->Metaclass()));
  Handle<ObjectArray> bases(
      &scope, runtime->newObjectArray(args.numArgs() - 2));
  for (word i = 0, j = 2; j < args.numArgs(); i++, j++) {
    bases->atPut(i, args.get(j));
  }

  Handle<Dictionary> dictionary(&scope, runtime->newDictionary());
  Handle<Object> key(&scope, runtime->symbols()->DunderName());
  runtime->dictionaryAtPutInValueCell(dictionary, key, name);
  // TODO(zekun): might need to do some kind of callback here and we want
  // backtraces to work correctly.  The key to doing that would be to put some
  // state on the stack in between the the incoming arguments from the builtin'
  // caller and the on-stack state for the class body function call.
  thread->runClassFunction(*body, *dictionary);

  Object** sp = caller->valueStackTop();
  *--sp = *metaclass;
  *--sp = *name;
  *--sp = *bases;
  *--sp = *dictionary;
  caller->setValueStackTop(sp);
  Handle<Class> result(&scope, builtinTypeCall(thread, caller, 4));
  caller->setValueStackTop(sp + 4);

  return *result;
}

static void printString(String* str) {
  for (word i = 0; i < str->length(); i++) {
    *builtInStdout << str->charAt(i);
  }
}

static void printQuotedString(String* str) {
  *builtInStdout << "'";
  for (word i = 0; i < str->length(); i++) {
    *builtInStdout << str->charAt(i);
  }
  *builtInStdout << "'";
}

static void printScalarTypes(Object* arg, std::ostream* ostream) {
  if (arg->isBoolean()) {
    *ostream << (Boolean::cast(arg)->value() ? "True" : "False");
  } else if (arg->isDouble()) {
    *ostream << Double::cast(arg)->value();
  } else if (arg->isSmallInteger()) {
    *ostream << SmallInteger::cast(arg)->value();
  } else if (arg->isString()) {
    printString(String::cast(arg));
  } else {
    UNIMPLEMENTED("Custom print unsupported");
  }
}

static bool supportedScalarType(Object* arg) {
  return (
      arg->isBoolean() || arg->isDouble() || arg->isSmallInteger() ||
      arg->isString());
}

// NB: The print functions do not represent the final state of builtin functions
// and should not be emulated when creating new builtins. They are minimal
// implementations intended to get the Richards & Pystone benchmark working.
static Object* doBuiltinPrint(
    const Arguments& args,
    word nargs,
    const Handle<Object>& end,
    std::ostream* ostream) {
  const char separator = ' ';

  for (word i = 0; i < nargs; i++) {
    Object* arg = args.get(i);
    if (supportedScalarType(arg)) {
      printScalarTypes(arg, ostream);
    } else if (arg->isList()) {
      *ostream << "[";
      HandleScope scope;
      Handle<List> list(&scope, arg);
      for (word i = 0; i < list->allocated(); i++) {
        if (supportedScalarType(list->at(i))) {
          printScalarTypes(list->at(i), ostream);
        } else {
          UNIMPLEMENTED("Custom print unsupported");
        }
        if (i != list->allocated() - 1) {
          *ostream << ", ";
        }
      }
      *ostream << "]";
    } else if (arg->isObjectArray()) {
      *ostream << "(";
      HandleScope scope;
      Handle<ObjectArray> array(&scope, arg);
      for (word i = 0; i < array->length(); i++) {
        if (supportedScalarType(array->at(i))) {
          printScalarTypes(array->at(i), ostream);
        } else {
          UNIMPLEMENTED("Custom print unsupported");
        }
        if (i != array->length() - 1) {
          *ostream << ", ";
        }
      }
      *ostream << ")";
    } else if (arg->isDictionary()) {
      *ostream << "{";
      HandleScope scope;
      Handle<Dictionary> dict(&scope, arg);
      Handle<ObjectArray> data(&scope, dict->data());
      word items = dict->numItems();
      // TODO: When rewriting this, use Bucket support
      // class in runtime.cpp
      for (word i = 0; i < data->length(); i += 3) {
        if (!data->at(i)->isNone()) {
          Handle<Object> key(&scope, data->at(i + 1));
          Handle<Object> value(&scope, data->at(i + 2));
          if (key->isString()) {
            printQuotedString(String::cast(*key));
          } else if (supportedScalarType(*key)) {
            printScalarTypes(*key, ostream);
          } else {
            UNIMPLEMENTED("Custom print unsupported");
          }
          *ostream << ": ";
          if (value->isString()) {
            printQuotedString(String::cast(*value));
          } else if (supportedScalarType(*value)) {
            printScalarTypes(*value, ostream);
          } else {
            UNIMPLEMENTED("Custom print unsupported");
          }
          if (items-- != 1) {
            *ostream << ", ";
          }
        }
      }
      *ostream << "}";
    } else {
      UNIMPLEMENTED("Custom print unsupported");
    }
    if ((i + 1) != nargs) {
      *ostream << separator;
    }
  }
  if (end->isNone()) {
    *ostream << "\n";
  } else if (end->isString()) {
    printString(String::cast(*end));
  } else {
    UNIMPLEMENTED("Unexpected type for end: %ld", end->layoutId());
  }

  return None::object();
}

Object* builtinPrint(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Handle<Object> end(&scope, None::object());
  Arguments args(frame, nargs);
  return doBuiltinPrint(args, nargs, end, builtInStdout);
}

Object* builtinPrintKw(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs + 1);
  HandleScope scope;
  Handle<Object> last_arg(&scope, args.get(nargs));
  if (!last_arg->isObjectArray()) {
    thread->throwTypeErrorFromCString("Keyword argument names must be a tuple");
    return Error::object();
  }
  Handle<ObjectArray> names(&scope, *last_arg);
  word num_keywords = names->length();
  if (num_keywords > 2) {
    thread->throwRuntimeErrorFromCString(
        "Too many keyword arguments supplied to print");
    return Error::object();
  }

  Runtime* runtime = thread->runtime();
  Object* end = None::object();
  std::ostream* ostream = builtInStdout;
  word num_positional = nargs - names->length();
  for (word i = 0; i < names->length(); i++) {
    Handle<Object> key(&scope, names->at(i));
    DCHECK(key->isString(), "Keyword argument names must be strings");
    Handle<Object> value(&scope, args.get(num_positional + i));
    if (*key == runtime->symbols()->File()) {
      if (value->isSmallInteger()) {
        word stream_val = SmallInteger::cast(*value)->value();
        switch (stream_val) {
          case STDOUT_FILENO:
            ostream = builtInStdout;
            break;
          case STDERR_FILENO:
            ostream = builtinStderr;
            break;
          default:
            thread->throwTypeErrorFromCString(
                "Unsupported argument type for 'file'");
            return Error::object();
        }
      } else {
        thread->throwTypeErrorFromCString(
            "Unsupported argument type for 'file'");
        return Error::object();
      }
    } else if (*key == runtime->symbols()->End()) {
      if ((value->isString() || value->isNone())) {
        end = *value;
      } else {
        thread->throwTypeErrorFromCString("Unsupported argument for 'end'");
        return Error::object();
      }
    } else {
      thread->throwRuntimeErrorFromCString("Unsupported keyword arguments");
      return Error::object();
    }
  }

  // Remove kw arg tuple and the value for the end keyword argument
  Arguments rest(
      frame->valueStackTop() + 1 + num_keywords, nargs - num_keywords);
  Handle<Object> end_val(&scope, end);
  return doBuiltinPrint(rest, nargs - num_keywords, end_val, ostream);
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

static Object* listOrDelegate(Thread* thread, const Handle<Object>& instance) {
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

Object* builtinLen(Thread* thread, Frame* callerFrame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString(
        "len() takes exactly one argument");
  }
  HandleScope scope(thread);
  Handle<Object> self(&scope, callerFrame->valueStackTop()[0]);
  if (self->isSet()) {
    return SmallInteger::fromWord(Set::cast(*self)->numItems());
  } else if (self->isDictionary()) {
    return SmallInteger::fromWord(Dictionary::cast(*self)->numItems());
  } else {
    Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
    if (list_or_error->isError()) {
      // TODO(T27377670): Support calling __len__
      return thread->throwTypeErrorFromCString(
          "Unsupported type in builtin 'len'");
    }
    return SmallInteger::fromWord(List::cast(*list_or_error)->allocated());
  }
}

// Boolean

Object* builtinBooleanBool(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isBoolean()) {
    return args.get(0);
  }
  return thread->throwTypeErrorFromCString("unsupported type for __bool__");
}

// Dictionary

Object* builtinDictionaryEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isDictionary() && args.get(1)->isDictionary()) {
    HandleScope scope(thread->handles());
    Runtime* runtime = thread->runtime();

    Handle<Dictionary> self(&scope, args.get(0));
    Handle<Dictionary> other(&scope, args.get(1));
    if (self->numItems() != other->numItems()) {
      return Boolean::falseObj();
    }
    Handle<ObjectArray> keys(&scope, runtime->dictionaryKeys(self));
    Handle<Object> left_key(&scope, None::object());
    Handle<Object> left(&scope, None::object());
    Handle<Object> right(&scope, None::object());
    word length = keys->length();
    for (word i = 0; i < length; i++) {
      left_key = keys->at(i);
      left = runtime->dictionaryAt(self, left_key);
      right = runtime->dictionaryAt(other, left_key);
      if (right->isError()) {
        return Boolean::falseObj();
      }
      Object* result = Interpreter::compareOperation(
          thread, caller, caller->valueStackTop(), EQ, left, right);
      if (result == Boolean::falseObj()) {
        return result;
      }
    }
    return Boolean::trueObj();
  }
  // TODO(cshapiro): handle user-defined subtypes of dictionary.
  return thread->runtime()->notImplemented();
}

// Double

Object* builtinDoubleEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() == right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleGe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() >= right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleGt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() > right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleLe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() <= right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleLt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() < right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

Object* builtinDoubleNe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isDouble() && other->isDouble()) {
    Double* left = Double::cast(self);
    Double* right = Double::cast(other);
    return Boolean::fromBool(left->value() != right->value());
  } else if (self->isInteger() || other->isInteger()) {
    UNIMPLEMENTED("integer to float conversion");
  }
  return thread->runtime()->notImplemented();
}

// List

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

Object* builtinListLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("__len__() takes no arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> list_or_error(&scope, listOrDelegate(thread, self));
  if (list_or_error->isError()) {
    return thread->throwTypeErrorFromCString(
        "__len__() only support list or its subclasses");
  }
  Handle<List> list(&scope, *list_or_error);
  return SmallInteger::fromWord(list->allocated());
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
    if (Boolean::cast(
            Interpreter::compareOperation(
                thread, frame, frame->valueStackTop(), EQ, item, value))
            ->value()) {
      thread->runtime()->listPop(list, i);
      return None::object();
    }
  }
  return thread->throwValueErrorFromCString("list.remove(x) x not in list");
}

// Descriptor
Object* functionDescriptorGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> instance(&scope, args.get(1));
  if (instance->isNone()) {
    return *self;
  }
  return thread->runtime()->newBoundMethod(self, instance);
}

Object* classmethodDescriptorGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> owner(&scope, args.get(2));

  Handle<Object> method(&scope, ClassMethod::cast(*self)->function());
  return thread->runtime()->newBoundMethod(method, owner);
}

Object* staticmethodDescriptorGet(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("__get__ needs 3 arguments");
  }
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  Handle<Object> self(&scope, args.get(0));

  return StaticMethod::cast(*self)->function();
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

// SmallInteger

Object* builtinSmallIntegerBool(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isSmallInteger()) {
    return Boolean::fromBool(args.get(0) != SmallInteger::fromWord(0));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __bool__");
}

Object* builtinSmallIntegerEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    return Boolean::fromBool(self == other);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerInvert(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isSmallInteger()) {
    SmallInteger* tos = SmallInteger::cast(args.get(0));
    return SmallInteger::fromWord(-(tos->value() + 1));
  }
  return thread->throwTypeErrorFromCString("unsupported type for __invert__");
}

Object* builtinSmallIntegerLe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() <= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerLt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() < right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerGe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() >= right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerGt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    SmallInteger* left = SmallInteger::cast(self);
    SmallInteger* right = SmallInteger::cast(other);
    return Boolean::fromBool(left->value() > right->value());
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerNe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isSmallInteger() && other->isSmallInteger()) {
    return Boolean::fromBool(self != other);
  }
  return thread->runtime()->notImplemented();
}

Object* builtinSmallIntegerNeg(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  Arguments args(caller, nargs);
  SmallInteger* tos = SmallInteger::cast(args.get(0));
  return SmallInteger::fromWord(-tos->value());
}

Object* builtinSmallIntegerPos(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("not enough arguments");
  }
  return SmallInteger::cast(*caller->valueStackTop());
}

// StaticMethod
Object* builtinStaticMethodNew(Thread* thread, Frame*, word) {
  return thread->runtime()->newStaticMethod();
}

Object* builtinStaticMethodInit(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString(
        "staticmethod expected 1 arguments");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<StaticMethod> staticmethod(&scope, args.get(0));
  Handle<Object> arg(&scope, args.get(1));
  staticmethod->setFunction(*arg);
  return *staticmethod;
}

// String

Object* builtinStringEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) == 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringGe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) >= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringGt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) > 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringLe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) <= 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringLt(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) < 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

Object* builtinStringNe(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  Object* self = args.get(0);
  Object* other = args.get(1);
  if (self->isString() && other->isString()) {
    return Boolean::fromBool(String::cast(self)->compare(other) != 0);
  }
  // TODO(cshapiro): handle user-defined subtypes of string.
  return thread->runtime()->notImplemented();
}

// Super

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

// Tuple

Object* builtinTupleEq(Thread* thread, Frame* caller, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("expected 1 argument");
  }
  Arguments args(caller, nargs);
  if (args.get(0)->isObjectArray() && args.get(1)->isObjectArray()) {
    HandleScope scope(thread->handles());
    Handle<ObjectArray> self(&scope, args.get(0));
    Handle<ObjectArray> other(&scope, args.get(1));
    if (self->length() != other->length()) {
      return Boolean::falseObj();
    }
    Handle<Object> left(&scope, None::object());
    Handle<Object> right(&scope, None::object());
    word length = self->length();
    for (word i = 0; i < length; i++) {
      left = self->at(i);
      right = other->at(i);
      Object* result = Interpreter::compareOperation(
          thread, caller, caller->valueStackTop(), EQ, left, right);
      if (result == Boolean::falseObj()) {
        return result;
      }
    }
    return Boolean::trueObj();
  }
  // TODO(cshapiro): handle user-defined subtypes of tuple.
  return thread->runtime()->notImplemented();
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

// "time" module

Object* builtinTime(Thread* thread, Frame*, word) {
  return thread->runtime()->newDouble(OS::currentTime());
}

} // namespace python
