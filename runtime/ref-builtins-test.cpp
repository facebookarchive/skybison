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
b = None
def f(ref):
    global b
    b = ref
weak = ref(a, f)
callback = weak.__callback__
)";
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  RawObject a = mainModuleAt(runtime_, "a");
  RawObject b = mainModuleAt(runtime_, "b");
  RawObject f = mainModuleAt(runtime_, "f");
  RawObject cb = mainModuleAt(runtime_, "callback");
  RawObject weak = mainModuleAt(runtime_, "weak");
  EXPECT_EQ(WeakRef::cast(weak).referent(), a);
  EXPECT_EQ(b, NoneType::object());
  EXPECT_TRUE(WeakRef::cast(weak).callback().isBoundMethod());
  EXPECT_EQ(BoundMethod::cast(WeakRef::cast(weak).callback()).self(), weak);
  EXPECT_EQ(f, cb);

  Module main(&scope, findMainModule(runtime_));
  Dict globals(&scope, main.dict());
  Str name(&scope, runtime_->newStrFromCStr("a"));
  dictRemoveByStr(thread_, globals, name);

  runtime_->collectGarbage();
  weak = mainModuleAt(runtime_, "weak");
  b = mainModuleAt(runtime_, "b");
  EXPECT_EQ(b, weak);
  EXPECT_EQ(WeakRef::cast(weak).referent(), NoneType::object());
  EXPECT_EQ(WeakRef::cast(weak).callback(), NoneType::object());
}

TEST_F(RefBuiltinsTest, DunderCallbackWithNoBoundMethodReturnsBoundMethod) {
  const char* src = R"(
from _weakref import ref
class Foo: pass
class Bar:
  def method(self, wr):
    pass

a = Foo()
b = Bar()
original_callback = b.method
weak = ref(a, original_callback)
callback = weak.__callback__
)";
  ASSERT_FALSE(runFromCStr(runtime_, src).isError());
  RawObject original_callback = mainModuleAt(runtime_, "original_callback");
  RawObject callback = mainModuleAt(runtime_, "callback");
  EXPECT_EQ(callback, original_callback);
}

TEST_F(RefBuiltinsTest, DunderCallReturnsObject) {
  HandleScope scope(thread_);
  Object obj(&scope, Str::empty());
  WeakRef ref(&scope, runtime_->newWeakRef(thread_, obj));
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
