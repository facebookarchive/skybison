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

  Handle<Dict> dict(&scope, runtime->newDict());
  Handle<Object> key(&scope, runtime->symbols()->DunderName());
  runtime->dictAtPutInValueCell(dict, key, name);
  // TODO: might need to do some kind of callback here and we want backtraces to
  // work correctly.  The key to doing that would be to put some state on the
  // stack in between the the incoming arguments from the builtin' caller and
  // the on-stack state for the class body function call.
  thread->runClassFunction(*body, *dict);

  Handle<Type> type(&scope, runtime->typeAt(LayoutId::kType));
  Handle<Function> dunder_call(
      &scope, runtime->lookupSymbolInMro(thread, type, SymbolId::kDunderCall));
  frame->pushValue(*dunder_call);
  frame->pushValue(*type);
  frame->pushValue(*name);
  frame->pushValue(*bases);
  frame->pushValue(*dict);
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

  Handle<Object> bootstrap(&scope, args.getKw(runtime->symbols()->Bootstrap()));
  if (bootstrap->isError()) {
    bootstrap = Bool::falseObj();
  }

  Handle<Object> metaclass(&scope, args.getKw(runtime->symbols()->Metaclass()));
  if (metaclass->isError()) {
    metaclass = runtime->typeAt(LayoutId::kType);
  }

  Handle<ObjectArray> bases(
      &scope, runtime->newObjectArray(args.numArgs() - args.numKeywords() - 1));
  for (word i = 0, j = 2; j < args.numArgs(); i++, j++) {
    bases->atPut(i, args.get(j));
  }

  Handle<Object> dict_obj(&scope, None::object());
  Handle<Object> type_obj(&scope, None::object());
  if (*bootstrap == Bool::falseObj()) {
    // An ordinary class initialization creates a new class dictionary.
    dict_obj = runtime->newDict();
  } else {
    // A bootstrap class initialization uses the existing class dictionary.
    CHECK(frame->previousFrame() != nullptr, "must have a caller frame");
    Handle<Dict> globals(&scope, frame->previousFrame()->globals());
    Handle<ValueCell> value_cell(&scope, runtime->dictAt(globals, name));
    CHECK(value_cell->value()->isType(), "name is not bound to a type object");
    Handle<Type> type(&scope, value_cell->value());
    type_obj = *type;
    dict_obj = type->dict();
  }

  // TODO(zekun): might need to do some kind of callback here and we want
  // backtraces to work correctly.  The key to doing that would be to put some
  // state on the stack in between the the incoming arguments from the builtin'
  // caller and the on-stack state for the class body function call.
  thread->runClassFunction(*body, *dict_obj);

  // A bootstrap class initialization is complete at this point.  Add a type
  // name to the type dictionary and return the initialized type object.
  if (*bootstrap == Bool::trueObj()) {
    Handle<Object> key(&scope, runtime->symbols()->DunderName());
    Handle<Dict> dict(&scope, *dict_obj);
    runtime->dictAtPutInValueCell(dict, key, name);
    return *type_obj;
  }

  Handle<Type> type(&scope, *metaclass);
  Handle<Function> dunder_call(
      &scope, runtime->lookupSymbolInMro(thread, type, SymbolId::kDunderCall));
  frame->pushValue(*dunder_call);
  frame->pushValue(*type);
  frame->pushValue(*name);
  frame->pushValue(*bases);
  frame->pushValue(*dict_obj);
  return Interpreter::call(thread, frame, 4);
}

Object* builtinCallable(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("callable expects one argument");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> arg(&scope, args.get(0));
  if (arg->isFunction() || arg->isBoundMethod() || arg->isType()) {
    return Bool::trueObj();
  }
  Runtime* runtime = thread->runtime();
  Handle<Type> type(&scope, runtime->typeOf(*arg));
  // If its type defines a __call__, it is also callable (even if __call__ is
  // not actually callable).
  // Note that this does not include __call__ defined on the particular
  // instance, only __call__ defined on the type.
  Handle<Object> callable(&scope, thread->runtime()->lookupSymbolInMro(
                                      thread, type, SymbolId::kDunderCall));
  return Bool::fromBool(!callable->isError());
}

Object* builtinChr(Thread* thread, Frame* frame_frame, word nargs) {
  if (nargs != 1) {
    return thread->throwTypeErrorFromCString("Unexpected 1 argumment in 'chr'");
  }
  Arguments args(frame_frame, nargs);
  Object* arg = args.get(0);
  if (!arg->isSmallInt()) {
    return thread->throwTypeErrorFromCString(
        "Unsupported type in builtin 'chr'");
  }
  word w = SmallInt::cast(arg)->value();
  const char s[2]{static_cast<char>(w), 0};
  return SmallStr::fromCString(s);
}

// TODO(mpage): isinstance (somewhat unsurprisingly at this point I guess) is
// actually far more complicated than one might expect. This is enough to get
// richards working.
Object* builtinIsinstance(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("isinstance expected 2 arguments");
  }

  Arguments args(frame, nargs);
  if (!args.get(1)->isType()) {
    // TODO(mpage): This error message is misleading. Ultimately, isinstance()
    // may accept a type or a tuple.
    return thread->throwTypeErrorFromCString("isinstance arg 2 must be a type");
  }

  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Handle<Object> obj(&scope, args.get(0));
  Handle<Type> type(&scope, args.get(1));
  return runtime->isInstance(obj, type);
}

Object* builtinIssubclass(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("issubclass expected 2 arguments");
  }

  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (!args.get(0)->isType()) {
    return thread->throwTypeErrorFromCString("issubclass arg 1 must be a type");
  }
  Handle<Type> type(&scope, args.get(0));
  Handle<Object> classinfo(&scope, args.get(1));
  if (runtime->isInstanceOfClass(*classinfo)) {
    Handle<Type> possible_superclass(&scope, *classinfo);
    return runtime->isSubClass(type, possible_superclass);
  }
  // If classinfo is not a tuple, then throw a TypeError.
  if (!classinfo->isObjectArray()) {
    return thread->throwTypeErrorFromCString(
        "issubclass() arg 2 must be a class of tuple of classes");
  }
  // If classinfo is a tuple, try each of the values, and return
  // True if the first argument is a subclass of any of them.
  Handle<ObjectArray> tuple_of_types(&scope, *classinfo);
  for (word i = 0; i < tuple_of_types->length(); i++) {
    // If any argument is not a type, then throw TypeError.
    if (!runtime->isInstanceOfClass(tuple_of_types->at(i))) {
      return thread->throwTypeErrorFromCString(
          "issubclass() arg 2 must be a class of tuple of classes");
    }
    Handle<Type> possible_superclass(&scope, tuple_of_types->at(i));
    Handle<Bool> result(&scope, runtime->isSubClass(type, possible_superclass));
    // If any of the types are a superclass, return true.
    if (result->value()) {
      return Bool::trueObj();
    }
  }
  // None of the types in the tuple were a superclass, so return false.
  return Bool::falseObj();
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
  return SmallInt::fromWord(str->charAt(0));
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
  if (arg->isBool()) {
    *ostream << (Bool::cast(arg)->value() ? "True" : "False");
  } else if (arg->isFloat()) {
    *ostream << Float::cast(arg)->value();
  } else if (arg->isSmallInt()) {
    *ostream << SmallInt::cast(arg)->value();
  } else if (arg->isString()) {
    printString(String::cast(arg));
  } else {
    UNIMPLEMENTED("Custom print unsupported");
  }
}

static bool supportedScalarType(Object* arg) {
  return (arg->isBool() || arg->isFloat() || arg->isSmallInt() ||
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
    } else if (arg->isDict()) {
      *ostream << "{";
      HandleScope scope;
      Handle<Dict> dict(&scope, arg);
      Handle<ObjectArray> data(&scope, dict->data());
      word items = dict->numItems();
      for (word i = 0; i < data->length(); i += 3) {
        if (!data->at(i)->isNone()) {
          Handle<Object> key(&scope, Dict::Bucket::key(*data, i));
          Handle<Object> value(&scope, Dict::Bucket::value(*data, i));
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
    if (file_arg->isSmallInt()) {
      word stream_val = SmallInt::cast(*file_arg)->value();
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
    if (!args.get(i)->isSmallInt()) {
      return thread->throwTypeErrorFromCString(
          "Arguments to range() must be an integers.");
    }
  }

  word start = 0;
  word stop = 0;
  word step = 1;

  if (nargs == 1) {
    stop = SmallInt::cast(args.get(0))->value();
  } else if (nargs == 2) {
    start = SmallInt::cast(args.get(0))->value();
    stop = SmallInt::cast(args.get(1))->value();
  } else if (nargs == 3) {
    start = SmallInt::cast(args.get(0))->value();
    stop = SmallInt::cast(args.get(1))->value();
    step = SmallInt::cast(args.get(2))->value();
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

Object* builtinGetattr(Thread* thread, Frame* frame, word nargs) {
  if (nargs < 2 || nargs > 3) {
    return thread->throwTypeErrorFromCString(
        "getattr expected 2 or 3 arguments.");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> name(&scope, args.get(1));
  if (!name->isString()) {
    return thread->throwTypeErrorFromCString(
        "getattr(): attribute name must be string.");
  }
  Handle<Object> result(&scope,
                        thread->runtime()->attributeAt(thread, self, name));
  if (result->isError() && nargs == 3) {
    result = args.get(2);
    // TODO(T32775277) Implement PyErr_ExceptionMatches
    thread->clearPendingException();
  }
  return *result;
}

Object* builtinHasattr(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 2) {
    return thread->throwTypeErrorFromCString("hasattr expected 2 arguments.");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> name(&scope, args.get(1));
  if (!name->isString()) {
    return thread->throwTypeErrorFromCString(
        "hasattr(): attribute name must be string.");
  }
  Handle<Object> result(&scope,
                        thread->runtime()->attributeAt(thread, self, name));
  if (result->isError()) {
    // TODO(T32775277) Implement PyErr_ExceptionMatches
    thread->clearPendingException();
    return Bool::falseObj();
  }
  return Bool::trueObj();
}

Object* builtinSetattr(Thread* thread, Frame* frame, word nargs) {
  if (nargs != 3) {
    return thread->throwTypeErrorFromCString("setattr expected 3 arguments.");
  }
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Handle<Object> self(&scope, args.get(0));
  Handle<Object> name(&scope, args.get(1));
  Handle<Object> value(&scope, args.get(2));
  if (!name->isString()) {
    return thread->throwTypeErrorFromCString(
        "setattr(): attribute name must be string.");
  }
  Handle<Object> result(
      &scope, thread->runtime()->attributeAtPut(thread, self, name, value));
  if (result->isError()) {
    // populate the exception
    return *result;
  }
  return None::object();
}

}  // namespace python
