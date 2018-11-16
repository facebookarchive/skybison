#include "gtest/gtest.h"

#include "builtins-module.h"
#include "dict-builtins.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(DictBuiltinsTest, DunderContainsWithExistingKeyReturnsTrue) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Handle<Dict> dict(&scope, runtime.newDict(1));
  Handle<Object> key(&scope, runtime.newStrFromCStr("foo"));
  Handle<Object> val(&scope, runtime.newInt(0));
  runtime.dictAtPut(dict, key, val);
  frame->setLocal(0, *dict);
  frame->setLocal(1, *key);
  Object* result = DictBuiltins::dunderContains(thread, frame, 2);
  ASSERT_TRUE(result->isBool());
  EXPECT_TRUE(Bool::cast(result)->value());
}

TEST(DictBuiltinsTest, DunderContainsWithNonexistentKeyReturnsFalse) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  frame->setLocal(0, runtime.newDict(0));
  frame->setLocal(1, runtime.newStrFromCStr("foo"));
  Object* result = DictBuiltins::dunderContains(thread, frame, 2);
  ASSERT_TRUE(result->isBool());
  EXPECT_FALSE(Bool::cast(result)->value());
}

TEST(DictBuiltinsTest, InWithExistingKeyReturnsTrue) {
  Runtime runtime;
  runtime.runFromCStr(R"(
d = {"foo": 1}
foo_in_d = "foo" in d
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Bool> foo_in_d(&scope, moduleAt(&runtime, main, "foo_in_d"));

  EXPECT_TRUE(foo_in_d->value());
}

TEST(DictBuiltinsTest, InWithNonexistentKeyReturnsFalse) {
  Runtime runtime;
  runtime.runFromCStr(R"(
d = {}
foo_in_d = "foo" in d
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Bool> foo_in_d(&scope, moduleAt(&runtime, main, "foo_in_d"));

  EXPECT_FALSE(foo_in_d->value());
}

TEST(DictBuiltinsTest, DunderDelItemOnExistingKeyReturnsNone) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Handle<Dict> dict(&scope, runtime.newDict(1));
  Handle<Object> key(&scope, runtime.newStrFromCStr("foo"));
  Handle<Object> val(&scope, runtime.newInt(0));
  runtime.dictAtPut(dict, key, val);
  frame->setLocal(0, *dict);
  frame->setLocal(1, *key);
  Object* result = DictBuiltins::dunderDelItem(thread, frame, 2);
  EXPECT_TRUE(result->isNone());
}

TEST(DictBuiltinsTest, DunderDelItemOnNonexistentKeyThrowsKeyError) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 2, 0);
  Handle<Dict> dict(&scope, runtime.newDict(1));
  Handle<Object> key(&scope, runtime.newStrFromCStr("foo"));
  Handle<Object> val(&scope, runtime.newInt(0));
  runtime.dictAtPut(dict, key, val);
  frame->setLocal(0, *dict);
  // "bar" doesn't exist in this dictionary, attempting to delete it should
  // cause a KeyError.
  frame->setLocal(1, runtime.newStrFromCStr("bar"));
  Object* result = DictBuiltins::dunderDelItem(thread, frame, 2);
  ASSERT_TRUE(result->isError());
}

TEST(DictBuiltinsTest, DelOnExistingKeyDeletesKey) {
  Runtime runtime;
  runtime.runFromCStr(R"(
d = {"foo": 1}
del d["foo"]
)");
  HandleScope scope;
  Handle<Module> main(&scope, findModule(&runtime, "__main__"));
  Handle<Dict> d(&scope, moduleAt(&runtime, main, "d"));
  Handle<Object> foo(&scope, runtime.newStrFromCStr("foo"));

  EXPECT_FALSE(runtime.dictIncludes(d, foo));
}

TEST(DictBuiltinsDeathTest, DelOnNonexistentKeyThrowsKeyError) {
  Runtime runtime;
  EXPECT_DEATH(runtime.runFromCStr(R"(
d = {}
del d["foo"]
)"),
               "aborting due to pending exception");
}

TEST(DictBuiltinsTest, DunderSetItemWithExistingKey) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 3, 0);
  Handle<Dict> dict(&scope, runtime.newDict(1));
  Handle<Object> key(&scope, runtime.newStrFromCStr("foo"));
  Handle<Object> val(&scope, runtime.newInt(0));
  Handle<Object> val2(&scope, runtime.newInt(1));
  runtime.dictAtPut(dict, key, val);
  frame->setLocal(0, *dict);
  frame->setLocal(1, *key);
  frame->setLocal(2, *val2);
  Handle<Object> result(&scope, DictBuiltins::dunderSetItem(thread, frame, 3));
  ASSERT_TRUE(result->isNone());
  ASSERT_EQ(dict->numItems(), 1);
  ASSERT_EQ(runtime.dictAt(dict, key), *val2);
}

TEST(DictBuiltinsTest, DunderSetItemWithNonExistentKey) {
  Runtime runtime;
  HandleScope scope;
  Thread* thread = Thread::currentThread();
  Frame* frame = thread->openAndLinkFrame(0, 3, 0);
  Handle<Dict> dict(&scope, runtime.newDict(1));
  Handle<Object> key(&scope, runtime.newStrFromCStr("foo"));
  Handle<Object> val(&scope, runtime.newInt(0));
  frame->setLocal(0, *dict);
  frame->setLocal(1, *key);
  frame->setLocal(2, *val);
  ASSERT_EQ(dict->numItems(), 0);
  Handle<Object> result(&scope, DictBuiltins::dunderSetItem(thread, frame, 3));
  ASSERT_TRUE(result->isNone());
  ASSERT_EQ(dict->numItems(), 1);
  ASSERT_EQ(runtime.dictAt(dict, key), *val);
}

}  // namespace python
