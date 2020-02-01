#include "marshal-module.h"

#include "gtest/gtest.h"

#include "bytes-builtins.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace py {

namespace testing {

using MarshalModuleTest = RuntimeFixture;

TEST_F(MarshalModuleTest, LoadsReadsSet) {
  HandleScope scope(thread_);
  // marshal.loads(set())
  const byte set_bytes[] = "\xbc\x00\x00\x00\x00";
  Bytes bytes(&scope, runtime_->newBytesWithAll(set_bytes));
  Object obj(&scope, runBuiltin(FUNC(marshal, loads), bytes));
  ASSERT_TRUE(obj.isSet());
  EXPECT_EQ(Set::cast(*obj).numItems(), 0);
}

TEST_F(MarshalModuleTest, LoadsWithBytesSubclassReadsSet) {
  HandleScope scope(thread_);
  // marshal.loads(set())
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class Foo(bytes): pass
foo = Foo(b"\xbc\x00\x00\x00\x00")
)")
                   .isError());
  Object bytes(&scope, mainModuleAt(runtime_, "foo"));
  Object obj(&scope, runBuiltin(FUNC(marshal, loads), bytes));
  ASSERT_TRUE(obj.isSet());
  EXPECT_EQ(Set::cast(*obj).numItems(), 0);
}

TEST_F(MarshalModuleTest, LoadsIgnoresExtraBytesAtEnd) {
  HandleScope scope(thread_);
  // marshal.loads(set() + some extra bytes)
  const byte set_bytes[] = "\xbc\x00\x00\x00\x00\x00\x00\x00\xAA\xBB\xCC";
  Bytes bytes(&scope, runtime_->newBytesWithAll(set_bytes));
  Object obj(&scope, runBuiltin(FUNC(marshal, loads), bytes));
  ASSERT_TRUE(obj.isSet());
  EXPECT_EQ(Set::cast(*obj).numItems(), 0);
}

}  // namespace testing

}  // namespace py
