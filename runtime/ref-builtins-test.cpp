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
  EXPECT_EQ(RawWeakRef::cast(weak)->referent(), a);
  EXPECT_EQ(RawWeakRef::cast(weak)->callback(), NoneType::object());

  Module main(&scope, findModule(&runtime, "__main__"));
  Dict globals(&scope, main.dict());
  Object key(&scope, runtime.newStrFromCStr("a"));
  runtime.dictRemove(globals, key);

  runtime.collectGarbage();
  weak = moduleAt(&runtime, "__main__", "weak");
  EXPECT_EQ(RawWeakRef::cast(weak)->referent(), NoneType::object());
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
  EXPECT_EQ(RawWeakRef::cast(weak)->referent(), a);
  EXPECT_TRUE(isIntEqualsWord(b, 1));

  Module main(&scope, findModule(&runtime, "__main__"));
  Dict globals(&scope, main.dict());
  Object key(&scope, runtime.newStrFromCStr("a"));
  runtime.dictRemove(globals, key);

  runtime.collectGarbage();
  weak = moduleAt(&runtime, "__main__", "weak");
  b = moduleAt(&runtime, "__main__", "b");
  EXPECT_TRUE(isIntEqualsWord(b, 2));
  EXPECT_EQ(RawWeakRef::cast(weak)->referent(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(weak)->callback(), NoneType::object());
}

TEST(RefBuiltinsTest, DunderCallReturnsObject) {
  Runtime runtime;
  runFromCStr(&runtime, R"(
from _weakref import ref
class C: pass
c = C()
r = ref(c)
result = r()
)");
  HandleScope scope;
  Object c(&scope, moduleAt(&runtime, "__main__", "c"));
  Object result(&scope, moduleAt(&runtime, "__main__", "result"));
  EXPECT_EQ(c, result);
}

TEST(RefBuiltinsTest, DunderCallWithNonRefRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, R"(
from _weakref import ref
ref.__call__(None)
)"),
                            LayoutId::kTypeError,
                            "'__call__' requires a 'ref' object"));
}

}  // namespace python
