#include "ref-builtins.h"

#include "gtest/gtest.h"

#include "builtins.h"
#include "dict-builtins.h"
#include "runtime.h"
#include "super-builtins.h"
#include "test-utils.h"

namespace py {
namespace testing {

using RefBuiltinsTest = RuntimeFixture;

TEST_F(RefBuiltinsTest, ReferentTest) {
  const char* src = R"(
from _weakref import ref
class Foo: pass
a = Foo()
weak = ref(a)
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  RawObject a = mainModuleAt(runtime_, "a");
  RawObject weak = mainModuleAt(runtime_, "weak");
  EXPECT_EQ(WeakRef::cast(weak).referent(), a);
  EXPECT_EQ(WeakRef::cast(weak).callback(), NoneType::object());

  Module main(&scope, findMainModule(runtime_));
  Dict globals(&scope, main.dict());
  Str name(&scope, runtime_->newStrFromCStr("a"));
  dictRemoveByStr(thread_, globals, name);

  runtime_->collectGarbage();
  weak = mainModuleAt(runtime_, "weak");
  EXPECT_EQ(WeakRef::cast(weak).referent(), NoneType::object());
}

TEST_F(RefBuiltinsTest, CallbackTest) {
  const char* src = R"(
from _weakref import ref
class Foo: pass
a = Foo()
b = 1
def f(ref):
    global b
    b = 2
weak = ref(a, f)
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  RawObject a = mainModuleAt(runtime_, "a");
  RawObject b = mainModuleAt(runtime_, "b");
  RawObject weak = mainModuleAt(runtime_, "weak");
  EXPECT_EQ(WeakRef::cast(weak).referent(), a);
  EXPECT_TRUE(isIntEqualsWord(b, 1));

  Module main(&scope, findMainModule(runtime_));
  Dict globals(&scope, main.dict());
  Str name(&scope, runtime_->newStrFromCStr("a"));
  dictRemoveByStr(thread_, globals, name);

  runtime_->collectGarbage();
  weak = mainModuleAt(runtime_, "weak");
  b = mainModuleAt(runtime_, "b");
  EXPECT_TRUE(isIntEqualsWord(b, 2));
  EXPECT_EQ(WeakRef::cast(weak).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(weak).callback(), NoneType::object());
}

TEST_F(RefBuiltinsTest, DunderCallReturnsObject) {
  HandleScope scope(thread_);
  Object obj(&scope, Str::empty());
  Object callback(&scope, NoneType::object());
  WeakRef ref(&scope, runtime_->newWeakRef(thread_, obj, callback));
  Object result(&scope, runBuiltin(METH(weakref, __call__), ref));
  EXPECT_EQ(result, obj);
}

TEST_F(RefBuiltinsTest, DunderHashWithDeadRefRaisesTypeError) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _weakref
class C:
  pass
ref = _weakref.ref(C())
)")
                   .isError());
  runtime_->collectGarbage();
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "ref.__hash__()"),
                            LayoutId::kTypeError, "weak object has gone away"));
}

TEST_F(RefBuiltinsTest, DunderHashCallsHashOfReferent) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _weakref
class C:
  def __hash__(self):
    raise Exception("foo")
c = C()
ref = _weakref.ref(c)
)")
                   .isError());
  EXPECT_TRUE(raisedWithStr(runFromCStr(runtime_, "ref.__hash__()"),
                            LayoutId::kException, "foo"));
}

TEST_F(RefBuiltinsTest, WeakRefUnderlyingReturnsUnderlyingRef) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _weakref
class SubRef(_weakref.ref):
  pass

class C:
  pass

c = C()
sub_ref = SubRef(c)
)")
                   .isError());
  HandleScope scope(thread_);
  Object sub_ref_obj(&scope, mainModuleAt(runtime_, "sub_ref"));
  WeakRef ref(&scope, weakRefUnderlying(*sub_ref_obj));
  EXPECT_EQ(ref.referent(), mainModuleAt(runtime_, "c"));
}

TEST_F(RefBuiltinsTest, RefSubclassReferentSetsToNone) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import _weakref
class SubRef(_weakref.ref):
  pass

class C:
  pass

c = C()
sub_ref = SubRef(c)
c = None
)")
                   .isError());
  runtime_->collectGarbage();
  HandleScope scope(thread_);
  WeakRef ref(&scope, weakRefUnderlying(mainModuleAt(runtime_, "sub_ref")));
  EXPECT_EQ(ref.referent(), NoneType::object());
}

}  // namespace testing
}  // namespace py
