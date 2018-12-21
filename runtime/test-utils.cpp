#include "test-utils.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>

#include "builtins-module.h"
#include "frame.h"
#include "handles.h"
#include "os.h"
#include "runtime.h"
#include "siphash.h"
#include "thread.h"
#include "utils.h"

namespace python {

std::ostream& operator<<(std::ostream& os, const Str& str) {
  char* data = str->toCStr();
  os.write(data, str->length());
  std::free(data);
  return os;
}

std::ostream& operator<<(std::ostream& os, CastError err) {
  switch (err) {
    case CastError::None:
      return os << "None";
    case CastError::Underflow:
      return os << "Underflow";
    case CastError::Overflow:
      return os << "Overflow";
  }
  return os << "<invalid>";
}

namespace testing {

// Turn a Python string object into a standard string.
static std::string pyStringToStdString(RawStr pystr) {
  std::string std_str;
  std_str.reserve(pystr->length());
  for (word i = 0; i < pystr->length(); i++) {
    std_str += pystr->charAt(i);
  }
  return std_str;
}

::testing::AssertionResult AssertPyStringEqual(const char* actual_string_expr,
                                               const char*, RawStr actual_str,
                                               std::string expected_string) {
  if (actual_str->equalsCStr(expected_string.c_str())) {
    return ::testing::AssertionSuccess();
  }

  return ::testing::AssertionFailure()
         << "      Expected: " << actual_string_expr << "\n"
         << "      Which is: \"" << pyStringToStdString(actual_str) << "\"\n"
         << "To be equal to: \"" << expected_string << "\"";
}

::testing::AssertionResult AssertPyStringEqual(const char* actual_string_expr,
                                               const char* expected_string_expr,
                                               RawStr actual_str,
                                               RawStr expected_str) {
  if (actual_str->equals(expected_str)) {
    return ::testing::AssertionSuccess();
  }

  return ::testing::AssertionFailure()
         << "      Expected: " << actual_string_expr << "\n"
         << "      Which is: \"" << pyStringToStdString(actual_str) << "\"\n"
         << "To be equal to: \"" << expected_string_expr << "\"\n"
         << "      Which is: \"" << pyStringToStdString(expected_str) << "\"";
}

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
  Thread* thread = Thread::currentThread();
  Runtime* runtime = thread->runtime();

  if (!actual->isList()) {
    return ::testing::AssertionFailure()
           << " Type of: " << actual_expr << "\n"
           << "  Actual: " << typeName(runtime, *actual) << "\n"
           << "Expected: list";
  }

  HandleScope scope(thread);
  List list(&scope, *actual);
  if (static_cast<size_t>(list->numItems()) != expected.size()) {
    return ::testing::AssertionFailure()
           << "Length of: " << actual_expr << "\n"
           << "   Actual: " << list->numItems() << "\n"
           << " Expected: " << expected.size();
  }

  for (size_t i = 0; i < expected.size(); i++) {
    Object actual_item(&scope, list->at(i));
    const Value& expected_item = expected[i];

    auto bad_type = [&](const char* expected_type) {
      return ::testing::AssertionFailure()
             << " Type of: " << actual_expr << '[' << i << "]\n"
             << "  Actual: " << typeName(runtime, *actual_item) << '\n'
             << "Expected: " << expected_type;
    };

    switch (expected_item.type()) {
      case Value::Type::None: {
        if (!actual_item->isNoneType()) return bad_type("RawNoneType");
        break;
      }

      case Value::Type::Bool: {
        if (!actual_item->isBool()) return bad_type("bool");
        auto const actual_val = RawBool::cast(*actual_item) == Bool::trueObj();
        auto const expected_val = expected_item.boolVal();
        if (actual_val != expected_val) {
          return badListValue(actual_expr, i, actual_val ? "True" : "False",
                              expected_val ? "True" : "False");
        }
        break;
      }

      case Value::Type::Int: {
        if (!actual_item->isInt()) return bad_type("int");
        Int actual_val(&scope, *actual_item);
        Int expected_val(&scope, runtime->newInt(expected_item.intVal()));
        if (actual_val->compare(*expected_val) != 0) {
          // TODO(bsimmers): Support multi-digit values when we can print them.
          return badListValue(actual_expr, i, actual_val->digitAt(0),
                              expected_item.intVal());
        }
        break;
      }

      case Value::Type::Float: {
        if (!actual_item->isFloat()) return bad_type("float");
        auto const actual_val = RawFloat::cast(*actual_item)->value();
        auto const expected_val = expected_item.floatVal();
        if (std::abs(actual_val - expected_val) >= DBL_EPSILON) {
          return badListValue(actual_expr, i, actual_val, expected_val);
        }
        break;
      }

      case Value::Type::Str: {
        if (!actual_item->isStr()) return bad_type("str");
        Str actual_val(&scope, *actual_item);
        const char* expected_val = expected_item.strVal();
        if (!actual_val->equalsCStr(expected_val)) {
          return badListValue(actual_expr, i, actual_val, expected_val);
        }
        break;
      }
    }
  }

  return ::testing::AssertionSuccess();
}

// helper function to redirect return stdout/stderr from running a module
static std::string compileAndRunImpl(Runtime* runtime, const char* src,
                                     std::ostream** ostream) {
  std::stringstream tmp_ostream;
  std::ostream* saved_ostream = *ostream;
  *ostream = &tmp_ostream;
  RawObject result = runFromCStr(runtime, src);
  CHECK(result == NoneType::object(), "unexpected result");
  *ostream = saved_ostream;
  return tmp_ostream.str();
}

std::string compileAndRunToString(Runtime* runtime, const char* src) {
  return compileAndRunImpl(runtime, src, &builtInStdout);
}

std::string compileAndRunToStderrString(Runtime* runtime, const char* src) {
  return compileAndRunImpl(runtime, src, &builtinStderr);
}

std::string callFunctionToString(const Function& func, const Tuple& args) {
  std::stringstream stream;
  std::ostream* old_stream = builtInStdout;
  builtInStdout = &stream;
  Thread* thread = Thread::currentThread();
  thread->pushNativeFrame(bit_cast<void*>(&callFunctionToString), 0);
  callFunction(func, args);
  thread->popFrame();
  builtInStdout = old_stream;
  return stream.str();
}

RawObject callFunction(const Function& func, const Tuple& args) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Code code(&scope, func->code());
  Frame* frame =
      thread->pushNativeFrame(bit_cast<void*>(&callFunction), args->length());
  frame->pushValue(*func);
  for (word i = 0; i < args->length(); i++) {
    frame->pushValue(args->at(i));
  }
  Object result(&scope, func->entry()(thread, frame, code->argcount()));
  thread->popFrame();
  return *result;
}

bool tupleContains(const Tuple& object_array, const Object& key) {
  for (word i = 0; i < object_array->length(); i++) {
    if (Object::equals(object_array->at(i), *key)) {
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
  if (mod_obj->isNoneType()) {
    return Error::object();
  }
  Module module(&scope, *mod_obj);
  return moduleAt(runtime, module, name);
}

std::string typeName(Runtime* runtime, RawObject obj) {
  RawStr name = RawStr::cast(RawType::cast(runtime->typeOf(obj))->name());
  word length = name->length();
  std::string result(length, '\0');
  name->copyTo(reinterpret_cast<byte*>(&result[0]), length);
  return result;
}

RawObject newIntWithDigits(Runtime* runtime, const std::vector<uword>& digits) {
  return runtime->newIntWithDigits(View<uword>(digits.data(), digits.size()));
}

RawLargeInt newLargeIntWithDigits(const std::vector<uword>& digits) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  LargeInt result(&scope,
                  thread->runtime()->heap()->createLargeInt(digits.size()));
  for (word i = 0, e = digits.size(); i < e; ++i) {
    result.digitAtPut(i, digits[i]);
  }
  return *result;
}

RawObject setFromRange(word start, word stop) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Set result(&scope, thread->runtime()->newSet());
  Object value(&scope, NoneType::object());
  for (word i = start; i < stop; i++) {
    value = SmallInt::fromWord(i);
    thread->runtime()->setAdd(result, value);
  }
  return *result;
}

RawObject runBuiltinImpl(BuiltinMethodType method,
                         View<std::reference_wrapper<const Object>> args) {
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, args.length(), 0);
  for (word i = 0; i < args.length(); i++) {
    frame->setLocal(i, *args.get(i).get());
  }
  HandleScope scope(thread);
  Object result(&scope, method(thread, frame, args.length()));
  thread->popFrame();
  return *result;
}

RawObject runBuiltin(BuiltinMethodType method) {
  return runBuiltinImpl(method,
                        View<std::reference_wrapper<const Object>>{nullptr, 0});
}

RawObject newEmptyCode(Runtime* runtime) {
  HandleScope scope;
  Object none(&scope, NoneType::object());
  Tuple empty_tuple(&scope, runtime->newTuple(0));
  return runtime->newCode(0,            // argcount
                          0,            // kwonlyargcount
                          0,            // nlocals
                          0,            // stacksize
                          0,            // flags
                          none,         // code
                          none,         // consts
                          none,         // names
                          empty_tuple,  // varnames
                          empty_tuple,  // freevars
                          empty_tuple,  // cellvars
                          none,         // filename
                          none,         // name
                          0,            // firlineno
                          none          // lnotab
  );
}

static std::unique_ptr<char[]> compileWithCache(const char* src) {
  // increment this if you change the caching code, to invalidate existing
  // cache entries.
  uint64_t seed[2] = {0, 1};
  word hash = 0;

  // Hash the input.
  ::siphash(reinterpret_cast<const uint8_t*>(src), strlen(src),
            reinterpret_cast<const uint8_t*>(seed),
            reinterpret_cast<uint8_t*>(&hash), sizeof(hash));

  const char* cache_env = OS::getenv("PYRO_CACHE_DIR");
  std::string cache_dir;
  if (cache_env != nullptr) {
    cache_dir = cache_env;
  } else {
    const char* home_env = OS::getenv("HOME");
    if (home_env != nullptr) {
      cache_dir = home_env;
      cache_dir += "/.pyro-compile-cache";
    }
  }

  char filename_buf[512] = {};
  snprintf(filename_buf, 512, "%s/%016zx", cache_dir.c_str(), hash);

  // Read compiled code from the cache
  if (!cache_dir.empty() && OS::fileExists(filename_buf)) {
    return std::unique_ptr<char[]>(OS::readFile(filename_buf));
  }

  // Cache miss, must run the compiler.
  word len;
  char* result = Runtime::compileWithLen(src, &len);

  // Cache the output if possible.
  if (!cache_dir.empty() && OS::dirExists(cache_dir.c_str())) {
    OS::writeFileExcl(filename_buf, result, len);
  }

  return std::unique_ptr<char[]>(result);
}

RawObject runFromCStr(Runtime* runtime, const char* c_str) {
  return runtime->run(compileWithCache(c_str).get());
}

// Equivalent to evaluating "list(range(start, stop))" in Python
RawObject listFromRange(word start, word stop) {
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  List result(&scope, thread->runtime()->newList());
  Object value(&scope, NoneType::object());
  for (word i = start; i < stop; i++) {
    value = SmallInt::fromWord(i);
    thread->runtime()->listAdd(result, value);
  }
  return *result;
}

}  // namespace testing
}  // namespace python
