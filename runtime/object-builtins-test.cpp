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
  char* c_str = a->toCStr();
  char* class_name = static_cast<char*>(std::malloc(a->length()));
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
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  Str b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_PYSTRING_EQ(*a, *b);
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
  Str a(&scope, moduleAt(&runtime, "__main__", "a"));
  Str b(&scope, moduleAt(&runtime, "__main__", "b"));
  EXPECT_PYSTRING_EQ(*a, *b);
}

TEST(ObjectBuiltinsTest, DunderInitDoesNotThrowIfNewIsDifferentButInitIsSame) {
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

TEST(ObjectBuiltinsDeathTest, DunderInitWithNoArgsThrowsTypeError) {
  Runtime runtime;
  // Passing no args to object.__init__ should throw a type error.
  EXPECT_DEATH(runFromCStr(&runtime, R"(
object.__init__()
)"),
               "aborting due to pending exception");
}

TEST(ObjectBuiltinsDeathTest, DunderInitWithArgsThrowsTypeError) {
  Runtime runtime;
  // Passing extra args to object.__init__, without overwriting __new__,
  // should throw a type error.
  EXPECT_DEATH(runFromCStr(&runtime, R"(
class Foo:
  pass

Foo.__init__(Foo(), 1)
)"),
               "aborting due to pending exception");
}

TEST(ObjectBuiltinsDeathTest, DunderInitWithNewAndInitThrowsTypeError) {
  Runtime runtime;
  // Passing extra args to object.__init__, and overwriting only __init__,
  // should throw a type error.
  EXPECT_DEATH(runFromCStr(&runtime, R"(
class Foo:
  def __init__(self):
    object.__init__(self, 1)

Foo()
)"),
               "aborting due to pending exception");
}

TEST(NoneBuiltinsTest, NewReturnsNone) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kNoneType));
  EXPECT_TRUE(runBuiltin(NoneBuiltins::dunderNew, type)->isNoneType());
}

TEST(NoneBuiltinsTest, NewWithExtraArgsThrows) {
  Runtime runtime;
  HandleScope scope;
  Type type(&scope, runtime.typeAt(LayoutId::kNoneType));
  Int num1(&scope, runtime.newInt(1));
  Int num2(&scope, runtime.newInt(2));
  Int num3(&scope, runtime.newInt(3));
  EXPECT_TRUE(
      runBuiltin(NoneBuiltins::dunderNew, type, num1, num2, num3)->isError());
}

TEST(NoneBuiltinsTest, DunderReprIsBoundMethod) {
  Runtime runtime;
  runtime.runFromCStr("a = None.__repr__");
  HandleScope scope;
  Object a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_TRUE(a->isBoundMethod());
}

TEST(NoneBuiltinsTest, DunderReprReturnsNone) {
  Runtime runtime;
  runtime.runFromCStr("a = None.__repr__()");
  HandleScope scope;
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "a"));
  ASSERT_TRUE(a_obj->isStr());
  Str a(&scope, *a_obj);
  EXPECT_PYSTRING_EQ(a, "None");
}

}  // namespace python
