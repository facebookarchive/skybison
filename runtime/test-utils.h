#pragma once

#include "handles.h"
#include "objects.h"

#include <iosfwd>
#include <string>
#include "gtest/gtest.h"

// Define EXPECT_DEBUG_ONLY_DEATH, which is like gtest's
// EXPECT_DEBUG_DEATH, except that it does not execute the given statment
// at all in release mode. This is necessary because most of our tests
// which rely on EXPECT_DEBUG_DEATH will cause unsafe operations to
// occur if the statement is executed.

#ifdef NDEBUG

#define EXPECT_DEBUG_ONLY_DEATH(stmt, pattern)

#else

#define EXPECT_DEBUG_ONLY_DEATH(stmt, pattern)                                 \
  EXPECT_DEBUG_DEATH((stmt), (pattern))

#endif

namespace python {
class Runtime;

std::ostream& operator<<(std::ostream& os, const Str& str);
std::ostream& operator<<(std::ostream& os, CastError err);

namespace testing {
// Compile the supplied python snippet, run it, and return stdout or stderr
std::string compileAndRunToString(Runtime* runtime, const char* src);
std::string compileAndRunToStderrString(Runtime* runtime, const char* src);

// A predicate-formatter for checking if a python::RawStr has the same contents
// as a std::string
::testing::AssertionResult AssertPyStringEqual(const char* actual_string_expr,
                                               const char* expected_string_expr,
                                               RawStr actual_str,
                                               std::string expected_string);

::testing::AssertionResult AssertPyStringEqual(const char* actual_string_expr,
                                               const char* expected_string_expr,
                                               RawStr actual_str,
                                               RawStr expected_str);

#define EXPECT_PYSTRING_EQ(s1, s2)                                             \
  EXPECT_PRED_FORMAT2(testing::AssertPyStringEqual, s1, s2)

// Basic variant wrapper for a subset of Python values, used by
// EXPECT_PYLIST_EQ().
class Value {
 public:
  enum class Type {
    None,
    Bool,
    Int,
    Float,
    Str,
  };

  Value(bool b) : bool_{b}, type_{Type::Bool} {}
  Value(int i) : Value{static_cast<word>(i)} {}
  Value(word i) : int_{i}, type_{Type::Int} {}
  Value(double f) : float_{f}, type_{Type::Float} {}
  Value(const char* s) : str_{s}, type_{Type::Str} {}

  static Value none();

  Type type() const;

  bool boolVal() const;
  word intVal() const;
  double floatVal() const;
  const char* strVal() const;

 private:
  union {
    bool bool_;
    word int_;
    double float_;
    const char* str_;
  };

  Value() : type_{Type::None} {}

  Type type_;
};

inline Value Value::none() { return Value{}; }

// Check if the given Object is a list containing the expected elements.
::testing::AssertionResult AssertPyListEqual(
    const char* actual_expr, const char* expected_expr, const Object& actual,
    const std::vector<Value>& expected);

// The preprocessor doesn't understand grouping by {}, so EXPECT_PYLIST_EQ uses
// this to support a nice-looking callsite.
inline std::vector<Value> make_list(std::vector<Value> l) { return l; }

// Used to verify the type, length, and contents of a list. Supports None, bool,
// int, float, and str values:
//
// Handle<Object> list(&scope, moduleAt(...));
// EXPECT_PYLIST_EQ(*list, {Value::None(), false, 42, 123.456, "a string"});
#define EXPECT_PYLIST_EQ(l1, ...)                                              \
  EXPECT_PRED_FORMAT2(::python::testing::AssertPyListEqual, l1,                \
                      ::python::testing::make_list(__VA_ARGS__))

// Calls func using the supplied arguments and captures output.
//
// This opens a new frame linked to the initial frame of the current thread,
// pushes all the arguments onto the stack, and invokes the interpreter.
//
// The caller is responsible for cleaning up any exception state.
std::string callFunctionToString(const Function& func, const ObjectArray& args);

// Calls func using the supplied arguments.
//
// This opens a new frame linked to the initial frame of the current thread,
// pushes all the arguments onto the stack, and invokes the interpreter.
//
// The caller is responsible for cleaning up any exception state.
RawObject callFunction(const Function& func, const ObjectArray& args);

bool objectArrayContains(const ObjectArray& object_array, const Object& key);

// Get the module bound to name in the given runtime. Returns Error::object() if
// not found.
RawObject findModule(Runtime* runtime, const char* name);

// Get the value bound to name in the supplied module. Returns Error::object()
// if not found.
RawObject moduleAt(Runtime* runtime, const Module& module, const char* name);

// Variant of moduleAt() that looks up the module and name at the same time,
// returning Error::object() if either can't be found.
RawObject moduleAt(Runtime* runtime, const char* module_name, const char* name);

// Get the name of the type of the given object.
std::string typeName(Runtime* runtime, RawObject obj);

// Helper to allow construction from initializer list, like
// newIntFromDigits(runtime, {-1, 1})
RawObject newIntWithDigits(Runtime* runtime, const std::vector<word>& digits);

// Helper to create set objects.
// Equivalent to evaluating "set(range(start, stop))" in Python.
RawObject setFromRange(word start, word stop);

}  // namespace testing
}  // namespace python
