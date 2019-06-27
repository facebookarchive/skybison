#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "test-utils.h"
#include "type-builtins.h"

namespace python {

using namespace testing;

using TypeBuiltinsTest = RuntimeFixture;

TEST_F(TypeBuiltinsTest, DunderBasesReturnsTuple) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A: pass
class B: pass
class C(A, B): pass
)")
                   .isError());
  Object a(&scope, moduleAt(&runtime_, "__main__", "A"));
  Object b(&scope, moduleAt(&runtime_, "__main__", "B"));
  Object c(&scope, moduleAt(&runtime_, "__main__", "C"));
  Object dunder_bases(&scope, runtime_.newStrFromCStr("__bases__"));
  Object result_obj(&scope, runtime_.attributeAt(thread_, c, dunder_bases));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), a);
  EXPECT_EQ(result.at(1), b);
}

TEST_F(TypeBuiltinsTest, DunderBasesOnObjectReturnsEmptyTuple) {
  HandleScope scope(thread_);
  Object type(&scope, runtime_.typeAt(LayoutId::kObject));
  Object dunder_bases(&scope, runtime_.newStrFromCStr("__bases__"));
  Object result_obj(&scope, runtime_.attributeAt(thread_, type, dunder_bases));
  ASSERT_TRUE(result_obj.isTuple());
  EXPECT_EQ(Tuple::cast(*result_obj).length(), 0);
}

TEST_F(TypeBuiltinsTest, DunderBasesOnBuiltinTypeReturnsTuple) {
  HandleScope scope(thread_);
  Object type(&scope, runtime_.typeAt(LayoutId::kInt));
  Object dunder_bases(&scope, runtime_.newStrFromCStr("__bases__"));
  Object result_obj(&scope, runtime_.attributeAt(thread_, type, dunder_bases));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_EQ(result.at(0), runtime_.typeAt(LayoutId::kObject));
}

TEST_F(TypeBuiltinsTest, DunderCallType) {
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C: pass
c = C()
)")
                   .isError());

  Type type(&scope, moduleAt(&runtime_, "__main__", "C"));
  ASSERT_FALSE(type.isError());
  Object instance(&scope, moduleAt(&runtime_, "__main__", "c"));
  ASSERT_FALSE(instance.isError());
  Object instance_type(&scope, runtime_.typeOf(*instance));
  ASSERT_FALSE(instance_type.isError());

  EXPECT_EQ(*type, *instance_type);
}

TEST_F(TypeBuiltinsTest, DunderCallTypeWithInit) {
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    global g
    g = 2

g = 1
C()
)")
                   .isError());

  Object global(&scope, moduleAt(&runtime_, "__main__", "g"));
  ASSERT_FALSE(global.isError());
  EXPECT_TRUE(isIntEqualsWord(*global, 2));
}

TEST_F(TypeBuiltinsTest, DunderCallTypeWithInitAndArgs) {
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self, x):
    global g
    g = x

g = 1
C(9)
)")
                   .isError());

  Object global(&scope, moduleAt(&runtime_, "__main__", "g"));
  ASSERT_FALSE(global.isError());
  EXPECT_TRUE(isIntEqualsWord(*global, 9));
}

TEST_F(TypeBuiltinsTest, DunderCallWithNonTypeRaisesTypeError) {
  ASSERT_TRUE(raisedWithStr(runFromCStr(&runtime_, "type.__call__(5)"),
                            LayoutId::kTypeError,
                            "self must be a type instance"));
}

TEST_F(TypeBuiltinsTest, DunderCallCallsDunderInit) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Callable:
  def __call__(self, obj):
    obj.x = 42
class C:
  __init__ = Callable()
c = C()
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime_, "__main__", "c"));
  Object x(&scope, runtime_.newStrFromCStr("x"));
  RawObject attr = runtime_.attributeAt(thread_, c, x);
  EXPECT_TRUE(isIntEqualsWord(attr, 42));
}

TEST_F(TypeBuiltinsTest,
       DunderCallWithNonTypeDudnerNewResultReturnsWithoutCallingDunderInit) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __new__(self, *args):
    return 17
  def __init__(self, *args):
    raise Exception("should not happen")
result = type.__call__(C, "C", (), {})
)")
                   .isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 17));
}

TEST_F(TypeBuiltinsTest, DunderDirReturnsList) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  x = 42
  def foo(): pass
class B(A):
  def bar(): pass
dir = type.__dir__(B)
)")
                   .isError());
  Object dir(&scope, moduleAt(&runtime_, "__main__", "dir"));
  Object x(&scope, runtime_.newStrFromCStr("x"));
  EXPECT_TRUE(listContains(dir, x));
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(listContains(dir, foo));
  Object bar(&scope, runtime_.newStrFromCStr("bar"));
  EXPECT_TRUE(listContains(dir, bar));
}

TEST_F(TypeBuiltinsTest, DunderDocOnEmptyTypeReturnsNone) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "class C: pass").isError());
  Object c(&scope, moduleAt(&runtime_, "__main__", "C"));
  Object doc(&scope, runtime_.attributeAtId(thread_, c, SymbolId::kDunderDoc));
  EXPECT_EQ(doc, NoneType::object());
}

TEST_F(TypeBuiltinsTest, DunderDocReturnsDocumentationString) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  """hello documentation"""
  pass
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime_, "__main__", "C"));
  Object doc(&scope, runtime_.attributeAtId(thread_, c, SymbolId::kDunderDoc));
  EXPECT_TRUE(isStrEqualsCStr(*doc, "hello documentation"));
}

TEST_F(TypeBuiltinsTest, DunderGetattributeReturnsAttribute) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  foo = -13
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime_, "__main__", "C"));
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(
      runBuiltin(TypeBuiltins::dunderGetattribute, c, name), -13));
}

TEST_F(TypeBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  pass
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime_, "__main__", "C"));
  Object name(&scope, runtime_.newInt(0));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(TypeBuiltins::dunderGetattribute, c, name),
      LayoutId::kTypeError, "attribute name must be string, not 'int'"));
}

TEST_F(TypeBuiltinsTest,
       DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  pass
)")
                   .isError());
  Object c(&scope, moduleAt(&runtime_, "__main__", "C"));
  Object name(&scope, runtime_.newStrFromCStr("xxx"));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(TypeBuiltins::dunderGetattribute, c, name),
      LayoutId::kAttributeError, "type object 'C' has no attribute 'xxx'"));
}

TEST_F(TypeBuiltinsTest, DunderReprForBuiltinReturnsStr) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = type.__repr__(object)").isError());
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "result"),
                              "<class 'object'>"));
}

TEST_F(TypeBuiltinsTest, DunderReprForUserDefinedTypeReturnsStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  pass
result = type.__repr__(Foo)
)")
                   .isError());
  // TODO(T32810595): Once module names are supported, this should become
  // "<class '__main__.Foo'>".
  EXPECT_TRUE(isStrEqualsCStr(moduleAt(&runtime_, "__main__", "result"),
                              "<class 'Foo'>"));
}

TEST_F(TypeBuiltinsTest, DunderNewWithOneArgReturnsTypeOfArg) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = type.__new__(type, 1);
b = type.__new__(type, "hello");
)")
                   .isError());
  Type a(&scope, moduleAt(&runtime_, "__main__", "a"));
  Type b(&scope, moduleAt(&runtime_, "__main__", "b"));

  EXPECT_EQ(Layout::cast(a.instanceLayout()).id(), LayoutId::kInt);
  EXPECT_EQ(Layout::cast(b.instanceLayout()).id(), LayoutId::kStr);
}

TEST_F(TypeBuiltinsTest, DunderNewWithOneMetaclassArgReturnsType) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(type):
  pass
a = type.__new__(type, Foo);
)")
                   .isError());
  Type a(&scope, moduleAt(&runtime_, "__main__", "a"));
  EXPECT_EQ(Layout::cast(a.instanceLayout()).id(), LayoutId::kType);
}

TEST_F(TypeBuiltinsTest, DunderSetattrSetsAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "class C: pass").isError());
  Object c_obj(&scope, moduleAt(&runtime_, "__main__", "C"));
  ASSERT_TRUE(c_obj.isType());
  Type c(&scope, *c_obj);
  Object name(&scope, runtime_.newStrFromCStr("foo"));
  Object value(&scope, runtime_.newInt(-7331));
  EXPECT_TRUE(
      runBuiltin(TypeBuiltins::dunderSetattr, c, name, value).isNoneType());
  Dict type_dict(&scope, c.dict());
  EXPECT_TRUE(
      isIntEqualsWord(runtime_.typeDictAt(thread_, type_dict, name), -7331));
}

TEST_F(TypeBuiltinsTest, DunderSetattrWithNonStrNameRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "class C: pass").isError());
  Object c_obj(&scope, moduleAt(&runtime_, "__main__", "C"));
  ASSERT_TRUE(c_obj.isType());
  Type c(&scope, *c_obj);
  Object name(&scope, NoneType::object());
  Object value(&scope, runtime_.newInt(1));
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(TypeBuiltins::dunderSetattr, c, name, value),
      LayoutId::kTypeError, "attribute name must be string, not 'NoneType'"));
}

TEST_F(TypeBuiltinsTest, TypeHasDunderMroAttribute) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = str.__class__.__mro__").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isTuple());
}

TEST_F(TypeBuiltinsTest, TypeHasDunderNameAttribute) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = str.__class__.__name__").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isStr());
  EXPECT_TRUE(isStrEqualsCStr(Str::cast(*result), "type"));
}

TEST_F(TypeBuiltinsTest, TypeHasDunderFlagsAttribute) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = str.__class__.__flags__").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isInt());
}

TEST_F(TypeBuiltinsTest, TypeHasDunderDictAttribute) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = str.__class__.__dict__").isError());
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result.isDict());
}

TEST_F(TypeBuiltinsTest, TypeLookupNameInMroReturnsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  foo = 2
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeLookupNameInMro(thread_, a, foo), 2));
}

TEST_F(TypeBuiltinsTest, TypeLookupNameInMroReturnsParentValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  foo = 2
class B(A):
  bar = 4
)")
                   .isError());
  Object b_obj(&scope, moduleAt(&runtime_, "__main__", "B"));
  ASSERT_TRUE(b_obj.isType());
  Type b(&scope, *b_obj);
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeLookupNameInMro(thread_, b, foo), 2));
}

TEST_F(TypeBuiltinsTest, TypeLookupNameInMroReturnsOverriddenValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  foo = 2
class B(A):
  foo = 4
)")
                   .isError());
  Object b_obj(&scope, moduleAt(&runtime_, "__main__", "B"));
  ASSERT_TRUE(b_obj.isType());
  Type b(&scope, *b_obj);
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeLookupNameInMro(thread_, b, foo), 4));
}

TEST_F(TypeBuiltinsTest, TypeLookupNameInMroWithNonExistentNameReturnsError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  bar = 2
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(typeLookupNameInMro(thread_, a, foo).isError());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(TypeBuiltinsTest, TypeLookupSymbolInMroReturnsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  __add__ = 3
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  EXPECT_TRUE(isIntEqualsWord(
      typeLookupSymbolInMro(thread_, a, SymbolId::kDunderAdd), 3));
}

TEST_F(TypeBuiltinsTest, DunderCallReceivesExArgs) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self, *args):
    self.args = args

  def num_args(self):
    return len(self.args)

result = C(*(1,2,3)).num_args()
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(*result, RawSmallInt::fromWord(3));
}

TEST_F(TypeBuiltinsTest, ClassMethodDunderCallReceivesExArgs) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  @classmethod
  def foo(cls, *args):
    return len(args)

result = Foo.foo(*(1,2,3))
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, moduleAt(&runtime_, "__main__", "result"));
  EXPECT_EQ(*result, RawSmallInt::fromWord(3));
}

TEST_F(TypeBuiltinsTest, TypeNewReceivesExArgs) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
ty = type.__new__(type, *("foo", (object,), {'a': 1}))
name = ty.__name__
)")
                   .isError());
  HandleScope scope;
  Object name(&scope, moduleAt(&runtime_, "__main__", "name"));
  EXPECT_TRUE(isStrEqualsCStr(*name, "foo"));
}

TEST_F(TypeBuiltinsTest, TypeCallWithInitReturningNonNoneRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class C:
  def __init__(self, *args, **kwargs):
    return 5
C()
)"),
                            LayoutId::kTypeError,
                            "C.__init__ returned non None"));
}

TEST_F(TypeBuiltinsTest, MroReturnsList) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  pass
result = C.mro()
)")
                   .isError());
  HandleScope scope;
  Object ctype(&scope, moduleAt(&runtime_, "__main__", "C"));
  Object result_obj(&scope, moduleAt(&runtime_, "__main__", "result"));
  ASSERT_TRUE(result_obj.isList());
  List result(&scope, *result_obj);
  EXPECT_EQ(result.at(0), *ctype);
  EXPECT_EQ(result.at(1), runtime_.typeAt(LayoutId::kObject));
}

TEST_F(TypeBuiltinsTest, MroWithMultipleInheritanceReturnsLinearization) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  pass
class B:
  pass
class C(A, B):
  pass
)")
                   .isError());
  HandleScope scope(thread_);
  Object atype(&scope, moduleAt(&runtime_, "__main__", "A"));
  Object btype(&scope, moduleAt(&runtime_, "__main__", "B"));
  Object ctype(&scope, moduleAt(&runtime_, "__main__", "C"));
  Object result_obj(&scope, runBuiltin(TypeBuiltins::mro, ctype));
  ASSERT_TRUE(result_obj.isList());
  List result(&scope, *result_obj);
  EXPECT_EQ(result.at(0), *ctype);
  EXPECT_EQ(result.at(1), *atype);
  EXPECT_EQ(result.at(2), *btype);
  EXPECT_EQ(result.at(3), runtime_.typeAt(LayoutId::kObject));
}

TEST_F(TypeBuiltinsTest, MroWithInvalidLinearizationRaisesTypeError) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_.newType());
  Tuple bases(&scope, runtime_.newTuple(2));
  bases.atPut(0, runtime_.typeAt(LayoutId::kObject));
  bases.atPut(1, runtime_.typeAt(LayoutId::kInt));
  type.setBases(*bases);
  EXPECT_TRUE(raisedWithStr(runBuiltin(TypeBuiltins::mro, type),
                            LayoutId::kTypeError,
                            "Cannot create a consistent method resolution "
                            "order (MRO) for bases object, int"));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeReturnsAttributeValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  x = 42
)")
                   .isError());
  Object c_obj(&scope, moduleAt(&runtime_, "__main__", "C"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, runtime_.newStrFromCStr("x"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread_, c, name), 42));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeReturnsMetaclassAttributeValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class M(type):
  x = 77
class C(metaclass=M): pass
)")
                   .isError());
  Object c_obj(&scope, moduleAt(&runtime_, "__main__", "C"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, runtime_.newStrFromCStr("x"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread_, c, name), 77));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeWithMissingAttributeReturnsError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "class C: pass").isError());
  Object c_obj(&scope, moduleAt(&runtime_, "__main__", "C"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, runtime_.newStrFromCStr("xxx"));
  EXPECT_TRUE(typeGetAttribute(thread_, c, name).isError());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeCallsDunderGetOnDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return (self, instance, owner)
class M(type):
  foo = D()
class A(metaclass=M): pass
)")
                   .isError());
  Object d_obj(&scope, moduleAt(&runtime_, "__main__", "D"));
  ASSERT_TRUE(d_obj.isType());
  Type d(&scope, *d_obj);
  Object m(&scope, moduleAt(&runtime_, "__main__", "M"));
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  Object result_obj(&scope, typeGetAttribute(thread_, a, foo));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);
  Type result_0_type(&scope, runtime_.typeOf(result.at(0)));
  EXPECT_TRUE(runtime_.isSubclass(result_0_type, d));
  EXPECT_EQ(result.at(1), a);
  EXPECT_EQ(result.at(2), m);
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeCallsDunderGetOnNonDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __get__(self, instance, owner): return 42
class M(type):
  foo = D()
class A(metaclass=M): pass
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread_, a, foo), 42));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributePrefersDataDescriptorOverTypeAttr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return 42
class M(type):
  foo = D()
class A(metaclass=M):
  foo = 12
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread_, a, foo), 42));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributePrefersFieldOverNonDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __get__(self, instance, owner): return 42
class M(type):
  foo = D()
class A(metaclass=M):
  foo = 12
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread_, a, foo), 12));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributePropagatesDunderGetException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): raise UserWarning()
class M(type):
  foo = D()
class A(metaclass=M): pass
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, runtime_.newStrFromCStr("foo"));
  EXPECT_TRUE(
      raised(typeGetAttribute(thread_, a, foo), LayoutId::kUserWarning));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeOnNoneTypeReturnsFunction) {
  HandleScope scope(thread_);
  Type none_type(&scope, runtime_.typeAt(LayoutId::kNoneType));
  Object name(&scope, runtime_.newStrFromCStr("__repr__"));
  EXPECT_TRUE(typeGetAttribute(thread_, none_type, name).isFunction());
}

TEST_F(TypeBuiltinsTest, TypeSetAttrSetsAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "class C: pass").isError());
  Object c_obj(&scope, moduleAt(&runtime_, "__main__", "C"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, runtime_.internStrFromCStr(thread_, "foobarbaz"));
  Object value(&scope, runtime_.newInt(-444));
  EXPECT_TRUE(typeSetAttr(thread_, c, name, value).isNoneType());
  Dict type_dict(&scope, c.dict());
  EXPECT_TRUE(
      isIntEqualsWord(runtime_.typeDictAt(thread_, type_dict, name), -444));
}

TEST_F(TypeBuiltinsTest, TypeSetAttrCallsDunderSetOnDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
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
  Object foo(&scope, moduleAt(&runtime_, "__main__", "foo"));
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_.newInt(77));
  EXPECT_TRUE(typeSetAttr(thread_, a, name, value).isNoneType());
  Object set_args_obj(&scope, moduleAt(&runtime_, "__main__", "set_args"));
  ASSERT_TRUE(set_args_obj.isTuple());
  Tuple set_args(&scope, *set_args_obj);
  ASSERT_EQ(set_args.length(), 3);
  EXPECT_EQ(set_args.at(0), foo);
  EXPECT_EQ(set_args.at(1), a);
  EXPECT_TRUE(isIntEqualsWord(set_args.at(2), 77));
}

TEST_F(TypeBuiltinsTest, TypeSetAttrPropagatesDunderSetException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class D:
  def __get__(self, instance, owner): pass
  def __set__(self, instance, value): raise UserWarning()
class M(type):
  foo = D()
class A(metaclass=M):
  pass
)")
                   .isError());
  Object a_obj(&scope, moduleAt(&runtime_, "__main__", "A"));
  ASSERT_TRUE(runtime_.isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_.newInt(1));
  EXPECT_TRUE(
      raised(typeSetAttr(thread_, a, name, value), LayoutId::kUserWarning));
}

TEST_F(TypeBuiltinsTest, TypeSetAttrOnBuiltinTypeRaisesTypeError) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_.typeAt(LayoutId::kInt));
  Object name(&scope, runtime_.internStrFromCStr(thread_, "foo"));
  Object value(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      typeSetAttr(thread_, type, name, value), LayoutId::kTypeError,
      "can't set attributes of built-in/extension type 'int'"));
}

TEST_F(TypeBuiltinsTest, TypeofSmallStrReturnsStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = type('a')
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"),
            runtime_.typeAt(LayoutId::kStr));
}

TEST_F(TypeBuiltinsTest, TypeofLargeStrReturnsStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = type('aaaaaaaaaaaaaaaaaaaaaaa')
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"),
            runtime_.typeAt(LayoutId::kStr));
}

TEST_F(TypeBuiltinsTest, TypeofSmallIntReturnsInt) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = type(5)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"),
            runtime_.typeAt(LayoutId::kInt));
}

TEST_F(TypeBuiltinsTest, TypeofLargeIntReturnsInt) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = type(99999999999999999999999999999999999999999)
)")
                   .isError());
  EXPECT_EQ(moduleAt(&runtime_, "__main__", "result"),
            runtime_.typeAt(LayoutId::kInt));
}

TEST_F(TypeBuiltinsTest, ResolveDescriptorGetReturnsNonDescriptor) {
  HandleScope scope(thread_);

  Object instance(&scope, runtime_.newInt(123));
  Object owner(&scope, NoneType::object());
  Object descr(&scope, runtime_.newInt(456));
  EXPECT_EQ(resolveDescriptorGet(thread_, descr, instance, owner), *descr);
}

TEST_F(TypeBuiltinsTest, ResolveDescriptorGetCallsDescriptorDunderGet) {
  HandleScope scope(thread_);

  Object instance(&scope, runtime_.newInt(123));
  Type owner(&scope, runtime_.typeOf(*instance));
  Object descr(&scope,
               typeLookupSymbolInMro(thread_, owner, SymbolId::kDunderAdd));
  ASSERT_TRUE(descr.isFunction());
  EXPECT_TRUE(
      resolveDescriptorGet(thread_, descr, instance, owner).isBoundMethod());
}

}  // namespace python
