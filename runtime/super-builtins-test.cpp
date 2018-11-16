#include "gtest/gtest.h"

#include "runtime.h"
#include "super-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(SuperBuiltinsTest, SuperTest1) {
  const char* src = R"(
class A:
    def f(self):
        return 1

class B(A):
    def f(self):
        return super(B, self).f() + 2

class C(A):
    def f(self):
        return super(C, self).f() + 3

class D(C, B):
    def f(self):
        return super(D, self).f() + 4

class E(D):
    pass

class F(E):
    f = E.f

class G(A):
    pass

print(D().f())
print(D.f(D()))
print(E().f())
print(E.f(E()))
print(F().f())
print(F.f(F()))
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "10\n10\n10\n10\n10\n10\n");
}

TEST(SuperBuiltinsTest, SuperTest2) {
  const char* src = R"(
class A:
    @classmethod
    def cm(cls):
        return (cls, 1)

class B(A):
    @classmethod
    def cm(cls):
        return (cls, super(B, cls).cm(), 2)

class C(A):
    @classmethod
    def cm(cls):
        return (cls, super(C, cls).cm(), 3)

class D(C, B):
    def cm(cls):
        return (cls, super(D, cls).cm(), 4)

class E(D):
    pass

class G(A):
    pass

print(A.cm() == (A, 1))
print(A().cm() == (A, 1))
print(G.cm() == (G, 1))
print(G().cm() == (G, 1))
d = D()
print(d.cm() == (d, (D, (D, (D, 1), 2), 3), 4))
e = E()
print(e.cm() == (e, (E, (E, (E, 1), 2), 3), 4))
)";
  Runtime runtime;
  std::string output = compileAndRunToString(&runtime, src);
  EXPECT_EQ(output, "True\nTrue\nTrue\nTrue\nTrue\nTrue\n");
}

TEST(SuperBuiltinsTest, SuperTestNoArgument) {
  const char* src = R"(
class A:
    @classmethod
    def cm(cls):
        return (cls, 1)

    def f(self):
        return 1

class B(A):
    @classmethod
    def cm(cls):
        return (cls, super().cm(), 2)

    def f(self):
        return super().f() + 2

class C(A):
    @classmethod
    def cm(cls):
        return (cls, super().cm(), 3)

    def f(self):
        return super().f() + 3

class D(C, B):
    def cm(cls):
        return (cls, super().cm(), 4)

    def f(self):
        return super().f() + 4

a = B().f()
b = D().f()
c = B.cm() == (B, (B, 1), 2)
d = D()
e = d.cm() == (d, (D, (D, (D, 1), 2), 3), 4)
)";
  Runtime runtime;
  compileAndRunToString(&runtime, src);
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Int> a(&scope, moduleAt(&runtime, main, "a"));
  Handle<Int> b(&scope, moduleAt(&runtime, main, "b"));
  Handle<Bool> c(&scope, moduleAt(&runtime, main, "c"));
  Handle<Bool> e(&scope, moduleAt(&runtime, main, "e"));
  EXPECT_EQ(a->asWord(), 3);
  EXPECT_EQ(b->asWord(), 10);
  EXPECT_EQ(*c, Bool::trueObj());
  EXPECT_EQ(*e, Bool::trueObj());
}

TEST(SuperDeathTest, NoArugmentThrow) {
  const char* src = R"(
a = super()
)";

  Runtime runtime;
  ASSERT_DEATH(runtime.runFromCString(src),
               "aborting due to pending exception: super\\(\\): no arguments");
  const char* src1 = R"(
def f(a):
    super()
f(1)
)";
  ASSERT_DEATH(runtime.runFromCString(src1),
               "aborting due to pending exception: super\\(\\): __class__ cell "
               "not found");
}

}  // namespace python
