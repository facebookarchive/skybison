#include "gtest/gtest.h"

#include "globals.h"
#include "marshal.h"
#include "runtime.h"

namespace python {

TEST(MarshalReaderTest, ReadString) {
  Runtime runtime;
  Marshal::Reader reader(&runtime, "hello, world");

  const byte* s1 = reader.readString(1);
  ASSERT_NE(s1, nullptr);
  EXPECT_EQ(*s1, 'h');

  const byte* s2 = reader.readString(2);
  ASSERT_NE(s2, nullptr);
  EXPECT_EQ(s2[0], 'e');
  EXPECT_EQ(s2[1], 'l');
}

TEST(MarshalReaderTest, ReadLong) {
  Runtime runtime;
  int32 a = Marshal::Reader(&runtime, "\x01\x00\x00\x00").readLong();
  EXPECT_EQ(a, 1);

  int32 b = Marshal::Reader(&runtime, "\x01\x02\x00\x00").readLong();
  ASSERT_EQ(b, 0x0201);

  int32 c = Marshal::Reader(&runtime, "\x01\x02\x03\x00").readLong();
  ASSERT_EQ(c, 0x030201);

  int32 d = Marshal::Reader(&runtime, "\x01\x02\x03\x04").readLong();
  ASSERT_EQ(d, 0x04030201);

  int32 e = Marshal::Reader(&runtime, "\x00\x00\x00\x80").readLong();
  ASSERT_EQ(e, -2147483648); // INT32_MIN
}

TEST(MarshalReaderTest, ReadShort) {
  Runtime runtime;
  int16 a = Marshal::Reader(&runtime, "\x01\x00").readShort();
  EXPECT_EQ(a, 1);

  int16 b = Marshal::Reader(&runtime, "\x01\x02").readShort();
  ASSERT_EQ(b, 0x0201);

  int16 c = Marshal::Reader(&runtime, "\x00\x80").readShort();
  ASSERT_EQ(c, -32768); // INT16_MIN
}

TEST(MarshalReaderTest, ReadObjectNull) {
  Runtime runtime;
  Object* a = Marshal::Reader(&runtime, "0").readObject();
  ASSERT_EQ(a, nullptr);
}

TEST(MarshalReaderTest, ReadObjectCode) {
  const char* buffer =
      "\x33\x0D\x0D\x0A\x3B\x5B\xB8\x59\x05\x00\x00\x00\xE3\x00\x00\x00\x00\x00"
      "\x00\x00\x00\x00\x00\x00\x00\x01\x00\x00\x00\x40\x00\x00\x00\x73\x04\x00"
      "\x00\x00\x64\x00\x53\x00\x29\x01\x4E\xA9\x00\x72\x01\x00\x00\x00\x72\x01"
      "\x00\x00\x00\x72\x01\x00\x00\x00\xFA\x07\x70\x61\x73\x73\x2E\x70\x79\xDA"
      "\x08\x3C\x6D\x6F\x64\x75\x6C\x65\x3E\x01\x00\x00\x00\x73\x00\x00\x00\x00";
  Runtime runtime;
  Marshal::Reader reader(&runtime, buffer);

  int32 magic = reader.readLong();
  EXPECT_EQ(magic, 0x0A0D0D33);
  int32 mtime = reader.readLong();
  EXPECT_EQ(mtime, 0x59B85B3B);
  int32 size = reader.readLong();
  EXPECT_EQ(size, 0x05);

  Object* code = reader.readObject();
  ASSERT_TRUE(code->isCode());
  EXPECT_EQ(Code::cast(code)->argcount(), 0);
}

TEST(MarshalTest, Good) {
  ASSERT_EQ(0, 0);
}

} // namespace python
