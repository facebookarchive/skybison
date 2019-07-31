#include "gtest/gtest.h"

#include "runtime.h"
#include "super-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

using SuperBuiltinsTest = RuntimeFixture;

TEST_F(SuperBuiltinsTest, DunderCallWorksInTypesWithNonDefaultMetaclass) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class M(type): pass
class A(metaclass=M):
    x = 2
class B(A):
    x = 4
    def getsuper(self):
        return super()
result = B().getsuper().x
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 2));
}

TEST_F(SuperBuiltinsTest, DunderGetattributeReturnsAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  foo = 8
class B(A):
  foo = 10
  def getsuper(self):
    return super()
s = B().getsuper()
)")
                   .isError());
  Object s(&scope, moduleAt(&runtime_, "__main__", "s"));
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(SuperBuiltins::dunderGetattribute, s, name), 8));
}

TEST_F(SuperBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A: pass
class B(A):
  def getsuper(self):
    return super()
s = B().getsuper()
)")
                   .isError());
  Object s(&scope, moduleAt(&runtime_, "__main__", "s"));
  Object name(&scope, runtime_.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SuperBuiltins::dunderGetattribute, s, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST_F(SuperBuiltinsTest,
       DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A: pass
class B(A):
  def getsuper(self):
    return super()
s = B().getsuper()
)")
                   .isError());
  Object s(&scope, moduleAt(&runtime_, "__main__", "s"));
  Object name(&scope, runtime_.newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(SuperBuiltins::dunderGetattribute, s, name),
      LayoutId::kAttributeError, "super object has no attribute 'xxx'"));
}

TEST_F(SuperBuiltinsTest, SuperTest1) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

result0 = D().f()
result1 = D.f(D())
result2 = E().f()
result3 = E.f(E())
result4 = F().f()
result5 = F.f(F())
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result0"), 10));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result1"), 10));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result2"), 10));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result3"), 10));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result4"), 10));
  EXPECT_TRUE(isIntEqualsWord(moduleAt(&runtime_, "__main__", "result5"), 10));
}

TEST_F(SuperBuiltinsTest, SuperTest2) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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

result0 = A.cm() == (A, 1)
result1 = A().cm() == (A, 1)
result2 = G.cm() == (G, 1)
result3 = G().cm() == (G, 1)
d = D()
result4 = d.cm() == (d, (D, (D, (D, 1), 2), 3), 4)
e = E()
result5 = e.cm() == (e, (E, (E, (E, 1), 2), 3), 4)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result0"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result1"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result2"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result3"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result4"), Bool::trueObj());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result5"), Bool::trueObj());
}

TEST_F(SuperBuiltinsTest, SuperTestNoArgument) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Object b(&scope, moduleAt(&runtime_, "__main__", "b"));
  Bool c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Bool e(&scope, moduleAt(&runtime_, "__main__", "e"));
  EXPECT_TRUE(isIntEqualsWord(*a, 3));
  EXPECT_TRUE(isIntEqualsWord(*b, 10));
  EXPECT_EQ(*c, Bool::trueObj());
  EXPECT_EQ(*e, Bool::trueObj());
}

TEST_F(SuperBuiltinsTest,
       SuperCalledFromFunctionWithCellVarReturnsSuperInstance) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class MetaA(type):
    x = 42
class MetaB(MetaA):
    def __new__(metacls, cls, bases, classdict):
        cellvar = None
        def foobar():
            return cellvar
        return super().__new__(metacls, cls, bases, classdict)
class C(metaclass=MetaB): pass
result = type(C()).x
)")
                   .isError());
  HandleScope scope(thread_);
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 42));
}

TEST_F(SuperBuiltinsTest, NoArgumentRaisesRuntimeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "super()"),
                            LayoutId::kRuntimeError, "super(): no arguments"));
  Thread::current()->clearPendingException();

  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
def f(a):
    super()
f(1)
)"),
                            LayoutId::kRuntimeError,
                            "super(): __class__ cell not found"));
}

TEST_F(SuperBuiltinsTest, SuperGetAttributeReturnsAttributeInSuperClass) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  x = 13
class B(A):
  x = 42
  def getsuper(self):
    return super()
s = B().getsuper()
)")
                   .isError());
  Object s_obj(&scope, moduleAt(&runtime_, "__main__", "s"));
  ASSERT_TRUE(s_obj.isSuper());
  Super s(&scope, *s_obj);
  Object name(&scope, runtime_.newStrFromCStr("x"));
  EXPECT_TRUE(isIntEqualsWord(superGetAttribute(thread_, s, name), 13));
}

TEST_F(SuperBuiltinsTest, SuperGetAttributeWithMissingAttributeReturnsError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A: pass
class B(A):
  x = 42
  def getsuper(self):
    return super()
s = B().getsuper()
)")
                   .isError());
  Object s_obj(&scope, moduleAt(&runtime_, "__main__", "s"));
  ASSERT_TRUE(s_obj.isSuper());
  Super s(&scope, *s_obj);
  Object name(&scope, runtime_.newStrFromCStr("x"));
  EXPECT_TRUE(superGetAttribute(thread_, s, name).isError());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(SuperBuiltinsTest, SuperGetAttributeCallsDunderGetOnDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return (self, instance, owner)
d = D()
class A:
  x = d
class B(A):
  x = 42
  def getsuper(self):
    return super()
i = B()
s = i.getsuper()
)")
                   .isError());
  Object d(&scope, moduleAt(&runtime_, "__main__", "d"));
  Object b(&scope, moduleAt(&runtime_, "__main__", "B"));
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));
  Object s_obj(&scope, moduleAt(&runtime_, "__main__", "s"));
  ASSERT_TRUE(s_obj.isSuper());
  Super s(&scope, *s_obj);
  Object name(&scope, runtime_.newStrFromCStr("x"));
  Object result_obj(&scope, superGetAttribute(thread_, s, name));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);
  EXPECT_EQ(result.at(0), d);
  EXPECT_EQ(result.at(1), i);
  EXPECT_EQ(result.at(2), b);
}

TEST_F(SuperBuiltinsTest, SuperGetAttributeCallsDunderGetOnNonDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __get__(self, instance, owner): return (self, instance, owner)
d = D()
class A:
  x = d
class B(A):
  x = 42
  def getsuper(self):
    return super()
i = B()
s = i.getsuper()
)")
                   .isError());
  Object d(&scope, moduleAt(&runtime_, "__main__", "d"));
  Object b(&scope, moduleAt(&runtime_, "__main__", "B"));
  Object i(&scope, moduleAt(&runtime_, "__main__", "i"));
  Object s_obj(&scope, moduleAt(&runtime_, "__main__", "s"));
  ASSERT_TRUE(s_obj.isSuper());
  Super s(&scope, *s_obj);
  Object name(&scope, runtime_.newStrFromCStr("x"));
  Object result_obj(&scope, superGetAttribute(thread_, s, name));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);
  EXPECT_EQ(result.at(0), d);
  EXPECT_EQ(result.at(1), i);
  EXPECT_EQ(result.at(2), b);
}

TEST_F(SuperBuiltinsTest, SuperGetAttributeDunderClassReturnsSuper) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def foo(self):
    return super()
s = C().foo()
)")
                   .isError());
  Object s_obj(&scope, moduleAt(&runtime_, "__main__", "s"));
  ASSERT_TRUE(s_obj.isSuper());
  Super s(&scope, *s_obj);
  Object name(&scope, runtime_.newStrFromCStr("__class__"));
  Type super_type(&scope, runtime_.typeAt(LayoutId::kSuper));
  EXPECT_EQ(superGetAttribute(thread_, s, name), super_type);
}

}  // namespace python
