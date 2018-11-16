#include "builtins.h"

#include <iostream>

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "mro.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

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

Object* builtinBuildClass(Thread* thread, Frame* caller, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread->handles());

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
  thread->runClassFunction(*body, *dictionary, caller);

  Handle<Class> result(&scope, runtime->newClass());
  result->setName(*name);
  result->setDictionary(*dictionary);

  Handle<ObjectArray> parents(&scope, runtime->newObjectArray(nargs - 2));
  for (word i = 0, j = 2; j < nargs; i++, j++) {
    parents->atPut(i, args.get(j));
  }
  Handle<Object> mro(&scope, computeMro(thread, result, parents));
  if (mro->isError()) {
    return *mro;
  }
  result->setMro(*mro);
  result->setInstanceSize(runtime->computeInstanceSize(result));

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
    } else {
      std::abort();
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

Object* builtinPrint(Thread*, Frame* frame, word nargs) {
  HandleScope scope;
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

  HandleScope scope;
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

} // namespace python
