#include "heap-profiler.h"

#include "gtest/gtest.h"

#include "dict-builtins.h"
#include "object-builtins.h"
#include "test-utils.h"

namespace py {
namespace testing {

using HeapProfilerTest = RuntimeFixture;
using HeapProfilerDeathTest = RuntimeFixture;

TEST_F(HeapProfilerTest, ConstructorCreatesEmptyBuffer) {
  HeapProfiler::Buffer buffer;
  EXPECT_EQ(buffer.size(), 0);
  EXPECT_EQ(buffer.data(), nullptr);
}

TEST_F(HeapProfilerTest, WriteWithEmptyBufferAllocatesSpace) {
  HeapProfiler::Buffer buffer;
  byte buf[] = {0, 1, 2, 3};
  buffer.write(buf, 4);
  EXPECT_NE(buffer.data(), nullptr);
  EXPECT_EQ(buffer.size(), 4);
  EXPECT_EQ(std::memcmp(buffer.data(), buf, 4), 0);
}

static void testWriter(const void* data, word size, void* stream) {
  Vector<byte>* result = reinterpret_cast<Vector<byte>*>(stream);
  for (word i = 0; i < size; i++) {
    result->push_back(reinterpret_cast<const byte*>(data)[i]);
  }
}

static uint8_t read8(const Vector<byte>& src, word* pos) {
  EXPECT_LT(*pos, src.size());
  return src[(*pos)++];
}

static int16_t read16(const Vector<byte>& src, word* pos) {
  EXPECT_LT(*pos + 1, src.size());
  uint16_t result = src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  return result;
}

static int32_t read32(const Vector<byte>& src, word* pos) {
  EXPECT_LT(*pos + 3, src.size());
  int32_t result = src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  return result;
}

static uint32_t readu32(const Vector<byte>& src, word* pos) {
  return read32(src, pos);
}

static int64_t read64(const Vector<byte>& src, word* pos) {
  EXPECT_LT(*pos + 7, src.size());
  int64_t result = src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  result <<= 8;
  result |= src[(*pos)++];
  return result;
}

static uint64_t readu64(const Vector<byte>& data, word* pos) {
  return read64(data, pos);
}

TEST_F(HeapProfilerTest, WriteCallsWriteCallback) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  byte buf[] = {0, 1, 2, 3};
  profiler.write(buf, 4);
  word pos = 0;
  EXPECT_EQ(read8(result, &pos), 0);
  EXPECT_EQ(read8(result, &pos), 1);
  EXPECT_EQ(read8(result, &pos), 2);
  EXPECT_EQ(read8(result, &pos), 3);
  EXPECT_EQ(pos, result.size());
}

static const char* tagStr(byte tag) {
  switch (tag) {
    case HeapProfiler::Tag::kStringInUtf8:
      return "STRING IN UTF8";
    case HeapProfiler::Tag::kLoadClass:
      return "LOAD CLASS";
    case HeapProfiler::Tag::kStackTrace:
      return "STACK TRACE";
    case HeapProfiler::Tag::kHeapDumpSegment:
      return "HEAP DUMP SEGMENT";
    case HeapProfiler::Tag::kHeapDumpEnd:
      return "HEAP DUMP END";
    default:
      return "<UNKNOWN>";
  }
}

static ::testing::AssertionResult readTag(const Vector<byte>& result, word* pos,
                                          HeapProfiler::Tag expected) {
  EXPECT_LT(*pos, result.size());
  byte tag = result[(*pos)++];
  if (tag != expected) {
    return ::testing::AssertionFailure()
           << "expected " << tagStr(expected) << " but found " << tagStr(tag)
           << " (" << tag << ")";
  }
  return ::testing::AssertionSuccess();
}

static ::testing::AssertionResult readStringLiteral(const Vector<byte>& result,
                                                    word* pos,
                                                    const char* c_str) {
  for (word char_idx = 0; *c_str != '\0'; (*pos)++, char_idx++, c_str++) {
    if (*pos >= result.size()) {
      return ::testing::AssertionFailure()
             << "output (length " << result.size()
             << ") not long enough to read c_str '" << c_str << "'";
    }
    char c = result[*pos];
    if (c != *c_str) {
      return ::testing::AssertionFailure()
             << "char " << char_idx << " ('" << c
             << "') differs from expected ('" << *c_str << "')";
    }
  }
  return ::testing::AssertionSuccess();
}

static ::testing::AssertionResult readStringInUtf8(const Vector<byte>& result,
                                                   word* pos, uword address,
                                                   const char* value) {
  EXPECT_TRUE(readTag(result, pos, HeapProfiler::kStringInUtf8));
  EXPECT_EQ(read32(result, pos), 0);
  EXPECT_EQ(readu32(result, pos), std::strlen(value) + kPointerSize);
  EXPECT_EQ(readu64(result, pos), address);
  EXPECT_TRUE(readStringLiteral(result, pos, value));
  return ::testing::AssertionSuccess();
}

TEST_F(HeapProfilerTest, StringIdWritesStringInUTF8Once) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  RawStr str = Str::cast(SmallStr::fromCStr("foo"));
  EXPECT_EQ(profiler.stringId(str), str.raw());

  word pos = 0;
  EXPECT_TRUE(readStringInUtf8(result, &pos, str.raw(), "foo"));
  EXPECT_EQ(pos, result.size());

  // Size shouldn't change
  EXPECT_EQ(profiler.stringId(str), str.raw());
  EXPECT_EQ(pos, result.size());
}

static ::testing::AssertionResult readLoadClass(const Vector<byte>& result,
                                                word* pos, uword id,
                                                uword name_id) {
  EXPECT_TRUE(readTag(result, pos, HeapProfiler::kLoadClass));
  EXPECT_EQ(read32(result, pos), 0);         // time
  EXPECT_EQ(read32(result, pos), 24);        // data length
  EXPECT_EQ(read32(result, pos), 1);         // class serial number
  EXPECT_EQ(readu64(result, pos), id);       // class object ID
  EXPECT_EQ(read32(result, pos), 0);         // stack trace serial number
  EXPECT_EQ(readu64(result, pos), name_id);  // class name string ID
  return ::testing::AssertionSuccess();
}

TEST_F(HeapProfilerTest, ClassIdWritesLoadClassOnce) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  RawLayout layout = Layout::cast(runtime_->layoutAt(LayoutId::kTuple));
  EXPECT_EQ(profiler.classId(layout), layout.raw());

  word pos = 0;
  uword tuple_address = runtime_->symbols()->at(ID(tuple)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, tuple_address, "tuple"));
  EXPECT_TRUE(readLoadClass(result, &pos, layout.raw(), tuple_address));
  EXPECT_EQ(pos, result.size());

  // Size shouldn't change
  EXPECT_EQ(profiler.classId(layout), layout.raw());
  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteStringInUTF8WithLargeStrWritesStringRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  HandleScope scope(thread_);
  Str str(&scope, runtime_->newStrFromCStr("deadbeef"));
  profiler.writeStringInUTF8(*str);
  word pos = 0;
  EXPECT_TRUE(readStringInUtf8(result, &pos, str.raw(), "deadbeef"));

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteCStringInUTF8WritesStringRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  const char* str = "deadbeef";
  profiler.writeCStringInUTF8(str);
  word pos = 0;
  EXPECT_TRUE(
      readStringInUtf8(result, &pos, reinterpret_cast<uword>(str), str));

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteEmptyRecordWritesRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), &profiler);
  }
  word pos = 0;
  EXPECT_EQ(read8(result, &pos), 0xa);  // tag
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 0);   // length

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteFakeStackTraceWritesStackTrace) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  profiler.writeFakeStackTrace();
  word pos = 0;
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::kStackTrace));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 12);  // data length
  EXPECT_EQ(read32(result, &pos), 0);   // stack trace serial number
  EXPECT_EQ(read32(result, &pos), 0);   // thread serial number
  EXPECT_EQ(read32(result, &pos), 0);   // number of frames

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteLoadClassWritesLoadClassRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  RawLayout layout = Layout::cast(runtime_->layoutAt(LayoutId::kTuple));
  profiler.writeLoadClass(layout);
  word pos = 0;
  uword tuple_address = runtime_->symbols()->at(ID(tuple)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, tuple_address, "tuple"));
  EXPECT_TRUE(readLoadClass(result, &pos, layout.raw(), tuple_address));

  EXPECT_EQ(pos, result.size());
}

static const char* subtagStr(byte subtag) {
  switch (subtag) {
    case HeapProfiler::Subtag::kRootJniGlobal:
      return "ROOT JNI GLOBAL";
    case HeapProfiler::Subtag::kRootJniLocal:
      return "ROOT JNI LOCAL";
    case HeapProfiler::Subtag::kRootJavaFrame:
      return "ROOT JAVA FRAME";
    case HeapProfiler::Subtag::kRootNativeStack:
      return "ROOT NATIVE STACK";
    case HeapProfiler::Subtag::kRootStickyClass:
      return "ROOT STICKY CLASS";
    case HeapProfiler::Subtag::kRootThreadBlock:
      return "ROOT THREAD BLOCK";
    case HeapProfiler::Subtag::kRootMonitorUsed:
      return "ROOT MONITOR USED";
    case HeapProfiler::Subtag::kRootThreadObject:
      return "ROOT THREAD OBJECT";
    case HeapProfiler::Subtag::kRootUnknown:
      return "ROOT UNKNOWN";
    case HeapProfiler::Subtag::kClassDump:
      return "CLASS DUMP";
    case HeapProfiler::Subtag::kInstanceDump:
      return "INSTANCE DUMP";
    case HeapProfiler::Subtag::kObjectArrayDump:
      return "OBJECT ARRAY DUMP";
    case HeapProfiler::Subtag::kPrimitiveArrayDump:
      return "PRIMITIVE ARRAY DUMP";
    default:
      return "<UNKNOWN>";
  }
}

static ::testing::AssertionResult readSubtag(const Vector<byte>& result,
                                             word* pos,
                                             HeapProfiler::Subtag expected) {
  EXPECT_LT(*pos, result.size());
  byte tag = result[(*pos)++];
  if (tag != expected) {
    return ::testing::AssertionFailure()
           << "expected " << subtagStr(expected) << " but found "
           << subtagStr(tag) << " (" << tag << ")";
  }
  return ::testing::AssertionSuccess();
}

static ::testing::AssertionResult readClassDumpPrelude(
    const Vector<byte>& result, word* pos, uword layout, uword super_layout) {
  EXPECT_EQ(readu64(result, pos), layout);  // class object ID
  EXPECT_EQ(read32(result, pos), 0);        // stack trace serial number
  EXPECT_EQ(readu64(result, pos),
            super_layout);            // super class object ID
  EXPECT_EQ(read64(result, pos), 0);  // class loader object ID
  EXPECT_EQ(read64(result, pos), 0);  // signers object ID
  EXPECT_EQ(read64(result, pos), 0);  // protection domain object ID
  EXPECT_EQ(read64(result, pos), 0);  // reserved
  EXPECT_EQ(read64(result, pos), 0);  // reserved
  return ::testing::AssertionSuccess();
}

TEST_F(HeapProfilerTest, WriteClassDumpWritesClassDumpSubRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  RawLayout tuple_layout = Layout::cast(runtime_->layoutAt(LayoutId::kTuple));
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeClassDump(tuple_layout);
    profiler.clearRecord();
  }
  word pos = 0;
  // tuple
  uword tuple_address = runtime_->symbols()->at(ID(tuple)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, tuple_address, "tuple"));
  EXPECT_TRUE(readLoadClass(result, &pos, tuple_layout.raw(), tuple_address));

  // object
  uword object_address = runtime_->symbols()->at(ID(object)).raw();
  RawLayout object_layout = Layout::cast(runtime_->layoutAt(LayoutId::kObject));
  EXPECT_TRUE(readStringInUtf8(result, &pos, object_address, "object"));
  EXPECT_TRUE(readLoadClass(result, &pos, object_layout.raw(), object_address));

  // _UserTuple__value
  uword user_tuple_value_address =
      runtime_->symbols()->at(ID(_UserTuple__value)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, user_tuple_value_address,
                               "_UserTuple__value"));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 80);  // length

  // Class dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kClassDump));
  EXPECT_TRUE(readClassDumpPrelude(result, &pos, tuple_layout.raw(),
                                   object_layout.raw()));
  EXPECT_EQ(read32(result, &pos),
            tuple_layout.instanceSize());  // instance size in bytes
  EXPECT_EQ(read16(result, &pos),
            0);  // size of constant pool and number of records that follow
  EXPECT_EQ(read16(result, &pos), 0);  // number of static fields
  EXPECT_EQ(read16(result, &pos), 1);  // number of instance fields

  // TODO(T61661597): Remove _UserTuple__value field from tuple layout
  // Field 0 (u8 name, u1 type)
  EXPECT_EQ(readu64(result, &pos), user_tuple_value_address);
  EXPECT_EQ(read8(result, &pos), HeapProfiler::BasicType::kObject);

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteClassDumpForUserClassWritesClassDumpRecord) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.a = 1
    self.b = 2
instance = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout c_layout(&scope, runtime_->layoutAt(instance.layoutId()));

  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeClassDump(*c_layout);
    profiler.clearRecord();
  }
  word pos = 0;

  // C
  word c_address = SmallStr::fromCStr("C").raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, c_address, "C"));
  EXPECT_TRUE(readLoadClass(result, &pos, c_layout.raw(), c_address));

  // object
  uword object_address = runtime_->symbols()->at(ID(object)).raw();
  RawLayout object_layout = Layout::cast(runtime_->layoutAt(LayoutId::kObject));
  EXPECT_TRUE(readStringInUtf8(result, &pos, object_address, "object"));
  EXPECT_TRUE(readLoadClass(result, &pos, object_layout.raw(), object_address));

  // a
  word a_address = SmallStr::fromCStr("a").raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, a_address, "a"));

  // b
  word b_address = SmallStr::fromCStr("b").raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, b_address, "b"));

  // <OVERFLOW>
  EXPECT_TRUE(readStringInUtf8(result, &pos,
                               reinterpret_cast<uword>(HeapProfiler::kOverflow),
                               HeapProfiler::kOverflow));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 98);  // length

  // Class dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kClassDump));
  EXPECT_TRUE(
      readClassDumpPrelude(result, &pos, c_layout.raw(), object_layout.raw()));
  EXPECT_EQ(read32(result, &pos),
            c_layout.instanceSize());  // instance size in bytes
  EXPECT_EQ(read16(result, &pos),
            0);  // size of constant pool and number of records that follow
  EXPECT_EQ(read16(result, &pos), 0);  // number of static fields
  EXPECT_EQ(read16(result, &pos), 3);  // number of instance fields
  // * Field 0 (u8 name, u1 type)
  EXPECT_EQ(read64(result, &pos), a_address);
  EXPECT_EQ(read8(result, &pos), HeapProfiler::BasicType::kObject);
  // * Field 1 (u8 name, u1 type)
  EXPECT_EQ(read64(result, &pos), b_address);
  EXPECT_EQ(read8(result, &pos), HeapProfiler::BasicType::kObject);
  // * Field 2 (u8 name, u1 type)
  EXPECT_EQ(readu64(result, &pos),
            reinterpret_cast<uword>(HeapProfiler::kOverflow));
  EXPECT_EQ(read8(result, &pos), HeapProfiler::BasicType::kObject);

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteClassDumpWithOverflowAttributes) {
  HandleScope scope(thread_);
  Layout empty(&scope, testing::layoutCreateEmpty(thread_));

  // Should fail to find an attribute that isn't present
  Object attr(&scope, Runtime::internStrFromCStr(thread_, "a"));
  AttributeInfo info;
  ASSERT_FALSE(Runtime::layoutFindAttribute(*empty, attr, &info));

  // Adding a new attribute should result in a new layout being created
  AttributeInfo info2;
  Layout layout(&scope,
                runtime_->layoutAddAttribute(thread_, empty, attr, 0, &info2));
  ASSERT_NE(*empty, *layout);
  EXPECT_TRUE(info2.isOverflow());
  EXPECT_EQ(info2.offset(), 0);

  Type type(&scope, runtime_->newType());
  type.setName(SmallStr::fromCStr("C"));
  type.setInstanceLayout(*layout);
  layout.setDescribedType(*type);

  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeClassDump(*layout);
    profiler.clearRecord();
  }
  word pos = 0;

  // C
  word c_address = SmallStr::fromCStr("C").raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, c_address, "C"));
  EXPECT_TRUE(readLoadClass(result, &pos, layout.raw(), c_address));

  // object
  uword object_address = runtime_->symbols()->at(ID(object)).raw();
  RawLayout object_layout = Layout::cast(runtime_->layoutAt(LayoutId::kObject));
  EXPECT_TRUE(readStringInUtf8(result, &pos, object_address, "object"));
  EXPECT_TRUE(readLoadClass(result, &pos, object_layout.raw(), object_address));

  // <OVERFLOW>
  EXPECT_TRUE(readStringInUtf8(result, &pos,
                               reinterpret_cast<uword>(HeapProfiler::kOverflow),
                               HeapProfiler::kOverflow));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 80);  // length

  // Class dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::kClassDump));
  EXPECT_TRUE(
      readClassDumpPrelude(result, &pos, layout.raw(), object_layout.raw()));
  EXPECT_EQ(read32(result, &pos),
            layout.instanceSize());  // instance size in bytes
  EXPECT_EQ(read16(result, &pos),
            0);  // size of constant pool and number of records that follow
  EXPECT_EQ(read16(result, &pos), 0);  // number of static fields
  EXPECT_EQ(read16(result, &pos), 1);  // number of instance fields
  // * Field 0 (u8 name, u1 type)
  EXPECT_EQ(readu64(result, &pos),
            reinterpret_cast<uword>(HeapProfiler::kOverflow));
  EXPECT_EQ(read8(result, &pos), HeapProfiler::BasicType::kObject);

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteClassDumpWithDictOverflow) {
  HandleScope scope(thread_);
  // Make a new type, C
  Layout layout(&scope, testing::layoutCreateEmpty(thread_));
  layout.setDictOverflowOffset(10);
  EXPECT_TRUE(layout.hasDictOverflow());
  Type type(&scope, runtime_->newType());
  type.setName(SmallStr::fromCStr("C"));
  type.setInstanceLayout(*layout);
  layout.setDescribedType(*type);

  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeClassDump(*layout);
    profiler.clearRecord();
  }
  word pos = 0;

  // C
  word c_address = SmallStr::fromCStr("C").raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, c_address, "C"));
  EXPECT_TRUE(readLoadClass(result, &pos, layout.raw(), c_address));

  // object
  uword object_address = runtime_->symbols()->at(ID(object)).raw();
  RawLayout object_layout = Layout::cast(runtime_->layoutAt(LayoutId::kObject));
  EXPECT_TRUE(readStringInUtf8(result, &pos, object_address, "object"));
  EXPECT_TRUE(readLoadClass(result, &pos, object_layout.raw(), object_address));
  EXPECT_TRUE(readStringInUtf8(result, &pos,
                               reinterpret_cast<uword>(HeapProfiler::kOverflow),
                               HeapProfiler::kOverflow));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 80);  // length

  // Class dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::kClassDump));
  EXPECT_TRUE(
      readClassDumpPrelude(result, &pos, layout.raw(), object_layout.raw()));
  EXPECT_EQ(read32(result, &pos),
            layout.instanceSize());  // instance size in bytes
  EXPECT_EQ(read16(result, &pos),
            0);  // size of constant pool and number of records that follow
  EXPECT_EQ(read16(result, &pos), 0);  // number of static fields
  EXPECT_EQ(read16(result, &pos), 1);  // number of instance fields

  EXPECT_EQ(readu64(result, &pos),
            reinterpret_cast<uword>(HeapProfiler::kOverflow));
  EXPECT_EQ(read8(result, &pos), HeapProfiler::BasicType::kObject);

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteInstanceWithDictOverflow) {
  HandleScope scope(thread_);
  // Make a new type, C
  Layout layout(&scope, testing::layoutCreateEmpty(thread_));
  layout.setDictOverflowOffset(10);
  EXPECT_TRUE(layout.hasDictOverflow());
  Type type(&scope, runtime_->newType());
  type.setName(SmallStr::fromCStr("C"));
  type.setInstanceLayout(*layout);
  layout.setDescribedType(*type);

  // Make an instance with an overflow attribute
  Instance instance(&scope, runtime_->newInstance(layout));

  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeInstanceDump(*instance);
    profiler.clearRecord();
  }
  word pos = 0;

  // C
  word c_address = SmallStr::fromCStr("C").raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, c_address, "C"));
  EXPECT_TRUE(readLoadClass(result, &pos, layout.raw(), c_address));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 33);  // length

  // Instance dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kInstanceDump));
  EXPECT_EQ(readu64(result, &pos), instance.raw());  // object ID
  EXPECT_EQ(read32(result, &pos), 0);              // stack trace serial number
  EXPECT_EQ(readu64(result, &pos), layout.raw());  // class object ID
  EXPECT_EQ(read32(result, &pos), kPointerSize);  // number of bytes that follow
  EXPECT_EQ(readu64(result, &pos), NoneType::object().raw());  // padding

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteInstanceWithOverflowAttributes) {
  HandleScope scope(thread_);
  // Make a new type, C
  Layout empty(&scope, testing::layoutCreateEmpty(thread_));
  Type type(&scope, runtime_->newType());
  type.setName(SmallStr::fromCStr("C"));
  type.setInstanceLayout(*empty);
  empty.setDescribedType(*type);

  // Make an instance with an overflow attribute
  Instance instance(&scope, runtime_->newInstance(empty));
  Object name(&scope, Runtime::internStrFromCStr(thread_, "a"));
  Object value(&scope, SmallInt::fromWord(1234));
  EXPECT_TRUE(instanceSetAttr(thread_, instance, name, value).isNoneType());
  Layout layout(&scope, runtime_->layoutOf(*instance));
  EXPECT_EQ(layout.inObjectAttributes(), runtime_->emptyTuple());
  EXPECT_TRUE(layout.hasTupleOverflow());

  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeInstanceDump(*instance);
    profiler.clearRecord();
  }
  word pos = 0;

  // C
  word c_address = SmallStr::fromCStr("C").raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, c_address, "C"));
  EXPECT_TRUE(readLoadClass(result, &pos, layout.raw(), c_address));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 33);  // length

  // Instance dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kInstanceDump));
  EXPECT_EQ(readu64(result, &pos), instance.raw());  // object ID
  EXPECT_EQ(read32(result, &pos), 0);              // stack trace serial number
  EXPECT_EQ(readu64(result, &pos), layout.raw());  // class object ID
  EXPECT_EQ(read32(result, &pos),
            kPointerSize);  // number of bytes that follow
  uword overflow_raw = read64(result, &pos);
  Tuple overflow(&scope, RawObject{overflow_raw});
  EXPECT_EQ(overflow.at(0), *value);

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest,
       WriteClassDumpForFloatWritesClassDumpRecordWithOneAttribute) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  RawLayout float_layout = Layout::cast(runtime_->layoutAt(LayoutId::kFloat));
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeClassDump(float_layout);
    profiler.clearRecord();
  }
  word pos = 0;
  // float
  uword float_address = runtime_->symbols()->at(ID(float)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, float_address, "float"));
  EXPECT_TRUE(readLoadClass(result, &pos, float_layout.raw(), float_address));

  // object
  uword object_address = runtime_->symbols()->at(ID(object)).raw();
  RawLayout object_layout = Layout::cast(runtime_->layoutAt(LayoutId::kObject));
  EXPECT_TRUE(readStringInUtf8(result, &pos, object_address, "object"));
  EXPECT_TRUE(readLoadClass(result, &pos, object_layout.raw(), object_address));

  // value
  uword value_address = runtime_->symbols()->at(ID(value)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, value_address, "value"));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 80);  // length

  // Class dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kClassDump));
  EXPECT_TRUE(readClassDumpPrelude(result, &pos, float_layout.raw(),
                                   object_layout.raw()));
  EXPECT_EQ(read32(result, &pos),
            float_layout.instanceSize());  // instance size in bytes
  EXPECT_EQ(read16(result, &pos),
            0);  // size of constant pool and number of records that follow
  EXPECT_EQ(read16(result, &pos), 0);  // number of static fields
  EXPECT_EQ(read16(result, &pos), 1);  // number of instance fields

  // Field 0 (u8 name, u1 type)
  EXPECT_EQ(readu64(result, &pos), value_address);
  EXPECT_EQ(read8(result, &pos), HeapProfiler::BasicType::kDouble);

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest,
       WriteInstanceDumpForUserClassWritesInstanceDumpRecord) {
  ASSERT_FALSE(runFromCStr(runtime_, R"(
class C:
  def __init__(self):
    self.a = 1
    self.b = 2
instance = C()
)")
                   .isError());
  HandleScope scope(thread_);
  Object instance(&scope, mainModuleAt(runtime_, "instance"));
  Layout c_layout(&scope, runtime_->layoutAt(instance.layoutId()));

  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeInstanceDump(Instance::cast(*instance));
    profiler.clearRecord();
  }
  word pos = 0;

  // C
  word c_address = SmallStr::fromCStr("C").raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, c_address, "C"));
  EXPECT_TRUE(readLoadClass(result, &pos, c_layout.raw(), c_address));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 49);  // length

  // Instance dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kInstanceDump));
  EXPECT_EQ(readu64(result, &pos), instance.raw());  // object ID
  EXPECT_EQ(read32(result, &pos), 0);  // stack trace serial number
  EXPECT_EQ(readu64(result, &pos), c_layout.raw());  // class object ID
  EXPECT_EQ(read32(result, &pos),
            3 * kPointerSize);  // number of bytes that follow
  EXPECT_EQ(readu64(result, &pos), SmallInt::fromWord(1).raw());
  EXPECT_EQ(readu64(result, &pos), SmallInt::fromWord(2).raw());
  EXPECT_EQ(readu64(result, &pos), runtime_->emptyTuple().raw());  // overflow

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteInstanceDumpForDictWritesInstanceDumpRecord) {
  HandleScope scope(thread_);
  Dict dict(&scope, runtime_->newDict());
  Object key(&scope, SmallStr::fromCStr("foo"));
  Object value(&scope, SmallStr::fromCStr("bar"));
  word hash = Int::cast(Interpreter::hash(thread_, key)).asWord();
  ASSERT_TRUE(dictAtPut(thread_, dict, key, hash, value).isNoneType());
  Layout dict_layout(&scope, runtime_->layoutAt(dict.layoutId()));

  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeInstanceDump(*dict);
    profiler.clearRecord();
  }
  word pos = 0;

  // dict
  uword dict_address = runtime_->symbols()->at(ID(dict)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, dict_address, "dict"));
  EXPECT_TRUE(readLoadClass(result, &pos, dict_layout.raw(), dict_address));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 57);  // length

  // Instance dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kInstanceDump));
  EXPECT_EQ(readu64(result, &pos), dict.raw());  // object ID
  EXPECT_EQ(read32(result, &pos), 0);            // stack trace serial number
  EXPECT_EQ(readu64(result, &pos), dict_layout.raw());  // class object ID
  EXPECT_EQ(read32(result, &pos),
            4 * kPointerSize);  // number of bytes that follow
  EXPECT_EQ(readu64(result, &pos), SmallInt::fromWord(1).raw());  // num items
  EXPECT_EQ(readu64(result, &pos), dict.data().raw());            // data
  EXPECT_EQ(readu64(result, &pos), dict.indices().raw());         // sparse
  // first empty item index
  EXPECT_EQ(readu64(result, &pos),
            SmallInt::fromWord(dict.firstEmptyItemIndex()).raw());
  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteInstanceDumpForFloatWritesInstanceDumpRecord) {
  HandleScope scope(thread_);
  Float obj(&scope, runtime_->newFloat(1.5));
  Layout float_layout(&scope, runtime_->layoutAt(obj.layoutId()));

  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeFloat(*obj);
    profiler.clearRecord();
  }
  word pos = 0;

  // float
  uword float_address = runtime_->symbols()->at(ID(float)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, float_address, "float"));
  EXPECT_TRUE(readLoadClass(result, &pos, float_layout.raw(), float_address));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 33);  // length

  // Instance dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kInstanceDump));
  EXPECT_EQ(readu64(result, &pos), obj.raw());  // object ID
  EXPECT_EQ(read32(result, &pos), 0);           // stack trace serial number
  EXPECT_EQ(readu64(result, &pos), float_layout.raw());  // class object ID
  EXPECT_EQ(read32(result, &pos),
            1 * kPointerSize);  // number of bytes that follow
  uint64_t value = read64(result, &pos);
  EXPECT_EQ(bit_cast<double>(value), obj.value());  // value

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteEllipsisWritesInstanceDumpRecord) {
  HandleScope scope(thread_);
  Ellipsis instance(&scope, runtime_->ellipsis());
  Layout ellipsis_layout(&scope, runtime_->layoutAt(instance.layoutId()));

  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeEllipsis(*instance);
    profiler.clearRecord();
  }
  word pos = 0;

  // ellipsis
  uword ellipsis_address = runtime_->symbols()->at(ID(ellipsis)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, ellipsis_address, "ellipsis"));
  EXPECT_TRUE(
      readLoadClass(result, &pos, ellipsis_layout.raw(), ellipsis_address));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 33);  // length

  // Instance dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kInstanceDump));
  EXPECT_EQ(readu64(result, &pos), instance.raw());  // object ID
  EXPECT_EQ(read32(result, &pos), 0);  // stack trace serial number
  EXPECT_EQ(readu64(result, &pos), ellipsis_layout.raw());  // class object ID
  EXPECT_EQ(read32(result, &pos), kPointerSize);  // number of bytes that follow
  EXPECT_EQ(readu64(result, &pos), Unbound::object().raw());  // padding

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteImmediateWritesInstanceDump) {
  Vector<byte> result;
  RawObject obj = SmallInt::fromWord(1337);
  RawLayout smallint_layout = Layout::cast(runtime_->layoutOf(obj));
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeImmediate(obj);
    profiler.clearRecord();
  }
  word pos = 0;

  // smallint
  uword smallint_address = runtime_->symbols()->at(ID(smallint)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, smallint_address, "smallint"));
  EXPECT_TRUE(
      readLoadClass(result, &pos, smallint_layout.raw(), smallint_address));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 25);  // length

  // Instance dump for 1337
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kInstanceDump));
  EXPECT_EQ(readu64(result, &pos), obj.raw());  // object ID
  EXPECT_EQ(read32(result, &pos), 0);           // stack trace serial number
  EXPECT_EQ(readu64(result, &pos), smallint_layout.raw());  // class object ID
  EXPECT_EQ(read32(result, &pos), 0);  // number of bytes to follow

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteFakeClassDumpWritesClassDump) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeFakeClassDump(HeapProfiler::FakeClass::kJavaLangClass,
                                HeapProfiler::kJavaLangClass,
                                HeapProfiler::FakeClass::kJavaLangObject);
    profiler.clearRecord();
  }
  word pos = 0;

  // java.lang.Class
  uword java_lang_class_id =
      reinterpret_cast<uword>(HeapProfiler::kJavaLangClass);
  EXPECT_TRUE(
      readStringInUtf8(result, &pos, java_lang_class_id, "java.lang.Class"));
  EXPECT_TRUE(readLoadClass(
      result, &pos, static_cast<uword>(HeapProfiler::FakeClass::kJavaLangClass),
      java_lang_class_id));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 71);  // length

  // Class dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kClassDump));
  EXPECT_TRUE(readClassDumpPrelude(
      result, &pos, static_cast<uword>(HeapProfiler::FakeClass::kJavaLangClass),
      static_cast<uword>(HeapProfiler::FakeClass::kJavaLangObject)));
  EXPECT_EQ(read32(result, &pos),
            0);  // instance size in bytes
  EXPECT_EQ(read16(result, &pos),
            0);  // size of constant pool and number of records that follow
  EXPECT_EQ(read16(result, &pos), 0);  // number of static fields
  EXPECT_EQ(read16(result, &pos), 0);  // number of instance fields

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteFakeLoadClassWritesLoadClass) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  profiler.writeFakeLoadClass(HeapProfiler::FakeClass::kJavaLangClass,
                              HeapProfiler::kJavaLangClass);
  word pos = 0;

  // java.lang.Class
  uword java_lang_class_id =
      reinterpret_cast<uword>(HeapProfiler::kJavaLangClass);
  EXPECT_TRUE(
      readStringInUtf8(result, &pos, java_lang_class_id, "java.lang.Class"));

  // Then write the LoadClass record
  EXPECT_TRUE(readLoadClass(
      result, &pos, static_cast<uword>(HeapProfiler::FakeClass::kJavaLangClass),
      java_lang_class_id));

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteHeaderWritesHeader) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  profiler.writeHeader();
  word pos = 0;
  EXPECT_TRUE(readStringLiteral(result, &pos, "JAVA PROFILE 1.0.2"));
  EXPECT_EQ(read8(result, &pos), 0);   // nul byte
  EXPECT_EQ(read32(result, &pos), 8);  // ID length in bytes
  read32(result, &pos);  // high value of current time in milliseconds
  read32(result, &pos);  // low value of current time in milliseconds

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, BeginAndEndHeapDumpSegmentWritesHeapDumpSegment) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    EXPECT_EQ(result.size(), 0);
    profiler.clearRecord();
  }
  word pos = 0;

  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);  // time
  EXPECT_EQ(read32(result, &pos), 0);  // length

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, RecordDestructorWritesRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  {
    HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), &profiler);
    record.write32(0x12345678);
  }
  word pos = 0;

  EXPECT_EQ(read8(result, &pos), 0xa);
  EXPECT_EQ(read32(result, &pos), 0);  // time
  EXPECT_EQ(read32(result, &pos), 4);  // length
  EXPECT_EQ(read32(result, &pos), 0x12345678);

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteHeapDumpEndWritesRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  profiler.writeHeapDumpEnd();
  word pos = 0;

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpEnd));
  EXPECT_EQ(read32(result, &pos), 0);  // time
  EXPECT_EQ(read32(result, &pos), 0);  // length

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteObjectArrayWritesObjectArrayRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  HandleScope scope(thread_);
  Object obj1(&scope, SmallInt::fromWord(0));
  Object obj2(&scope, SmallInt::fromWord(1));
  Object obj3(&scope, SmallInt::fromWord(2));
  Tuple tuple(&scope, runtime_->newTupleWith3(obj1, obj2, obj3));
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeObjectArray(*tuple);
    profiler.clearRecord();
  }
  word pos = 0;

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 49);  // length

  // Object array
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kObjectArrayDump));
  EXPECT_EQ(readu64(result, &pos), tuple.raw());  // array object ID
  EXPECT_EQ(read32(result, &pos), 0);             // stack trace serial number
  EXPECT_EQ(read32(result, &pos), 3);             // number of elements
  EXPECT_EQ(readu64(result, &pos),
            static_cast<uword>(
                HeapProfiler::FakeClass::kObjectArray));  // array class ID
  EXPECT_EQ(readu64(result, &pos), SmallInt::fromWord(0).raw());  // element 0
  EXPECT_EQ(readu64(result, &pos), SmallInt::fromWord(1).raw());  // element 1
  EXPECT_EQ(readu64(result, &pos), SmallInt::fromWord(2).raw());  // element 2

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteBytesWritesPrimitiveArrayRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  HandleScope scope(thread_);
  byte source[] = "hello";
  Bytes bytes(&scope, runtime_->newBytesWithAll(source));
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeBytes(*bytes);
    profiler.clearRecord();
  }
  word pos = 0;

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 24);  // length

  // Byte array
  EXPECT_TRUE(
      readSubtag(result, &pos, HeapProfiler::Subtag::kPrimitiveArrayDump));
  EXPECT_EQ(readu64(result, &pos), bytes.raw());  // array object ID
  EXPECT_EQ(read32(result, &pos), 0);             // stack trace serial number
  EXPECT_EQ(readu32(result, &pos), sizeof(source));  // number of elements
  EXPECT_EQ(read8(result, &pos),
            HeapProfiler::BasicType::kByte);  // element type
  EXPECT_EQ(read8(result, &pos), 'h');        // element 0
  EXPECT_EQ(read8(result, &pos), 'e');        // element 1
  EXPECT_EQ(read8(result, &pos), 'l');        // element 2
  EXPECT_EQ(read8(result, &pos), 'l');        // element 3
  EXPECT_EQ(read8(result, &pos), 'o');        // element 4
  EXPECT_EQ(read8(result, &pos), '\0');       // element 5

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest,
       WriteClassDumpForComplexWritesClassDumpRecordWithTwoAttributes) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  RawLayout complex_layout =
      Layout::cast(runtime_->layoutAt(LayoutId::kComplex));
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeClassDump(complex_layout);
    profiler.clearRecord();
  }
  word pos = 0;

  // complex
  uword complex_address = runtime_->symbols()->at(ID(complex)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, complex_address, "complex"));
  EXPECT_TRUE(
      readLoadClass(result, &pos, complex_layout.raw(), complex_address));

  // object
  uword object_address = runtime_->symbols()->at(ID(object)).raw();
  RawLayout object_layout = Layout::cast(runtime_->layoutAt(LayoutId::kObject));
  EXPECT_TRUE(readStringInUtf8(result, &pos, object_address, "object"));
  EXPECT_TRUE(readLoadClass(result, &pos, object_layout.raw(), object_address));

  // real
  uword real_address = runtime_->symbols()->at(ID(real)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, real_address, "real"));

  // imag
  uword imag_address = runtime_->symbols()->at(ID(imag)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, imag_address, "imag"));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 89);  // length

  // Class dump subrecord
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kClassDump));
  EXPECT_TRUE(readClassDumpPrelude(result, &pos, complex_layout.raw(),
                                   object_layout.raw()));
  EXPECT_EQ(read32(result, &pos),
            complex_layout.instanceSize());  // instance size in bytes
  EXPECT_EQ(read16(result, &pos),
            0);  // size of constant pool and number of records that follow
  EXPECT_EQ(read16(result, &pos), 0);  // number of static fields
  EXPECT_EQ(read16(result, &pos), 2);  // number of instance fields

  // Field 0 (u8 name, u1 type)
  EXPECT_EQ(readu64(result, &pos), real_address);
  EXPECT_EQ(read8(result, &pos), HeapProfiler::BasicType::kDouble);

  // Field 1 (u8 name, u1 type)
  EXPECT_EQ(readu64(result, &pos), imag_address);
  EXPECT_EQ(read8(result, &pos), HeapProfiler::BasicType::kDouble);

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteComplexWritesInstanceDump) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  HandleScope scope(thread_);
  Complex obj(&scope, runtime_->newComplex(1.0, 2.0));
  Layout complex_layout(&scope, runtime_->layoutOf(*obj));
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeComplex(*obj);
    profiler.clearRecord();
  }
  word pos = 0;

  // complex
  uword complex_address = runtime_->symbols()->at(ID(complex)).raw();
  EXPECT_TRUE(readStringInUtf8(result, &pos, complex_address, "complex"));
  EXPECT_TRUE(
      readLoadClass(result, &pos, complex_layout.raw(), complex_address));

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 41);  // length

  // Complex "instance" dump
  EXPECT_TRUE(readSubtag(result, &pos, HeapProfiler::Subtag::kInstanceDump));
  EXPECT_EQ(readu64(result, &pos), obj.raw());  // object ID
  EXPECT_EQ(read32(result, &pos), 0);           // stack trace serial number
  EXPECT_EQ(readu64(result, &pos), complex_layout.raw());  // class object ID
  EXPECT_EQ(read32(result, &pos),
            2 * kDoubleSize);             // number of bytes that follow
  uint64_t real = readu64(result, &pos);  // real
  EXPECT_EQ(bit_cast<double>(real), 1.0);
  uint64_t imag = readu64(result, &pos);  // imag
  EXPECT_EQ(bit_cast<double>(imag), 2.0);

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteLargeIntWritesPrimitiveArrayRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  HandleScope scope(thread_);
  Int obj(&scope, runtime_->newInt(kMaxWord));
  Int two(&scope, SmallInt::fromWord(2));
  obj = runtime_->intMultiply(thread_, obj, two);
  CHECK(obj.isLargeInt(), "multiply failed");
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeLargeInt(LargeInt::cast(*obj));
    profiler.clearRecord();
  }
  word pos = 0;

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 34);  // length

  // Long array
  EXPECT_TRUE(
      readSubtag(result, &pos, HeapProfiler::Subtag::kPrimitiveArrayDump));
  EXPECT_EQ(readu64(result, &pos), obj.raw());  // array object ID
  EXPECT_EQ(read32(result, &pos), 0);           // stack trace serial number
  EXPECT_EQ(read32(result, &pos), 2);           // number of elements
  EXPECT_EQ(read8(result, &pos),
            HeapProfiler::BasicType::kLong);  // element type
  EXPECT_EQ(read64(result, &pos), -2);        // element 0
  EXPECT_EQ(read64(result, &pos), 0);         // element 1

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, WriteLargeStrWritesPrimitiveArrayRecord) {
  Vector<byte> result;
  HeapProfiler profiler(thread_, testWriter, &result);
  HandleScope scope(thread_);
  LargeStr obj(&scope, runtime_->newStrFromCStr("foobarbaz"));
  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeLargeStr(*obj);
    profiler.clearRecord();
  }
  word pos = 0;

  // Heap dump segment
  EXPECT_TRUE(readTag(result, &pos, HeapProfiler::Tag::kHeapDumpSegment));
  EXPECT_EQ(read32(result, &pos), 0);   // time
  EXPECT_EQ(read32(result, &pos), 27);  // length

  // Byte array
  EXPECT_TRUE(
      readSubtag(result, &pos, HeapProfiler::Subtag::kPrimitiveArrayDump));
  EXPECT_EQ(readu64(result, &pos), obj.raw());  // array object ID
  EXPECT_EQ(read32(result, &pos), 0);           // stack trace serial number
  EXPECT_EQ(read32(result, &pos), 9);           // number of elements
  EXPECT_EQ(read8(result, &pos),
            HeapProfiler::BasicType::kByte);  // element type
  EXPECT_EQ(read8(result, &pos), 'f');        // element 0
  EXPECT_EQ(read8(result, &pos), 'o');        // element 1
  EXPECT_EQ(read8(result, &pos), 'o');        // element 2
  EXPECT_EQ(read8(result, &pos), 'b');        // element 3
  EXPECT_EQ(read8(result, &pos), 'a');        // element 4
  EXPECT_EQ(read8(result, &pos), 'r');        // element 5
  EXPECT_EQ(read8(result, &pos), 'b');        // element 6
  EXPECT_EQ(read8(result, &pos), 'a');        // element 7
  EXPECT_EQ(read8(result, &pos), 'z');        // element 8

  EXPECT_EQ(pos, result.size());
}

TEST_F(HeapProfilerTest, RecordConstructorSetsFields) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  EXPECT_EQ(record.tag(), 10);
  EXPECT_EQ(record.length(), 0);
  EXPECT_EQ(record.body(), nullptr);
}

static ::testing::AssertionResult recordEqualsBytes(
    const HeapProfiler::Record& record, View<byte> expected) {
  EXPECT_EQ(record.length(), expected.length());
  const uint8_t* body = record.body();
  for (word i = 0; i < record.length(); i++) {
    if (body[i] != expected.get(i)) {
      return ::testing::AssertionFailure() << "byte " << i << " differs";
    }
  }
  return ::testing::AssertionSuccess();
}

TEST_F(HeapProfilerTest, RecordWriteWritesToBody) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  byte buf[] = {0, 1, 2, 3};
  record.write(buf, 4);
  EXPECT_TRUE(recordEqualsBytes(record, buf));
}

TEST_F(HeapProfilerTest, RecordWrite8WritesToBody) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  record.write8(0x7d);
  EXPECT_EQ(record.length(), 1);
  EXPECT_EQ(*record.body(), 0x7d);
}

TEST_F(HeapProfilerTest, RecordWrite16WritesToBodyInBigEndian) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  record.write16(0xbeef);
  byte expected[] = {0xbe, 0xef};
  EXPECT_TRUE(recordEqualsBytes(record, expected));
}

TEST_F(HeapProfilerTest, RecordWrite32WritesToBodyInBigEndian) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  record.write32(0xdeadbeef);
  byte expected[] = {0xde, 0xad, 0xbe, 0xef};
  EXPECT_TRUE(recordEqualsBytes(record, expected));
}

TEST_F(HeapProfilerTest, RecordWrite64WritesToBodyInBigEndian) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  record.write64(0xdec0ffeedeadbeef);
  byte expected[] = {0xde, 0xc0, 0xff, 0xee, 0xde, 0xad, 0xbe, 0xef};
  EXPECT_TRUE(recordEqualsBytes(record, expected));
}

TEST_F(HeapProfilerTest, RecordWriteObjectIdWritesToBodyInBigEndian) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  record.writeObjectId(0xdec0ffeedeadbeef);
  byte expected[] = {0xde, 0xc0, 0xff, 0xee, 0xde, 0xad, 0xbe, 0xef};
  EXPECT_TRUE(recordEqualsBytes(record, expected));
}

TEST_F(HeapProfilerTest, SubRecordConstructorWritesTag) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  HeapProfiler::SubRecord subrecord(static_cast<HeapProfiler::Subtag>(20),
                                    &record);
  EXPECT_EQ(record.length(), 1);
  EXPECT_EQ(*record.body(), 20);
}

TEST_F(HeapProfilerTest, SubRecordWriteWritesToRecord) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  HeapProfiler::SubRecord subrecord(static_cast<HeapProfiler::Subtag>(20),
                                    &record);
  byte buf[] = {0, 1, 2, 3};
  subrecord.write(buf, 4);
  byte expected[] = {20, 0, 1, 2, 3};
  EXPECT_TRUE(recordEqualsBytes(record, expected));
}

TEST_F(HeapProfilerTest, SubRecordWrite8WritesToRecord) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  HeapProfiler::SubRecord subrecord(static_cast<HeapProfiler::Subtag>(20),
                                    &record);
  subrecord.write8(0x7d);
  byte expected[] = {20, 0x7d};
  EXPECT_TRUE(recordEqualsBytes(record, expected));
}

TEST_F(HeapProfilerTest, SubRecordWrite16WritesToRecordInBigEndian) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  HeapProfiler::SubRecord subrecord(static_cast<HeapProfiler::Subtag>(20),
                                    &record);
  subrecord.write16(0xbeef);
  byte expected[] = {20, 0xbe, 0xef};
  EXPECT_TRUE(recordEqualsBytes(record, expected));
}

TEST_F(HeapProfilerTest, SubRecordWrite32WritesToRecordInBigEndian) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  HeapProfiler::SubRecord subrecord(static_cast<HeapProfiler::Subtag>(20),
                                    &record);
  subrecord.write32(0xdeadbeef);
  byte expected[] = {20, 0xde, 0xad, 0xbe, 0xef};
  EXPECT_TRUE(recordEqualsBytes(record, expected));
}

TEST_F(HeapProfilerTest, SubRecordWrite64WritesToBodyInBigEndian) {
  HeapProfiler::Record record(static_cast<HeapProfiler::Tag>(10), nullptr);
  HeapProfiler::SubRecord subrecord(static_cast<HeapProfiler::Subtag>(20),
                                    &record);
  subrecord.write64(0xdec0ffeedeadbeef);
  byte expected[] = {20, 0xde, 0xc0, 0xff, 0xee, 0xde, 0xad, 0xbe, 0xef};
  EXPECT_TRUE(recordEqualsBytes(record, expected));
}

}  // namespace testing

}  // namespace py
