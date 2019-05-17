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

TEST(TypeBuiltinTest, DunderBasesOnObjectReturnsEmptyTuple) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Object type(&scope, runtime.typeAt(LayoutId::kObject));
  Object dunder_bases(&scope, runtime.newStrFromCStr("__bases__"));
  Object result_obj(&scope, runtime.attributeAt(thread, type, dunder_bases));
  ASSERT_TRUE(result_obj.isTuple());
  EXPECT_EQ(Tuple::cast(*result_obj).length(), 0);
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

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C: pass
c = C()
)")
                   .isError());

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

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self):
    global g
    g = 2

g = 1
C()
)")
                   .isError());

  Object global(&scope, moduleAt(&runtime, "__main__", "g"));
  ASSERT_FALSE(global.isError());
  EXPECT_TRUE(isIntEqualsWord(*global, 2));
}

TEST(TypeBuiltinsTest, DunderCallTypeWithInitAndArgs) {
  Runtime runtime;
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self, x):
    global g
    g = x

g = 1
C(9)
)")
                   .isError());

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
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Callable:
  def __call__(self, obj):
    obj.x = 42
class C:
  __init__ = Callable()
c = C()
)")
                   .isError());
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
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __new__(self, *args):
    return 17
  def __init__(self, *args):
    raise Exception("should not happen")
result = type.__call__(C, "C", (), {})
)")
                   .isError());
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

TEST(TypeBuiltinsTest, DunderGetattributeReturnsAttribute) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  foo = -13
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "C"));
  Object name(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(TypeBuiltins::dunderGetattribute, c, name), -13));
}

TEST(TypeBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  pass
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "C"));
  Object name(&scope, runtime.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(TypeBuiltins::dunderGetattribute, c, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST(TypeBuiltinsTest,
     DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  pass
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime, "__main__", "C"));
  Object name(&scope, runtime.newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(TypeBuiltins::dunderGetattribute, c, name),
      LayoutId::kAttributeError, "type object 'C' has no attribute 'xxx'"));
}

TEST(TypeBuiltinsTest, DunderReprForBuiltinReturnsStr) {
  Runtime runtime;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = type.__repr__(object)").isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"),
                              "<class 'object'>"));
}

TEST(TypeBuiltinsTest, DunderReprForUserDefinedTypeReturnsStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  pass
result = type.__repr__(Foo)
)")
                   .isError());
  // TODO(T32810595): Once module names are supported, this should become
  // "<class '__main__.Foo'>".
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime, "__main__", "result"),
                              "<class 'Foo'>"));
}

TEST(TypeBuiltinsTest, DunderNewWithOneArgReturnsTypeOfArg) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
a = type.__new__(type, 1);
b = type.__new__(type, "hello");
)")
                   .isError());
  Type a(&scope, moduleAt(&runtime, "__main__", "a"));
  Type b(&scope, moduleAt(&runtime, "__main__", "b"));

  EXPECT_EQ(Layout::cast(a.instanceLayout()).id(), LayoutId::kInt);
  EXPECT_EQ(Layout::cast(b.instanceLayout()).id(), LayoutId::kStr);
}

TEST(TypeBuiltinsTest, DunderNewWithOneMetaclassArgReturnsType) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo(type):
  pass
a = type.__new__(type, Foo);
)")
                   .isError());
  Type a(&scope, moduleAt(&runtime, "__main__", "a"));
  EXPECT_EQ(Layout::cast(a.instanceLayout()).id(), LayoutId::kType);
}

TEST(TypeBuiltinsTest, DunderSetattrSetsAttribute) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "class C: pass").isError());
  Object c_obj(&scope, moduleAt(&runtime, "__main__", "C"));
  ASSERT_TRUE(c_obj.isType());
  Type c(&scope, *c_obj);
  Object name(&scope, runtime.newStrFromCStr("foo"));
  Object value(&scope, runtime.newInt(-7331));
  EXPECT_TRUE(
      runBuiltin(TypeBuiltins::dunderSetattr, c, name, value).isNoneType());
  Dict type_dict(&scope, c.dict());
  EXPECT_TRUE(
      isIntEqualsWord(runtime.typeDictAt(thread, type_dict, name), -7331));
}

TEST(TypeBuiltinsTest, DunderSetattrWithNonStrNameRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "class C: pass").isError());
  Object c_obj(&scope, moduleAt(&runtime, "__main__", "C"));
  ASSERT_TRUE(c_obj.isType());
  Type c(&scope, *c_obj);
  Object name(&scope, NoneType::object());
  Object value(&scope, runtime.newInt(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(TypeBuiltins::dunderSetattr, c, name, value),
      LayoutId::kTypeError, "attribute name must be string, not 'NoneType'"));
}

TEST(TypeBuiltinsTest, TypeHasDunderMroAttribute) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = str.__class__.__mro__").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
}

TEST(TypeBuiltinsTest, TypeHasDunderNameAttribute) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = str.__class__.__name__").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(Str::cast(*result), "type"));
}

TEST(TypeBuiltinsTest, TypeHasDunderFlagsAttribute) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = str.__class__.__flags__").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isInt());
}

TEST(TypeBuiltinsTest, TypeHasDunderDictAttribute) {
  Runtime runtime;
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime, "result = str.__class__.__dict__").isError());
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result.isDict());
}

TEST(TypeBuiltinsTest, TypeLookupNameInMroReturnsValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  foo = 2
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "A"));
  ASSERT_TRUE(runtime.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeLookupNameInMro(thread, a, foo), 2));
}

TEST(TypeBuiltinsTest, TypeLookupNameInMroReturnsParentValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  foo = 2
class B(A):
  bar = 4
)")
                   .isError());
  Object b_obj(&scope, moduleAt(&runtime, "__main__", "B"));
  ASSERT_TRUE(b_obj.isType());
  Type b(&scope, *b_obj);
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeLookupNameInMro(thread, b, foo), 2));
}

TEST(TypeBuiltinsTest, TypeLookupNameInMroReturnsOverriddenValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  foo = 2
class B(A):
  foo = 4
)")
                   .isError());
  Object b_obj(&scope, moduleAt(&runtime, "__main__", "B"));
  ASSERT_TRUE(b_obj.isType());
  Type b(&scope, *b_obj);
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeLookupNameInMro(thread, b, foo), 4));
}

TEST(TypeBuiltinsTest, TypeLookupNameInMroWithNonExistentNameReturnsError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  bar = 2
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "A"));
  ASSERT_TRUE(runtime.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(typeLookupNameInMro(thread, a, foo).isError());
  EXPECT_TRUE(!thread->hasPendingException());
}

TEST(TypeBuiltinsTest, TypeLookupSymbolInMroReturnsValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class A:
  __add__ = 3
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "A"));
  ASSERT_TRUE(runtime.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  EXPECT_TRUE(isIntEqualsWord(
      typeLookupSymbolInMro(thread, a, SymbolId::kDunderAdd), 3));
}

TEST(TypeBuiltinsTest, DunderCallReceivesExArgs) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  def __init__(self, *args):
    self.args = args

  def num_args(self):
    return len(self.args)

result = C(*(1,2,3)).num_args()
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, RawSmallInt::fromWord(3));
}

TEST(TypeBuiltinsTest, ClassMethodDunderCallReceivesExArgs) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class Foo:
  @classmethod
  def foo(cls, *args):
    return len(args)

result = Foo.foo(*(1,2,3))
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(*result, RawSmallInt::fromWord(3));
}

TEST(TypeBuiltinsTest, TypeNewReceivesExArgs) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
ty = type.__new__(type, *("foo", (object,), {'a': 1}))
name = ty.__name__
)")
                   .isError());
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
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  pass
result = C.mro()
)")
                   .isError());
  HandleScope scope;
  Object ctype(&scope, moduleAt(&runtime, "__main__", "C"));
  Object result_obj(&scope, moduleAt(&runtime, "__main__", "result"));
  ASSERT_TRUE(result_obj.isList());
  List result(&scope, *result_obj);
  EXPECT_EQ(result.at(0), *ctype);
  EXPECT_EQ(result.at(1), runtime.typeAt(LayoutId::kObject));
}

TEST(TypeBuiltinTest, TypeGetAttributeReturnsAttributeValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class C:
  x = 42
)")
                   .isError());
  Object c_obj(&scope, moduleAt(&runtime, "__main__", "C"));
  ASSERT_TRUE(runtime.isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, runtime.newStrFromCStr("x"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread, c, name), 42));
}

TEST(TypeBuiltinTest, TypeGetAttributeReturnsMetaclassAttributeValue) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class M(type):
  x = 77
class C(metaclass=M): pass
)")
                   .isError());
  Object c_obj(&scope, moduleAt(&runtime, "__main__", "C"));
  ASSERT_TRUE(runtime.isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, runtime.newStrFromCStr("x"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread, c, name), 77));
}

TEST(TypeBuiltinTest, TypeGetAttributeWithMissingAttributeReturnsError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "class C: pass").isError());
  Object c_obj(&scope, moduleAt(&runtime, "__main__", "C"));
  ASSERT_TRUE(runtime.isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, runtime.newStrFromCStr("xxx"));
  EXPECT_TRUE(typeGetAttribute(thread, c, name).isError());
  EXPECT_FALSE(thread->hasPendingException());
}

TEST(TypeBuiltinsTest, TypeGetAttributeCallsDunderGetOnDataDescriptor) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return (self, instance, owner)
class M(type):
  foo = D()
class A(metaclass=M): pass
)")
                   .isError());
  Object d_obj(&scope, moduleAt(&runtime, "__main__", "D"));
  ASSERT_TRUE(d_obj.isType());
  Type d(&scope, *d_obj);
  Object m(&scope, moduleAt(&runtime, "__main__", "M"));
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "A"));
  ASSERT_TRUE(runtime.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  Object result_obj(&scope, typeGetAttribute(thread, a, foo));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);
  Type result_0_type(&scope, runtime.typeOf(result.at(0)));
  EXPECT_TRUE(runtime.isSubclass(result_0_type, d));
  EXPECT_EQ(result.at(1), a);
  EXPECT_EQ(result.at(2), m);
}

TEST(TypeBuiltinsTest, TypeGetAttributeCallsDunderGetOnNonDataDescriptor) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __get__(self, instance, owner): return 42
class M(type):
  foo = D()
class A(metaclass=M): pass
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "A"));
  ASSERT_TRUE(runtime.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread, a, foo), 42));
}

TEST(TypeBuiltinsTest, TypeGetAttributePrefersDataDescriptorOverTypeAttr) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return 42
class M(type):
  foo = D()
class A(metaclass=M):
  foo = 12
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "A"));
  ASSERT_TRUE(runtime.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread, a, foo), 42));
}

TEST(TypeBuiltinsTest, TypeGetAttributePrefersFieldOverNonDataDescriptor) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __get__(self, instance, owner): return 42
class M(type):
  foo = D()
class A(metaclass=M):
  foo = 12
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "A"));
  ASSERT_TRUE(runtime.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread, a, foo), 12));
}

TEST(TypeBuiltinsTest, TypeGetAttributePropagatesDunderGetException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): raise UserWarning()
class M(type):
  foo = D()
class A(metaclass=M): pass
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "A"));
  ASSERT_TRUE(runtime.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime.newStrFromCStr("foo"));
  EXPECT_TRUE(raised(typeGetAttribute(thread, a, foo), LayoutId::kUserWarning));
}

TEST(TypeBuiltinsTest, TypeGetAttributeOnNoneTypeReturnsFunction) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type none_type(&scope, runtime.typeAt(LayoutId::kNoneType));
  Object name(&scope, runtime.newStrFromCStr("__repr__"));
  EXPECT_TRUE(typeGetAttribute(thread, none_type, name).isFunction());
}

TEST(TypeBuiltinsTest, TypeSetAttrSetsAttribute) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, "class C: pass").isError());
  Object c_obj(&scope, moduleAt(&runtime, "__main__", "C"));
  ASSERT_TRUE(runtime.isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, runtime.internStrFromCStr(thread, "foobarbaz"));
  Object value(&scope, runtime.newInt(-444));
  EXPECT_TRUE(typeSetAttr(thread, c, name, value).isNoneType());
  Dict type_dict(&scope, c.dict());
  EXPECT_TRUE(
      isIntEqualsWord(runtime.typeDictAt(thread, type_dict, name), -444));
}

TEST(TypeBuiltinsTest, TypeSetAttrCallsDunderSetOnDataDescriptor) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __get__(self, instance, owner): pass
  def __set__(self, instance, value):
    global set_args
    set_args = (self, instance, value)
    return "ignored result"
foo = D()
class M(type):
  foo = foo
class A(metaclass=M):
  foo = "hidden by data descriptor"
)")
                   .isError());
  Object foo(&scope, moduleAt(&runtime, "__main__", "foo"));
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "A"));
  ASSERT_TRUE(runtime.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, runtime.internStrFromCStr(thread, "foo"));
  Object value(&scope, runtime.newInt(77));
  EXPECT_TRUE(typeSetAttr(thread, a, name, value).isNoneType());
  Object set_args_obj(&scope, moduleAt(&runtime, "__main__", "set_args"));
  ASSERT_TRUE(set_args_obj.isTuple());
  Tuple set_args(&scope, *set_args_obj);
  ASSERT_EQ(set_args.length(), 3);
  EXPECT_EQ(set_args.at(0), foo);
  EXPECT_EQ(set_args.at(1), a);
  EXPECT_TRUE(isIntEqualsWord(set_args.at(2), 77));
}

TEST(TypeBuiltinsTest, TypeSetAttrPropagatesDunderSetException) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  ASSERT_FALSE(runFromCStr(&runtime, R"(
class D:
  def __get__(self, instance, owner): pass
  def __set__(self, instance, value): raise UserWarning()
class M(type):
  foo = D()
class A(metaclass=M):
  pass
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime, "__main__", "A"));
  ASSERT_TRUE(runtime.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, runtime.internStrFromCStr(thread, "foo"));
  Object value(&scope, runtime.newInt(1));
  EXPECT_TRUE(
      raised(typeSetAttr(thread, a, name, value), LayoutId::kUserWarning));
}

TEST(TypeBuiltinsTest, TypeSetAttrOnBuiltinTypeRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::current();
  HandleScope scope(thread);
  Type type(&scope, runtime.typeAt(LayoutId::kInt));
  Object name(&scope, runtime.internStrFromCStr(thread, "foo"));
  Object value(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      typeSetAttr(thread, type, name, value), LayoutId::kTypeError,
      "can't set attributes of built-in/extension type 'int'"));
}

TEST(TypeBuiltinTest, TypeofSmallStrReturnsStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = type('a')
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"),
            runtime.typeAt(LayoutId::kStr));
}

TEST(TypeBuiltinTest, TypeofLargeStrReturnsStr) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = type('aaaaaaaaaaaaaaaaaaaaaaa')
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"),
            runtime.typeAt(LayoutId::kStr));
}

TEST(TypeBuiltinTest, TypeofSmallIntReturnsInt) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = type(5)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"),
            runtime.typeAt(LayoutId::kInt));
}

TEST(TypeBuiltinTest, TypeofLargeIntReturnsInt) {
  Runtime runtime;
  ASSERT_FALSE(runFromCStr(&runtime, R"(
result = type(99999999999999999999999999999999999999999)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime, "__main__", "result"),
            runtime.typeAt(LayoutId::kInt));
}

}  // namespace python
