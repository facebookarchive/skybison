#include "gtest/gtest.h"

#include "ref-builtins.h"
#include "runtime.h"
#include "super-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(RefBuiltinsTest, ReferentTest) {
  const char* src = R"(
from _weakref import ref
class Foo: pass
a = Foo()
weak = ref(a)
)";
  Runtime runtime;
  HandleScope scope;
  compileAndRunToString(&runtime, src);
  RawObject a = moduleAt(&runtime, "__main__", "a");
  RawObject weak = moduleAt(&runtime, "__main__", "weak");
  EXPECT_EQ(RawWeakRef::cast(weak).referent(), a);
  EXPECT_EQ(RawWeakRef::cast(weak).callback(), NoneType::object());

  Module main(&scope, findModule(&runtime, "__main__"));
  Dict globals(&scope, main.dict());
  Object key(&scope, runtime.newStrFromCStr("a"));
  runtime.dictRemove(globals, key);

  runtime.collectGarbage();
  weak = moduleAt(&runtime, "__main__", "weak");
  EXPECT_EQ(RawWeakRef::cast(weak).referent(), NoneType::object());
}

TEST(RefBuiltinsTest, CallbackTest) {
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
  Runtime runtime;
  HandleScope scope;
  compileAndRunToString(&runtime, src);
  RawObject a = moduleAt(&runtime, "__main__", "a");
  RawObject b = moduleAt(&runtime, "__main__", "b");
  RawObject weak = moduleAt(&runtime, "__main__", "weak");
  EXPECT_EQ(RawWeakRef::cast(weak).referent(), a);
  EXPECT_TRUE(isIntEqualsWord(b, 1));

  Module main(&scope, findModule(&runtime, "__main__"));
  Dict globals(&scope, main.dict());
  Object key(&scope, runtime.newStrFromCStr("a"));
  runtime.dictRemove(globals, key);

  runtime.collectGarbage();
  weak = moduleAt(&runtime, "__main__", "weak");
  b = moduleAt(&runtime, "__main__", "b");
  EXPECT_TRUE(isIntEqualsWord(b, 2));
  EXPECT_EQ(RawWeakRef::cast(weak).referent(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(weak).callback(), NoneType::object());
}

TEST(RefBuiltinsTest, DunderCallReturnsObject) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, Str::empty());
  Object callback(&scope, NoneType::object());
  WeakRef ref(&scope, runtime.newWeakRef(thread, obj, callback));
  Object result(&scope, runBuiltin(RefBuiltins::dunderCall, ref));
  EXPECT_EQ(result, obj);
}

TEST(RefBuiltinsTest, DunderCallWithNonRefRaisesTypeError) {
  Runtime runtime;
  HandleScope scope;
  Object obj(&scope, NoneType::object());
  Object result(&scope, runBuiltin(RefBuiltins::dunderCall, obj));
  EXPECT_TRUE(raisedWithStr(*result, LayoutId::kTypeError,
                            "'__call__' requires a 'ref' object"));
}

}  // namespace python
