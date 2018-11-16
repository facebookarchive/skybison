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
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Object* a = moduleAt(&runtime, main, "a");
  Object* weak = moduleAt(&runtime, main, "weak");
  EXPECT_EQ(WeakRef::cast(weak)->referent(), a);
  EXPECT_EQ(WeakRef::cast(weak)->callback(), None::object());

  Handle<Dictionary> globals(&scope, main->dictionary());
  Handle<Object> key(&scope, runtime.newStringFromCString("a"));
  runtime.dictionaryRemove(globals, key, &a);

  runtime.collectGarbage();
  weak = moduleAt(&runtime, main, "weak");
  EXPECT_EQ(WeakRef::cast(weak)->referent(), None::object());
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
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Object* a = moduleAt(&runtime, main, "a");
  Object* b = moduleAt(&runtime, main, "b");
  Object* weak = moduleAt(&runtime, main, "weak");
  EXPECT_EQ(WeakRef::cast(weak)->referent(), a);
  EXPECT_EQ(SmallInteger::cast(b)->value(), 1);

  Handle<Dictionary> globals(&scope, main->dictionary());
  Handle<Object> key(&scope, runtime.newStringFromCString("a"));
  runtime.dictionaryRemove(globals, key, &a);

  runtime.collectGarbage();
  weak = moduleAt(&runtime, main, "weak");
  b = moduleAt(&runtime, main, "b");
  EXPECT_EQ(SmallInteger::cast(b)->value(), 2);
  EXPECT_EQ(WeakRef::cast(weak)->referent(), None::object());
  EXPECT_EQ(WeakRef::cast(weak)->callback(), None::object());
}

} // namespace python
