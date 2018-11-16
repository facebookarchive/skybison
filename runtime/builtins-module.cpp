#include "builtins-module.h"

#include <unistd.h>
#include <iostream>

#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "int-builtins.h"
#include "interpreter.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"
#include "type-builtins.h"

namespace python {

std::ostream* builtInStdout = &std::cout;
std::ostream* builtinStderr = &std::cerr;

Object* builtinBuildClass(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (nargs < 2) {
    std::abort();  // TODO: throw a TypeError exception.
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isFunction()) {
    std::abort();  // TODO: throw a TypeError exception.
  }
  if (!args.get(1)->isString()) {
    std::abort();  // TODO: throw a TypeError exception.
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

  Handle<Class> klass(&scope, runtime->classAt(LayoutId::kType));
  Handle<Object> call_name(&scope, runtime->symbols()->DunderCall());
  Handle<Function> dunder_call(
      &scope, runtime->lookupNameInMro(thread, klass, call_name));
  frame->pushValue(*dunder_call);
  frame->pushValue(*klass);
  frame->pushValue(*name);
  frame->pushValue(*bases);
  frame->pushValue(*dictionary);
  return Interpreter::call(thread, frame, 4);
}

Object* builtinBuildClassKw(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  KwArguments args(frame, nargs);
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
  Handle<Object> kw_val(&scope, args.getKw(runtime->symbols()->Metaclass()));
  if (kw_val->isError()) {
    return thread->throwTypeErrorFromCString("metaclass not found.");
  }
  Handle<Class> metaclass(&scope, *kw_val);
  Handle<ObjectArray> bases(&scope,
                            runtime->newObjectArray(args.numArgs() - 2));
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

  Handle<Object> call_name(&scope, runtime->symbols()->DunderCall());
  Handle<Function> dunder_call(
      &scope, runtime->lookupNameInMro(thread, metaclass, call_name));
  frame->pushValue(*dunder_call);
  frame->pushValue(*metaclass);
  frame->pushValue(*name);
  frame->pushValue(*bases);
  frame->pushValue(*dictionary);
  return Interpreter::call(thread, frame, 4);
}

Object* builtinChr(Thread* thread, Frame* frame_frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("Unexpected 1 argumment in 'chr'");
  }
  Arguments args(frame_frame, nargs);
  Object* arg = args.get(0);
  if (!arg->isSmallInteger()) {
    return thread->throwTypeErrorFromCString(
        "Unsupported type in builtin 'chr'");
  }
  word w = SmallInteger::cast(arg)->value();
  const char s[2]{static_cast<char>(w), 0};
  return SmallString::fromCString(s);
}

// TODO(mpage): isinstance (somewhat unsurprisingly at this point I guess) is
// actually far more complicated than one might expect. This is enough to get
// richards working.
Object* builtinIsinstance(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("isinstance expected 2 arguments");
  }

  Arguments args(frame, nargs);
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

Object* builtinLen(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString(
        "len() takes exactly one argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> method(&scope, Interpreter::lookupMethod(
                                    thread, frame, self, SymbolId::kDunderLen));
  if (method->isError()) {
    return thread->throwTypeErrorFromCString("object has no len()");
  }
  return Interpreter::callMethod1(thread, frame, method, self);
}

Object* builtinOrd(Thread* thread, Frame* frame_frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("Unexpected 1 argumment in 'ord'");
  }
  Arguments args(frame_frame, nargs);
  Object* arg = args.get(0);
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
  return (arg->isBoolean() || arg->isDouble() || arg->isSmallInteger() ||
          arg->isString());
}

// NB: The print functions do not represent the final state of builtin functions
// and should not be emulated when creating new builtins. They are minimal
// implementations intended to get the Richards & Pystone benchmark working.
static Object* doBuiltinPrint(const Arguments& args, word nargs,
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
      for (word i = 0; i < data->length(); i += 3) {
        if (!data->at(i)->isNone()) {
          Handle<Object> key(&scope, Dictionary::Bucket::key(*data, i));
          Handle<Object> value(&scope, Dictionary::Bucket::value(*data, i));
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
    } else if (arg->isNone()) {
      *ostream << "None";
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
    UNIMPLEMENTED("Unexpected type for end: %ld",
                  static_cast<word>(end->layoutId()));
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
  KwArguments kw_args(frame, nargs);
  HandleScope scope(thread);
  if (kw_args.numKeywords() > 2) {
    return thread->throwRuntimeErrorFromCString(
        "Too many keyword arguments supplied to print");
  }

  Runtime* runtime = thread->runtime();
  Object* end = None::object();
  std::ostream* ostream = builtInStdout;

  Handle<Object> file_arg(&scope, kw_args.getKw(runtime->symbols()->File()));
  if (!file_arg->isError()) {
    if (file_arg->isSmallInteger()) {
      word stream_val = SmallInteger::cast(*file_arg)->value();
      switch (stream_val) {
        case STDOUT_FILENO:
          ostream = builtInStdout;
          break;
        case STDERR_FILENO:
          ostream = builtinStderr;
          break;
        default:
          return thread->throwTypeErrorFromCString(
              "Unsupported argument type for 'file'");
      }
    } else {
      return thread->throwTypeErrorFromCString(
          "Unsupported argument type for 'file'");
    }
  }

  Handle<Object> end_arg(&scope, kw_args.getKw(runtime->symbols()->End()));
  if (!end_arg->isError()) {
    if ((end_arg->isString() || end_arg->isNone())) {
      end = *end_arg;
    } else {
      return thread->throwTypeErrorFromCString(
          "Unsupported argument for 'end'");
    }
  }

  // Remove kw arg tuple and the value for the end keyword argument
  Arguments rest(frame, nargs - kw_args.numKeywords() - 1);
  Handle<Object> end_val(&scope, end);
  return doBuiltinPrint(rest, nargs - kw_args.numKeywords() - 1, end_val,
                        ostream);
}

Object* builtinRange(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 1 || nargs > 3) {
    return thread->throwTypeErrorFromCString(
        "Incorrect number of arguments to range()");
  }

  Arguments args(frame, nargs);

  for (word i = 0; i < nargs; i++) {
    if (!args.get(i)->isSmallInteger()) {
      return thread->throwTypeErrorFromCString(
          "Arguments to range() must be an integers.");
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
    return thread->throwValueErrorFromCString(
        "range() step argument must not be zero");
  }

  return thread->runtime()->newRange(start, stop, step);
}

Object* builtinRepr(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString(
        "repr() takes exactly one argument");
  }
  Arguments args(frame, nargs);

  HandleScope scope(thread);
  Handle<Object> obj(&scope, args.get(0));
  // Only one argument, the value to be repr'ed.
  Handle<Object> method(&scope, Interpreter::lookupMethod(
                                    thread, frame, obj, SymbolId::kDunderRepr));
  CHECK(!method->isError(),
        "__repr__ doesn't exist for this object, which is impossible since "
        "object has a __repr__, and everything descends from object");
  Object* ret = Interpreter::callMethod1(thread, frame, method, obj);
  if (!ret->isString() && !ret->isError()) {
    // TODO(T31744782): Change this to allow subtypes of string.
    // If __repr__ doesn't return a string or error, throw a type error
    return thread->throwTypeErrorFromCString("__repr__ returned non-string");
  }
  return ret;
}

}  // namespace python
