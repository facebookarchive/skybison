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
  EXPECT_EQ(array.numBytes(), 3);
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
  EXPECT_EQ(bytes.length(), 0);

  array.setBytes(runtime.newBytes(10, 0));
  array.setNumBytes(3);
  bytes = byteArrayAsBytes(thread, &runtime, array);
  EXPECT_EQ(bytes.length(), 3);
  EXPECT_EQ(bytes.byteAt(0), 0);
  EXPECT_EQ(bytes.byteAt(1), 0);
  EXPECT_EQ(bytes.byteAt(2), 0);
}

}  // namespace python
