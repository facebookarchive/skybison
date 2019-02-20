#include "gtest/gtest.h"

#include "bytearray-builtins.h"
#include "test-utils.h"

namespace python {

using namespace testing;

TEST(ByteArrayBuiltinsTest, Add) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  ByteArray array(&scope, runtime.newByteArray());
  byteArrayAdd(thread, &runtime, array, 0);
  byteArrayAdd(thread, &runtime, array, 1);
  byteArrayAdd(thread, &runtime, array, 2);
  EXPECT_GE(array.capacity(), 3);
  EXPECT_EQ(array.numItems(), 3);
  EXPECT_EQ(array.byteAt(0), 0);
  EXPECT_EQ(array.byteAt(1), 1);
  EXPECT_EQ(array.byteAt(2), 2);
}

TEST(ByteArrayBuiltinsTest, AsBytes) {
  Runtime runtime;
  Thread* thread = Thread::currentThread();
  HandleScope scope(thread);

  ByteArray array(&scope, runtime.newByteArray());
  Bytes bytes(&scope, byteArrayAsBytes(thread, &runtime, array));
  EXPECT_TRUE(isBytesEqualsBytes(bytes, {}));

  array.setBytes(runtime.newBytes(10, 0));
  array.setNumItems(3);
  bytes = byteArrayAsBytes(thread, &runtime, array);
  EXPECT_TRUE(isBytesEqualsBytes(bytes, {0, 0, 0}));
}

TEST(ByteArrayBuiltinsTest, DunderNewWithNonTypeRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray.__new__(3)"),
                            LayoutId::kTypeError, "not a type object"));
}

TEST(ByteArrayBuiltinsTest, DunderNewWithNonByteArrayRaisesTypeError) {
  Runtime runtime;
  EXPECT_TRUE(raisedWithStr(runFromCStr(&runtime, "bytearray.__new__(int)"),
                            LayoutId::kTypeError,
                            "not a subtype of bytearray"));
}

TEST(ByteArrayBuiltinsTest, DunderNewReturnsEmptyByteArray) {
  Runtime runtime;
  HandleScope scope;
  Type cls(&scope, runtime.typeAt(LayoutId::kByteArray));
  ByteArray self(&scope, runBuiltin(ByteArrayBuiltins::dunderNew, cls));
  EXPECT_EQ(self.numItems(), 0);
}

}  // namespace python
