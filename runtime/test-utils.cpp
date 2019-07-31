#include "test-utils.h"

#include <cmath>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

#include "builtins-module.h"
#include "bytearray-builtins.h"
#include "bytes-builtins.h"
#include "compile.h"
#include "debugging.h"
#include "exception-builtins.h"
#include "frame.h"
#include "handles.h"
#include "int-builtins.h"
#include "os.h"
#include "runtime.h"
#include "str-builtins.h"
#include "sys-module.h"
#include "thread.h"
#include "utils.h"

namespace python {

namespace testing {

Value::Type Value::type() const { return type_; }

bool Value::boolVal() const {
  DCHECK(type() == Type::Bool, "expected bool");
  return bool_;
}

word Value::intVal() const {
  DCHECK(type() == Type::Int, "expected int");
  return int_;
}

double Value::floatVal() const {
  DCHECK(type() == Type::Float, "expected float");
  return float_;
}

const char* Value::strVal() const {
  DCHECK(type() == Type::Str, "expected str");
  return str_;
}

template <typename T1, typename T2>
::testing::AssertionResult badListValue(const char* actual_expr, word i,
                                        const T1& actual, const T2& expected) {
  return ::testing::AssertionFailure()
         << "Value of: " << actual_expr << '[' << i << "]\n"
         << "  Actual: " << actual << '\n'
         << "Expected: " << expected;
}

::testing::AssertionResult AssertPyListEqual(
    const char* actual_expr, const char* /* expected_expr */,
    const Object& actual, const std::vector<Value>& expected) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();

  if (!actual.isList()) {
    return ::testing::AssertionFailure()
           << " Type of: " << actual_expr << "\n"
           << "  Actual: " << typeName(runtime, *actual) << "\n"
           << "Expected: list";
  }

  HandleScope scope(thread);
  List list(&scope, *actual);
  if (static_cast<size_t>(list.numItems()) != expected.size()) {
    return ::testing::AssertionFailure()
           << "Length of: " << actual_expr << "\n"
           << "   Actual: " << list.numItems() << "\n"
           << " Expected: " << expected.size();
  }

  for (size_t i = 0; i < expected.size(); i++) {
    Object actual_item(&scope, list.at(i));
    const Value& expected_item = expected[i];

    auto bad_type = [&](const char* expected_type) {
      return ::testing::AssertionFailure()
             << " Type of: " << actual_expr << '[' << i << "]\n"
             << "  Actual: " << typeName(runtime, *actual_item) << '\n'
             << "Expected: " << expected_type;
    };

    switch (expected_item.type()) {
      case Value::Type::None: {
        if (!actual_item.isNoneType()) return bad_type("RawNoneType");
        break;
      }

      case Value::Type::Bool: {
        if (!actual_item.isBool()) return bad_type("bool");
        auto const actual_val = Bool::cast(*actual_item) == Bool::trueObj();
        auto const expected_val = expected_item.boolVal();
        if (actual_val != expected_val) {
          return badListValue(actual_expr, i, actual_val ? "True" : "False",
                              expected_val ? "True" : "False");
        }
        break;
      }

      case Value::Type::Int: {
        if (!actual_item.isInt()) return bad_type("int");
        Int actual_val(&scope, *actual_item);
        Int expected_val(&scope, runtime->newInt(expected_item.intVal()));
        if (actual_val.compare(*expected_val) != 0) {
          // TODO(bsimmers): Support multi-digit values when we can print them.
          return badListValue(actual_expr, i, actual_val.digitAt(0),
                              expected_item.intVal());
        }
        break;
      }

      case Value::Type::Float: {
        if (!actual_item.isFloat()) return bad_type("float");
        auto const actual_val = Float::cast(*actual_item).value();
        auto const expected_val = expected_item.floatVal();
        if (std::abs(actual_val - expected_val) >= DBL_EPSILON) {
          return badListValue(actual_expr, i, actual_val, expected_val);
        }
        break;
      }

      case Value::Type::Str: {
        if (!actual_item.isStr()) return bad_type("str");
        Str actual_val(&scope, *actual_item);
        const char* expected_val = expected_item.strVal();
        if (!actual_val.equalsCStr(expected_val)) {
          return badListValue(actual_expr, i, actual_val, expected_val);
        }
        break;
      }
    }
  }

  return ::testing::AssertionSuccess();
}

RawObject callFunction(const Function& func, const Tuple& args) {
  Thread* thread = Thread::current();
  Frame* frame = thread->currentFrame();
  frame->pushValue(*func);
  word args_length = args.length();
  for (word i = 0; i < args_length; i++) {
    frame->pushValue(args.at(i));
  }
  return Interpreter::call(thread, frame, args_length);
}

bool tupleContains(const Tuple& object_array, const Object& key) {
  for (word i = 0; i < object_array.length(); i++) {
    if (Object::equals(object_array.at(i), *key)) {
      return true;
    }
  }
  return false;
}

bool listContains(const Object& list_obj, const Object& key) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  if (!thread->runtime()->isInstanceOfList(*list_obj)) {
    return false;
  }
  List list(&scope, *list_obj);
  for (word i = 0, num_items = list.numItems(); i < num_items; i++) {
    if (Object::equals(list.at(i), *key)) {
      return true;
    }
  }
  return false;
}

RawObject findModule(Runtime* runtime, const char* name) {
  HandleScope scope;
  Object key(&scope, runtime->newStrFromCStr(name));
  return runtime->findModule(key);
}

RawObject moduleAt(Runtime* runtime, const Module& module, const char* name) {
  HandleScope scope;
  Object key(&scope, runtime->newStrFromCStr(name));
  return runtime->moduleAt(module, key);
}

RawObject moduleAt(Runtime* runtime, const char* module_name,
                   const char* name) {
  HandleScope scope;
  Object mod_obj(&scope, findModule(runtime, module_name));
  if (mod_obj.isNoneType()) {
    return Error::notFound();
  }
  Module module(&scope, *mod_obj);
  return moduleAt(runtime, module, name);
}

std::string typeName(Runtime* runtime, RawObject obj) {
  if (obj.layoutId() == LayoutId::kError) return "Error";
  RawStr name = Str::cast(Type::cast(runtime->typeOf(obj)).name());
  word length = name.length();
  std::string result(length, '\0');
  name.copyTo(reinterpret_cast<byte*>(&result[0]), length);
  return result;
}

RawCode newEmptyCode() {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Bytes code(&scope, runtime->newBytes(0, 0));
  Tuple empty_tuple(&scope, runtime->emptyTuple());
  Object empty_string(&scope, Str::empty());
  Object empty_bytes(&scope, Bytes::empty());
  word flags = Code::Flags::OPTIMIZED | Code::Flags::NEWLOCALS;
  return Code::cast(runtime->newCode(/*argcount=*/0, /*posonlyargcount=*/0,
                                     /*kwonlyargcount=*/0, /*nlocals=*/0,
                                     /*stacksize=*/0, flags, code,
                                     /*consts=*/empty_tuple,
                                     /*names=*/empty_tuple,
                                     /*varnames=*/empty_tuple,
                                     /*freevars=*/empty_tuple,
                                     /*cellvars=*/empty_tuple,
                                     /*filename=*/empty_string,
                                     /*name=*/empty_string,
                                     /*firstlineno=*/0,
                                     /*lnotab=*/empty_bytes));
}

RawFunction newEmptyFunction() {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Code code(&scope, newEmptyCode());
  Object qualname(&scope, Str::empty());
  Module main(&scope, runtime->findOrCreateMainModule());
  Dict globals(&scope, main.dict());
  return Function::cast(
      runtime->newFunctionWithCode(thread, qualname, code, globals));
}

RawObject newIntWithDigits(Runtime* runtime, View<uword> digits) {
  return runtime->newIntWithDigits(View<uword>(digits.data(), digits.length()));
}

RawLargeInt newLargeIntWithDigits(View<uword> digits) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  LargeInt result(&scope,
                  thread->runtime()->heap()->createLargeInt(digits.length()));
  for (word i = 0, e = digits.length(); i < e; ++i) {
    result.digitAtPut(i, digits.get(i));
  }
  return *result;
}

RawObject newMemoryView(View<byte> bytes, const char* format,
                        ReadOnly read_only) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Bytes bytes_obj(&scope, runtime->newBytesWithAll(bytes));
  MemoryView result(
      &scope,
      runtime->newMemoryView(thread, bytes_obj, bytes_obj.length(), read_only));
  result.setFormat(Str::cast(runtime->newStrFromCStr(format)));
  return *result;
}

RawObject setFromRange(word start, word stop) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Set result(&scope, thread->runtime()->newSet());
  Object value(&scope, NoneType::object());
  for (word i = start; i < stop; i++) {
    value = SmallInt::fromWord(i);
    thread->runtime()->setAdd(thread, result, value);
  }
  return *result;
}

RawObject runBuiltinImpl(NativeMethodType method,
                         View<std::reference_wrapper<const Object>> args) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  // Push an empty function so we have one at the expected place in the stack.
  word args_length = args.length();
  Runtime* runtime = thread->runtime();
  Tuple parameter_names(&scope, runtime->newTuple(args_length));
  for (word i = 0; i < args_length; i++) {
    parameter_names.atPut(i, runtime->newStrFromFmt("arg%w", i));
  }

  Object name(&scope, runtime->symbols()->Anonymous());
  Code code(&scope, runtime->newBuiltinCode(args_length, /*posonlyargcount=*/0,
                                            /*kwonlyargcount=*/0,
                                            /*flags=*/0, method,
                                            parameter_names, name));
  Module main(&scope, runtime->findOrCreateMainModule());
  Dict globals(&scope, main.dict());
  Function function(&scope,
                    runtime->newFunctionWithCode(thread, name, code, globals));

  Frame* frame = thread->currentFrame();
  frame->pushValue(*function);
  for (word i = 0; i < args_length; i++) {
    frame->pushValue(*args.get(i).get());
  }
  return Interpreter::call(thread, frame, args_length);
}

RawObject runBuiltin(NativeMethodType method) {
  return runBuiltinImpl(method,
                        View<std::reference_wrapper<const Object>>{nullptr, 0});
}

RawObject runCode(const Code& code) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Module main(&scope, runtime->findOrCreateMainModule());
  Object qualname(&scope, runtime->symbols()->Anonymous());
  Dict globals(&scope, main.dict());
  Function function(
      &scope, runtime->newFunctionWithCode(thread, qualname, code, globals));
  return Interpreter::callFunction0(thread, thread->currentFrame(), function);
}

RawObject runFromCStr(Runtime* runtime, const char* c_str) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Code code(&scope, compileFromCStr(c_str, "<test string>"));
  Module main_module(&scope, runtime->findOrCreateMainModule());
  Object result(&scope, runtime->executeModule(code, main_module));

  // Barebones emulation of the top-level SystemExit handling, to allow for
  // testing of handleSystemExit().
  DCHECK(thread->isErrorValueOk(*result), "error/exception mismatch");
  if (result.isError()) {
    Type type(&scope, thread->pendingExceptionType());
    if (type.builtinBase() == LayoutId::kSystemExit) handleSystemExit(thread);
  }
  return *result;
}

// Equivalent to evaluating "list(range(start, stop))" in Python
RawObject listFromRange(word start, word stop) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  List result(&scope, thread->runtime()->newList());
  Object value(&scope, NoneType::object());
  for (word i = start; i < stop; i++) {
    value = SmallInt::fromWord(i);
    thread->runtime()->listAdd(thread, result, value);
  }
  return *result;
}

::testing::AssertionResult isByteArrayEqualsBytes(const Object& result,
                                                  View<byte> expected) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (result.isError()) {
    if (result.isErrorException()) {
      Type type(&scope, thread->pendingExceptionType());
      unique_c_ptr<char> name(Str::cast(type.name()).toCStr());
      return ::testing::AssertionFailure()
             << "pending '" << name.get() << "' exception";
    }
    return ::testing::AssertionFailure() << "is an " << result;
  }
  if (!runtime->isInstanceOfByteArray(*result)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, *result) << "'";
  }
  ByteArray result_array(&scope, *result);
  Bytes result_bytes(&scope, byteArrayAsBytes(thread, runtime, result_array));
  Bytes expected_bytes(&scope, runtime->newBytesWithAll(expected));
  if (result_bytes.compare(*expected_bytes) != 0) {
    return ::testing::AssertionFailure()
           << "bytearray(" << result_bytes << ") is not equal to bytearray("
           << expected_bytes << ")";
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult isByteArrayEqualsCStr(const Object& result,
                                                 const char* expected) {
  return isByteArrayEqualsBytes(
      result, View<byte>(reinterpret_cast<const byte*>(expected),
                         static_cast<word>(std::strlen(expected))));
}

::testing::AssertionResult isBytesEqualsBytes(const Object& result,
                                              View<byte> expected) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (result.isError()) {
    if (result.isErrorException()) {
      Type type(&scope, thread->pendingExceptionType());
      unique_c_ptr<char> name(Str::cast(type.name()).toCStr());
      return ::testing::AssertionFailure()
             << "pending '" << name.get() << "' exception";
    }
    return ::testing::AssertionFailure() << "is an " << result;
  }
  if (!runtime->isInstanceOfBytes(*result)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, *result) << "'";
  }
  Bytes result_bytes(&scope, bytesUnderlying(thread, result));
  Bytes expected_bytes(&scope, runtime->newBytesWithAll(expected));
  if (result_bytes.compare(*expected_bytes) != 0) {
    return ::testing::AssertionFailure()
           << result_bytes << " is not equal to " << expected_bytes;
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult isMutableBytesEqualsBytes(const Object& result,
                                                     View<byte> expected) {
  if (!result.isError() && !result.isMutableBytes()) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(Thread::current()->runtime(), *result)
           << "'";
  }
  return isBytesEqualsBytes(result, expected);
}

::testing::AssertionResult isBytesEqualsCStr(const Object& result,
                                             const char* expected) {
  return isBytesEqualsBytes(
      result, View<byte>(reinterpret_cast<const byte*>(expected),
                         static_cast<word>(std::strlen(expected))));
}

::testing::AssertionResult isStrEquals(const Object& str1, const Object& str2) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*str1)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, *str1) << "'";
  }
  if (!runtime->isInstanceOfStr(*str2)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, *str2) << "'";
  }
  Str s1(&scope, *str1);
  if (!s1.equals(*str2)) {
    Str s2(&scope, *str2);
    unique_c_ptr<char> s2_ptr(s2.toCStr());
    return ::testing::AssertionFailure()
           << "is not equal to '" << s2_ptr.get() << "'";
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult isStrEqualsCStr(RawObject obj, const char* c_str) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object str_obj(&scope, obj);
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*str_obj)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, *str_obj) << "'";
  }
  Str str(&scope, strUnderlying(thread, str_obj));
  if (!str.equalsCStr(c_str)) {
    unique_c_ptr<char> str_ptr(str.toCStr());
    return ::testing::AssertionFailure()
           << "'" << str_ptr.get() << "' is not equal to '" << c_str << "'";
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult isSymbolIdEquals(SymbolId result,
                                            SymbolId expected) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  if (result == expected) return ::testing::AssertionSuccess();
  const char* result_name =
      result == SymbolId::kInvalid
          ? "<Invalid>"
          : runtime->symbols()->predefinedSymbolAt(result);
  return ::testing::AssertionFailure()
         << "Expected '" << runtime->symbols()->predefinedSymbolAt(expected)
         << "', but got '" << result_name << "'";
}

::testing::AssertionResult isIntEqualsWord(RawObject obj, word value) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (obj.isError()) {
    if (obj.isErrorException()) {
      Type type(&scope, thread->pendingExceptionType());
      Str type_name(&scope, type.name());
      return ::testing::AssertionFailure()
             << "pending " << type_name << " exception";
    }
    return ::testing::AssertionFailure() << "is an " << obj;
  }
  if (!runtime->isInstanceOfInt(obj)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, obj) << "'";
  }
  Object object(&scope, obj);
  Int value_int(&scope, intUnderlying(thread, object));
  if (value_int.numDigits() > 1 || value_int.asWord() != value) {
    return ::testing::AssertionFailure()
           << value_int << " is not equal to " << value;
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult isIntEqualsDigits(RawObject obj,
                                             View<uword> digits) {
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (obj.isError()) {
    if (obj.isErrorException()) {
      Type type(&scope, thread->pendingExceptionType());
      Str type_name(&scope, type.name());
      return ::testing::AssertionFailure()
             << "pending " << type_name << " exception";
    }
    return ::testing::AssertionFailure() << "is an " << obj;
  }
  if (!runtime->isInstanceOfInt(obj)) {
    return ::testing::AssertionFailure()
           << "is a '" << typeName(runtime, obj) << "'";
  }
  Int expected(&scope, newIntWithDigits(runtime, digits));
  Object value_obj(&scope, obj);
  Int value_int(&scope, intUnderlying(thread, value_obj));
  if (expected.compare(*value_int) != 0) {
    return ::testing::AssertionFailure()
           << value_int << " is not equal to " << expected;
  }
  return ::testing::AssertionSuccess();
}

::testing::AssertionResult raised(RawObject return_value, LayoutId layout_id) {
  return raisedWithStr(return_value, layout_id, nullptr);
}

::testing::AssertionResult raisedWithStr(RawObject return_value,
                                         LayoutId layout_id,
                                         const char* message) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);
  Object return_value_obj(&scope, return_value);

  if (!return_value_obj.isError()) {
    Type type(&scope, runtime->typeOf(*return_value_obj));
    Str name(&scope, type.name());
    return ::testing::AssertionFailure()
           << "call returned " << name << ", not Error";
  }

  if (!thread->hasPendingException()) {
    return ::testing::AssertionFailure() << "no exception pending";
  }

  Type expected_type(&scope, runtime->typeAt(layout_id));
  Type exception_type(&scope, thread->pendingExceptionType());
  if (!runtime->isSubclass(exception_type, expected_type)) {
    Str expected_name(&scope, expected_type.name());
    Str actual_name(&scope, exception_type.name());
    return ::testing::AssertionFailure()
           << "\npending exception has type:\n  " << actual_name
           << "\nexpected:\n  " << expected_name << "\n";
  }

  if (message == nullptr) return ::testing::AssertionSuccess();

  Object exc_value(&scope, thread->pendingExceptionValue());
  if (!runtime->isInstanceOfStr(*exc_value)) {
    if (runtime->isInstanceOfBaseException(*exc_value)) {
      BaseException exc(&scope, *exc_value);
      Tuple args(&scope, exc.args());
      if (args.length() == 0) {
        return ::testing::AssertionFailure()
               << "pending exception args tuple is empty";
      }
      exc_value = args.at(0);
    }

    if (!runtime->isInstanceOfStr(*exc_value)) {
      return ::testing::AssertionFailure()
             << "pending exception value is not str";
    }
  }

  Str exc_msg(&scope, *exc_value);
  if (!exc_msg.equalsCStr(message)) {
    return ::testing::AssertionFailure()
           << "\npending exception value:\n  '" << exc_msg
           << "'\nexpected:\n  '" << message << "'\n";
  }

  return ::testing::AssertionSuccess();
}

TemporaryDirectory::TemporaryDirectory() {
  std::unique_ptr<char[]> tempdir(OS::temporaryDirectory("PyroTest"));
  path = tempdir.get();
  CHECK(!path.empty(), "must not be empty");
  if (path.back() != '/') path += "/";
}

TemporaryDirectory::~TemporaryDirectory() {
  std::string cleanup = "rm -rf " + path;
  CHECK(system(cleanup.c_str()) == 0, "failed to cleanup temporary directory");
}

void writeFile(const std::string& path, const std::string& contents) {
  CHECK(!path.empty() && path.front() == '/', "Should be an absolute path");
  std::ofstream of(path);
  CHECK(of.good(), "file creation failed");
  of << contents;
  CHECK(of.good(), "file write failed");
  of.close();
}

}  // namespace testing
}  // namespace python
