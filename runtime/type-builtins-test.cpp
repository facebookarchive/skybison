#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"
#include "type-builtins.h"

namespace python {

using namespace testing;

TEST(TypeBuiltinTest, DunderBasesReturnsTuple) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A: pass
class B: pass
class C(A, B): pass
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime, "__main__", "A"));
  Object b(&scope, moduleAt(&runtime, "__main__", "B"));
  Object c(&scope, moduleAt(&runtime, "__main__", "C"));
  Object dunder_bases(&scope, runtime.newStrFromCStr("__bases__"));
  Object result_obj(&scope, runtime.attributeAt(thread, c, dunder_bases));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), a);
  EXPECT_EQ(result.at(1), b);
}

TEST(TypeBuiltinsTest, DunderBasesOnBuiltinTypeReturnsTuple) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object type(&scope, runtime.typeAt(LayoutId::kInt));
  Object dunder_bases(&scope, runtime.newStrFromCStr("__bases__"));
  Object result_obj(&scope, runtime.attributeAt(thread, type, dunder_bases));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_EQ(result.at(0), runtime.typeAt(LayoutId::kObject));
}

TEST(TypeBuiltinsTest, DunderCallType) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C: pass
c = C()
)");

  Type type(&scope, moduleAt(&runtime, "__main__", "C"));
  ASSERT_FALSE(type.isError());
  Object instance(&scope, moduleAt(&runtime, "__main__", "c"));
  ASSERT_FALSE(instance.isError());
  Object instance_type(&scope, runtime.typeOf(*instance));
  ASSERT_FALSE(instance_type.isError());

  EXPECT_EQ(*type, *instance_type);
}

TEST(TypeBuiltinsTest, DunderCallTypeWithInit) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    global g
    g = 2

g = 1
C()
)");

  Object global(&scope, moduleAt(&runtime, "__main__", "g"));
  ASSERT_FALSE(global.isError());
  EXPECT_TRUE(isIntEqualsWord(*global, 2));
}

TEST(TypeBuiltinsTest, DunderCallTypeWithInitAndArgs) {
  Runtime runtime;
  HandleScope scope;

  runFromCStr(&runtime, R"(
class C:
  def __init__(self, x):
    global g
    g = x

g = 1
C(9)
)");

  Object global(&scope, moduleAt(&runtime, "__main__", "g"));
  ASSERT_FALSE(global.isError());
  EXPECT_TRUE(isIntEqualsWord(*global, 9));
}

TEST(TypeBuiltinsTest, DunderCallWithNonTypeRaisesTypeError) {
  Runtime runtime;
  ASSERT_TRUE(raisedWithStr(runFromCStr(&runtime, "type.__call__(5)"),
                            LayoutId::kTypeError,
                            "self must be a type instance"));
}

TEST(TypeBuiltinsTest, DunderCallCallsDunderInit) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Callable:
  def __call__(self, obj):
    obj.x = 42
class C:
  __init__ = Callable()
c = C()
)");
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Thread* thread = Thread::current();
  Object x(&scope, runtime.newStrFromCStr("x"));
  RawObject attr = runtime.attributeAt(thread, c, x);
  EXPECT_TRUE(isIntEqualsWord(attr, 42));
}

TEST(TypeBuiltinsTest,
     DunderCallWithNonTypeDudnerNewResultReturnsWithoutCallingDunderInit) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class C:
  def __new__(self, *args):
    return 17
  def __init__(self, *args):
    raise Exception("should not happen")
result = type.__call__(C, "C", (), {})
)");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 17));
}

TEST(TypeBuiltinsTest, DunderDirReturnsList) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  x = 42
  def foo(): pass
class B(A):
  def bar(): pass
dir = type.__dir__(B)
)")
                   .isError());
  Object dir(&scope, moduleAt(&runtime, "__main__", "dir"));
  Object x(&scope, runtime.newStrFromCStr("x"));
  EXPECT_TRUE(listContains(dir, x));
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(listContains(dir, foo));
  Object bar(&scope, runtime.newStrFromCStr("bar"));
  EXPECT_TRUE(listContains(dir, bar));
}

TEST(TypeBuiltinsTest, DunderDocOnEmptyTypeReturnsNone) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "class C: pass").isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "C"));
  Object doc(&scope, runtime.attributeAtId(thread, c, SymbolId::kDunderDoc));
  EXPECT_EQ(doc, NoneType::object());
}

TEST(TypeBuiltinsTest, DunderDocReturnsDocumentationString) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  """hello documentation"""
  pass
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "C"));
  Object doc(&scope, runtime.attributeAtId(thread, c, SymbolId::kDunderDoc));
  EXPECT_TRUE(isStrEqualsCStr(*doc, "hello documentation"));
}

TEST(TypeBuiltinsTest, DunderReprForBuiltinReturnsStr) {
  Runtime runtime;
  runFromCStr(&runtime, "result = type.__repr__(object)");
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"),
                              "<class 'object'>"));
}

TEST(TypeBuiltinsTest, DunderReprForUserDefinedTypeReturnsStr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  pass
result = type.__repr__(Foo)
)");
  // TODO(T32810595): Once module names are supported, this should become
  // "<class '__main__.Foo'>".
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"),
                              "<class 'Foo'>"));
}

TEST(TypeBuiltinsTest, DunderNewWithOneArgReturnsTypeOfArg) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
a = type.__new__(type, 1);
b = type.__new__(type, "hello");
)");
  Type a(&scope, moduleAt(&runtime, "__main__", "a"));
  Type b(&scope, moduleAt(&runtime, "__main__", "b"));

  EXPECT_EQ(RawLayout::cast(a.instanceLayout()).id(), LayoutId::kInt);
  EXPECT_EQ(RawLayout::cast(b.instanceLayout()).id(), LayoutId::kStr);
}

TEST(TypeBuiltinsTest, DunderNewWithOneMetaclassArgReturnsType) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, R"(
class Foo(type):
  pass
a = type.__new__(type, Foo);
)");
  Type a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_EQ(RawLayout::cast(a.instanceLayout()).id(), LayoutId::kType);
}

TEST(TypeBuiltinsTest, TypeHasDunderMroAttribute) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = str.__class__.__mro__");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
}

TEST(TypeBuiltinsTest, TypeHasDunderNameAttribute) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = str.__class__.__name__");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(RawStr::cast(*result), "type"));
}

TEST(TypeBuiltinsTest, TypeHasDunderFlagsAttribute) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = str.__class__.__flags__");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isInt());
}

TEST(TypeBuiltinsTest, TypeHasDunderDictAttribute) {
  Runtime runtime;
  HandleScope scope;
  runFromCStr(&runtime, "result = str.__class__.__dict__");
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isDict());
}

TEST(TypeBuiltinsTest, DunderCallReceivesExArgs) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  def __init__(self, *args):
    self.args = args

  def num_args(self):
    return len(self.args)

result = C(*(1,2,3)).num_args()
)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, RawSmallInt::fromWord(3));
}

TEST(TypeBuiltinsTest, ClassMethodDunderCallReceivesExArgs) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class Foo:
  @classmethod
  def foo(cls, *args):
    return len(args)

result = Foo.foo(*(1,2,3))
)");
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, RawSmallInt::fromWord(3));
}

TEST(TypeBuiltinsTest, TypeNewReceivesExArgs) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
ty = type.__new__(type, *("foo", (object,), {'a': 1}))
name = ty.__name__
)");
  HandleScope scope;
  Object name(&scope, moduleAt(&runtime, "__main__", "name"));
  EXPECT_TRUE(isStrEqualsCStr(*name, "foo"));
}

TEST(TypeBuiltinsTest, TypeCallWithInitReturningNonNoneRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
class C:
  def __init__(self, *args, **kwargs):
    return 5
C()
)"),
                            LayoutId::kTypeError,
                            "C.__init__ returned non None"));
}

TEST(TypeBuiltinTest, MroReturnsList) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
class C:
  pass
result = C.mro()
)");
  HandleScope scope;
  Object ctype(&scope, moduleAt(&runtime, "__main__", "C"));
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isList());
  List result(&scope, *result_obj);
  EXPECT_EQ(result.at(0), *ctype);
  EXPECT_EQ(result.at(1), runtime.typeAt(LayoutId::kObject));
}

TEST(TypeBuiltinTest, TypeofSmallStrReturnsStr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = type('a')
)");
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"),
            runtime.typeAt(LayoutId::kStr));
}

TEST(TypeBuiltinTest, TypeofLargeStrReturnsStr) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = type('aaaaaaaaaaaaaaaaaaaaaaa')
)");
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"),
            runtime.typeAt(LayoutId::kStr));
}

TEST(TypeBuiltinTest, TypeofSmallIntReturnsInt) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = type(5)
)");
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"),
            runtime.typeAt(LayoutId::kInt));
}

TEST(TypeBuiltinTest, TypeofLargeIntReturnsInt) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
result = type(99999999999999999999999999999999999999999)
)");
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"),
            runtime.typeAt(LayoutId::kInt));
}

}  // namespace python
