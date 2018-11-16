#include "gtest/gtest.h"

#include "marshal.h"
#include "runtime.h"
#include "thread.h"

namespace python {

class MyRuntimeTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    Runtime::initialize();
  }
};

TEST_F(MyRuntimeTest, RunEmptyFunction) {
  const char* buffer =
      "\x33\x0D\x0D\x0A\x3B\x5B\xB8\x59\x05\x00\x00\x00\xE3\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x40\x00\x00\x00\x73\x04\x00"
      "\x00\x00\x64\x00\x53\x00\x29\x01\x4E\xA9\x00\x72\x01\x00\x00\x00\x72\x01"
      "\x00\x00\x00\x72\x01\x00\x00\x00\xFA\x07\x70\x61\x73\x73\x2E\x70\x79\xDA"
      "\x08\x3C\x6D\x6F\x64\x75\x6C\x65\x3E\x01\x00\x00\x00\x73\x00\x00\x00\x00";
  Marshal::Reader reader(buffer);

  int32 magic = reader.readLong();
  EXPECT_EQ(magic, 0x0A0D0D33);
  int32 mtime = reader.readLong();
  EXPECT_EQ(mtime, 0x59B85B3B);
  int32 size = reader.readLong();
  EXPECT_EQ(size, 5);

  Object* code = reader.readObject();
  ASSERT_TRUE(code->isCode());
  EXPECT_EQ(Code::cast(code)->argcount(), 0);

  Thread thread(1 * KiB);
  Object* result = thread.run(code);
  ASSERT_EQ(result, None::object()); // returns None
}

TEST_F(MyRuntimeTest, RunHelloWorld) {
  const char* buffer =
      "\x33\x0D\x0D\x0A\x1B\x69\xC1\x59\x16\x00\x00\x00\xE3\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x02\x00\x00\x00\x40\x00\x00\x00\x73\x0C\x00"
      "\x00\x00\x65\x00\x64\x00\x83\x01\x01\x00\x64\x01\x53\x00\x29\x02\x7A\x0C"
      "\x68\x65\x6C\x6C\x6F\x2C\x20\x77\x6F\x72\x6C\x64\x4E\x29\x01\xDA\x05\x70"
      "\x72\x69\x6E\x74\xA9\x00\x72\x02\x00\x00\x00\x72\x02\x00\x00\x00\xFA\x0D"
      "\x68\x65\x6C\x6C\x6F\x77\x6F\x72\x6C\x64\x2E\x70\x79\xDA\x08\x3C\x6D\x6F"
      "\x64\x75\x6C\x65\x3E\x01\x00\x00\x00\x73\x00\x00\x00\x00";
  Marshal::Reader reader(buffer);

  int32 magic = reader.readLong();
  EXPECT_EQ(magic, 0x0A0D0D33);
  int32 mtime = reader.readLong();
  EXPECT_EQ(mtime, 0x59C1691B);
  int32 size = reader.readLong();
  EXPECT_EQ(size, 22);

  Object* code = reader.readObject();
  ASSERT_TRUE(code->isCode());
  EXPECT_EQ(Code::cast(code)->argcount(), 0);

  Thread thread(1 * KiB);
  Object* result = thread.run(code);
  ASSERT_EQ(result, None::object()); // returns None
}

} // namespace python
