#include "gtest/gtest.h"

#include "frame.h"
#include "object-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ObjectBuiltinsTest, ObjectDunderReprReturnsTypeNameAndPointer) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

a = object.__repr__(Foo())
)");
  HandleScope scope;
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  // Storage for the class name. It must be shorter than the length of the whole
  // string.
  char* c_str = a.toCStr();
  char* class_name = static_cast<char*>(std::malloc(a.length()));
  void* ptr = nullptr;
  int num_written = std::sscanf(c_str, "<%s object at %p>", class_name, &ptr);
  ASSERT_EQ(num_written, 2);
  EXPECT_STREQ(class_name, "Foo");
  // No need to check the actual pointer value, just make sure something was
  // there.
  EXPECT_NE(ptr, nullptr);
  free(class_name);
  free(c_str);
}

TEST(ObjectBuiltinsTest, DunderEqWithIdenticalObjectsReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = object.__eq__(None, None)
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, Bool::trueObj());
}

TEST(ObjectBuiltinsTest, DunderEqWithNonIdenticalObjectsReturnsNotImplemented) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = object.__eq__(object(), object())
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(result.isNotImplementedType());
}

TEST(ObjectBuiltinsTest, DunderSizeofWithNonHeapObjectReturnsSizeofRawObject) {
  Runtime runtime;
  HandleScope scope;
  Object small_int(&scope, SmallInt::fromWord(6));
  Object result(&scope, runBuiltin(ObjectBuiltins::dunderSizeof, small_int));
  EXPECT_TRUE(isIntEqualsWord(*result, kPointerSize));
}

TEST(ObjectBuiltinsTest, DunderSizeofWithLargeStrReturnsSizeofHeapObject) {
  Runtime runtime;
  HandleScope scope;
  HeapObject large_str(&scope, runtime.heap()->createLargeStr(40));
  Object result(&scope, runBuiltin(ObjectBuiltins::dunderSizeof, large_str));
  EXPECT_TRUE(isIntEqualsWord(*result, large_str.size()));
}

TEST(
    ObjectBuiltinsTest,
    DunderNeWithSelfImplementingDunderEqReturningNotImplementedReturnsNotImplemented) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
  def __eq__(self, b): return NotImplemented

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_TRUE(moduleAt(&runtime, "__main__", "result").isNotImplementedType());
}

TEST(ObjectBuiltinsTest,
     DunderNeWithSelfImplementingDunderEqReturningZeroReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
  def __eq__(self, b): return 0

result = object.__ne__(Foo(), None)
)")
                   .isError());
  // 0 is converted to False, and flipped again for __ne__ from __eq__.
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(ObjectBuiltinsTest,
     DunderNeWithSelfImplementingDunderEqReturningOneReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
  def __eq__(self, b): return 1

result = object.__ne__(Foo(), None)
)")
                   .isError());
  // 1 is converted to True, and flipped again for __ne__ from __eq__.
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(ObjectBuiltinsTest,
     DunderNeWithSelfImplementingDunderEqReturningFalseReturnsTrue) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
  def __eq__(self, b): return False

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::trueObj());
}

TEST(ObjectBuiltinsTest,
     DunderNeWithSelfImplementingDunderEqReturningTrueReturnsFalse) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo():
  def __eq__(self, b): return True

result = object.__ne__(Foo(), None)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"), Bool::falseObj());
}

TEST(ObjectBuiltinsTest, DunderStrReturnsDunderRepr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

f = Foo()
a = object.__str__(f)
b = object.__repr__(f)
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST(ObjectBuiltinsTest, UserDefinedTypeInheritsDunderStr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass

f = Foo()
a = object.__str__(f)
b = f.__str__()
)");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_TRUE(isStrEquals(a, b));
}

TEST(ObjectBuiltinsTest, DunderInitDoesNotRaiseIfNewIsDifferentButInitIsSame) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  def __new__(cls):
    return object.__new__(cls)

Foo.__init__(Foo(), 1)
)");
  // It doesn't matter what the output is, just that it doesn't throw a
  // TypeError.
}

TEST(ObjectBuiltinsTest, DunderInitWithNonInstanceIsOk) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
object.__init__(object)
)");
  // It doesn't matter what the output is, just that it doesn't throw a
  // TypeError.
}

TEST(ObjectBuiltinsTest, DunderInitWithNoArgsRaisesTypeError) {
  Runtime runtime;
  // Passing no args to object.__init__ should throw a type error.
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime, R"(
object.__init__()
)"),
      LayoutId::kTypeError,
      "TypeError: 'object.__init__' takes 1 positional arguments but 0 given"));
}

TEST(ObjectBuiltinsTest, DunderInitWithArgsRaisesTypeError) {
  Runtime runtime;
  // Passing extra args to object.__init__, without overwriting __new__,
  // should throw a type error.
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
  pass

Foo.__init__(Foo(), 1)
)"),
                            LayoutId::kTypeError,
                            "object.__init__() takes no parameters"));
}

TEST(ObjectBuiltinsTest, DunderInitWithNewAndInitRaisesTypeError) {
  Runtime runtime;
  // Passing extra args to object.__init__, and overwriting only __init__,
  // should throw a type error.
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class Foo:
  def __init__(self):
    object.__init__(self, 1)

Foo()
)"),
                            LayoutId::kTypeError,
                            "object.__init__() takes no parameters"));
}

TEST(NoneBuiltinsTest, NewReturnsNone) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kNoneType));
  EXPECT_TRUE(runBuiltin(NoneBuiltins::dunderNew, type).isNoneType());
}

TEST(NoneBuiltinsTest, NewWithExtraArgsRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(
      raised(runFromCStr(&runtime, "NoneType.__new__(NoneType, 1, 2, 3, 4, 5)"),
             LayoutId::kTypeError));
}

TEST(NoneBuiltinsTest, DunderReprIsBoundMethod) {
  Runtime runtime;
  runFromCStr(&runtime, "a = None.__repr__");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a.isBoundMethod());
}

TEST(NoneBuiltinsTest, DunderReprReturnsNone) {
  Runtime runtime;
  runFromCStr(&runtime, "a = None.__repr__()");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "None"));
}

}  // namespace python
