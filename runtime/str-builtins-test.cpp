#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(StrBuiltinsTest, RichCompareStringEQ) {  // pystone dependency
  const char* src = R"(
a = "__main__"
if (a == "__main__"):
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "foo\n");
}

TEST(StrBuiltinsTest, RichCompareStringNE) {  // pystone dependency
  const char* src = R"(
a = "__main__"
if (a != "__main__"):
  print("foo")
else:
  print("bar")
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "bar\n");
}

TEST(StrBuiltinsTest, RichCompareSingleCharLE) {
  Runtime runtime;

  runtime.runFromCString(R"(
a_le_b = 'a' <= 'b'
b_le_a = 'a' >= 'b'
a_le_a = 'a' <= 'a'
)");

  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));

  Handle<Object> a_le_b(&scope, moduleAt(&runtime, main, "a_le_b"));
  EXPECT_EQ(*a_le_b, Boolean::trueObj());

  Handle<Object> b_le_a(&scope, moduleAt(&runtime, main, "b_le_a"));
  EXPECT_EQ(*b_le_a, Boolean::falseObj());

  Handle<Object> a_le_a(&scope, moduleAt(&runtime, main, "a_le_a"));
  EXPECT_EQ(*a_le_a, Boolean::trueObj());
}

TEST(StrBuiltinsTest, DunderNewCallsDunderStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
class Foo:
    def __str__(self):
        return "foo"
a = str.__new__(str, Foo())
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "foo");
}

TEST(StrBuiltinsTest, DunderNewCallsReprIfNoDunderStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
class Foo:
  pass
f = Foo()
a = str.__new__(str, f)
b = repr(f)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<String> b(&scope, moduleAt(&runtime, main, "b"));
  EXPECT_PYSTRING_EQ(*a, *b);
}

TEST(StrBuiltinsTest, DunderNewWithNoArgsExceptTypeReturnsEmptyString) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = str.__new__(str)
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "");
}

TEST(StrBuiltinsTest, DunderNewWithStrReturnsSameStr) {
  Runtime runtime;
  runtime.runFromCString(R"(
a = str.__new__(str, "hello")
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<String> a(&scope, moduleAt(&runtime, main, "a"));
  EXPECT_PYSTRING_EQ(*a, "hello");
}

TEST(StrBuiltinsDeathTest, DunderNewWithNoArgsThrows) {
  Runtime runtime;
  const char* src = R"(
str.__new__()
)";
  EXPECT_DEATH(runtime.runFromCString(src),
               "aborting due to pending exception");
}

TEST(StrBuiltinsDeathTest, DunderNewWithTooManyArgsThrows) {
  Runtime runtime;
  const char* src = R"(
str.__new__(str, 1, 2, 3, 4)
)";
  EXPECT_DEATH(runtime.runFromCString(src),
               "aborting due to pending exception");
}

TEST(StrBuiltinsDeathTest, DunderNewWithNonTypeArgThrows) {
  Runtime runtime;
  const char* src = R"(
str.__new__(1)
)";
  EXPECT_DEATH(runtime.runFromCString(src),
               "aborting due to pending exception");
}

TEST(StrBuiltinsDeathTest, DunderNewWithNonSubtypeArgThrows) {
  Runtime runtime;
  const char* src = R"(
str.__new__(object)
)";
  EXPECT_DEATH(runtime.runFromCString(src),
               "aborting due to pending exception");
}

}  // namespace python
