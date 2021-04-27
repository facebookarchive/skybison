#pragma once

#include <functional>
#include <string>

#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"

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

namespace py {

namespace testing {

Runtime* createTestRuntime();

bool useCppInterpreter();

class RuntimeFixture : public ::testing::Test {
 protected:
  void SetUp() override {
    runtime_ = createTestRuntime();
    thread_ = runtime_->mainThread();
  }

  void TearDown() override { delete runtime_; }

  Runtime* runtime_;
  Thread* thread_;
};

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

inline RawObject valueCellValue(RawObject value_cell_obj) {
  DCHECK(value_cell_obj.isValueCell(), "expected valuecell");
  return ValueCell::cast(value_cell_obj).value();
}

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
  EXPECT_PRED_FORMAT2(::py::testing::AssertPyListEqual, l1,                    \
                      ::py::testing::make_list(__VA_ARGS__))

// Calls func using the supplied arguments.
//
// This opens a new frame linked to the initial frame of the current thread,
// pushes all the arguments onto the stack, and invokes the interpreter.
//
// The caller is responsible for cleaning up any exception state.
RawObject callFunction(const Function& func, const Tuple& args);

bool tupleContains(const Tuple& object_array, const Object& key);

bool listContains(const Object& list_obj, const Object& key);

// Get the module instance bount to name "__main__".
// Return Error::object() if not found.
RawObject findMainModule(Runtime* runtime);

// Get the value bound to name in the main module.
// Returns Error::object() if not found.
RawObject mainModuleAt(Runtime* runtime, const char* name);

// Get the value bound to name in the module under the supplied name.
// Returns Error::object() if not found.
RawObject moduleAtByCStr(Runtime* runtime, const char* module_name,
                         const char* name);

// Get the name of the type of the given object.
std::string typeName(Runtime* runtime, RawObject obj);

RawCode newEmptyCode();

RawFunction newEmptyFunction();

RawBytes newBytesFromCStr(Thread* thread, const char* str);

RawBytearray newBytearrayFromCStr(Thread* thread, const char* str);

// Helper for creating weakrefs with the callback as a BoundMethod.
RawObject newWeakRefWithCallback(Runtime* runtime, Thread* thread,
                                 const Object& referent,
                                 const Object& callback);

// Helper to allow construction from initializer list, like
// newIntFromDigits(runtime, {-1, 1})
RawObject newIntWithDigits(Runtime* runtime, View<uword> digits);

// Helper to allow construction of a RawLargeInt from an initializer list.
// May construct an invalid LargeInt depending on the digits so we can test
// normalizeLargeInt().
RawLargeInt newLargeIntWithDigits(View<uword> digits);

// Creates memoryview objects. This is a convenience helper around
// Runtime::newMemoryView().
RawObject newMemoryView(View<byte> bytes, const char* format,
                        ReadOnly read_only = ReadOnly::ReadOnly);

RawTuple newTupleWithNone(word length);

// Helper to create set objects.
// Equivalent to evaluating "set(range(start, stop))" in Python.
RawObject setFromRange(word start, word stop);

bool setIncludes(Thread* thread, const SetBase& set, const Object& key);

void setHashAndAdd(Thread* thread, const SetBase& set, const Object& value);

RawObject runBuiltinImpl(BuiltinFunction function,
                         View<std::reference_wrapper<const Object>> args);

RawObject runBuiltin(BuiltinFunction function);

// Create an ad-hoc function with 0 parameters for `code` and execute it in the
// current thread.
NODISCARD RawObject runCode(const Code& code);

// Same as `runCode()` but disables bytecode rewriting.
NODISCARD RawObject runCodeNoBytecodeRewriting(const Code& code);

// Helper to compile and run a snippet of Python code.
NODISCARD RawObject runFromCStr(Runtime* runtime, const char* c_str);

void addBuiltin(const char* name, BuiltinFunction function,
                View<const char*> parameter_names, word code_flags);

template <typename... Args>
RawObject runBuiltin(BuiltinFunction function, const Args&... args) {
  using ref = std::reference_wrapper<const Object>;
  ref args_array[] = {ref(args)...};
  return runBuiltinImpl(function, args_array);
}

struct FileDeleter {
  void operator()(char* filename) const {
    std::remove(filename);
    delete[] filename;
  }
};
using unique_file_ptr = std::unique_ptr<char, FileDeleter>;

RawObject listFromRange(word start, word stop);

RawObject icLookupAttr(RawMutableTuple caches, word index, LayoutId layout_id);

RawObject icLookupBinaryOp(RawMutableTuple caches, word index,
                           LayoutId left_layout_id, LayoutId right_layout_id,
                           BinaryOpFlags* flags_out);

::testing::AssertionResult containsBytecode(const Function& function,
                                            Bytecode bc);

::testing::AssertionResult isBytearrayEqualsBytes(const Object& result,
                                                  View<byte> expected);

::testing::AssertionResult isBytearrayEqualsCStr(const Object& result,
                                                 const char* expected);

::testing::AssertionResult isBytesEqualsBytes(const Object& result,
                                              View<byte> expected);

::testing::AssertionResult isFloatEqualsDouble(RawObject obj, double expected);

::testing::AssertionResult isMutableBytesEqualsBytes(const Object& result,
                                                     View<byte> expected);

::testing::AssertionResult isBytesEqualsCStr(const Object& result,
                                             const char* expected);

::testing::AssertionResult isStrEquals(const Object& str1, const Object& str2);

::testing::AssertionResult isStrEqualsCStr(RawObject obj, const char* c_str);

::testing::AssertionResult isSymbolIdEquals(SymbolId result, SymbolId expected);

::testing::AssertionResult isIntEqualsWord(RawObject obj, word value);

::testing::AssertionResult isIntEqualsDigits(RawObject obj, View<uword> digits);

RawObject layoutCreateEmpty(Thread* thread);

// Check if the return value of a call and current thread state indicate that an
// exception was raised with the given type.
::testing::AssertionResult raised(RawObject return_value, LayoutId layout_id);

// Check if the return value from a call and current thread state indicate that
// an exception was raised with the given type and message.
//
// Since Python exceptions can contain any value, two conventions for finding a
// string message are supported: directly in thread->pendingExceptionValue()
// or, if pendingExceptionValue() contains an exception instance, the first
// element of its args tuple. These situations result from
// thread->raiseFooError(a_string) in C++ or "raise FooError('a string')" in
// Python, respectively.
//
// message may be nullptr to indicate that the exception value should not be
// checked.
::testing::AssertionResult raisedWithStr(RawObject return_value,
                                         LayoutId layout_id,
                                         const char* message);

void writeFile(const std::string& path, const std::string& contents);

// Simple PointerVisitor that remembers the objects visited.
class RememberingVisitor : public PointerVisitor {
 public:
  virtual void visitPointer(RawObject* pointer, PointerKind) {
    pointers_.push_back(*pointer);
  }

  word count() { return pointers_.size(); }

  bool hasVisited(RawObject object) {
    for (auto elt : pointers_) {
      if (elt == object) {
        return true;
      }
    }
    return false;
  }

 private:
  std::vector<RawObject> pointers_;
};

// Creates a temporary directory and cleans it up when the object is destroyed.
class TemporaryDirectory {
 public:
  TemporaryDirectory();
  ~TemporaryDirectory();

  std::string path;
};

const uword kHighbitUword = uword{1} << (kBitsPerWord - 1);

}  // namespace testing
}  // namespace py
