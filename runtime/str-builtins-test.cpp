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

}  // namespace python
