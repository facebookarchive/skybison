#include "gtest/gtest.h"

#include "runtime.h"
#include "super-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(RefTest, ReferentTest) {
  const char* src = R"(
from _weakref import ref
class Foo: pass
a = Foo()
weak = ref(a)
)";
  Runtime runtime;
  HandleScope scope;
  compileAndRunToString(&runtime, src);
  Module main(&scope, findModule(&runtime, "__main__"));
  RawObject a = moduleAt(&runtime, main, "a");
  RawObject weak = moduleAt(&runtime, main, "weak");
  EXPECT_EQ(RawWeakRef::cast(weak)->referent(), a);
  EXPECT_EQ(RawWeakRef::cast(weak)->callback(), NoneType::object());

  Dict globals(&scope, main->dict());
  Object key(&scope, runtime.newStrFromCStr("a"));
  runtime.dictRemove(globals, key);

  runtime.collectGarbage();
  weak = moduleAt(&runtime, main, "weak");
  EXPECT_EQ(RawWeakRef::cast(weak)->referent(), NoneType::object());
}

TEST(RefTest, CallbackTest) {
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
  Module main(&scope, findModule(&runtime, "__main__"));
  RawObject a = moduleAt(&runtime, main, "a");
  RawObject b = moduleAt(&runtime, main, "b");
  RawObject weak = moduleAt(&runtime, main, "weak");
  EXPECT_EQ(RawWeakRef::cast(weak)->referent(), a);
  EXPECT_EQ(RawSmallInt::cast(b)->value(), 1);

  Dict globals(&scope, main->dict());
  Object key(&scope, runtime.newStrFromCStr("a"));
  runtime.dictRemove(globals, key);

  runtime.collectGarbage();
  weak = moduleAt(&runtime, main, "weak");
  b = moduleAt(&runtime, main, "b");
  EXPECT_EQ(RawSmallInt::cast(b)->value(), 2);
  EXPECT_EQ(RawWeakRef::cast(weak)->referent(), NoneType::object());
  EXPECT_EQ(RawWeakRef::cast(weak)->callback(), NoneType::object());
}

}  // namespace python
