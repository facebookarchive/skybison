#include "gtest/gtest.h"

#include "builtins-module.h"
#include "dict-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DictBuiltinsTest, DunderDelItemOnExistingKeyReturnsNone) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Handle<Dict> dict(&scope, runtime.newDict(1));
  Handle<Object> key(&scope, runtime.newStringFromCString("foo"));
  Handle<Object> val(&scope, runtime.newInt(0));
  runtime.dictAtPut(dict, key, val);
  frame->setLocal(0, *dict);
  frame->setLocal(1, *key);
  Object* result = builtinDictDelItem(thread, frame, 2);
  EXPECT_TRUE(result->isNone());
}

TEST(DictBuiltinsTest, DunderDelItemOnNonexistentKeyThrowsKeyError) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Handle<Dict> dict(&scope, runtime.newDict(1));
  Handle<Object> key(&scope, runtime.newStringFromCString("foo"));
  Handle<Object> val(&scope, runtime.newInt(0));
  runtime.dictAtPut(dict, key, val);
  frame->setLocal(0, *dict);
  // "bar" doesn't exist in this dictionary, attempting to delete it should
  // cause a KeyError.
  frame->setLocal(1, runtime.newStringFromCString("bar"));
  Object* result = builtinDictDelItem(thread, frame, 2);
  ASSERT_TRUE(result->isError());
}

TEST(DictBuiltinsTest, DelOnExistingKeyDeletesKey) {
  Runtime runtime;
  runtime.runFromCString(R"(
d = {"foo": 1}
del d["foo"]
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Dict> d(&scope, moduleAt(&runtime, main, "d"));
  Handle<Object> foo(&scope, runtime.newStringFromCString("foo"));

  EXPECT_FALSE(runtime.dictIncludes(d, foo));
}

TEST(DictBuiltinsDeathTest, DelOnNonexistentKeyThrowsKeyError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCString(R"(
d = {}
del d["foo"]
)"),
               "aborting due to pending exception");
}

}  // namespace python
