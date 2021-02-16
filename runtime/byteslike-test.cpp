#include "byteslike.h"

#include "gtest/gtest.h"

#include "handles.h"
#include "objects.h"
#include "test-utils.h"
#include "view.h"

namespace py {
namespace testing {

using ByteslikeTest = RuntimeFixture;

TEST_F(ByteslikeTest, ConstructWithSmallBytes) {
  HandleScope scope(thread_);
  View<byte> bytes(reinterpret_cast<const byte*>("hello"), 5);
  Object value(&scope, SmallBytes::fromBytes(bytes));
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_TRUE(byteslike.isValid());
  EXPECT_EQ(byteslike.length(), 5);
  EXPECT_EQ(byteslike.byteAt(0), 'h');
  EXPECT_EQ(byteslike.byteAt(4), 'o');
  EXPECT_EQ(std::memcmp(reinterpret_cast<const byte*>(byteslike.address()),
                        "hello", 5),
            0);
}

TEST_F(ByteslikeTest, ConstructWithLargeBytes) {
  HandleScope scope(thread_);
  byte bytes[] = "hello foo bar\0baz\n";
  Object value(&scope, runtime_->newBytesWithAll(bytes));
  EXPECT_TRUE(value.isLargeBytes());
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_TRUE(byteslike.isValid());
  runtime_->collectGarbage();
  EXPECT_EQ(byteslike.length(), static_cast<word>(ARRAYSIZE(bytes)));
  EXPECT_EQ(byteslike.byteAt(0), 'h');
  EXPECT_EQ(byteslike.byteAt(4), 'o');
  EXPECT_EQ(byteslike.byteAt(13), '\0');
  EXPECT_EQ(byteslike.byteAt(15), 'a');
  EXPECT_EQ(byteslike.byteAt(17), '\n');
  EXPECT_EQ(byteslike.address(), LargeBytes::cast(*value).address());
}

TEST_F(ByteslikeTest, ConstructWithMutableBytes) {
  HandleScope scope(thread_);
  byte bytes[] = "hello foo bar\0baz\n";
  MutableBytes value(&scope,
                     runtime_->newMutableBytesUninitialized(ARRAYSIZE(bytes)));
  value.replaceFromWithAll(0, {bytes, ARRAYSIZE(bytes)});
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_TRUE(byteslike.isValid());
  runtime_->collectGarbage();
  EXPECT_EQ(byteslike.length(), static_cast<word>(ARRAYSIZE(bytes)));
  EXPECT_EQ(byteslike.byteAt(0), 'h');
  EXPECT_EQ(byteslike.byteAt(4), 'o');
  EXPECT_EQ(byteslike.byteAt(13), '\0');
  EXPECT_EQ(byteslike.byteAt(15), 'a');
  EXPECT_EQ(byteslike.byteAt(17), '\n');
  EXPECT_EQ(byteslike.address(), value.address());
}

TEST_F(ByteslikeTest, ConstructWithByteSubclassWithSmallBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(bytes): pass
value = C(b"hello")
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  EXPECT_TRUE(bytesUnderlying(*value).isSmallBytes());
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_TRUE(byteslike.isValid());
  EXPECT_EQ(byteslike.length(), 5);
  EXPECT_EQ(byteslike.byteAt(0), 'h');
  EXPECT_EQ(byteslike.byteAt(4), 'o');
  EXPECT_EQ(std::memcmp(reinterpret_cast<const byte*>(byteslike.address()),
                        "hello", 5),
            0);
}

TEST_F(ByteslikeTest, ConstructWithByteSubclassWithLargeBytes) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C(bytes): pass
value = C(b"hello foo bar\0baz\n")
)")
                   .isError());
  Object value(&scope, mainModuleAt(runtime_, "value"));
  EXPECT_TRUE(bytesUnderlying(*value).isLargeBytes());
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_TRUE(byteslike.isValid());
  runtime_->collectGarbage();
  EXPECT_EQ(byteslike.length(), 18);
  EXPECT_EQ(byteslike.byteAt(0), 'h');
  EXPECT_EQ(byteslike.byteAt(4), 'o');
  EXPECT_EQ(byteslike.byteAt(13), '\0');
  EXPECT_EQ(byteslike.byteAt(15), 'a');
  EXPECT_EQ(byteslike.byteAt(17), '\n');
  EXPECT_EQ(byteslike.address(),
            LargeBytes::cast(bytesUnderlying(*value)).address());
}

TEST_F(ByteslikeTest, ConstructWithBytearray) {
  HandleScope scope(thread_);
  Bytearray value(&scope, runtime_->newBytearray());
  runtime_->bytearrayEnsureCapacity(thread_, value, 32);
  byte bytes[] = "hello foo bar\0baz\n";
  runtime_->bytearrayExtend(thread_, value, bytes);
  // We want to test the case where numItems() != items.length
  EXPECT_NE(value.numItems(), MutableBytes::cast(value.items()).length());
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_TRUE(byteslike.isValid());
  runtime_->collectGarbage();
  EXPECT_EQ(byteslike.length(), static_cast<word>(ARRAYSIZE(bytes)));
  EXPECT_EQ(byteslike.byteAt(0), 'h');
  EXPECT_EQ(byteslike.byteAt(4), 'o');
  EXPECT_EQ(byteslike.byteAt(13), '\0');
  EXPECT_EQ(byteslike.byteAt(15), 'a');
  EXPECT_EQ(byteslike.byteAt(17), '\n');
  EXPECT_EQ(byteslike.address(), MutableBytes::cast(value.items()).address());
}

TEST_F(ByteslikeTest, ConstructWithArray) {
  HandleScope scope(thread_);
  ASSERT_FALSE(runFromCStr(runtime_, R"(
import array
value = array.array("Q")
value.append(0xcafebabe12345678)
)")
                   .isError());
  Array value(&scope, mainModuleAt(runtime_, "value"));
  ASSERT_EQ(value.length(), 1);
  // We want to test the case where the array buffer is bigger than the
  // contents. If this fails adjust test.
  ASSERT_TRUE(MutableBytes::cast(value.buffer()).length() > 8);

  static_assert(endian::native == endian::little,
                "test designed for little endian systems");
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_TRUE(byteslike.isValid());
  runtime_->collectGarbage();
  EXPECT_EQ(byteslike.length(), 8);
  EXPECT_EQ(byteslike.byteAt(0), 0x78);
  EXPECT_EQ(byteslike.byteAt(7), 0xca);
  EXPECT_EQ(byteslike.byteAt(6), 0xfe);
  EXPECT_EQ(byteslike.address(), MutableBytes::cast(value.buffer()).address());
}

TEST_F(ByteslikeTest, ConstructWithMemoryviewWithSmallBytes) {
  HandleScope scope(thread_);
  byte bytes[] = "hello!";
  Object bytes_obj(&scope, SmallBytes::fromBytes(bytes));
  MemoryView value(&scope,
                   runtime_->newMemoryView(thread_, bytes_obj, bytes_obj, 3,
                                           ReadOnly::ReadOnly));
  value.setStart(3);
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_TRUE(byteslike.isValid());
  EXPECT_EQ(byteslike.length(), 3);
  EXPECT_EQ(byteslike.byteAt(0), 'l');
  EXPECT_EQ(byteslike.byteAt(1), 'o');
  EXPECT_EQ(byteslike.byteAt(2), '!');
}

TEST_F(ByteslikeTest, ConstructWithMemoryviewWithLargeBytes) {
  HandleScope scope(thread_);
  byte bytes[] = "hello foo bar\0baz\n";
  LargeBytes bytes_obj(&scope, runtime_->newBytesWithAll(bytes));
  MemoryView value(&scope,
                   runtime_->newMemoryView(thread_, bytes_obj, bytes_obj, 11,
                                           ReadOnly::ReadOnly));
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_TRUE(byteslike.isValid());
  EXPECT_EQ(byteslike.length(), 11);
  EXPECT_EQ(byteslike.byteAt(0), 'h');
  EXPECT_EQ(byteslike.byteAt(4), 'o');
  EXPECT_EQ(byteslike.address(), bytes_obj.address());
}

TEST_F(ByteslikeTest, ConstructWithMemoryviewWithPointer) {
  HandleScope scope(thread_);
  byte bytes[] = "hello foo bar\0baz\n";
  Pointer pointer(&scope, runtime_->newPointer(bytes, sizeof(bytes)));
  MemoryView value(&scope, runtime_->newMemoryView(thread_, pointer, pointer,
                                                   15, ReadOnly::ReadOnly));
  word start = 1;
  value.setStart(start);
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_TRUE(byteslike.isValid());
  EXPECT_EQ(byteslike.length(), 15);
  EXPECT_EQ(byteslike.byteAt(0), 'e');
  EXPECT_EQ(byteslike.byteAt(5), 'f');
  EXPECT_EQ(byteslike.byteAt(12), '\0');
  EXPECT_EQ(byteslike.byteAt(14), 'a');
  EXPECT_EQ(byteslike.address(), bit_cast<uword>(&bytes) + start);
}

TEST_F(ByteslikeTest, IsValidWithNonByteslikeReturnsFalse) {
  HandleScope scope(thread_);
  Object value(&scope, runtime_->newFloat(42.42));
  Byteslike byteslike(&scope, thread_, *value);
  EXPECT_FALSE(byteslike.isValid());
}

}  // namespace testing
}  // namespace py
