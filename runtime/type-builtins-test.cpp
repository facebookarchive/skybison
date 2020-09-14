#include "type-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "dict-builtins.h"
#include "handles.h"
#include "ic.h"
#include "module-builtins.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "test-utils.h"

namespace py {
namespace testing {

using TypeBuiltinsDeathTest = RuntimeFixture;
using TypeBuiltinsTest = RuntimeFixture;

TEST_F(TypeBuiltinsTest, TypeAtReturnsNoPlaceholderValue) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->newType());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "__eq__"));
  Object value(&scope, Runtime::internStrFromCStr(thread_, "__eq__'s value"));
  typeAtPut(thread_, type, name, value);
  EXPECT_EQ(typeAt(type, name), *value);
  EXPECT_EQ(typeAtById(thread_, type, ID(__eq__)), *value);
}

TEST_F(TypeBuiltinsTest, TypeAtReturnsErrorNotFoundForPlaceholder) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->newType());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "__eq__"));
  Object value(&scope, Runtime::internStrFromCStr(thread_, "__eq__'s value"));
  ValueCell value_cell(&scope, typeAtPut(thread_, type, name, value));
  value_cell.makePlaceholder();
  EXPECT_TRUE(typeAt(type, name).isErrorNotFound());
  EXPECT_TRUE(typeAtById(thread_, type, ID(__eq__)).isErrorNotFound());
}

TEST_F(TypeBuiltinsTest, TypeAtPutPutsValueInValueCell) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->newType());
  Object name(&scope, Runtime::internStrFromCStr(thread_, "__eq__"));
  Object value(&scope, Runtime::internStrFromCStr(thread_, "__eq__'s value"));

  ValueCell result(&scope, typeAtPut(thread_, type, name, value));
  ASSERT_EQ(result.value(), *value);
  EXPECT_EQ(typeAt(type, name), *value);
  result.setValue(NoneType::object());

  result = typeAtPutById(thread_, type, ID(__eq__), value);
  ASSERT_EQ(result.value(), *value);
  EXPECT_EQ(typeAtById(thread_, type, ID(__eq__)), *value);
}

TEST_F(TypeBuiltinsTest, TypeAtPutByStrInvalidatesCache) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  def foo(self): return 4

def cache_a_foo(a):
  return a.foo

a = A()
cache_a_foo(a)
)")
                   .isError());
  HandleScope scope(thread_);
  Function cache_a_foo(&scope, mainModuleAt(runtime_, "cache_a_foo"));
  MutableTuple caches(&scope, cache_a_foo.caches());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_FALSE(icLookupAttr(*caches, 1, a.layoutId()).isErrorNotFound());

  Type type_a(&scope, mainModuleAt(runtime_, "A"));
  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object none(&scope, NoneType::object());
  typeAtPut(thread_, type_a, foo, none);
  EXPECT_TRUE(icLookupAttr(*caches, 1, a.layoutId()).isErrorNotFound());
}

TEST_F(TypeBuiltinsTest, TypeAtPutByIdInvalidatesCache) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  def __eq__(self, other): return True

def cache_a_eq(a):
  return a.__eq__

a = A()
cache_a_eq(a)
)")
                   .isError());
  HandleScope scope(thread_);
  Function cache_a_eq(&scope, mainModuleAt(runtime_, "cache_a_eq"));
  MutableTuple caches(&scope, cache_a_eq.caches());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_FALSE(icLookupAttr(*caches, 1, a.layoutId()).isErrorNotFound());

  Type type_a(&scope, mainModuleAt(runtime_, "A"));
  Object none(&scope, NoneType::object());
  typeAtPutById(thread_, type_a, ID(__eq__), none);
  EXPECT_TRUE(icLookupAttr(*caches, 1, a.layoutId()).isErrorNotFound());
}

TEST_F(TypeBuiltinsTest, TypeAtPutDoesNotGrowOnTombstones) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->newType());
  ASSERT_TRUE(type.attributes().isTuple());
  word initial_capacity = Tuple::cast(type.attributes()).length();
  // Insert and remove symbols to fill the dictionary with tombstones.
  Object name(&scope, NoneType::object());
  for (word i = 0; i < static_cast<word>(SymbolId::kMaxId); i++) {
    name = runtime_->symbols()->at(static_cast<SymbolId>(i));
    typeValueCellAtPut(thread_, type, name);
    typeRemove(thread_, type, name);
  }
  EXPECT_EQ(Tuple::cast(type.attributes()).length(), initial_capacity);
}

TEST_F(TypeBuiltinsTest, TypeRemoveForNonExistingEntryReturnsErrorNotFound) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  def __eq__(self, other): return True
)")
                   .isError());
  HandleScope scope(thread_);
  Type type(&scope, mainModuleAt(runtime_, "A"));
  Str dunder_gt(&scope, runtime_->newStrFromCStr("__gt__"));
  EXPECT_TRUE(typeRemove(thread_, type, dunder_gt).isErrorNotFound());
}

TEST_F(TypeBuiltinsTest, TypeRemoveRemovesAssociatedEntry) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  def __eq__(self, other): return True
)")
                   .isError());
  HandleScope scope(thread_);
  Type type(&scope, mainModuleAt(runtime_, "A"));
  Object dunder_eq(&scope, Runtime::internStrFromCStr(thread_, "__eq__"));
  ASSERT_FALSE(typeAt(type, dunder_eq).isErrorNotFound());
  ASSERT_FALSE(typeRemove(thread_, type, dunder_eq).isErrorNotFound());
  EXPECT_TRUE(typeAt(type, dunder_eq).isErrorNotFound());
}

TEST_F(TypeBuiltinsTest, TypeRemoveInvalidatesCache) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  def __eq__(self, other): return True

def cache_a_eq(a):
  return a.__eq__

a = A()
cache_a_eq(a)
)")
                   .isError());
  HandleScope scope(thread_);
  Function cache_a_eq(&scope, mainModuleAt(runtime_, "cache_a_eq"));
  MutableTuple caches(&scope, cache_a_eq.caches());
  Object a(&scope, mainModuleAt(runtime_, "a"));
  ASSERT_FALSE(icLookupAttr(*caches, 1, a.layoutId()).isErrorNotFound());

  Type type_a(&scope, mainModuleAt(runtime_, "A"));
  Str dunder_eq(&scope, runtime_->newStrFromCStr("__eq__"));
  ASSERT_FALSE(typeRemove(thread_, type_a, dunder_eq).isErrorNotFound());
  EXPECT_TRUE(icLookupAttr(*caches, 1, a.layoutId()).isErrorNotFound());
}

TEST_F(TypeBuiltinsTest, TypeKeysFiltersOutPlaceholders) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->newType());

  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object baz(&scope, Runtime::internStrFromCStr(thread_, "baz"));
  Str value(&scope, runtime_->newStrFromCStr("value"));

  typeAtPut(thread_, type, foo, value);
  typeAtPut(thread_, type, bar, value);
  typeAtPut(thread_, type, baz, value);

  ValueCell::cast(typeValueCellAt(type, bar)).makePlaceholder();

  List keys(&scope, typeKeys(thread_, type));
  EXPECT_EQ(keys.numItems(), 2);
  EXPECT_EQ(keys.at(0), *foo);
  EXPECT_EQ(keys.at(1), *baz);
}

TEST_F(TypeBuiltinsTest, TypeLenReturnsItemCountExcludingPlaceholders) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->newType());

  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object baz(&scope, Runtime::internStrFromCStr(thread_, "baz"));
  Str value(&scope, runtime_->newStrFromCStr("value"));

  typeAtPut(thread_, type, foo, value);
  typeAtPut(thread_, type, bar, value);
  typeAtPut(thread_, type, baz, value);

  SmallInt previous_len(&scope, typeLen(thread_, type));

  ValueCell::cast(typeValueCellAt(type, bar)).makePlaceholder();

  SmallInt after_len(&scope, typeLen(thread_, type));
  EXPECT_EQ(previous_len.value(), after_len.value() + 1);
}

TEST_F(TypeBuiltinsTest, TypeValuesFiltersOutPlaceholders) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->newType());

  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object foo_value(&scope, runtime_->newStrFromCStr("foo_value"));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  Object bar_value(&scope, runtime_->newStrFromCStr("bar_value"));
  Object baz(&scope, Runtime::internStrFromCStr(thread_, "baz"));
  Object baz_value(&scope, runtime_->newStrFromCStr("baz_value"));

  typeAtPut(thread_, type, foo, foo_value);
  typeAtPut(thread_, type, bar, bar_value);
  typeAtPut(thread_, type, baz, baz_value);

  ValueCell::cast(typeValueCellAt(type, bar)).makePlaceholder();

  List values(&scope, typeValues(thread_, type));
  EXPECT_TRUE(listContains(values, foo_value));
  EXPECT_FALSE(listContains(values, bar_value));
  EXPECT_TRUE(listContains(values, baz_value));
}

TEST_F(TypeBuiltinsTest, DunderBasesReturnsTuple) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A: pass
class B: pass
class C(A, B): pass
)")
                   .isError());
  Object a(&scope, mainModuleAt(runtime_, "A"));
  Object b(&scope, mainModuleAt(runtime_, "B"));
  Object c(&scope, mainModuleAt(runtime_, "C"));
  Object result_obj(&scope,
                    runtime_->attributeAtById(thread_, c, ID(__bases__)));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 2);
  EXPECT_EQ(result.at(0), a);
  EXPECT_EQ(result.at(1), b);
}

TEST_F(TypeBuiltinsTest, DunderBasesOnObjectReturnsEmptyTuple) {
  HandleScope scope(thread_);
  Object type(&scope, runtime_->typeAt(LayoutId::kObject));
  Object result_obj(&scope,
                    runtime_->attributeAtById(thread_, type, ID(__bases__)));
  ASSERT_TRUE(result_obj.isTuple());
  EXPECT_EQ(Tuple::cast(*result_obj).length(), 0);
}

TEST_F(TypeBuiltinsTest, DunderBasesOnBuiltinTypeReturnsTuple) {
  HandleScope scope(thread_);
  Object type(&scope, runtime_->typeAt(LayoutId::kInt));
  Object result_obj(&scope,
                    runtime_->attributeAtById(thread_, type, ID(__bases__)));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 1);
  EXPECT_EQ(result.at(0), runtime_->typeAt(LayoutId::kObject));
}

TEST_F(TypeBuiltinsTest, DunderCallType) {
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C: pass
c = C()
)")
                   .isError());

  Type type(&scope, mainModuleAt(runtime_, "C"));
  ASSERT_FALSE(type.isError());
  Object instance(&scope, mainModuleAt(runtime_, "c"));
  ASSERT_FALSE(instance.isError());
  Object instance_type(&scope, runtime_->typeOf(*instance));
  ASSERT_FALSE(instance_type.isError());

  EXPECT_EQ(*type, *instance_type);
}

TEST_F(TypeBuiltinsTest, DunderCallTypeWithInit) {
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    global g
    g = 2

g = 1
C()
)")
                   .isError());

  Object global(&scope, mainModuleAt(runtime_, "g"));
  ASSERT_FALSE(global.isError());
  EXPECT_TRUE(isIntEqualsWord(*global, 2));
}

TEST_F(TypeBuiltinsTest, DunderCallTypeWithInitAndArgs) {
  HandleScope scope;

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self, x):
    global g
    g = x

g = 1
C(9)
)")
                   .isError());

  Object global(&scope, mainModuleAt(runtime_, "g"));
  ASSERT_FALSE(global.isError());
  EXPECT_TRUE(isIntEqualsWord(*global, 9));
}

TEST_F(TypeBuiltinsTest, DunderCallWithNonTypeRaisesTypeError) {
  ASSERT_TRUE(raisedWithStr(
      runFromCStr(runtime_, "type.__call__(5)"), LayoutId::kTypeError,
      "'__call__' requires a 'type' object but received a 'int'"));
}

TEST_F(TypeBuiltinsTest,
       DunderCallWithNonTypeDudnerNewResultReturnsWithoutCallingDunderInit) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __new__(self, *args):
    return 17
  def __init__(self, *args):
    raise Exception("should not happen")
result = type.__call__(C, "C", (), {})
)")
                   .isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 17));
}

TEST_F(TypeBuiltinsTest, DunderDirReturnsList) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  x = 42
  def foo(): pass
class B(A):
  def bar(): pass
dir = type.__dir__(B)
)")
                   .isError());
  Object dir(&scope, mainModuleAt(runtime_, "dir"));
  Object x(&scope, Runtime::internStrFromCStr(thread_, "x"));
  EXPECT_TRUE(listContains(dir, x));
  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(listContains(dir, foo));
  Object bar(&scope, Runtime::internStrFromCStr(thread_, "bar"));
  EXPECT_TRUE(listContains(dir, bar));
}

TEST_F(TypeBuiltinsTest, DunderDocOnEmptyTypeReturnsNone) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "class C: pass").isError());
  Object c(&scope, mainModuleAt(runtime_, "C"));
  Object doc(&scope, runtime_->attributeAtById(thread_, c, ID(__doc__)));
  EXPECT_EQ(doc, NoneType::object());
}

TEST_F(TypeBuiltinsTest, DunderDocReturnsDocumentationString) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  """hello documentation"""
  pass
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "C"));
  Object doc(&scope, runtime_->attributeAtById(thread_, c, ID(__doc__)));
  EXPECT_TRUE(isStrEqualsCStr(*doc, "hello documentation"));
}

TEST_F(TypeBuiltinsTest, DunderGetattributeReturnsAttribute) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  foo = -13
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "C"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(
      isIntEqualsWord(runBuiltin(METH(type, __getattribute__), c, name), -13));
}

TEST_F(TypeBuiltinsTest, DunderGetattributeWithNonStringNameRaisesTypeError) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "C"));
  Object name(&scope, runtime_->newInt(0));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(type, __getattribute__), c, name),
                            LayoutId::kTypeError,
                            "attribute name must be string, not 'int'"));
}

TEST_F(TypeBuiltinsTest,
       DunderGetattributeWithMissingAttributeRaisesAttributeError) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass
)")
                   .isError());
  Object c(&scope, mainModuleAt(runtime_, "C"));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "xxx"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(type, __getattribute__), c, name),
                            LayoutId::kAttributeError,
                            "type object 'C' has no attribute 'xxx'"));
}

TEST_F(TypeBuiltinsTest, DunderReprForBuiltinReturnsStr) {
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = type.__repr__(object)").isError());
  EXPECT_TRUE(
      isStrEqualsCStr(mainModuleAt(runtime_, "result"), "<class 'object'>"));
}

TEST_F(TypeBuiltinsTest, DunderReprForUserDefinedTypeReturnsStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  pass
result = type.__repr__(Foo)
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(runtime_, "result"),
                              "<class '__main__.Foo'>"));
}

TEST_F(TypeBuiltinsTest, DunderNewWithOneArgReturnsTypeOfArg) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(runtime_, R"(
a = type.__new__(type, 1);
b = type.__new__(type, "hello");
)")
                   .isError());
  Type a(&scope, mainModuleAt(runtime_, "a"));
  Type b(&scope, mainModuleAt(runtime_, "b"));

  EXPECT_EQ(a.instanceLayoutId(), LayoutId::kInt);
  EXPECT_EQ(b.instanceLayoutId(), LayoutId::kStr);
}

TEST_F(TypeBuiltinsTest, DunderNewWithOneMetaclassArgReturnsType) {
  HandleScope scope;
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(type):
  pass
a = type.__new__(type, Foo);
)")
                   .isError());
  Type a(&scope, mainModuleAt(runtime_, "a"));
  EXPECT_EQ(a.instanceLayoutId(), LayoutId::kType);
}

TEST_F(TypeBuiltinsTest, DunderSetattrSetsAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "class C: pass").isError());
  Object c_obj(&scope, mainModuleAt(runtime_, "C"));
  ASSERT_TRUE(c_obj.isType());
  Type c(&scope, *c_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_->newInt(-7331));
  EXPECT_TRUE(runBuiltin(METH(type, __setattr__), c, name, value).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(typeAt(c, name), -7331));
}

TEST_F(TypeBuiltinsTest, DunderSetattrWithNonStrNameRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "class C: pass").isError());
  Object c_obj(&scope, mainModuleAt(runtime_, "C"));
  ASSERT_TRUE(c_obj.isType());
  Type c(&scope, *c_obj);
  Object name(&scope, NoneType::object());
  Object value(&scope, runtime_->newInt(1));
  EXPECT_TRUE(raisedWithStr(runBuiltin(METH(type, __setattr__), c, name, value),
                            LayoutId::kTypeError,
                            "attribute name must be string, not 'NoneType'"));
}

TEST_F(TypeBuiltinsTest, DunderSlotsCreatesLayoutWithInObjectAttributes) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  __slots__ = "x", "y", "z"
)")
                   .isError());
  HandleScope scope(thread_);
  Type c(&scope, mainModuleAt(runtime_, "C"));
  EXPECT_TRUE(c.hasFlag(Type::Flag::kHasSlots));

  Layout layout(&scope, c.instanceLayout());
  EXPECT_TRUE(layout.isSealed());

  Tuple attributes(&scope, layout.inObjectAttributes());
  ASSERT_EQ(attributes.length(), 3);

  Tuple elt0(&scope, attributes.at(0));
  EXPECT_TRUE(isStrEqualsCStr(elt0.at(0), "x"));
  AttributeInfo info0(elt0.at(1));
  EXPECT_TRUE(info0.isInObject());
  EXPECT_TRUE(info0.isFixedOffset());

  Tuple elt1(&scope, attributes.at(1));
  EXPECT_TRUE(isStrEqualsCStr(elt1.at(0), "y"));
  AttributeInfo info1(elt1.at(1));
  EXPECT_TRUE(info1.isInObject());
  EXPECT_TRUE(info1.isFixedOffset());

  Tuple elt2(&scope, attributes.at(2));
  EXPECT_TRUE(isStrEqualsCStr(elt2.at(0), "z"));
  AttributeInfo info2(elt2.at(1));
  EXPECT_TRUE(info2.isInObject());
  EXPECT_TRUE(info2.isFixedOffset());
}

TEST_F(TypeBuiltinsTest, DunderSlotsWithEmptyTupleCreatesEmptyLayout) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  __slots__ = ()
)")
                   .isError());
  HandleScope scope(thread_);
  Type c(&scope, mainModuleAt(runtime_, "C"));
  EXPECT_FALSE(c.hasFlag(Type::Flag::kHasSlots));

  Layout layout(&scope, c.instanceLayout());
  EXPECT_TRUE(layout.isSealed());

  Tuple attributes(&scope, layout.inObjectAttributes());
  ASSERT_EQ(attributes.length(), 0);
}

TEST_F(TypeBuiltinsTest, DunderSlotsAreInheritedFromLayoutBase) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass

class D(C):
  __slots__ = "x", "y"

class E(D, C):
  __slots__ = "z"
)")
                   .isError());
  HandleScope scope(thread_);
  Type e(&scope, mainModuleAt(runtime_, "E"));
  Layout layout(&scope, e.instanceLayout());
  // The layout of E is not sealed due to C.
  EXPECT_FALSE(layout.isSealed());
  Tuple attributes(&scope, layout.inObjectAttributes());
  // D is chosen as the layout base for E since D is a subtype of C.
  ASSERT_EQ(attributes.length(), 3);

  Tuple elt0(&scope, attributes.at(0));
  EXPECT_TRUE(isStrEqualsCStr(elt0.at(0), "x"));
  AttributeInfo info0(elt0.at(1));
  EXPECT_TRUE(info0.isInObject());
  EXPECT_TRUE(info0.isFixedOffset());

  Tuple elt1(&scope, attributes.at(1));
  EXPECT_TRUE(isStrEqualsCStr(elt1.at(0), "y"));
  AttributeInfo info1(elt1.at(1));
  EXPECT_TRUE(info1.isInObject());
  EXPECT_TRUE(info1.isFixedOffset());

  Tuple elt2(&scope, attributes.at(2));
  EXPECT_TRUE(isStrEqualsCStr(elt2.at(0), "z"));
  AttributeInfo info2(elt2.at(1));
  EXPECT_TRUE(info2.isInObject());
  EXPECT_TRUE(info2.isFixedOffset());
}

TEST_F(TypeBuiltinsTest, DunderSlotsSealsTypeWhenAllBasesAreSealed) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  __slots__ = ()

class D:
  __slots__ = ()

class E(C, D):
  __slots__ = "x"
)")
                   .isError());
  HandleScope scope(thread_);
  Type e(&scope, mainModuleAt(runtime_, "E"));
  Layout layout(&scope, e.instanceLayout());
  EXPECT_TRUE(layout.isSealed());
}

TEST_F(TypeBuiltinsTest, DunderSlotsDoesNotSealTypeWhenBaseHasDunderDict) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  pass

class D:
  __slots__ = ()

class E(C, D):
  __slots__ = "x"
)")
                   .isError());
  HandleScope scope(thread_);
  Type c(&scope, mainModuleAt(runtime_, "C"));
  ASSERT_FALSE(Layout::cast(c.instanceLayout()).isSealed());

  Type e(&scope, mainModuleAt(runtime_, "E"));
  Layout layout(&scope, e.instanceLayout());
  EXPECT_FALSE(layout.isSealed());
}

TEST_F(TypeBuiltinsTest,
       DunderSlotsWithNonObjectBuiltinBaseAddsInObjectAttributes) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(dict):
  __slots__ = "x"
)")
                   .isError());
  HandleScope scope(thread_);
  Layout dict_layout(&scope, runtime_->layoutAt(LayoutId::kDict));
  ASSERT_TRUE(dict_layout.isSealed());
  Tuple dict_attributes(&scope, dict_layout.inObjectAttributes());
  ASSERT_EQ(dict_attributes.length(), 4);
  Tuple dict_elt3(&scope, dict_attributes.at(3));
  AttributeInfo dict_elt3_info(dict_elt3.at(1));
  EXPECT_TRUE(dict_elt3_info.isInObject());
  EXPECT_TRUE(dict_elt3_info.isFixedOffset());

  Type c(&scope, mainModuleAt(runtime_, "C"));
  Layout layout(&scope, c.instanceLayout());
  // C's layout is sealed since it has only a sealed base.
  EXPECT_TRUE(layout.isSealed());
  Tuple attributes(&scope, layout.inObjectAttributes());
  ASSERT_EQ(attributes.length(), 5);

  Tuple elt0(&scope, attributes.at(4));
  EXPECT_TRUE(isStrEqualsCStr(elt0.at(0), "x"));
  AttributeInfo info0(elt0.at(1));
  EXPECT_TRUE(info0.isInObject());
  EXPECT_TRUE(info0.isFixedOffset());
  EXPECT_EQ(info0.offset(), dict_elt3_info.offset() + kPointerSize);
}

TEST_F(TypeBuiltinsTest,
       DunderSlotsWithConflictingLayoutBasesOfUserTypeRaisesTypeError) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  __slots__ = "x"

class D:
  __slots__ = "y"
)")
                   .isError());
  ASSERT_TRUE(raisedWithStr(runFromCStr(runtime_, "class E(C, D): pass"),
                            LayoutId::kTypeError,
                            "multiple bases have instance lay-out conflict"));
}

TEST_F(TypeBuiltinsTest,
       DunderSlotsWithConflictingLayoutBasesOfBuiltinTypeRaisesTypeError) {
  // Confliction between a builtin type and a user-defined type.
  ASSERT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C:
  __slots__ = "x" # This is conflicting with dict's in-object attributes.

class D(C, dict):
  pass
)"),
                            LayoutId::kTypeError,
                            "multiple bases have instance lay-out conflict"));
}

TEST_F(TypeBuiltinsTest,
       DunderSlotsWithEmptyTupleDoesNotConflictWithOtherDunderSlots) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  __slots__ = ()

class D:
  __slots__ = "x"

class E(C, D):
  pass
)")
                   .isError());
  HandleScope scope(thread_);
  Type e(&scope, mainModuleAt(runtime_, "E"));
  Layout layout(&scope, e.instanceLayout());
  Tuple attributes(&scope, layout.inObjectAttributes());
  ASSERT_EQ(attributes.length(), 1);

  Tuple elt0(&scope, attributes.at(0));
  EXPECT_TRUE(isStrEqualsCStr(elt0.at(0), "x"));
  AttributeInfo info0(elt0.at(1));
  EXPECT_TRUE(info0.isInObject());
  EXPECT_TRUE(info0.isFixedOffset());
}

TEST_F(TypeBuiltinsTest, DunderSlotsSharingSameLayoutBaseCanServceAsBases) {
  // Although F's bases, D, E do not appear in the same type hierarchy
  // (neither D is a subtype of E nor E is a subtype of D), but
  // D's layout base (C) is the supertype of E, which makes the type checking
  // succeed.
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  __slots__ = "x"

class D(C):
  pass

class E(C):
  __slots__ = "y"

class F(D, E):
  pass
)")
                   .isError());
  HandleScope scope(thread_);
  Type f(&scope, mainModuleAt(runtime_, "F"));
  Layout layout(&scope, f.instanceLayout());
  Tuple attributes(&scope, layout.inObjectAttributes());
  ASSERT_EQ(attributes.length(), 2);

  Tuple elt0(&scope, attributes.at(0));
  EXPECT_TRUE(isStrEqualsCStr(elt0.at(0), "x"));
  AttributeInfo info0(elt0.at(1));
  EXPECT_TRUE(info0.isInObject());
  EXPECT_TRUE(info0.isFixedOffset());

  Tuple elt1(&scope, attributes.at(1));
  EXPECT_TRUE(isStrEqualsCStr(elt1.at(0), "y"));
  AttributeInfo info1(elt1.at(1));
  EXPECT_TRUE(info1.isInObject());
  EXPECT_TRUE(info1.isFixedOffset());
}

TEST_F(TypeBuiltinsTest, DunderSlotsPopulatesSlotDescriptorsWithCorrectValues) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  __slots__ = "x"

class D(C):
  __slots__ = "y"
)")
                   .isError());
  HandleScope scope(thread_);
  Type c(&scope, mainModuleAt(runtime_, "C"));
  Layout c_layout(&scope, c.instanceLayout());
  // Checking descriptor for "x" in C.
  {
    Str x(&scope, runtime_->newStrFromCStr("x"));
    SlotDescriptor descriptor_x(&scope, typeAt(c, x));
    EXPECT_EQ(descriptor_x.type(), *c);
    EXPECT_TRUE(isStrEqualsCStr(descriptor_x.name(), "x"));
    AttributeInfo info;
    ASSERT_TRUE(Runtime::layoutFindAttribute(*c_layout, x, &info));
    EXPECT_EQ(descriptor_x.offset(), info.offset());
  }

  Type d(&scope, mainModuleAt(runtime_, "D"));
  Layout d_layout(&scope, d.instanceLayout());
  // Checking descriptors for "x" and "y" in D.
  {
    // "x" is inherited from C to D.
    Str x(&scope, runtime_->newStrFromCStr("x"));
    ASSERT_TRUE(typeAt(d, x).isErrorNotFound());
    SlotDescriptor descriptor_x(&scope, typeLookupInMro(thread_, d, x));
    EXPECT_EQ(descriptor_x.type(), *c);
    EXPECT_TRUE(isStrEqualsCStr(descriptor_x.name(), "x"));
    AttributeInfo info;
    ASSERT_TRUE(Runtime::layoutFindAttribute(*d_layout, x, &info));
    EXPECT_EQ(descriptor_x.offset(), info.offset());

    // "y" is populated in D itself.
    Str y(&scope, runtime_->newStrFromCStr("y"));
    SlotDescriptor descriptor_y(&scope, typeAt(d, y));
    EXPECT_EQ(descriptor_y.type(), *d);
    EXPECT_TRUE(isStrEqualsCStr(descriptor_y.name(), "y"));
    ASSERT_TRUE(Runtime::layoutFindAttribute(*d_layout, y, &info));
    EXPECT_EQ(descriptor_y.offset(), info.offset());
  }
}

static RawObject newExtensionType(PyObject* extension_type) {
  Thread* thread = Thread::current();
  Runtime* runtime = thread->runtime();
  HandleScope scope(thread);

  Str name(&scope, runtime->newStrFromCStr("ExtType"));
  Object object_type(&scope, runtime->typeAt(LayoutId::kObject));
  Tuple bases(&scope, runtime->newTupleWith1(object_type));
  Dict dict(&scope, runtime->newDict());
  Type type(&scope, typeNew(thread, LayoutId::kType, name, bases, dict,
                            Type::Flag::kHasNativeData, false));

  extension_type->reference_ = type.raw();
  return *type;
}

TEST_F(TypeBuiltinsTest,
       DunderSlotsWithExtensionTypeAsBaseAllocatesExtraSpace) {
  // Create a main module.
  ASSERT_FALSE(runFromCStr(runtime_, "").isError());
  HandleScope scope(thread_);
  PyObject extension_type;
  Type type(&scope, newExtensionType(&extension_type));
  ASSERT_TRUE(type.hasFlag(Type::Flag::kHasNativeData));

  Module main_module(&scope, findMainModule(runtime_));
  Str type_name(&scope, runtime_->newStrFromCStr("ExtType"));
  moduleAtPut(thread_, main_module, type_name, type);

  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(ExtType):
  __slots__ = "x", "y"
)")
                   .isError());
  Type c(&scope, mainModuleAt(runtime_, "C"));
  ASSERT_TRUE(c.hasFlag(Type::Flag::kHasNativeData));

  Layout layout(&scope, c.instanceLayout());
  Tuple attributes(&scope, layout.inObjectAttributes());
  // "x" and "y" are added as a regular attribute.
  ASSERT_EQ(attributes.length(), 2);
  // However, more speace was allocated to be a native proxy.
  EXPECT_EQ(layout.instanceSize(),
            2 * kPointerSize + NativeProxy::kSizeFromEnd);

  Tuple elt0(&scope, attributes.at(0));
  EXPECT_TRUE(isStrEqualsCStr(elt0.at(0), "x"));
  AttributeInfo info0(elt0.at(1));
  EXPECT_TRUE(info0.isInObject());
  EXPECT_TRUE(info0.isFixedOffset());

  Tuple elt1(&scope, attributes.at(1));
  EXPECT_TRUE(isStrEqualsCStr(elt1.at(0), "y"));
  AttributeInfo info1(elt1.at(1));
  EXPECT_TRUE(info1.isInObject());
  EXPECT_TRUE(info1.isFixedOffset());
}

TEST_F(TypeBuiltinsTest, TypeHasDunderMroAttribute) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = str.__class__.__mro__").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  ASSERT_TRUE(result.isTuple());
}

TEST_F(TypeBuiltinsTest, TypeHasDunderNameAttribute) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = str.__class__.__name__").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "type"));
}

TEST_F(TypeBuiltinsTest, TypeHasDunderDictAttribute) {
  HandleScope scope;
  ASSERT_FALSE(
      runFromCStr(runtime_, "result = str.__class__.__dict__").isError());
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_TRUE(result.isMappingProxy());
}

TEST_F(TypeBuiltinsTest, TypeLookupNameInMroReturnsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  foo = 2
)")
                   .isError());
  Object a_obj(&scope, mainModuleAt(runtime_, "A"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object foo(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(isIntEqualsWord(typeLookupInMro(thread_, a, foo), 2));
}

TEST_F(TypeBuiltinsTest, TypeLookupNameInMroReturnsParentValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  foo = 2
class B(A):
  bar = 4
)")
                   .isError());
  Object b_obj(&scope, mainModuleAt(runtime_, "B"));
  ASSERT_TRUE(b_obj.isType());
  Type b(&scope, *b_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(isIntEqualsWord(typeLookupInMro(thread_, b, name), 2));
}

TEST_F(TypeBuiltinsTest, TypeLookupNameInMroReturnsOverriddenValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  foo = 2
class B(A):
  foo = 4
)")
                   .isError());
  Object b_obj(&scope, mainModuleAt(runtime_, "B"));
  ASSERT_TRUE(b_obj.isType());
  Type b(&scope, *b_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(isIntEqualsWord(typeLookupInMro(thread_, b, name), 4));
}

TEST_F(TypeBuiltinsTest, TypeLookupNameInMroWithNonExistentNameReturnsError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  bar = 2
)")
                   .isError());
  Object a_obj(&scope, mainModuleAt(runtime_, "A"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(typeLookupInMro(thread_, a, name).isError());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(TypeBuiltinsTest, TypeLookupSymbolInMroReturnsValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class A:
  __add__ = 3
)")
                   .isError());
  Object a_obj(&scope, mainModuleAt(runtime_, "A"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  EXPECT_TRUE(isIntEqualsWord(typeLookupInMroById(thread_, a, ID(__add__)), 3));
}

TEST_F(TypeBuiltinsTest, DunderCallReceivesExArgs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self, *args):
    self.args = args

  def num_args(self):
    return len(self.args)

result = C(*(1,2,3)).num_args()
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(*result, RawSmallInt::fromWord(3));
}

TEST_F(TypeBuiltinsTest, TypeNewWithNonStrKeyInDictRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
ty = type.__new__(type, *("foo", (object,), {4: 1}))
name = ty.__name__N
)"),
                            LayoutId::kTypeError,
                            "attribute name must be string, not 'int'"));
}

TEST_F(TypeBuiltinsTest, ClassMethodDunderCallReceivesExArgs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo:
  @classmethod
  def foo(cls, *args):
    return len(args)

result = Foo.foo(*(1,2,3))
)")
                   .isError());
  HandleScope scope;
  Object result(&scope, mainModuleAt(runtime_, "result"));
  EXPECT_EQ(*result, RawSmallInt::fromWord(3));
}

TEST_F(TypeBuiltinsTest, TypeNewReceivesExArgs) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
ty = type.__new__(type, *("foo", (object,), {'a': 1}))
name = ty.__name__
)")
                   .isError());
  HandleScope scope;
  Object name(&scope, mainModuleAt(runtime_, "name"));
  EXPECT_TRUE(isStrEqualsCStr(*name, "foo"));
}

TEST_F(TypeBuiltinsTest, TypeCallWithInitReturningNonNoneRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, R"(
class C:
  def __init__(self, *args, **kwargs):
    return 5
C()
)"),
                            LayoutId::kTypeError,
                            "C.__init__ returned non None"));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeReturnsAttributeValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  x = 42
)")
                   .isError());
  Object c_obj(&scope, mainModuleAt(runtime_, "C"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "x"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread_, c, name), 42));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeReturnsMetaclassAttributeValue) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class M(type):
  x = 77
class C(metaclass=M): pass
)")
                   .isError());
  Object c_obj(&scope, mainModuleAt(runtime_, "C"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "x"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread_, c, name), 77));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeWithMissingAttributeReturnsError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "class C: pass").isError());
  Object c_obj(&scope, mainModuleAt(runtime_, "C"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "xxx"));
  EXPECT_TRUE(typeGetAttribute(thread_, c, name).isError());
  EXPECT_FALSE(thread_->hasPendingException());
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeCallsDunderGetOnDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return (self, instance, owner)
class M(type):
  foo = D()
class A(metaclass=M): pass
)")
                   .isError());
  Object d_obj(&scope, mainModuleAt(runtime_, "D"));
  ASSERT_TRUE(d_obj.isType());
  Type d(&scope, *d_obj);
  Object m(&scope, mainModuleAt(runtime_, "M"));
  Object a_obj(&scope, mainModuleAt(runtime_, "A"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object result_obj(&scope, typeGetAttribute(thread_, a, name));
  ASSERT_TRUE(result_obj.isTuple());
  Tuple result(&scope, *result_obj);
  ASSERT_EQ(result.length(), 3);
  Type result_0_type(&scope, runtime_->typeOf(result.at(0)));
  EXPECT_TRUE(typeIsSubclass(result_0_type, d));
  EXPECT_EQ(result.at(1), a);
  EXPECT_EQ(result.at(2), m);
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeCallsDunderGetOnNonDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __get__(self, instance, owner): return 42
class M(type):
  foo = D()
class A(metaclass=M): pass
)")
                   .isError());
  Object a_obj(&scope, mainModuleAt(runtime_, "A"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread_, a, name), 42));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributePrefersDataDescriptorOverTypeAttr) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): return 42
class M(type):
  foo = D()
class A(metaclass=M):
  foo = 12
)")
                   .isError());
  Object a_obj(&scope, mainModuleAt(runtime_, "A"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread_, a, name), 42));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributePrefersFieldOverNonDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __get__(self, instance, owner): return 42
class M(type):
  foo = D()
class A(metaclass=M):
  foo = 12
)")
                   .isError());
  Object a_obj(&scope, mainModuleAt(runtime_, "A"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(isIntEqualsWord(typeGetAttribute(thread_, a, name), 12));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributePropagatesDunderGetException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __set__(self, instance, value): pass
  def __get__(self, instance, owner): raise UserWarning()
class M(type):
  foo = D()
class A(metaclass=M): pass
)")
                   .isError());
  Object a_obj(&scope, mainModuleAt(runtime_, "A"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  EXPECT_TRUE(
      raised(typeGetAttribute(thread_, a, name), LayoutId::kUserWarning));
}

TEST_F(TypeBuiltinsTest, TypeGetAttributeOnNoneTypeReturnsFunction) {
  HandleScope scope(thread_);
  Type none_type(&scope, runtime_->typeAt(LayoutId::kNoneType));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "__repr__"));
  EXPECT_TRUE(typeGetAttribute(thread_, none_type, name).isFunction());
}

TEST_F(TypeBuiltinsTest, TypeSetAttrSetsAttribute) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, "class C: pass").isError());
  Object c_obj(&scope, mainModuleAt(runtime_, "C"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*c_obj));
  Type c(&scope, *c_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foobarbaz"));
  Object value(&scope, runtime_->newInt(-444));
  EXPECT_TRUE(typeSetAttr(thread_, c, name, value).isNoneType());
  EXPECT_TRUE(isIntEqualsWord(typeAt(c, name), -444));
}

TEST_F(TypeBuiltinsTest, TypeSetAttrCallsDunderSetOnDataDescriptor) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
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
  Object foo(&scope, mainModuleAt(runtime_, "foo"));
  Object a_obj(&scope, mainModuleAt(runtime_, "A"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_->newInt(77));
  EXPECT_TRUE(typeSetAttr(thread_, a, name, value).isNoneType());
  Object set_args_obj(&scope, mainModuleAt(runtime_, "set_args"));
  ASSERT_TRUE(set_args_obj.isTuple());
  Tuple set_args(&scope, *set_args_obj);
  ASSERT_EQ(set_args.length(), 3);
  EXPECT_EQ(set_args.at(0), foo);
  EXPECT_EQ(set_args.at(1), a);
  EXPECT_TRUE(isIntEqualsWord(set_args.at(2), 77));
}

TEST_F(TypeBuiltinsTest, TypeSetAttrPropagatesDunderSetException) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class D:
  def __get__(self, instance, owner): pass
  def __set__(self, instance, value): raise UserWarning()
class M(type):
  foo = D()
class A(metaclass=M):
  pass
)")
                   .isError());
  Object a_obj(&scope, mainModuleAt(runtime_, "A"));
  ASSERT_TRUE(runtime_->isInstanceOfType(*a_obj));
  Type a(&scope, *a_obj);
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object value(&scope, runtime_->newInt(1));
  EXPECT_TRUE(
      raised(typeSetAttr(thread_, a, name, value), LayoutId::kUserWarning));
}

TEST_F(TypeBuiltinsTest, TypeSetAttrOnBuiltinTypeRaisesTypeError) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->typeAt(LayoutId::kInt));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "foo"));
  Object value(&scope, NoneType::object());
  EXPECT_TRUE(raisedWithStr(
      typeSetAttr(thread_, type, name, value), LayoutId::kTypeError,
      "can't set attributes of built-in/extension type 'int'"));
}

TEST_F(TypeBuiltinsTest, TypeofSmallStrReturnsStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = type('a')
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), runtime_->typeAt(LayoutId::kStr));
}

TEST_F(TypeBuiltinsTest, TypeofLargeStrReturnsStr) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = type('aaaaaaaaaaaaaaaaaaaaaaa')
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), runtime_->typeAt(LayoutId::kStr));
}

TEST_F(TypeBuiltinsTest, TypeofSmallIntReturnsInt) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = type(5)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), runtime_->typeAt(LayoutId::kInt));
}

TEST_F(TypeBuiltinsTest, TypeofLargeIntReturnsInt) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
result = type(99999999999999999999999999999999999999999)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(runtime_, "result"), runtime_->typeAt(LayoutId::kInt));
}

TEST_F(TypeBuiltinsTest, ResolveDescriptorGetReturnsNonDescriptor) {
  HandleScope scope(thread_);

  Object instance(&scope, runtime_->newInt(123));
  Object owner(&scope, NoneType::object());
  Object descr(&scope, runtime_->newInt(456));
  EXPECT_EQ(resolveDescriptorGet(thread_, descr, instance, owner), *descr);
}

TEST_F(TypeBuiltinsTest, ResolveDescriptorGetCallsDescriptorDunderGet) {
  HandleScope scope(thread_);

  Object instance(&scope, runtime_->newInt(123));
  Type owner(&scope, runtime_->typeOf(*instance));
  Object descr(&scope, typeLookupInMroById(thread_, owner, ID(__add__)));
  ASSERT_TRUE(descr.isFunction());
  EXPECT_TRUE(
      resolveDescriptorGet(thread_, descr, instance, owner).isBoundMethod());
}

TEST_F(
    TypeBuiltinsTest,
    TerminateIfUnimplementedTypeAttrCacheInvalidationDoesNotTerminateRuntimeForSupportedCacheInvalidation) {
  // __len__ supports cache invalidation.
  EXPECT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __len__(self): return 0

C.__len__ = lambda self: 4
)")
                   .isError());

  // __setattr__ does not support cache invalidation, but it is not populated in
  // C so we do not terminate the runtime.
  EXPECT_FALSE(runFromCStr(runtime_, R"(
class C: pass

C.__setattr__ = lambda self, key: 5
)")
                   .isError());
}

TEST_F(
    TypeBuiltinsDeathTest,
    TerminateIfUnimplementedTypeAttrCacheInvalidationTerminatesRuntimeForUnsupportedCacheInvalidation) {
  // Redefining the existing __setattr__ terminates the runtime due to the
  // unsupported cache invalidation for it.
  ASSERT_DEATH(static_cast<void>(runFromCStr(runtime_, R"(
class C:
  def __setattr__(self, key): pass

C.__setattr__ = lambda self, key: 5
)")),
               "unimplemented cache invalidation for type.__setattr__ update");
}

TEST_F(TypeBuiltinsTest, TypeIsMarkedAsCustomDict) {
  HandleScope scope(thread_);
  Type type(&scope, runtime_->typeAt(LayoutId::kType));
  EXPECT_TRUE(type.hasFlag(Type::Flag::kHasCustomDict));
  EXPECT_TRUE(Layout::cast(type.instanceLayout()).isSealed());
}

TEST_F(TypeBuiltinsTest, TypeSubclassLayoutIsSealed) {
  HandleScope scope(thread_);
  EXPECT_FALSE(runFromCStr(runtime_, R"(
class Meta(type): pass
)")
                   .isError());
  Type meta(&scope, mainModuleAt(runtime_, "Meta"));
  EXPECT_TRUE(meta.hasFlag(Type::Flag::kHasCustomDict));
  EXPECT_TRUE(Layout::cast(meta.instanceLayout()).isSealed());
}

}  // namespace testing
}  // namespace py
