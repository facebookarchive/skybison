#include "gtest/gtest.h"

#include "builtins-module.h"
#include "module-builtins.h"
#include "runtime.h"
#include "test-utils.h"
#include "trampolines.h"

namespace python {

using namespace testing;

using BuiltinsModuleTest = RuntimeFixture;
using BuiltinsModuleDeathTest = RuntimeFixture;

TEST_F(BuiltinsModuleTest, BuiltinCallableOnTypeReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  pass

a = callable(Foo)
  )")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(a.value());
}

TEST_F(BuiltinsModuleTest, BuiltinCallableOnMethodReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def bar():
    return None

a = callable(Foo.bar)
b = callable(Foo().bar)
  )")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, mainModuleAt(&runtime_, "a"));
  Bool b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_TRUE(a.value());
  EXPECT_TRUE(b.value());
}

TEST_F(BuiltinsModuleTest, BuiltinCallableOnNonCallableReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = callable(1)
b = callable("hello")
  )")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, mainModuleAt(&runtime_, "a"));
  Bool b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_FALSE(a.value());
  EXPECT_FALSE(b.value());
}

TEST_F(BuiltinsModuleTest, BuiltinCallableOnObjectWithCallOnTypeReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __call__(self):
    pass

f = Foo()
a = callable(f)
  )")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(a.value());
}

TEST_F(BuiltinsModuleTest,
       BuiltinCallableOnObjectWithInstanceCallButNoTypeCallReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  pass

def fakecall():
  pass

f = Foo()
f.__call__ = fakecall
a = callable(f)
  )")
                   .isError());
  HandleScope scope(thread_);
  Bool a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_FALSE(a.value());
}

TEST_F(BuiltinsModuleTest, DirCallsDunderDirReturnsSortedList) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __dir__(self):
    return ["B", "A"]
c = C()
d = dir(c)
)")
                   .isError());
  Object d_obj(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d_obj.isList());
  List d(&scope, *d_obj);
  ASSERT_EQ(d.numItems(), 2);
  EXPECT_TRUE(isStrEqualsCStr(d.at(0), "A"));
  EXPECT_TRUE(isStrEqualsCStr(d.at(1), "B"));
}

TEST_F(BuiltinsModuleDeathTest, DirWithoutObjectCallsLocals) {
  // locals() is not implemented yet, so we will die here.
  ASSERT_DEATH(static_cast<void>(runFromCStr(&runtime_, R"(
dir()
)")),
               "'_unimplemented' called in function 'dir'");
}

TEST_F(BuiltinsModuleTest, EllipsisMatchesEllipsis) {
  EXPECT_EQ(moduleAtByCStr(&runtime_, "builtins", "Ellipsis"),
            runtime_.ellipsis());
}

TEST_F(BuiltinsModuleTest, IdReturnsInt) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_.newInt(12345));
  EXPECT_TRUE(runBuiltin(BuiltinsModule::id, obj).isInt());
}

TEST_F(BuiltinsModuleTest, IdDoesNotChangeAfterGC) {
  HandleScope scope(thread_);
  Object obj(&scope, runtime_.newStrFromCStr("hello world foobar"));
  Object id_before(&scope, runBuiltin(BuiltinsModule::id, obj));
  runtime_.collectGarbage();
  Object id_after(&scope, runBuiltin(BuiltinsModule::id, obj));
  EXPECT_EQ(*id_before, *id_after);
}

TEST_F(BuiltinsModuleTest, IdReturnsDifferentValueForDifferentObject) {
  HandleScope scope(thread_);
  Object obj1(&scope, runtime_.newStrFromCStr("hello world foobar"));
  Object obj2(&scope, runtime_.newStrFromCStr("hello world foobarbaz"));
  EXPECT_NE(runBuiltin(BuiltinsModule::id, obj1),
            runBuiltin(BuiltinsModule::id, obj2));
}

TEST_F(BuiltinsModuleTest, BuiltinLenGetLenFromDict) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
len0 = len({})
len1 = len({'one': 1})
len5 = len({'one': 1, 'two': 2, 'three': 3, 'four': 4, 'five': 5})
)")
                   .isError());

  HandleScope scope(thread_);
  Object len0(&scope, mainModuleAt(&runtime_, "len0"));
  EXPECT_EQ(*len0, SmallInt::fromWord(0));
  Object len1(&scope, mainModuleAt(&runtime_, "len1"));
  EXPECT_EQ(*len1, SmallInt::fromWord(1));
  Object len5(&scope, mainModuleAt(&runtime_, "len5"));
  EXPECT_EQ(*len5, SmallInt::fromWord(5));
}

TEST_F(BuiltinsModuleTest, BuiltinLenGetLenFromList) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
len0 = len([])
len1 = len([1])
len5 = len([1,2,3,4,5])
)")
                   .isError());

  HandleScope scope(thread_);
  Object len0(&scope, mainModuleAt(&runtime_, "len0"));
  EXPECT_EQ(*len0, SmallInt::fromWord(0));
  Object len1(&scope, mainModuleAt(&runtime_, "len1"));
  EXPECT_EQ(*len1, SmallInt::fromWord(1));
  Object len5(&scope, mainModuleAt(&runtime_, "len5"));
  EXPECT_EQ(*len5, SmallInt::fromWord(5));
}

TEST_F(BuiltinsModuleTest, BuiltinLenGetLenFromSet) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
len1 = len({1})
len5 = len({1,2,3,4,5})
)")
                   .isError());

  HandleScope scope(thread_);
  // TODO(cshapiro): test the empty set when we have builtins.set defined.
  Object len1(&scope, mainModuleAt(&runtime_, "len1"));
  EXPECT_EQ(*len1, SmallInt::fromWord(1));
  Object len5(&scope, mainModuleAt(&runtime_, "len5"));
  EXPECT_EQ(*len5, SmallInt::fromWord(5));
}

TEST_F(BuiltinsModuleTest, BuiltinOrd) {
  HandleScope scope(thread_);
  Str str(&scope, runtime_.newStrFromCStr("A"));
  EXPECT_TRUE(isIntEqualsWord(runBuiltin(BuiltinsModule::ord, str), 65));
  Int one(&scope, SmallInt::fromWord(1));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BuiltinsModule::ord, one),
                            LayoutId::kTypeError,
                            "Unsupported type in builtin 'ord'"));
}

TEST_F(BuiltinsModuleTest, BuiltinOrdSupportNonASCII) {
  HandleScope scope(thread_);
  Str two_bytes(&scope, runtime_.newStrFromCStr("\xC3\xA9"));
  Object two_ord(&scope, runBuiltin(BuiltinsModule::ord, two_bytes));
  EXPECT_TRUE(isIntEqualsWord(*two_ord, 0xE9));

  Str three_bytes(&scope, runtime_.newStrFromCStr("\xE2\xB3\x80"));
  Object three_ord(&scope, runBuiltin(BuiltinsModule::ord, three_bytes));
  EXPECT_TRUE(isIntEqualsWord(*three_ord, 0x2CC0));

  Str four_bytes(&scope, runtime_.newStrFromCStr("\xF0\x9F\x86\x92"));
  Object four_ord(&scope, runBuiltin(BuiltinsModule::ord, four_bytes));
  EXPECT_TRUE(isIntEqualsWord(*four_ord, 0x1F192));
}

TEST_F(BuiltinsModuleTest, BuiltinOrdWithMultiCodepointStringRaisesError) {
  HandleScope scope(thread_);
  Str two_chars(&scope, runtime_.newStrFromCStr("ab"));
  EXPECT_TRUE(raisedWithStr(runBuiltin(BuiltinsModule::ord, two_chars),
                            LayoutId::kTypeError,
                            "Builtin 'ord' expects string of length 1"));
}

TEST_F(BuiltinsModuleTest, BuiltInReprOnUserTypeWithDunderRepr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __repr__(self):
    return "foo"

a = repr(Foo())
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
}

TEST_F(BuiltinsModuleTest, BuiltInReprOnClass) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = repr(int)").isError());
  HandleScope scope(thread_);
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isStrEqualsCStr(*result, "<class 'int'>"));
}

TEST_F(BuiltinsModuleTest, BuiltInAsciiCallsDunderRepr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo:
  def __repr__(self):
    return "foo"

a = ascii(Foo())
)")
                   .isError());
  HandleScope scope(thread_);
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(isStrEqualsCStr(*a, "foo"));
}

TEST_F(BuiltinsModuleTest, DunderBuildClassWithNonFunctionRaisesTypeError) {
  HandleScope scope(thread_);
  Object body(&scope, NoneType::object());
  Object name(&scope, runtime_.newStrFromCStr("a"));
  Object metaclass(&scope, Unbound::object());
  Object bootstrap(&scope, Bool::falseObj());
  Object bases(&scope, runtime_.emptyTuple());
  Object kwargs(&scope, runtime_.newDict());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BuiltinsModule::dunderBuildClass, body, name, metaclass,
                 bootstrap, bases, kwargs),
      LayoutId::kTypeError, "__build_class__: func must be a function"));
}

TEST_F(BuiltinsModuleTest, DunderBuildClassWithNonStringRaisesTypeError) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "def f(): pass").isError());
  Object body(&scope, mainModuleAt(&runtime_, "f"));
  Object name(&scope, NoneType::object());
  Object metaclass(&scope, Unbound::object());
  Object bootstrap(&scope, Bool::falseObj());
  Object bases(&scope, runtime_.emptyTuple());
  Object kwargs(&scope, runtime_.newDict());
  EXPECT_TRUE(raisedWithStr(
      runBuiltin(BuiltinsModule::dunderBuildClass, body, name, metaclass,
                 bootstrap, bases, kwargs),
      LayoutId::kTypeError, "__build_class__: name is not a string"));
}

TEST_F(BuiltinsModuleTest, DunderBuildClassCallsMetaclass) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Meta(type):
  def __new__(mcls, name, bases, namespace, *args, **kwargs):
    return (mcls, name, bases, namespace, args, kwargs)
class C(int, float, metaclass=Meta, hello="world"):
  x = 42
)")
                   .isError());
  Object meta(&scope, mainModuleAt(&runtime_, "Meta"));
  Object c_obj(&scope, mainModuleAt(&runtime_, "C"));
  ASSERT_TRUE(c_obj.isTuple());
  Tuple c(&scope, *c_obj);
  ASSERT_EQ(c.length(), 6);
  EXPECT_EQ(c.at(0), meta);
  EXPECT_TRUE(isStrEqualsCStr(c.at(1), "C"));

  ASSERT_TRUE(c.at(2).isTuple());
  Tuple c_bases(&scope, c.at(2));
  ASSERT_EQ(c_bases.length(), 2);
  EXPECT_EQ(c_bases.at(0), runtime_.typeAt(LayoutId::kInt));
  EXPECT_EQ(c_bases.at(1), runtime_.typeAt(LayoutId::kFloat));

  ASSERT_TRUE(c.at(3).isDict());
  Dict c_namespace(&scope, c.at(3));
  Object x(&scope, runtime_.newStrFromCStr("x"));
  EXPECT_TRUE(runtime_.dictIncludes(thread_, c_namespace, x));
  ASSERT_TRUE(c.at(4).isTuple());
  EXPECT_EQ(Tuple::cast(c.at(4)).length(), 0);
  Object hello(&scope, runtime_.newStrFromCStr("hello"));
  ASSERT_TRUE(c.at(5).isDict());
  Dict c_kwargs(&scope, c.at(5));
  EXPECT_EQ(c_kwargs.numItems(), 1);
  EXPECT_TRUE(
      isStrEqualsCStr(runtime_.dictAt(thread_, c_kwargs, hello), "world"));
}

TEST_F(BuiltinsModuleTest, DunderBuildClassCalculatesMostSpecificMetaclass) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Meta(type): pass
class C1(int, metaclass=Meta): pass
class C2(C1, metaclass=type): pass
t1 = type(C1)
t2 = type(C2)
)")
                   .isError());
  Object meta(&scope, mainModuleAt(&runtime_, "Meta"));
  Object t1(&scope, mainModuleAt(&runtime_, "t1"));
  Object t2(&scope, mainModuleAt(&runtime_, "t2"));
  ASSERT_TRUE(t1.isType());
  ASSERT_TRUE(t2.isType());
  EXPECT_EQ(t1, meta);
  EXPECT_EQ(t2, meta);
}

TEST_F(BuiltinsModuleTest,
       DunderBuildClassWithIncompatibleMetaclassesRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, R"(
class M1(type): pass
class M2(type): pass
class C1(metaclass=M1): pass
class C2(C1, metaclass=M2): pass
)"),
      LayoutId::kTypeError,
      "metaclass conflict: the metaclass of a derived class must be a "
      "(non-strict) subclass of the metaclasses of all its bases"));
}

TEST_F(BuiltinsModuleTest, DunderBuildClassWithMeetMetaclassUsesMeet) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class M1(type): pass
class M2(type): pass
class M3(M1, M2): pass
class C1(metaclass=M1): pass
class C2(metaclass=M2): pass
class C3(C1, C2, metaclass=M3): pass
t1 = type(C1)
t2 = type(C2)
t3 = type(C3)
)")
                   .isError());
  Object m1(&scope, mainModuleAt(&runtime_, "M1"));
  Object m2(&scope, mainModuleAt(&runtime_, "M2"));
  Object m3(&scope, mainModuleAt(&runtime_, "M3"));
  Object t1(&scope, mainModuleAt(&runtime_, "t1"));
  Object t2(&scope, mainModuleAt(&runtime_, "t2"));
  Object t3(&scope, mainModuleAt(&runtime_, "t3"));
  ASSERT_TRUE(t1.isType());
  ASSERT_TRUE(t2.isType());
  ASSERT_TRUE(t3.isType());
  EXPECT_EQ(t1, m1);
  EXPECT_EQ(t2, m2);
  EXPECT_EQ(t3, m3);
}

TEST_F(BuiltinsModuleTest, DunderBuildClassPropagatesDunderPrepareError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Meta(type):
  @classmethod
  def __prepare__(cls, *args, **kwds):
    raise IndentationError("foo")
class C(metaclass=Meta):
  pass
)"),
                            LayoutId::kIndentationError, "foo"));
}

TEST_F(BuiltinsModuleTest, DunderBuildClassWithNonDictPrepareRaisesTypeError) {
  EXPECT_TRUE(
      raisedWithStr(runFromCStr(&runtime_, R"(
class Meta(type):
  @classmethod
  def __prepare__(cls, *args, **kwds):
    return 42
class C(metaclass=Meta):
  pass
)"),
                    LayoutId::kTypeError,
                    "Meta.__prepare__() must return a mapping, not int"));
}

TEST_F(BuiltinsModuleTest,
       DunderBuildClassWithNonTypeMetaclassAndNonDictPrepareRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, R"(
class Meta:
  def __prepare__(self, *args, **kwds):
    return 42
class C(metaclass=Meta()):
  pass
)"),
      LayoutId::kTypeError,
      "<metaclass>.__prepare__() must return a mapping, not int"));
}

TEST_F(BuiltinsModuleTest, DunderBuildClassUsesDunderPrepareForClassDict) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Meta(type):
  @classmethod
  def __prepare__(cls, *args, **kwds):
    return {"foo": 42}
class C(metaclass=Meta):
  pass
result = C.foo
)")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(isIntEqualsWord(*result, 42));
}

TEST_F(BuiltinsModuleTest, DunderBuildClassPassesNameBasesAndKwargsToPrepare) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Meta(type):
  def __init__(metacls, name, bases, namespace, **kwargs):
    pass
  def __new__(metacls, name, bases, namespace, **kwargs):
    return super().__new__(metacls, name, bases, namespace)
  @classmethod
  def __prepare__(metacls, name, bases, **kwargs):
    return {"foo": name, "bar": bases[0], "baz": kwargs["answer"]}
class C(int, metaclass=Meta, answer=42):
  pass
name = C.foo
base = C.bar
answer = C.baz
)")
                   .isError());
  Object name(&scope, mainModuleAt(&runtime_, "name"));
  Object base(&scope, mainModuleAt(&runtime_, "base"));
  Object answer(&scope, mainModuleAt(&runtime_, "answer"));
  EXPECT_TRUE(isStrEqualsCStr(*name, "C"));
  EXPECT_EQ(base, runtime_.typeAt(LayoutId::kInt));
  EXPECT_TRUE(isIntEqualsWord(*answer, 42));
}

TEST_F(BuiltinsModuleTest, DunderBuildClassWithRaisingBodyPropagatesException) {
  EXPECT_TRUE(raised(runFromCStr(&runtime_, R"(
class C:
  raise UserWarning()
)"),
                     LayoutId::kUserWarning));
}

TEST_F(BuiltinsModuleTest, GetAttrFromClassReturnsValue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = getattr(Foo, 'bar')
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object obj(&scope, mainModuleAt(&runtime_, "obj"));
  EXPECT_EQ(*obj, SmallInt::fromWord(1));
}

TEST_F(BuiltinsModuleTest, GetAttrFromInstanceReturnsValue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = getattr(Foo(), 'bar')
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object obj(&scope, mainModuleAt(&runtime_, "obj"));
  EXPECT_EQ(*obj, SmallInt::fromWord(1));
}

TEST_F(BuiltinsModuleTest, GetAttrFromInstanceWithMissingAttrReturnsDefault) {
  const char* src = R"(
class Foo: pass
obj = getattr(Foo(), 'bar', 2)
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object obj(&scope, mainModuleAt(&runtime_, "obj"));
  EXPECT_EQ(*obj, SmallInt::fromWord(2));
}

TEST_F(BuiltinsModuleTest, GetAttrWithNonStringAttrRaisesTypeError) {
  const char* src = R"(
class Foo: pass
getattr(Foo(), 1)
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "getattr(): attribute name must be string"));
}

TEST_F(BuiltinsModuleTest, GetAttrWithNonStringAttrAndDefaultRaisesTypeError) {
  const char* src = R"(
class Foo: pass
getattr(Foo(), 1, 2)
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "getattr(): attribute name must be string"));
}

TEST_F(BuiltinsModuleTest,
       GetAttrFromClassMissingAttrWithoutDefaultRaisesAttributeError) {
  const char* src = R"(
class Foo:
  bar = 1
getattr(Foo, 'foo')
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src),
                            LayoutId::kAttributeError,
                            "type object 'Foo' has no attribute 'foo'"));
}

TEST_F(BuiltinsModuleTest, HasAttrFromClassMissingAttrReturnsFalse) {
  const char* src = R"(
class Foo: pass
obj = hasattr(Foo, 'bar')
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object obj(&scope, mainModuleAt(&runtime_, "obj"));
  EXPECT_EQ(*obj, Bool::falseObj());
}

TEST_F(BuiltinsModuleTest, HasAttrFromClassWithAttrReturnsTrue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = hasattr(Foo, 'bar')
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object obj(&scope, mainModuleAt(&runtime_, "obj"));
  EXPECT_EQ(*obj, Bool::trueObj());
}

TEST_F(BuiltinsModuleTest, HasAttrWithNonStringAttrRaisesTypeError) {
  const char* src = R"(
class Foo:
  bar = 1
hasattr(Foo, 1)
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "hasattr(): attribute name must be string"));
}

TEST_F(BuiltinsModuleTest, HasAttrFromInstanceMissingAttrReturnsFalse) {
  const char* src = R"(
class Foo: pass
obj = hasattr(Foo(), 'bar')
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object obj(&scope, mainModuleAt(&runtime_, "obj"));
  EXPECT_EQ(*obj, Bool::falseObj());
}

TEST_F(BuiltinsModuleTest, HasAttrFromInstanceWithAttrReturnsTrue) {
  const char* src = R"(
class Foo:
  bar = 1
obj = hasattr(Foo(), 'bar')
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object obj(&scope, mainModuleAt(&runtime_, "obj"));
  EXPECT_EQ(*obj, Bool::trueObj());
}

TEST_F(BuiltinsModuleTest,
       HashWithObjectWithNotCallableDunderHashRaisesTypeError) {
  const char* src = R"(
class C:
  __hash__ = None

hash(C())
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "unhashable type: 'C'"));
}

TEST_F(BuiltinsModuleTest, HashWithObjectReturningNonIntRaisesTypeError) {
  const char* src = R"(
class C:
  def __hash__(self): return "10"

hash(C())
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "__hash__ method should return an integer"));
}

TEST_F(BuiltinsModuleTest, HashWithObjectReturnsObjectDunderHashValue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __hash__(self): return 10

h = hash(C())
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "h"), SmallInt::fromWord(10));
}

TEST_F(BuiltinsModuleTest,
       HashWithObjectWithModifiedDunderHashReturnsClassDunderHashValue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __hash__(self): return 10

def fake_hash(): return 0
c = C()
c.__hash__ = fake_hash
h = hash(c)
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "h"), SmallInt::fromWord(10));
}

TEST_F(BuiltinsModuleTest, BuiltInSetAttr) {
  const char* src = R"(
class Foo:
  bar = 1
a = setattr(Foo, 'foo', 2)
b = Foo.foo
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  EXPECT_EQ(*a, NoneType::object());
  EXPECT_EQ(*b, SmallInt::fromWord(2));
}

TEST_F(BuiltinsModuleTest, BuiltInSetAttrRaisesTypeError) {
  const char* src = R"(
class Foo:
  bar = 1
a = setattr(Foo, 2, 'foo')
)";
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, src), LayoutId::kTypeError,
                            "setattr(): attribute name must be string"));
}

TEST_F(BuiltinsModuleTest, ModuleAttrReturnsBuiltinsName) {
  // TODO(eelizondo): Parameterize test for all builtin types
  const char* src = R"(
a = hasattr(object, '__module__')
b = getattr(object, '__module__')
c = hasattr(list, '__module__')
d = getattr(list, '__module__')
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Object a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_EQ(*a, Bool::trueObj());
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("builtins"));

  Object c(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_EQ(*c, Bool::trueObj());
  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("builtins"));
}

TEST_F(BuiltinsModuleTest, QualnameAttrReturnsTypeName) {
  // TODO(eelizondo): Parameterize test for all builtin types
  const char* src = R"(
a = hasattr(object, '__qualname__')
b = getattr(object, '__qualname__')
c = hasattr(list, '__qualname__')
d = getattr(list, '__qualname__')
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, src).isError());

  Object a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_EQ(*a, Bool::trueObj());
  Object b(&scope, mainModuleAt(&runtime_, "b"));
  ASSERT_TRUE(b.isStr());
  EXPECT_TRUE(Str::cast(*b).equalsCStr("object"));

  Object c(&scope, mainModuleAt(&runtime_, "c"));
  EXPECT_EQ(*c, Bool::trueObj());
  Object d(&scope, mainModuleAt(&runtime_, "d"));
  ASSERT_TRUE(d.isStr());
  EXPECT_TRUE(Str::cast(*d).equalsCStr("list"));
}

TEST_F(BuiltinsModuleTest, BuiltinCompile) {
  HandleScope scope(thread_);
  ASSERT_FALSE(
      runFromCStr(
          &runtime_,
          R"(code = compile("a = 1\nb = 2", "<string>", "eval", dont_inherit=True))")
          .isError());
  Str filename(&scope, runtime_.newStrFromCStr("<string>"));
  Code code(&scope, mainModuleAt(&runtime_, "code"));
  ASSERT_TRUE(code.filename().isStr());
  EXPECT_TRUE(Str::cast(code.filename()).equals(*filename));

  ASSERT_TRUE(code.names().isTuple());
  Tuple names(&scope, code.names());
  ASSERT_EQ(names.length(), 2);
  ASSERT_TRUE(names.contains(runtime_.newStrFromCStr("a")));
  ASSERT_TRUE(names.contains(runtime_.newStrFromCStr("b")));
}

TEST_F(BuiltinsModuleTest, BuiltinCompileBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
data = b'a = 1; b = 2'
code = compile(data, "<string>", "eval", dont_inherit=True)
)")
                   .isError());
  Code code(&scope, mainModuleAt(&runtime_, "code"));
  Object filename(&scope, code.filename());
  EXPECT_TRUE(isStrEqualsCStr(*filename, "<string>"));

  ASSERT_TRUE(code.names().isTuple());
  Tuple names(&scope, code.names());
  ASSERT_EQ(names.length(), 2);
  ASSERT_TRUE(names.contains(runtime_.newStrFromCStr("a")));
  ASSERT_TRUE(names.contains(runtime_.newStrFromCStr("b")));
}

TEST_F(BuiltinsModuleTest, BuiltinCompileWithBytesSubclass) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class Foo(bytes): pass
data = Foo(b"a = 1; b = 2")
code = compile(data, "<string>", "eval", dont_inherit=True)
)")
                   .isError());
  Code code(&scope, mainModuleAt(&runtime_, "code"));
  Object filename(&scope, code.filename());
  EXPECT_TRUE(isStrEqualsCStr(*filename, "<string>"));

  ASSERT_TRUE(code.names().isTuple());
  Tuple names(&scope, code.names());
  ASSERT_EQ(names.length(), 2);
  ASSERT_TRUE(names.contains(runtime_.newStrFromCStr("a")));
  ASSERT_TRUE(names.contains(runtime_.newStrFromCStr("b")));
}

TEST_F(BuiltinsModuleDeathTest, BuiltinCompileRaisesTypeErrorGivenTooFewArgs) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "compile(1)"), LayoutId::kTypeError,
      "TypeError: 'compile' takes min 3 positional arguments but 1 given"));
}

TEST_F(BuiltinsModuleDeathTest, BuiltinCompileRaisesTypeErrorGivenTooManyArgs) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "compile(1, 2, 3, 4, 5, 6, 7, 8, 9)"),
      LayoutId::kTypeError,
      "TypeError: 'compile' takes max 6 positional arguments but 9 given"));
}

TEST_F(BuiltinsModuleTest, BuiltinCompileRaisesTypeErrorGivenBadMode) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_,
                  "compile('hello', 'hello', 'hello', dont_inherit=True)"),
      LayoutId::kValueError,
      "Expected mode to be 'exec', 'eval', or 'single' in 'compile'"));
}

TEST_F(BuiltinsModuleTest, BuiltinExecSetsGlobal) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1337
exec("a = 1338")
  )")
                   .isError());
  // We can't use runBuiltin here because it does not set up the frame properly
  // for functions that need globals, implicitGlobals, etc
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST_F(BuiltinsModuleTest, BuiltinExecSetsGlobalGivenGlobals) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, "").isError());
  Module main(&scope, findMainModule(&runtime_));
  Dict globals(&scope, main.dict());
  Str globals_name(&scope, runtime_.newStrFromCStr("gl"));
  moduleAtPut(thread_, main, globals_name, globals);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1337
result = exec("a = 1338", gl)
  )")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result.isNoneType());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST_F(BuiltinsModuleTest, BuiltinExecWithEmptyGlobalsFailsToSetGlobal) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
a = 1337
result = exec("a = 1338", {})
  )")
                   .isError());
  Object result(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result.isNoneType());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1337));
}

TEST_F(BuiltinsModuleDeathTest, BuiltinExecWithNonDictGlobalsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
a = 1337
result = exec("a = 1338", 7)
  )"),
                            LayoutId::kTypeError,
                            "Expected 'globals' to be dict in 'exec'"));
}

TEST_F(BuiltinsModuleTest,
       BuiltinExecWithModuleProxyForGlobalsUsesContainedModuleDict) {
  HandleScope scope(thread_);
  Str name(&scope, runtime_.newStrFromCStr("testing_module"));
  Module module(&scope, runtime_.newModule(name));
  ModuleProxy module_proxy(&scope, module.moduleProxy());

  Str key(&scope, runtime_.newStrFromCStr("module_var_key"));
  SmallInt value(&scope, SmallInt::fromWord(5));
  moduleAtPut(thread_, module, key, value);

  Str code(&scope, runtime_.newStrFromCStr(R"(
global module_var_key
module_var_key += 1
  )"));

  Object locals(&scope, NoneType::object());
  EXPECT_TRUE(runBuiltin(BuiltinsModule::exec, code, module_proxy, locals)
                  .isNoneType());

  Object updated_value(&scope, moduleAt(thread_, module, key));
  EXPECT_TRUE(isIntEqualsWord(*updated_value, 5 + 1));
}

TEST_F(BuiltinsModuleTest,
       BuiltinExecWithModuleProxyForLocalsUsesContainedModuleDict) {
  HandleScope scope(thread_);
  Str name(&scope, runtime_.newStrFromCStr("testing_module"));
  Module module(&scope, runtime_.newModule(name));
  ModuleProxy module_proxy(&scope, module.moduleProxy());

  Str key(&scope, runtime_.newStrFromCStr("module_var_key"));
  SmallInt value(&scope, SmallInt::fromWord(5));
  moduleAtPut(thread_, module, key, value);

  Str code(&scope, runtime_.newStrFromCStr(R"(
module_var_key += 1
  )"));

  Object locals(&scope, NoneType::object());
  EXPECT_TRUE(runBuiltin(BuiltinsModule::exec, code, module_proxy, locals)
                  .isNoneType());

  Object updated_value(&scope, moduleAt(thread_, module, key));
  EXPECT_TRUE(isIntEqualsWord(*updated_value, 5 + 1));
}

TEST_F(BuiltinsModuleTest, BuiltinExecExWithTupleCallsExec) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exec(*("a = 1338",))
  )")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST_F(BuiltinsModuleTest, BuiltinExecExWithListCallsExec) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
exec(*["a = 1338"])
  )")
                   .isError());
  Object a(&scope, mainModuleAt(&runtime_, "a"));
  EXPECT_TRUE(isIntEqualsWord(*a, 1338));
}

TEST_F(BuiltinsModuleTest, AllOnListWithOnlyTrueReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = all([True, True])
  )")
                   .isError());
  HandleScope scope(thread_);
  Bool result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(result.value());
}

TEST_F(BuiltinsModuleTest, AllOnListWithFalseReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = all([True, False, True])
  )")
                   .isError());
  HandleScope scope(thread_);
  Bool result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_FALSE(result.value());
}

TEST_F(BuiltinsModuleTest, AnyOnListWithOnlyFalseReturnsFalse) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = any([False, False])
  )")
                   .isError());
  HandleScope scope(thread_);
  Bool result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_FALSE(result.value());
}

TEST_F(BuiltinsModuleTest, AnyOnListWithTrueReturnsTrue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = any([False, True, False])
  )")
                   .isError());
  HandleScope scope(thread_);
  Bool result(&scope, mainModuleAt(&runtime_, "result"));
  EXPECT_TRUE(result.value());
}

TEST_F(BuiltinsModuleTest, FilterWithNonIterableArgumentRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "filter(None, 1)"),
                            LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST_F(BuiltinsModuleTest,
       FilterWithNoneFuncAndIterableReturnsItemsOfTrueBoolValue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
f = filter(None, [1,0,2,0])
r0 = f.__next__()
r1 = f.__next__()
exhausted = False
try:
  f.__next__()
except StopIteration:
  exhausted = True
)")
                   .isError());
  HandleScope scope(thread_);
  Object r0(&scope, mainModuleAt(&runtime_, "r0"));
  Object r1(&scope, mainModuleAt(&runtime_, "r1"));
  Object exhausted(&scope, mainModuleAt(&runtime_, "exhausted"));
  EXPECT_TRUE(isIntEqualsWord(*r0, 1));
  EXPECT_TRUE(isIntEqualsWord(*r1, 2));
  EXPECT_EQ(*exhausted, Bool::trueObj());
}

TEST_F(
    BuiltinsModuleTest,
    FilterWithFuncReturningBoolAndIterableReturnsItemsEvaluatedToTrueByFunc) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def even(e): return e % 2 == 0

f = filter(even, [1,2,3,4])
r0 = f.__next__()
r1 = f.__next__()
exhausted = False
try:
  f.__next__()
except StopIteration:
  exhausted = True
)")
                   .isError());
  HandleScope scope(thread_);
  Object r0(&scope, mainModuleAt(&runtime_, "r0"));
  Object r1(&scope, mainModuleAt(&runtime_, "r1"));
  Object exhausted(&scope, mainModuleAt(&runtime_, "exhausted"));
  EXPECT_TRUE(isIntEqualsWord(*r0, 2));
  EXPECT_TRUE(isIntEqualsWord(*r1, 4));
  EXPECT_EQ(*exhausted, Bool::trueObj());
}

TEST_F(BuiltinsModuleTest, FormatWithNonStrFmtSpecRaisesTypeError) {
  EXPECT_TRUE(
      raised(runFromCStr(&runtime_, "format('hi', 1)"), LayoutId::kTypeError));
}

TEST_F(BuiltinsModuleTest, FormatCallsDunderFormat) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __format__(self, fmt_spec):
    return "foobar"
result = format(C(), 'hi')
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "foobar"));
}

TEST_F(BuiltinsModuleTest, FormatRaisesWhenDunderFormatReturnsNonStr) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __format__(self, fmt_spec):
    return 1
)")
                   .isError());
  EXPECT_TRUE(raised(runFromCStr(&runtime_, "format(C(), 'hi')"),
                     LayoutId::kTypeError));
}

TEST_F(BuiltinsModuleTest, IterWithIterableCallsDunderIter) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
l = list(iter([1, 2, 3]))
)")
                   .isError());
  HandleScope scope(thread_);
  Object l(&scope, mainModuleAt(&runtime_, "l"));
  EXPECT_PYLIST_EQ(l, {1, 2, 3});
}

TEST_F(BuiltinsModuleTest, IterWithNonIterableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
iter(None)
)"),
                            LayoutId::kTypeError,
                            "'NoneType' object is not iterable"));
}

TEST_F(BuiltinsModuleTest, IterWithRaisingDunderIterPropagatesException) {
  EXPECT_TRUE(raised(runFromCStr(&runtime_, R"(
class C:
  def __iter__(self):
    raise UserWarning()
iter(C())
)"),
                     LayoutId::kUserWarning));
}

TEST_F(BuiltinsModuleTest, IterWithCallableReturnsIterator) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __init__(self):
    self.x = 0
  def __call__(self):
    self.x += 1
    return self.x
c = C()
callable_iter = iter(c, 3)
reduced = callable_iter.__reduce__()
l = list(callable_iter)
)")
                   .isError());
  HandleScope scope(thread_);
  Object l(&scope, mainModuleAt(&runtime_, "l"));
  EXPECT_PYLIST_EQ(l, {1, 2});

  Object iter(&scope, moduleAtByCStr(&runtime_, "builtins", "iter"));
  Object c(&scope, mainModuleAt(&runtime_, "c"));
  Object reduced_obj(&scope, mainModuleAt(&runtime_, "reduced"));
  ASSERT_TRUE(reduced_obj.isTuple());
  Tuple reduced(&scope, *reduced_obj);
  ASSERT_EQ(reduced.length(), 2);
  EXPECT_EQ(reduced.at(0), iter);
  ASSERT_TRUE(reduced.at(1).isTuple());
  Tuple inner(&scope, reduced.at(1));
  ASSERT_EQ(inner.length(), 2);
  EXPECT_EQ(inner.at(0), c);
  EXPECT_TRUE(isIntEqualsWord(inner.at(1), 3));
}

TEST_F(BuiltinsModuleTest, NextWithoutIteratorRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class C:
  pass
next(C())
)"),
                            LayoutId::kTypeError,
                            "'C' object is not iterable"));
}

TEST_F(BuiltinsModuleTest, NextWithIteratorFetchesNextItem) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __iter__(self):
    self.a = 1
    return self

  def __next__(self):
    x = self.a
    self.a += 1
    return x

itr = iter(C())
c = next(itr)
d = next(itr)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "c"), 1));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "d"), 2));
}

TEST_F(BuiltinsModuleTest, NextWithIteratorAndDefaultFetchesNextItem) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __iter__(self):
    self.a = 1
    return self

  def __next__(self):
    x = self.a
    self.a += 1
    return x

itr = iter(C())
c = next(itr, 0)
d = next(itr, 0)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "c"), 1));
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "d"), 2));
}

TEST_F(BuiltinsModuleTest, NextWithIteratorRaisesStopIteration) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class C:
  def __iter__(self):
    return self

  def __next__(self):
    raise StopIteration('stopit')

itr = iter(C())
next(itr)
)"),
                            LayoutId::kStopIteration, "stopit"));
}

TEST_F(BuiltinsModuleTest, NextWithIteratorAndDefaultReturnsDefault) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class C:
  def __iter__(self):
    return self

  def __next__(self):
    raise StopIteration('stopit')
itr = iter(C())
c = next(itr, None)
)")
                   .isError());
  EXPECT_TRUE(mainModuleAt(&runtime_, "c").isNoneType());
}

TEST_F(BuiltinsModuleTest, SortedReturnsSortedList) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
unsorted = [5, 7, 8, 6]
result = sorted(unsorted)
)")
                   .isError());

  HandleScope scope(thread_);
  Object unsorted_obj(&scope, mainModuleAt(&runtime_, "unsorted"));
  ASSERT_TRUE(unsorted_obj.isList());
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result_obj.isList());
  EXPECT_NE(*unsorted_obj, *result_obj);

  List unsorted(&scope, *unsorted_obj);
  ASSERT_EQ(unsorted.numItems(), 4);
  EXPECT_EQ(unsorted.at(0), SmallInt::fromWord(5));
  EXPECT_EQ(unsorted.at(1), SmallInt::fromWord(7));
  EXPECT_EQ(unsorted.at(2), SmallInt::fromWord(8));
  EXPECT_EQ(unsorted.at(3), SmallInt::fromWord(6));

  List result(&scope, *result_obj);
  ASSERT_EQ(result.numItems(), 4);
  EXPECT_EQ(result.at(0), SmallInt::fromWord(5));
  EXPECT_EQ(result.at(1), SmallInt::fromWord(6));
  EXPECT_EQ(result.at(2), SmallInt::fromWord(7));
  EXPECT_EQ(result.at(3), SmallInt::fromWord(8));
}

TEST_F(BuiltinsModuleTest, SortedWithReverseReturnsReverseSortedList) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
unsorted = [1, 2, 3, 4]
result = sorted(unsorted, reverse=True)
)")
                   .isError());

  HandleScope scope(thread_);
  Object unsorted_obj(&scope, mainModuleAt(&runtime_, "unsorted"));
  ASSERT_TRUE(unsorted_obj.isList());
  Object result_obj(&scope, mainModuleAt(&runtime_, "result"));
  ASSERT_TRUE(result_obj.isList());
  EXPECT_NE(*unsorted_obj, *result_obj);

  List unsorted(&scope, *unsorted_obj);
  ASSERT_EQ(unsorted.numItems(), 4);
  EXPECT_EQ(unsorted.at(0), SmallInt::fromWord(1));
  EXPECT_EQ(unsorted.at(1), SmallInt::fromWord(2));
  EXPECT_EQ(unsorted.at(2), SmallInt::fromWord(3));
  EXPECT_EQ(unsorted.at(3), SmallInt::fromWord(4));

  List result(&scope, *result_obj);
  ASSERT_EQ(result.numItems(), 4);
  EXPECT_EQ(result.at(0), SmallInt::fromWord(4));
  EXPECT_EQ(result.at(1), SmallInt::fromWord(3));
  EXPECT_EQ(result.at(2), SmallInt::fromWord(2));
  EXPECT_EQ(result.at(3), SmallInt::fromWord(1));
}

TEST_F(BuiltinsModuleTest, MaxWithEmptyIterableRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "max([])"),
                            LayoutId::kValueError,
                            "max() arg is an empty sequence"));
}

TEST_F(BuiltinsModuleTest, MaxWithMultipleArgsReturnsMaximum) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = max(1, 3, 5, 2, -1)").isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 5));
}

TEST_F(BuiltinsModuleTest, MaxWithNoArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "max()"), LayoutId::kTypeError,
      "TypeError: 'max' takes min 1 positional arguments but 0 given"));
}

TEST_F(BuiltinsModuleTest, MaxWithIterableReturnsMaximum) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = max((1, 3, 5, 2, -1))").isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 5));
}

TEST_F(BuiltinsModuleTest, MaxWithEmptyIterableAndDefaultReturnsDefault) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = max([], default=42)").isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 42));
}

TEST_F(BuiltinsModuleTest, MaxWithKeyOrdersByKeyFunction) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = max((1, 2, 3), key=lambda x: -x)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 1));
}

TEST_F(BuiltinsModuleTest, MaxWithEmptyIterableAndKeyAndDefaultReturnsDefault) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = max((), key=lambda x: x, default='empty')
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "empty"));
}

TEST_F(BuiltinsModuleTest, MaxWithMultipleArgsAndDefaultRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "max(1, 2, default=0)"), LayoutId::kTypeError,
      "Cannot specify a default for max() with multiple positional arguments"));
}

TEST_F(BuiltinsModuleTest, MaxWithKeyReturnsFirstOccuranceOfEqualValues) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  pass

first = A()
second = A()
result = max(first, second, key=lambda x: 1) is first
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "result"), Bool::trueObj());
}

TEST_F(BuiltinsModuleTest, MaxWithoutKeyReturnsFirstOccuranceOfEqualValues) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A():
  def __gt__(self, _):
    return False

first = A()
second = A()
result = max(first, second) is first
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "result"), Bool::trueObj());
}

TEST_F(BuiltinsModuleTest, MinWithEmptyIterableRaisesValueError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "min([])"),
                            LayoutId::kValueError,
                            "min() arg is an empty sequence"));
}

TEST_F(BuiltinsModuleTest, MinWithMultipleArgsReturnsMinimum) {
  ASSERT_FALSE(runFromCStr(&runtime_, "result = min(4, 3, 1, 2, 5)").isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 1));
}

TEST_F(BuiltinsModuleTest, MinWithNoArgsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "min()"), LayoutId::kTypeError,
      "TypeError: 'min' takes min 1 positional arguments but 0 given"));
}

TEST_F(BuiltinsModuleTest, MinWithIterableReturnsMinimum) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = min((4, 3, 1, 2, 5))").isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 1));
}

TEST_F(BuiltinsModuleTest, MinWithEmptyIterableAndDefaultReturnsDefault) {
  ASSERT_FALSE(
      runFromCStr(&runtime_, "result = min([], default=42)").isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 42));
}

TEST_F(BuiltinsModuleTest, MinWithKeyOrdersByKeyFunction) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = min((1, 2, 3), key=lambda x: -x)
)")
                   .isError());
  EXPECT_TRUE(isIntEqualsWord(mainModuleAt(&runtime_, "result"), 3));
}

TEST_F(BuiltinsModuleTest, MinWithEmptyIterableAndKeyAndDefaultReturnsDefault) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
result = min((), key=lambda x: x, default='empty')
)")
                   .isError());
  EXPECT_TRUE(isStrEqualsCStr(mainModuleAt(&runtime_, "result"), "empty"));
}

TEST_F(BuiltinsModuleTest, MinWithMultipleArgsAndDefaultRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(
      runFromCStr(&runtime_, "min(1, 2, default=0)"), LayoutId::kTypeError,
      "Cannot specify a default for min() with multiple positional arguments"));
}

TEST_F(BuiltinsModuleTest, MinReturnsFirstOccuranceOfEqualValues) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A:
  pass

first = A()
second = A()
result = min(first, second, key=lambda x: 1) is first
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "result"), Bool::trueObj());
}

TEST_F(BuiltinsModuleTest, MinWithoutKeyReturnsFirstOccuranceOfEqualValues) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
class A():
  def __lt__(self, _):
    return False

first = A()
second = A()
result = min(first, second) is first
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "result"), Bool::trueObj());
}

TEST_F(BuiltinsModuleTest, MapWithNonIterableArgumentRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "map(1,1)"),
                            LayoutId::kTypeError,
                            "'int' object is not iterable"));
}

TEST_F(BuiltinsModuleTest,
       MapWithIterableDunderNextReturnsFuncAppliedElementsSequentially) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def inc(e):
  return e + 1

m = map(inc, [1,2])
r0 = m.__next__()
r1 = m.__next__()
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "r0"), SmallInt::fromWord(2));
  EXPECT_EQ(mainModuleAt(&runtime_, "r1"), SmallInt::fromWord(3));
}

TEST_F(
    BuiltinsModuleTest,
    MapWithMultipleIterablesDunderNextReturnsFuncAppliedElementsSequentially) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def inc(e0, e1):
  return e0 + e1

m = map(inc, [1,2], [100,200])
r0 = m.__next__()
r1 = m.__next__()
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "r0"), SmallInt::fromWord(101));
  EXPECT_EQ(mainModuleAt(&runtime_, "r1"), SmallInt::fromWord(202));
}

TEST_F(BuiltinsModuleTest, MapDunderNextFinishesByRaisingStopIteration) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def inc(e):
  return e + 1

m = map(inc, [1,2])
m.__next__()
m.__next__()
exc_raised = False
try:
  m.__next__()
except StopIteration:
  exc_raised = True
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "exc_raised"), Bool::trueObj());
}

TEST_F(
    BuiltinsModuleTest,
    MapWithMultipleIterablesDunderNextFinishesByRaisingStopIterationOnShorterOne) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
def inc(e0, e1):
  return e0, e1

m = map(inc, [1,2], [100])
m.__next__()
exc_raised = False
try:
  m.__next__()
except StopIteration:
  exc_raised = True
)")
                   .isError());
  EXPECT_EQ(mainModuleAt(&runtime_, "exc_raised"), Bool::trueObj());
}

TEST_F(BuiltinsModuleTest, EnumerateWithNonIterableRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, "enumerate(1.0)"),
                            LayoutId::kTypeError,
                            "'float' object is not iterable"));
}

TEST_F(BuiltinsModuleTest, EnumerateReturnsEnumeratedTuples) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
e = enumerate([7, 3])
res1 = e.__next__()
res2 = e.__next__()
exhausted = False
try:
  e.__next__()
except StopIteration:
  exhausted = True
)")
                   .isError());
  HandleScope scope(thread_);
  Object res1(&scope, mainModuleAt(&runtime_, "res1"));
  ASSERT_TRUE(res1.isTuple());
  EXPECT_EQ(Tuple::cast(*res1).at(0), SmallInt::fromWord(0));
  EXPECT_EQ(Tuple::cast(*res1).at(1), SmallInt::fromWord(7));
  Object res2(&scope, mainModuleAt(&runtime_, "res2"));
  ASSERT_TRUE(res2.isTuple());
  EXPECT_EQ(Tuple::cast(*res2).at(0), SmallInt::fromWord(1));
  EXPECT_EQ(Tuple::cast(*res2).at(1), SmallInt::fromWord(3));
  EXPECT_EQ(mainModuleAt(&runtime_, "exhausted"), Bool::trueObj());
}

TEST_F(BuiltinsModuleTest, AbsReturnsAbsoluteValue) {
  ASSERT_FALSE(runFromCStr(&runtime_, R"(
res1 = abs(10)
res2 = abs(-10)
)")
                   .isError());

  HandleScope scope(thread_);
  Object res1(&scope, mainModuleAt(&runtime_, "res1"));
  EXPECT_TRUE(isIntEqualsWord(*res1, 10));
  Object res2(&scope, mainModuleAt(&runtime_, "res2"));
  EXPECT_TRUE(isIntEqualsWord(*res2, 10));
}

TEST_F(BuiltinsModuleTest, AbsWithoutDunderAbsRaisesTypeError) {
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime_, R"(
class Foo(): pass
res1 = abs(Foo())
)"),
                            LayoutId::kTypeError,
                            "bad operand type for abs(): 'Foo'"));
}

}  // namespace python
