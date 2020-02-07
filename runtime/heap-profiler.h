#include <set>

#include "globals.h"
#include "handles.h"
#include "objects.h"
#include "symbols.h"
#include "vector.h"

namespace py {

class WordVisitor {
 public:
  virtual void visit(uword element) = 0;
};

class WordSet {
 public:
  // Return true if element is in set, otherwise return false.
  bool contains(uword element) { return set_.find(element) != set_.end(); }

  // Return true if element was found when adding, otherwise return false.
  bool add(uword element) {
    if (contains(element)) return true;
    set_.insert(element);
    return false;
  }

  void visitElements(WordVisitor* visitor) {
    for (uword w : set_) {
      visitor->visit(w);
    }
  }

 private:
  std::set<uword> set_;
};

using HeapProfilerWriteCallback = void (*)(const void* data, word length,
                                           void* stream);

RawObject heapDump(Thread* thread, const char* filename);

// A HeapProfiler writes a snapshot of the heap for off-line analysis. The heap
// is written in binary HPROF format, which is a sequence of self describing
// records. A description of the HPROF format can be found at
//
// http://hg.openjdk.java.net/jdk6/jdk6/jdk/raw-file/tip/src/share/demo/jvmti/hprof/manual.html
//
// HPROF was not designed for Pyro, but most Pyro concepts can be mapped
// directly into HPROF. Some features, such as immediate objects and variable
// length objects, require a translation.
class HeapProfiler {
 public:
  HeapProfiler(Thread* thread, HeapProfilerWriteCallback callback,
               void* stream);

  // A growable array of bytes used to build a record body.
  // TODO(T61870494): Write the heap in two passes to avoid the need for a
  // Buffer per Record
  class Buffer {
   public:
    Buffer();

    // Writes an array of bytes to the buffer, increasing the capacity as
    // needed.
    void write(const byte* data, word size);

    // Returns the underlying element storage.
    const byte* data() const {
      return data_.empty() ? nullptr : &data_.front();
    }

    // Returns the number of elements written to the buffer.
    word size() const { return data_.size(); }

   private:
    Vector<byte> data_;

    DISALLOW_COPY_AND_ASSIGN(Buffer);
  };

  // Tags for describing element and field types.
  enum BasicType {
    kObject = 2,
    kBoolean = 4,
    kChar = 5,
    kFloat = 6,
    kDouble = 7,
    kByte = 8,
    kShort = 9,
    kInt = 10,
    kLong = 11,
  };

  enum Tag {
    kStringInUtf8 = 0x01,
    kLoadClass = 0x02,
    kStackTrace = 0x05,
    kHeapDumpSegment = 0x1C,
    kHeapDumpEnd = 0x2C,
  };

  // These classes do not exist in the Python object model but must be
  // represented in the HPROF dump for the Java tooling (JHAT, MAT, etc) to
  // load the dump.
  // They are tagged in this funny fashion because they need to look like real
  // honest-to-goodness heap objects but not collide with anything on the heap.
  enum class FakeClass {
    kJavaLangClass =
        (0 << RawObject::kPrimaryTagBits) | RawObject::kHeapObjectTag,
    kJavaLangClassLoader =
        (1 << RawObject::kPrimaryTagBits) | RawObject::kHeapObjectTag,
    kJavaLangObject =
        (2 << RawObject::kPrimaryTagBits) | RawObject::kHeapObjectTag,
    kJavaLangString =
        (3 << RawObject::kPrimaryTagBits) | RawObject::kHeapObjectTag,
    kByteArray = (4 << RawObject::kPrimaryTagBits) | RawObject::kHeapObjectTag,
    kDoubleArray =
        (5 << RawObject::kPrimaryTagBits) | RawObject::kHeapObjectTag,
    kLongArray = (6 << RawObject::kPrimaryTagBits) | RawObject::kHeapObjectTag,
    kObjectArray =
        (7 << RawObject::kPrimaryTagBits) | RawObject::kHeapObjectTag,
    kCharArray = (8 << RawObject::kPrimaryTagBits) | RawObject::kHeapObjectTag,
  };

  // A top-level data record.
  class Record {
   public:
    Record(Tag tag, HeapProfiler* profiler);
    // Destroy and write the record to the profiler's stream.
    ~Record();

    // Returns the tag describing the record format.
    Tag tag() const { return tag_; }

    // Returns a millisecond time delta, always 0.
    uint8_t time() const { return 0; }

    // Returns the record length in bytes.
    uint32_t length() const { return body_.size(); }

    // Returns the record body.
    const byte* body() const { return body_.data(); }

    // Appends an array of 8-bit values to the record body.
    void write(const byte* value, word size);

    // Appends an 8-, 16-, 32- or 64-bit value to the body in big-endian
    // format.
    void write8(uint8_t value);
    void write16(uint16_t value);
    void write32(uint32_t value);
    void write64(uint64_t value);

    // Appends an ID to the body.
    void writeObjectId(uword value);

   private:
    // A tag value that describes the record format.
    Tag tag_;

    // The payload of the record as described by the tag.
    Buffer body_;

    // The parent profiler to flush to when destructed.
    HeapProfiler* profiler_;

    DISALLOW_COPY_AND_ASSIGN(Record);
  };

  // Sub-record tags describe sub-records within a heap dump or heap dump
  // segment.
  enum Subtag {
    kRootJniGlobal = 0x01,
    kRootJniLocal = 0x02,
    kRootJavaFrame = 0x03,
    kRootNativeStack = 0x04,
    kRootStickyClass = 0x05,
    kRootThreadBlock = 0x06,
    kRootMonitorUsed = 0x07,
    kRootThreadObject = 0x08,
    kClassDump = 0x20,
    kInstanceDump = 0x21,
    kObjectArrayDump = 0x22,
    kPrimitiveArrayDump = 0x23,
    kRootUnknown = 0xFF
  };

  // A sub-record within a heap dump record.  Write calls are
  // forwarded to the profilers heap dump record instance.
  class SubRecord {
   public:
    // Starts a new sub-record within the heap dump record.
    SubRecord(Subtag sub_tag, Record* record);

    // Appends an array of 8-bit values to the heap dump record.
    void write(const byte* value, word size);

    // Appends an 8-, 16-, 32- or 64-bit value to the heap dump
    // record.
    void write8(uint8_t value);
    void write16(uint16_t value);
    void write32(uint32_t value);
    void write64(uint64_t value);

    // Appends an ID to the current heap dump record.
    void writeObjectId(uword value);

    // INSTANCE DUMP - 0x21
    //
    // Begin instance dump. Callers must follow this call with N bytes.
    //
    // Format:
    //  ID - object ID
    //  u4 - stack trace serial number
    //  ID - class object ID
    //  u4 - N, number of bytes that follow
    void beginInstanceDump(RawObject obj, uword stack_trace, uword num_bytes,
                           uword layout_id);

    // PRIMITIVE ARRAY DUMP - 0x23
    //
    // Begin primitive array dump. Callers must follow this call a packed array
    // of elements.
    //
    // Format:
    //  ID - array object ID
    //  u4 - stack trace serial number
    //  u4 - number of elements
    //  u1 - element type
    //  [u1]* - elements (packed array)
    void beginPrimitiveArrayDump(uword object_id, uword stack_trace,
                                 uword length, BasicType type);

   private:
    // The record instance that receives forwarded write calls.
    Record* record_;
  };

  // Set the current record.
  void setRecord(Record* record);

  // Unset the current record.
  void clearRecord();

  // Write a record that indicates the end of a series of Heap Dump Segments.
  void writeHeapDumpEnd();

  // Id canonizers.
  uword classId(RawLayout layout);
  uword cStringId(const char* c_str);
  uword objectId(RawObject obj);
  uword stringId(RawStr str);

  // Invokes the write callback.
  void write(const void* data, word size);

  // Invokes the write callback with an 8-, 16-, 32- or 64-bit value. Writes in
  // big-endian format.
  void write8(uint8_t value);
  void write16(uint16_t value);
  void write32(uint32_t value);
  void write64(uint64_t value);

  // This function writes a dummy load class record. Class loaders are a
  // Java-only concept that we have to mimic in Pyro to make the various memory
  // analysis tools happy.
  void writeFakeLoadClass(FakeClass fake_class, const char* class_name);

  // This function writes a dummy class dump subrecord for a class that only
  // exists in Java. It also writes a dummy load class record.
  void writeFakeClassDump(FakeClass fake_class, const char* class_name,
                          FakeClass fake_super_class);

  // This function writes a dummy stacktrace, as one is required by the HPROF
  // format.
  void writeFakeStackTrace();

  // Invoke the write callback with the HPROF header.
  void writeHeader();

  // This function writes a load class record. Class loaders are a Java-only
  // concept that we have to mimic in Pyro to make the various memory analysis
  // tools happy.
  void writeLoadClass(RawLayout layout);

  // This function writes a class dump record.
  void writeClassDump(RawLayout layout);

  // This function writes an instance dump record.
  void writeInstanceDump(RawInstance obj);

  // Write an immediate as an instance dump.
  void writeImmediate(RawObject obj);

  // Write all the immediates collected as instance dumps.
  void writeImmediates();

  // Write an Object Array (tuple) SubRecord.
  void writeObjectArray(RawTuple tuple);

  // Write a byte Array (Small/LargeBytes) SubRecord.
  void writeBytes(RawBytes bytes);

  // Write a byte Array (LargeStr) SubRecord.
  void writeLargeStr(RawLargeStr str);

  // Write a Complex as a Double Array SubRecord.
  void writeComplex(RawComplex obj);

  // Write the Ellipsis singleton.
  void writeEllipsis(RawEllipsis obj);

  // Write a Float as a Double Array SubRecord.
  void writeFloat(RawFloat obj);

  // Write a LargeInt as a Long Array SubRecord.
  void writeLargeInt(RawLargeInt obj);

  // Write out the various types of root SubRecords.
  void writeRuntimeRoot(RawObject obj);
  void writeThreadRoot(Thread* thread);
  void writeHandleRoot(RawObject obj);
  void writeStackRoot(RawObject obj);
  void writeStickyClassRoot(RawObject obj);
  void writeApiHandleRoot(RawObject obj);
  void writeUnknownRoot(RawObject obj);

  // Writes a string in UTF-8 record to the output stream.
  void writeCStringInUTF8(const char* c_str);
  void writeStringInUTF8(RawStr str);

  static const char kByteArrayClassName[];
  static const char kDoubleArrayClassName[];
  static const char kInvalid[];
  static const char kOverflow[];
  static const char kJavaLangClass[];
  static const char kJavaLangClassLoader[];
  static const char kJavaLangObject[];
  static const char kJavaLangString[];
  static const char kLongArrayClassName[];
  static const char kObjectArrayClassName[];

 private:
  // Sets to ensure that we don't dump objects twice.
  WordSet class_dump_table_;
  WordSet load_class_table_;
  WordSet heap_object_table_;
  WordSet immediate_table_;
  WordSet layout_table_;
  WordSet string_table_;

  Record* current_record_ = nullptr;
  Thread* thread_ = nullptr;
  void* output_stream_;
  HeapProfilerWriteCallback write_callback_;

  DISALLOW_COPY_AND_ASSIGN(HeapProfiler);
};

}  // namespace py
