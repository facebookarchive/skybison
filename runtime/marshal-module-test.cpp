#include "gtest/gtest.h"

#include "marshal-module.h"
#include "os.h"
#include "runtime.h"
#include "test-utils.h"

namespace python {

namespace testing {

TEST(MarshalModuleTest, LoadsWithTooFewArgumentsRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  EXPECT_TRUE(raised(runBuiltin(MarshalModule::loads), LayoutId::kTypeError));
}

TEST(MarshalModuleTest, LoadsWithTooManyArgumentsRaisesTypeError) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);
  Object obj(&scope, RawNoneType::object());
  EXPECT_TRUE(raised(runBuiltin(MarshalModule::loads, obj, obj, obj),
                     LayoutId::kTypeError));
}

TEST(MarshalModuleTest, LoadsReadsSet) {
  Runtime runtime;
  HandleScope scope;
  // marshal.loads(set())
  const byte set_bytes[] = "\xbc\x00\x00\x00\x00";
  Bytes bytes(&scope, runtime.newBytesWithAll(set_bytes));
  Object obj(&scope, runBuiltin(MarshalModule::loads, bytes));
  ASSERT_TRUE(obj->isSet());
  EXPECT_EQ(RawSet::cast(*obj).numItems(), 0);
}

TEST(MarshalModuleTest, LoadsIgnoresExtraBytesAtEnd) {
  Runtime runtime;
  HandleScope scope;
  // marshal.loads(set() + some extra bytes)
  const byte set_bytes[] = "\xbc\x00\x00\x00\x00\x00\x00\x00\xAA\xBB\xCC";
  Bytes bytes(&scope, runtime.newBytesWithAll(set_bytes));
  Object obj(&scope, runBuiltin(MarshalModule::loads, bytes));
  ASSERT_TRUE(obj->isSet());
  EXPECT_EQ(RawSet::cast(*obj).numItems(), 0);
}

}  // namespace testing

}  // namespace python
