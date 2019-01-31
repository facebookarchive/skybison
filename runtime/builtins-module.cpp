#include "builtins-module.h"

#include <unistd.h>
#include <iostream>

#include "exception-builtins.h"
#include "frame.h"
#include "globals.h"
#include "handles.h"
#include "int-builtins.h"
#include "interpreter.h"
#include "marshal.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "type-builtins.h"

namespace python {

std::ostream* builtInStdout = &std::cout;
std::ostream* builtinStderr = &std::cerr;

RawObject getAttribute(Thread* thread, const Object& self, const Object& name) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseTypeErrorWithCStr(
        "getattr(): attribute name must be string");
  }
  return runtime->attributeAt(thread, self, name);
}

RawObject hasAttribute(Thread* thread, const Object& self, const Object& name) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseTypeErrorWithCStr(
        "hasattr(): attribute name must be string");
  }

  HandleScope scope(thread);
  Object result(&scope, runtime->attributeAt(thread, self, name));
  if (!result->isError()) {
    return Bool::trueObj();
  }

  Type given(&scope, thread->pendingExceptionType());
  Type exc(&scope, runtime->typeAt(LayoutId::kAttributeError));
  if (givenExceptionMatches(thread, given, exc)) {
    thread->clearPendingException();
    return Bool::falseObj();
  }

  return Error::object();
}

RawObject setAttribute(Thread* thread, const Object& self, const Object& name,
                       const Object& value) {
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*name)) {
    return thread->raiseTypeErrorWithCStr(
        "setattr(): attribute name must be string");
  }
  return runtime->attributeAtPut(thread, self, name, value);
}

RawObject Builtins::buildClass(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (nargs < 2) {
    std::abort();  // TODO(cshapiro): throw a TypeError exception.
  }
  Arguments args(frame, nargs);
  if (!args.get(0)->isFunction()) {
    std::abort();  // TODO(cshapiro): throw a TypeError exception.
  }
  if (!args.get(1)->isStr()) {
    std::abort();  // TODO(cshapiro): throw a TypeError exception.
  }

  Function body(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Tuple bases(&scope, runtime->newTuple(nargs - 2));
  for (word i = 0, j = 2; j < nargs; i++, j++) {
    bases->atPut(i, args.get(j));
  }

  // TODO(cshapiro): might need to do some kind of callback here and we want
  // backtraces to work correctly.  The key to doing that would be to put some
  // state on the stack in between the the incoming arguments from the builtin
  // caller and the on-stack state for the class body function call.
  Dict dict(&scope, runtime->newDict());
  // TODO(T39559070): Add __qualname__ to the type dict
  thread->runClassFunction(body, dict);

  Type type(&scope, runtime->typeAt(LayoutId::kType));
  Function dunder_call(
      &scope, runtime->lookupSymbolInMro(thread, type, SymbolId::kDunderCall));
  frame->pushValue(*dunder_call);
  frame->pushValue(*type);
  frame->pushValue(*name);
  frame->pushValue(*bases);
  frame->pushValue(*dict);
  return Interpreter::call(thread, frame, 4);
}

static bool isPass(const Code& code) {
  HandleScope scope;
  Bytes bytes(&scope, code->code());
  // const_loaded is the index into the consts array that is returned
  word const_loaded = bytes->byteAt(1);
  return bytes->length() == 4 && bytes->byteAt(0) == LOAD_CONST &&
         RawTuple::cast(code->consts()).at(const_loaded).isNoneType() &&
         bytes->byteAt(2) == RETURN_VALUE && bytes->byteAt(3) == 0;
}

static void patchFunctionAttrs(Thread* thread, const Function& base,
                               const Function& patch) {
  HandleScope scope(thread);
  Str method_name(&scope, patch->name());
  Code patch_code(&scope, patch->code());
  CHECK(isPass(patch_code),
        "Redefinition of native code method %s in managed code",
        method_name->toCStr());
  patch_code->setCode(NoneType::object());
  base->setCode(*patch_code);
  // The Python implementation will be used for all of its attributes, except
  // for its code.
  if (base->annotations()->isNoneType() &&
      !patch->annotations()->isNoneType()) {
    base->setAnnotations(patch->annotations());
  }
  if (base->defaults()->isNoneType() && !patch->defaults()->isNoneType()) {
    base->setDefaults(patch->defaults());
  }
  if (base->doc()->isNoneType() && !patch->doc()->isNoneType()) {
    base->setDoc(patch->doc());
  }
  if (base->kwDefaults()->isNoneType() && !patch->kwDefaults()->isNoneType()) {
    base->setKwDefaults(patch->kwDefaults());
  }
  if (base->qualname()->isNoneType() && !patch->qualname()->isNoneType()) {
    base->setQualname(patch->qualname());
  }
}

// Given a native implementation of a class method and an annotated
// counterpart, merge the two definitions. This is handy for allowing type
// annotations from declarations in managed code to supplement methods defined
// in the runtime.
void patchFunctionAttrsInTypeDict(Thread* thread, const Dict& type_dict,
                                  const Function& patch) {
  HandleScope scope(thread);
  Object patch_code_obj(&scope, patch->code());
  if (!patch_code_obj->isCode()) {
    return;
  }

  Str method_name(&scope, patch->name());
  Runtime* runtime = thread->runtime();
  Object base_obj(&scope, runtime->typeDictAt(type_dict, method_name));
  CHECK(base_obj->isFunction(),
        "Python annotation of non-function native object");
  Function base(&scope, *base_obj);

  // The Python implementation will be used only for its attributes, and
  // not for its code.
  if (base->annotations()->isNoneType() &&
      !patch->annotations()->isNoneType()) {
    base->setAnnotations(patch->annotations());
  }
  if (base->defaults()->isNoneType() && !patch->defaults()->isNoneType()) {
    base->setDefaults(patch->defaults());
  }
  if (base->doc()->isNoneType() && !patch->doc()->isNoneType()) {
    base->setDoc(patch->doc());
  }
  if (base->kwDefaults()->isNoneType() && !patch->kwDefaults()->isNoneType()) {
    base->setKwDefaults(patch->kwDefaults());
  }
  if (base->qualname()->isNoneType() && !patch->qualname()->isNoneType()) {
    base->setQualname(patch->qualname());
  }
  patchFunctionAttrs(thread, base, patch);
}

void patchTypeDict(Thread* thread, const Dict& base, const Dict& patch) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Tuple patch_data(&scope, patch->data());
  for (word i = Dict::Bucket::kFirst;
       Dict::Bucket::nextItem(*patch_data, &i);) {
    Str key(&scope, Dict::Bucket::key(*patch_data, i));
    Object patch_value_cell(&scope, Dict::Bucket::value(*patch_data, i));
    DCHECK(patch_value_cell->isValueCell(),
           "Values in type dict should be ValueCell");
    Object patch_obj(&scope, RawValueCell::cast(*patch_value_cell).value());

    if (runtime->dictIncludes(base, key)) {
      // Key is present in the base, so patch the base.
      CHECK(patch_obj->isFunction(), "Python should only annotate functions");
      Function patch_fn(&scope, *patch_obj);
      patchFunctionAttrsInTypeDict(thread, base, patch_fn);
    } else {
      // Key is not present in the base, so copy the value into the base.
      runtime->typeDictAtPut(base, key, patch_obj);
    }
  }
}

RawObject Builtins::buildClassKw(Thread* thread, Frame* frame, word nargs) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  KwArguments args(frame, nargs);
  if (args.numArgs() < 2) {
    return thread->raiseTypeErrorWithCStr("not enough args for build class.");
  }
  if (!args.get(0)->isFunction()) {
    return thread->raiseTypeErrorWithCStr("class body is not function.");
  }

  if (!args.get(1)->isStr()) {
    return thread->raiseTypeErrorWithCStr("class name is not string.");
  }

  Object bootstrap(&scope, args.getKw(runtime->symbols()->Bootstrap()));
  if (bootstrap->isError()) {
    bootstrap = Bool::falseObj();
  }

  Object metaclass(&scope, args.getKw(runtime->symbols()->Metaclass()));
  if (metaclass->isError()) {
    metaclass = runtime->typeAt(LayoutId::kType);
  }

  Tuple bases(&scope,
              runtime->newTuple(args.numArgs() - args.numKeywords() - 1));
  for (word i = 0, j = 2; j < args.numArgs(); i++, j++) {
    bases->atPut(i, args.get(j));
  }

  Dict type_dict(&scope, runtime->newDict());
  Function body(&scope, args.get(0));
  Str name(&scope, args.get(1));
  // TODO(T39559070): Add the __qualname__ to the type dict
  if (*bootstrap == Bool::falseObj()) {
    // An ordinary class initialization creates a new class dictionary.
    thread->runClassFunction(body, type_dict);
  } else {
    // A bootstrap class initialization uses the existing class dictionary.
    CHECK(frame->previousFrame() != nullptr, "must have a caller frame");
    Dict globals(&scope, frame->previousFrame()->globals());
    Object type_obj(&scope, runtime->moduleDictAt(globals, name));
    CHECK(type_obj->isType(),
          "Name '%s' is not bound to a type object. "
          "You may need to add it to the builtins module.",
          name->toCStr());
    Type type(&scope, *type_obj);
    type_dict = type->dict();

    Dict patch_type(&scope, runtime->newDict());
    thread->runClassFunction(body, patch_type);
    patchTypeDict(thread, type_dict, patch_type);
    // A bootstrap type initialization is complete at this point.
    return *type;
  }

  Type type(&scope, *metaclass);
  Function dunder_call(
      &scope, runtime->lookupSymbolInMro(thread, type, SymbolId::kDunderCall));
  frame->pushValue(*dunder_call);
  frame->pushValue(*type);
  frame->pushValue(*name);
  frame->pushValue(*bases);
  frame->pushValue(*type_dict);
  return Interpreter::call(thread, frame, 4);
}

RawObject Builtins::callable(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object arg(&scope, args.get(0));
  if (arg->isFunction() || arg->isBoundMethod() || arg->isType()) {
    return Bool::trueObj();
  }
  Runtime* runtime = thread->runtime();
  Type type(&scope, runtime->typeOf(*arg));
  // If its type defines a __call__, it is also callable (even if __call__ is
  // not actually callable).
  // Note that this does not include __call__ defined on the particular
  // instance, only __call__ defined on the type.
  Object callable(&scope, thread->runtime()->lookupSymbolInMro(
                              thread, type, SymbolId::kDunderCall));
  return Bool::fromBool(!callable->isError());
}

RawObject Builtins::chr(Thread* thread, Frame* frame_frame, word nargs) {
  Arguments args(frame_frame, nargs);
  RawObject arg = args.get(0);
  if (!arg->isSmallInt()) {
    return thread->raiseTypeErrorWithCStr("Unsupported type in builtin 'chr'");
  }
  word w = RawSmallInt::cast(arg)->value();
  const char s[2]{static_cast<char>(w), 0};
  return SmallStr::fromCStr(s);
}

static RawObject compileStr(Thread* thread, const Str& source) {
  HandleScope scope(thread);
  unique_c_ptr<char[]> source_str(source->toCStr());
  std::unique_ptr<char[]> bytecode_str(Runtime::compile(source_str.get()));
  source_str.reset();
  Marshal::Reader reader(&scope, thread->runtime(), bytecode_str.get());
  reader.readLong();  // magic
  reader.readLong();  // mtime
  reader.readLong();  // size
  return reader.readObject();
}

RawObject Builtins::compile(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  if (!args.get(0).isStr()) {
    return thread->raiseTypeErrorWithCStr(
        "compile() does not yet support non-str source");
  }
  Str source_str(&scope, args.get(0));
  Str filename(&scope, args.get(1));
  Str mode(&scope, args.get(2));
  // TODO(emacs): Refactor into sane argument-fetching code
  if (args.get(3) != SmallInt::fromWord(0)) {  // not the default
    return thread->raiseTypeErrorWithCStr(
        "compile() does not yet support user-supplied flags");
  }
  if (args.get(4) != Bool::falseObj()) {  // not the default
    return thread->raiseTypeErrorWithCStr(
        "compile() does not yet support user-supplied dont_inherit");
  }
  if (args.get(5) != SmallInt::fromWord(-1)) {  // not the default
    return thread->raiseTypeErrorWithCStr(
        "compile() does not yet support user-supplied optimize");
  }
  // Note: mode doesn't actually do anything yet.
  if (!mode->equalsCStr("exec") && !mode->equalsCStr("eval") &&
      !mode->equalsCStr("single")) {
    return thread->raiseValueErrorWithCStr(
        "Expected mode to be 'exec', 'eval', or 'single' in 'compile'");
  }

  Code code(&scope, compileStr(thread, source_str));
  code->setFilename(*filename);
  return *code;
}

RawObject Builtins::exec(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object source_obj(&scope, args.get(0));
  if (!source_obj->isCode() && !source_obj->isStr()) {
    return thread->raiseTypeErrorWithCStr(
        "Expected 'source' to be str or code in 'exec'");
  }
  // Per the docs:
  //   In all cases, if the optional parts are omitted, the code is executed in
  //   the current scope. If only globals is provided, it must be a dictionary,
  //   which will be used for both the global and the local variables.
  Object globals_obj(&scope, args.get(1));
  Object locals(&scope, args.get(2));
  Runtime* runtime = thread->runtime();
  if (globals_obj->isNoneType() &&
      locals->isNoneType()) {  // neither globals nor locals are provided
    Frame* caller_frame = frame->previousFrame();
    globals_obj = caller_frame->globals();
    DCHECK(globals_obj->isDict(),
           "Expected caller_frame->globals() to be dict in 'exec'");
    if (caller_frame->globals() != caller_frame->implicitGlobals()) {
      // TODO(T37888835): Fix 1 argument case
      // globals == implicitGlobals when code is being executed in a module
      // context. If we're not in a module context, this case is unimplemented.
      UNIMPLEMENTED("exec() with 1 argument not at the module level");
    }
    locals = *globals_obj;
  } else if (!globals_obj->isNoneType()) {  // only globals is provided
    if (!runtime->isInstanceOfDict(*globals_obj)) {
      return thread->raiseTypeErrorWithCStr(
          "Expected 'globals' to be dict in 'exec'");
    }
    locals = *globals_obj;
  } else {  // both globals and locals are provided
    if (!runtime->isInstanceOfDict(*globals_obj)) {
      return thread->raiseTypeErrorWithCStr(
          "Expected 'globals' to be dict in 'exec'");
    }
    if (!runtime->isMapping(thread, locals)) {
      return thread->raiseTypeErrorWithCStr(
          "Expected 'locals' to be a mapping in 'exec'");
    }
    // TODO(T37888835): Fix 3 argument case
    UNIMPLEMENTED("exec() with both globals and locals");
  }
  if (source_obj->isStr()) {
    Str source(&scope, *source_obj);
    source_obj = compileStr(thread, source);
    DCHECK(source_obj->isCode(), "compileStr must return code object");
  }
  Code code(&scope, *source_obj);
  if (code->numFreevars() != 0) {
    return thread->raiseTypeErrorWithCStr(
        "Expected 'source' not to have free variables in 'exec'");
  }
  Dict globals(&scope, *globals_obj);
  return thread->exec(code, globals, locals);
}

static RawObject isinstanceImpl(Thread* thread, const Object& obj,
                                const Object& type_obj) {
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  if (runtime->isInstanceOfType(*type_obj)) {
    Type type(&scope, *type_obj);
    return Bool::fromBool(runtime->isInstance(obj, type));
  }

  if (runtime->isInstanceOfTuple(*type_obj)) {
    Tuple types(&scope, *type_obj);
    Object elem(&scope, NoneType::object());
    Object result(&scope, NoneType::object());
    for (word i = 0, len = types->length(); i < len; i++) {
      elem = types->at(i);
      result = isinstanceImpl(thread, obj, elem);
      if (result->isError() || result == Bool::trueObj()) return *result;
    }
    return Bool::falseObj();
  }

  return thread->raiseTypeErrorWithCStr(
      "isinstance() arg 2 must be a type or tuple of types");
}

// TODO(mpage): isinstance (somewhat unsurprisingly at this point I guess) is
// actually far more complicated than one might expect. This is enough to get
// richards working.
RawObject Builtins::isinstance(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  Object type(&scope, args.get(1));
  return isinstanceImpl(thread, obj, type);
}

RawObject Builtins::issubclass(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (!args.get(0)->isType()) {
    return thread->raiseTypeErrorWithCStr("issubclass arg 1 must be a type");
  }
  Type type(&scope, args.get(0));
  Object classinfo(&scope, args.get(1));
  if (runtime->isInstanceOfType(*classinfo)) {
    Type possible_superclass(&scope, *classinfo);
    return Bool::fromBool(runtime->isSubclass(type, possible_superclass));
  }
  // If classinfo is not a tuple, then throw a TypeError.
  if (!classinfo->isTuple()) {
    return thread->raiseTypeErrorWithCStr(
        "issubclass() arg 2 must be a class of tuple of classes");
  }
  // If classinfo is a tuple, try each of the values, and return
  // True if the first argument is a subclass of any of them.
  Tuple tuple_of_types(&scope, *classinfo);
  for (word i = 0; i < tuple_of_types->length(); i++) {
    // If any argument is not a type, then throw TypeError.
    if (!runtime->isInstanceOfType(tuple_of_types->at(i))) {
      return thread->raiseTypeErrorWithCStr(
          "issubclass() arg 2 must be a class of tuple of classes");
    }
    Type possible_superclass(&scope, tuple_of_types->at(i));
    // If any of the types are a superclass, return true.
    if (runtime->isSubclass(type, possible_superclass)) return Bool::trueObj();
  }
  // None of the types in the tuple were a superclass, so return false.
  return Bool::falseObj();
}

RawObject Builtins::ord(Thread* thread, Frame* frame_frame, word nargs) {
  Arguments args(frame_frame, nargs);
  RawObject arg = args.get(0);
  if (!arg->isStr()) {
    return thread->raiseTypeErrorWithCStr("Unsupported type in builtin 'ord'");
  }
  auto str = RawStr::cast(arg);
  if (str->length() != 1) {
    return thread->raiseTypeErrorWithCStr(
        "Builtin 'ord' expects string of length 1");
  }
  return SmallInt::fromWord(str->charAt(0));
}

void printStr(RawStr str, std::ostream* ostream) {
  for (word i = 0; i < str->length(); i++) {
    *ostream << str->charAt(i);
  }
}

RawObject Builtins::underPrintStr(Thread* thread, Frame* frame_frame,
                                  word nargs) {
  Arguments args(frame_frame, nargs);
  HandleScope scope(thread);
  CHECK(args.get(0).isStr(), "Unsupported argument type for 'obj'");
  Str str(&scope, args.get(0));
  CHECK(args.get(1).isSmallInt(), "Unsupported argument type for 'file'");
  word fileno = RawSmallInt::cast(args.get(1)).value();
  if (fileno == STDOUT_FILENO) {
    printStr(*str, builtInStdout);
  } else if (fileno == STDERR_FILENO) {
    printStr(*str, builtinStderr);
  } else {
    return thread->raiseTypeErrorWithCStr(
        "Unsupported argument type for 'file'");
  }
  return NoneType::object();
}

// TODO(T39322942): Turn this into the Range constructor (__init__ or __new__)
RawObject Builtins::range(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  for (word i = 0; i < nargs; i++) {
    if (!args.get(i)->isSmallInt() && !args.get(i).isUnboundValue()) {
      return thread->raiseTypeErrorWithCStr(
          "Arguments to range() must be integers");
    }
  }

  word start = 0;
  word stop = 0;
  word step = 1;
  if (args.get(1).isUnboundValue() && args.get(2).isUnboundValue()) {
    stop = RawSmallInt::cast(args.get(0))->value();
  } else if (args.get(2).isUnboundValue()) {
    start = RawSmallInt::cast(args.get(0))->value();
    stop = RawSmallInt::cast(args.get(1))->value();
  } else {
    start = RawSmallInt::cast(args.get(0))->value();
    stop = RawSmallInt::cast(args.get(1))->value();
    step = RawSmallInt::cast(args.get(2))->value();
  }

  if (step == 0) {
    return thread->raiseValueErrorWithCStr(
        "range() step argument must not be zero");
  }

  return thread->runtime()->newRange(start, stop, step);
}

RawObject Builtins::repr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);

  HandleScope scope(thread);
  Object obj(&scope, args.get(0));
  // Only one argument, the value to be repr'ed.
  Object method(&scope, Interpreter::lookupMethod(thread, frame, obj,
                                                  SymbolId::kDunderRepr));
  CHECK(!method->isError(),
        "__repr__ doesn't exist for this object, which is impossible since "
        "object has a __repr__, and everything descends from object");
  RawObject ret = Interpreter::callMethod1(thread, frame, method, obj);
  if (!ret->isStr() && !ret->isError()) {
    // TODO(T31744782): Change this to allow subtypes of string.
    // If __repr__ doesn't return a string or error, throw a type error
    return thread->raiseTypeErrorWithCStr("__repr__ returned non-string");
  }
  return ret;
}

RawObject Builtins::getattr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object default_obj(&scope, args.get(2));
  Object result(&scope, getAttribute(thread, self, name));
  Runtime* runtime = thread->runtime();
  if (result->isError() && !default_obj.isUnboundValue()) {
    Type given(&scope, thread->pendingExceptionType());
    Type exc(&scope, runtime->typeAt(LayoutId::kAttributeError));
    if (givenExceptionMatches(thread, given, exc)) {
      thread->clearPendingException();
      result = *default_obj;
    }
  }
  return *result;
}

RawObject Builtins::hasattr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  return hasAttribute(thread, self, name);
}

RawObject Builtins::setattr(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Object name(&scope, args.get(1));
  Object value(&scope, args.get(2));
  return setAttribute(thread, self, name, value);
}

RawObject Builtins::underAddress(Thread* thread, Frame* frame, word nargs) {
  Arguments args(frame, nargs);
  return thread->runtime()->newInt(args.get(0).raw());
}

RawObject Builtins::underPatch(Thread* thread, Frame* frame, word nargs) {
  HandleScope scope(thread);
  Arguments args(frame, nargs);
  if (nargs != 1) {
    return thread->raiseTypeErrorWithCStr("_patch expects 1 argument");
  }

  Object patch_fn_obj(&scope, args.get(0));
  if (!patch_fn_obj->isFunction()) {
    return thread->raiseTypeErrorWithCStr("_patch expects function argument");
  }
  Function patch_fn(&scope, *patch_fn_obj);
  Str fn_name(&scope, patch_fn->name());
  Runtime* runtime = thread->runtime();
  Object module_name(&scope, patch_fn->module());
  Module module(&scope, runtime->findModule(module_name));
  Object base_fn_obj(&scope, runtime->moduleAt(module, fn_name));
  if (!base_fn_obj->isFunction()) {
    return thread->raiseTypeErrorWithCStr("_patch can only patch functions");
  }
  Function base_fn(&scope, *base_fn_obj);
  patchFunctionAttrs(thread, base_fn, patch_fn);
  return *base_fn;
}

}  // namespace python
