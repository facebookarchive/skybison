#include "gtest/gtest.h"

#include "descriptor-builtins.h"
#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DescriptorBuiltinsTest, Classmethod) {
  const char* src = R"(
class Foo():
  a = 1
  @classmethod
  def bar(cls):
    print(cls.a)
a = Foo()
a.bar()
Foo.a = 2
Foo.bar()
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "1\n2\n");
}

TEST(DescriptorBuiltinsTest, StaticmethodObjAccess) {
  const char* src = R"(
class E:
    @staticmethod
    def f(x):
        return x + 1

e = E()
print(e.f(5))
)";

  Runtime runtime;
  const std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "6\n");
}

TEST(DescriptorBuiltinsTest, StaticmethodClsAccess) {
  const char* src = R"(
class E():
    @staticmethod
    def f(x, y):
        return x + y

print(E.f(1,2))
)";

  Runtime runtime;
  const std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "3\n");
}

} // namespace python
