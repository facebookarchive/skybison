#include "heap-profiler.h"

#include <cerrno>

#include "capi-handles.h"
#include "file.h"
#include "handles.h"
#include "os.h"
#include "runtime.h"
#include "thread.h"

namespace py {

const char HeapProfiler::kByteArrayClassName[] = "byte[]";
const char HeapProfiler::kDoubleArrayClassName[] = "double[]";
const char HeapProfiler::kInvalid[] = "<INVALID>";
const char HeapProfiler::kOverflow[] = "<OVERFLOW>";
const char HeapProfiler::kJavaLangClass[] = "java.lang.Class";
const char HeapProfiler::kJavaLangClassLoader[] = "java.lang.ClassLoader";
const char HeapProfiler::kJavaLangObject[] = "java.lang.Object";
const char HeapProfiler::kJavaLangString[] = "java.lang.String";
const char HeapProfiler::kLongArrayClassName[] = "long[]";
const char HeapProfiler::kObjectArrayClassName[] = "java.lang.Object[]";

HeapProfiler::HeapProfiler(Thread* thread, HeapProfilerWriteCallback callback,
                           void* stream)
    : thread_(thread), output_stream_(stream), write_callback_(callback) {}

void HeapProfiler::write(const void* data, word size) {
  (*write_callback_)(data, size, output_stream_);
}

// LOAD CLASS - 0x02
//
// Format:
//   u4 - class serial number (always > 0)
//   ID - class object ID
//   u4 - stack trace serial number
//   ID - class name string ID
void HeapProfiler::writeFakeLoadClass(FakeClass fake_class,
                                      const char* class_name) {
  Record record(kLoadClass, this);
  // class serial number (always > 0)
  record.write32(1);
  // class object ID
  record.writeObjectId(static_cast<uword>(fake_class));
  // stack trace serial number
  record.write32(0);
  // TODO(T61807224): Dump type names discriminated by layout ID
  // class name string ID
  record.writeObjectId(cStringId(class_name));
}

// CLASS DUMP - 0x20
//
// Format:
//   u4 - class serial number (always > 0)
//   ID - class object ID
//   u4 - stack trace serial number
//   ID - class name string ID
void HeapProfiler::writeFakeClassDump(FakeClass fake_class,
                                      const char* class_name,
                                      FakeClass fake_super_class) {
  writeFakeLoadClass(fake_class, class_name);
  CHECK(!class_dump_table_.add(static_cast<uword>(fake_class)),
        "cannot dump object twice");
  SubRecord sub(Subtag::kClassDump, current_record_);
  // class object ID
  sub.writeObjectId(static_cast<uword>(fake_class));
  // stack trace serial number
  sub.write32(0);
  // super class object ID
  sub.writeObjectId(static_cast<uword>(fake_super_class));
  // class loader object ID
  sub.writeObjectId(0);
  // signers object ID
  sub.writeObjectId(0);
  // protection domain object ID
  sub.writeObjectId(0);
  // reserved
  sub.writeObjectId(0);
  // reserved
  sub.writeObjectId(0);
  // instance size (in bytes)
  sub.write32(0);
  // size of constant pool and number of records that follow
  sub.write16(0);
  // Number of static fields
  sub.write16(0);
  // Number of instance fields (not include super class's)
  sub.write16(0);
}

// STACK TRACE - 0x05
//
//  u4 - stack trace serial number
//  u4 - thread serial number
//  u4 - number of frames
//  [ID]* - series of stack frame ID's
void HeapProfiler::writeFakeStackTrace() {
  Record record(kStackTrace, this);
  // stack trace serial number
  record.write32(0);
  // thread serial number
  record.write32(0);
  // number of frames
  record.write32(0);
}

void HeapProfiler::writeHeader() {
  const char magic[] = "JAVA PROFILE 1.0.2";
  write(magic, sizeof(magic));
  write32(kPointerSize);
  double seconds_double = OS::currentTime();
  uint64_t seconds = static_cast<uint64_t>(seconds_double);
  seconds += (seconds_double - seconds);
  uint64_t milliseconds = seconds * kMillisecondsPerSecond;
  uint32_t hi =
      static_cast<uint32_t>((milliseconds >> 32) & 0x00000000FFFFFFFF);
  write32(hi);
  uint32_t lo = static_cast<uint32_t>(milliseconds & 0x00000000FFFFFFFF);
  write32(lo);
}

// LOAD CLASS - 0x02
//
// Format:
//   u4 - class serial number (always > 0)
//   ID - class object ID
//   u4 - stack trace serial number
//   ID - class name string ID
void HeapProfiler::writeLoadClass(RawLayout layout) {
  CHECK(!load_class_table_.add(layout.raw()), "cannot dump object twice");
  Record record(kLoadClass, this);
  // class serial number (always > 0)
  record.write32(1);
  // class object ID
  record.writeObjectId(objectId(layout));
  // stack trace serial number
  record.write32(0);
  // class name string ID
  HandleScope scope(thread_);
  Type type(&scope, thread_->runtime()->concreteTypeAt(layout.id()));
  Str name(&scope, type.name());
  record.writeObjectId(stringId(*name));
}

// CLASS DUMP - 0x20
//
// Format:
//  ID - class object ID
//  u4 - stack trace serial number
//  ID - super class object ID
//  ID - class loader object ID
//  ID - signers object ID
//  ID - protection domain object ID
//  ID - reserved
//  ID - reserved
//  u4 - instance size (in bytes)
//  u2 - size of constant pool and number of records that follow
//  u2 - constant pool index
//  u1 - type of entry: (See Basic Type)
//  value - value of entry (u1, u2, u4, or u8 based on type of entry)
//  u2 - Number of static fields:
//  ID - static field name string ID
//  u1 - type of field: (See Basic Type)
//  value - value of entry (u1, u2, u4, or u8 based on type of field)
//  u2 - Number of instance fields (not including super class's)
//  ID - field name string ID
//  u1 - type of field: (See Basic Type)
void HeapProfiler::writeClassDump(RawLayout layout) {
  CHECK(!class_dump_table_.add(layout.raw()), "cannot dump object twice");
  SubRecord sub(kClassDump, current_record_);
  // class object ID
  sub.writeObjectId(classId(layout));
  // stack trace serial number
  sub.write32(0);
  // super class object ID
  if (layout.id() == LayoutId::kObject) {
    // Superclass == 0 => object is java.lang.Object
    sub.writeObjectId(0);
  } else {
    // Since there is not much of a concept of inheritance in the Layout
    // system, pretend all Layouts' super is "object". This allows much easier
    // dumping of attributes.
    // TODO(emacs): Figure out how to dump class hierarchies
    RawLayout super_layout =
        Layout::cast(thread_->runtime()->layoutAt(LayoutId::kObject));
    sub.writeObjectId(classId(super_layout));
  }
  // class loader object ID
  sub.writeObjectId(0);
  // signers object ID
  sub.writeObjectId(0);
  // protection domain object ID
  sub.writeObjectId(0);
  // reserved
  sub.writeObjectId(0);
  // reserved
  sub.writeObjectId(0);

  // instance size (in bytes)
  sub.write32(layout.instanceSize());
  // size of constant pool and number of records that follow
  // Constant pool is variable-length and empty here
  sub.write16(0);
  // number of static fields
  // Static fields are variable-length and empty here
  sub.write16(0);
  Runtime* runtime = thread_->runtime();
  if (layout.id() == LayoutId::kComplex) {
    // Two instance fields: "real", "imag"
    sub.write16(2);
    sub.writeObjectId(stringId(Str::cast(runtime->symbols()->at(ID(real)))));
    sub.write8(BasicType::kDouble);
    sub.writeObjectId(stringId(Str::cast(runtime->symbols()->at(ID(imag)))));
    sub.write8(BasicType::kDouble);
    return;
  }
  if (layout.id() == LayoutId::kFloat) {
    // One instance field: "value"
    sub.write16(1);
    sub.writeObjectId(stringId(Str::cast(runtime->symbols()->at(ID(value)))));
    sub.write8(BasicType::kDouble);
    return;
  }
  // number of instance fields (not include super class's)
  word num = layout.numInObjectAttributes();
  word num_overflow_slots = layout.hasTupleOverflow() ? 1 : 0;
  sub.write16(num + num_overflow_slots);
  // instance fields
  RawTuple in_object = Tuple::cast(layout.inObjectAttributes());
  for (word i = 0; i < num; i++) {
    if (i < in_object.length()) {
      // allocated on the layout for an attribute
      RawObject name = Tuple::cast(in_object.at(i)).at(0);
      if (name.isNoneType()) {
        sub.writeObjectId(cStringId(kInvalid));
      } else {
        sub.writeObjectId(stringId(Str::cast(name)));
      }
    } else {
      // This instance variable has not yet been allocated for an attribute
      sub.writeObjectId(cStringId(kInvalid));
    }
    sub.write8(BasicType::kObject);
  }
  // TODO(emacs): Remove this special case once tuple overflow fits neatly into
  // the allocated in-object attributes
  if (layout.hasTupleOverflow()) {
    sub.writeObjectId(cStringId(kOverflow));
    sub.write8(BasicType::kObject);
  }
}

void HeapProfiler::writeInstanceDump(RawInstance obj) {
  CHECK(!heap_object_table_.add(obj.raw()), "cannot dump object twice");
  SubRecord sub(kInstanceDump, current_record_);
  RawLayout layout = Layout::cast(Thread::current()->runtime()->layoutOf(obj));
  // TODO(emacs): Remove this when we don't have kMinimumSize anymore.
  word num_instance_variables = Utils::maximum(1L, obj.headerCountOrOverflow());
  sub.beginInstanceDump(obj, /*stack_trace=*/0,
                        num_instance_variables * kPointerSize, classId(layout));
  // write in-object attributes
  for (word i = 0; i < num_instance_variables; i++) {
    sub.writeObjectId(
        objectId(Instance::cast(obj).instanceVariableAt(i * kPointerSize)));
  }
}

void HeapProfiler::writeImmediate(RawObject obj) {
  DCHECK(!obj.isHeapObject(), "obj must be an immediate");
  SubRecord sub(kInstanceDump, current_record_);
  RawLayout layout = Layout::cast(Thread::current()->runtime()->layoutOf(obj));
  sub.beginInstanceDump(obj, /*stack_trace=*/0, /*num_bytes=*/0,
                        classId(layout));
}

class ImmediateVisitor : public WordVisitor {
 public:
  ImmediateVisitor(HeapProfiler* profiler) : profiler_(profiler) {}
  void visit(uword element) { profiler_->writeImmediate(RawObject{element}); }

 private:
  HeapProfiler* profiler_;
};

void HeapProfiler::writeImmediates() {
  ImmediateVisitor visitor(this);
  immediate_table_.visitElements(&visitor);
}

// OBJECT ARRAY DUMP - 0x22
//
// Format:
//  ID - array object ID
//  u4 - stack trace serial number
//  u4 - number of elements
//  ID - array class object id
//  [ID]* - elements
void HeapProfiler::writeObjectArray(RawTuple tuple) {
  SubRecord sub(kObjectArrayDump, current_record_);
  // array object id
  sub.writeObjectId(objectId(tuple));
  // stack trace serial number
  sub.write32(0);
  // number of elements
  word length = tuple.length();
  CHECK(length < kMaxUint32, "length %ld too big for Java length field",
        length);
  sub.write32(static_cast<uint32_t>(length));
  // array class object id
  sub.writeObjectId(static_cast<uword>(FakeClass::kObjectArray));
  // elements
  for (word i = 0; i < length; i++) {
    sub.writeObjectId(objectId(tuple.at(i)));
  }
}

void HeapProfiler::writeBytes(RawBytes bytes) {
  CHECK(!heap_object_table_.add(bytes.raw()), "cannot dump object twice");
  SubRecord sub(kPrimitiveArrayDump, current_record_);
  sub.beginPrimitiveArrayDump(objectId(bytes), /*stack_trace=*/0,
                              bytes.length(), BasicType::kByte);
  for (word i = 0; i < bytes.length(); i++) {
    sub.write8(bytes.byteAt(i));
  }
}

void HeapProfiler::writeLargeStr(RawLargeStr str) {
  CHECK(!heap_object_table_.add(str.raw()), "cannot dump object twice");
  SubRecord sub(kPrimitiveArrayDump, current_record_);
  sub.beginPrimitiveArrayDump(objectId(str), /*stack_trace=*/0,
                              str.charLength(), BasicType::kByte);
  for (word i = 0; i < str.charLength(); i++) {
    sub.write8(str.charAt(i));
  }
}

void HeapProfiler::writeComplex(RawComplex obj) {
  CHECK(!heap_object_table_.add(obj.raw()), "cannot dump object twice");
  SubRecord sub(kInstanceDump, current_record_);
  RawLayout layout = Layout::cast(Thread::current()->runtime()->layoutOf(obj));
  sub.beginInstanceDump(obj, /*stack_trace=*/0, 2 * kDoubleSize,
                        classId(layout));
  sub.write64(bit_cast<uint64_t>(obj.real()));
  sub.write64(bit_cast<uint64_t>(obj.imag()));
}

void HeapProfiler::writeEllipsis(RawEllipsis obj) {
  CHECK(!heap_object_table_.add(obj.raw()), "cannot dump object twice");
  SubRecord sub(kInstanceDump, current_record_);
  RawLayout layout =
      Layout::cast(thread_->runtime()->layoutAt(LayoutId::kEllipsis));
  sub.beginInstanceDump(obj, /*stack_trace=*/0, layout.instanceSize(),
                        classId(layout));
  for (word i = 0; i < layout.instanceSize(); i += kPointerSize) {
    sub.writeObjectId(objectId(Unbound::object()));
  }
}

void HeapProfiler::writeFloat(RawFloat obj) {
  CHECK(!heap_object_table_.add(obj.raw()), "cannot dump object twice");
  SubRecord sub(kInstanceDump, current_record_);
  RawLayout layout = Layout::cast(Thread::current()->runtime()->layoutOf(obj));
  sub.beginInstanceDump(obj, /*stack_trace=*/0, kDoubleSize, classId(layout));
  sub.write64(bit_cast<uint64_t>(obj.value()));
}

void HeapProfiler::writeLargeInt(RawLargeInt obj) {
  CHECK(!heap_object_table_.add(obj.raw()), "cannot dump object twice");
  SubRecord sub(kPrimitiveArrayDump, current_record_);
  sub.beginPrimitiveArrayDump(objectId(obj), /*stack_trace=*/0, obj.numDigits(),
                              BasicType::kLong);
  for (word i = 0; i < obj.numDigits(); i++) {
    sub.write64(obj.digitAt(i));
  }
}

// HEAP DUMP SEGMENT - 0x1C
void HeapProfiler::setRecord(Record* current_record) {
  DCHECK(current_record != nullptr, "record should be non-null");
  DCHECK(current_record_ == nullptr, "current record already exists");
  current_record_ = current_record;
}

void HeapProfiler::clearRecord() {
  DCHECK(current_record_ != nullptr, "current record does not exist");
  current_record_ = nullptr;
}

// HEAP DUMP END - 0x2C
void HeapProfiler::writeHeapDumpEnd() { Record record(kHeapDumpEnd, this); }

uword HeapProfiler::objectId(RawObject obj) {
  uword id = obj.raw();
  if (!obj.isError() && !obj.isHeapObject()) {
    immediate_table_.add(id);
  }
  return id;
}

uword HeapProfiler::classId(RawLayout layout) {
  uword id = layout.raw();
  if (!layout_table_.add(id)) {
    writeLoadClass(layout);
  }
  return id;
}

uword HeapProfiler::cStringId(const char* c_str) {
  uword id = reinterpret_cast<uword>(c_str);
  if (!string_table_.add(id)) {
    writeCStringInUTF8(c_str);
  }
  return id;
}

uword HeapProfiler::stringId(RawStr str) {
  uword id = objectId(str);
  if (!string_table_.add(id)) {
    writeStringInUTF8(str);
  }
  return id;
}

// ROOT UNKNOWN - 0xFF
//
// Describes a root of unknown provenance.
//
// Format:
//   ID - object ID
void HeapProfiler::writeRuntimeRoot(RawObject obj) {
  SubRecord sub(kRootUnknown, current_record_);
  // object ID
  sub.writeObjectId(objectId(obj));
}

// ROOT STICKY CLASS - 0x05
//
// Describes a built-in Layout.
//
// Format:
//   ID - object ID
void HeapProfiler::writeStickyClassRoot(RawObject obj) {
  SubRecord sub(kRootStickyClass, current_record_);
  // object ID
  sub.writeObjectId(objectId(obj));
}

// ROOT JAVA FRAME - 0x03
//
// Describes a value found in a Frame in the Python stack.
//
// Format:
//   ID - object ID
//   u4 - thread serial number
//   u4 - frame number in stack trace (-1 for empty)
void HeapProfiler::writeStackRoot(RawObject obj) {
  SubRecord sub(kRootJavaFrame, current_record_);
  // object ID
  sub.writeObjectId(objectId(obj));
  // thread serial number
  sub.write32(0);
  // frame number in stack trace
  sub.write32(-1);
}

// ROOT THREAD OBJECT - 0x08
//
// Describes a Thread object.
//
// Format:
//   ID - object ID
//   u4 - thread serial number
//   u4 - stack trace serial number
void HeapProfiler::writeThreadRoot(Thread* thread) {
  SubRecord sub(kRootThreadObject, current_record_);
  // object ID
  sub.writeObjectId(reinterpret_cast<uword>(thread));
  // thread serial number
  sub.write32(0);
  // stack trace serial number
  sub.write32(0);
}

// ROOT JNI GLOBAL - 0x01
//
// Describes an object wrapped in an ApiHandle.
//
// Format:
//   ID - object ID
//   ID - ApiHandle address
void HeapProfiler::writeApiHandleRoot(RawObject obj) {
  SubRecord sub(kRootJniGlobal, current_record_);
  // object ID
  sub.writeObjectId(objectId(obj));
  // ApiHandle address
  // TODO(emacs): Propagate the ApiHandle pointer through to this function
  // instead of looking it up again.
  ApiHandle* handle = ApiHandle::borrowedReference(thread_, obj);
  sub.writeObjectId(reinterpret_cast<uword>(handle));
}

// ROOT UNKNOWN - 0xFF
//
// Describes an object of unknown provenance (typically Runtime or Thread root).
//
// Format:
//   ID - object ID
void HeapProfiler::writeUnknownRoot(RawObject obj) {
  SubRecord sub(kRootUnknown, current_record_);
  // object ID
  sub.writeObjectId(objectId(obj));
}

// ROOT NATIVE STACK - 0x04
//
// Describes an object inside a native frame (in a Handle).
//
// Format:
//   ID - object ID
//   u4 - thread serial number
void HeapProfiler::writeHandleRoot(RawObject obj) {
  SubRecord sub(kRootNativeStack, current_record_);
  // object ID
  sub.writeObjectId(objectId(obj));
  // thread serial number
  // TODO(emacs): Write an ID from the thread the handle belongs to
  sub.write32(thread_->id());
}

// STRING IN UTF8 - 0x01
//
// Format:
//   ID - ID for this string
//   [u1]* - UTF8 characters for string (NOT NULL terminated)
void HeapProfiler::writeStringInUTF8(RawStr str) {
  Record record(kStringInUtf8, this);
  record.writeObjectId(str.raw());
  for (word i = 0; i < str.charLength(); i++) {
    record.write8(str.charAt(i));
  }
}

void HeapProfiler::writeCStringInUTF8(const char* c_str) {
  Record record(kStringInUtf8, this);
  record.writeObjectId(reinterpret_cast<uword>(c_str));
  for (; *c_str != '\0'; ++c_str) {
    record.write8(*c_str);
  }
}

HeapProfiler::Buffer::Buffer() : data_(Vector<uint8_t>()) {}

void HeapProfiler::Buffer::write(const uint8_t* data, word size) {
  for (word i = 0; i < size; i++) {
    data_.push_back(data[i]);
  }
}

void HeapProfiler::write8(uint8_t value) { write(&value, sizeof(value)); }

void HeapProfiler::write16(uint16_t value) {
  for (word i = 1; i >= 0; i--) {
    write8((value >> (i * kBitsPerByte)) & 0xff);
  }
}

void HeapProfiler::write32(uint32_t value) {
  for (word i = 3; i >= 0; i--) {
    write8((value >> (i * kBitsPerByte)) & 0xff);
  }
}

void HeapProfiler::write64(uint64_t value) {
  for (word i = 7; i >= 0; i--) {
    write8((value >> (i * kBitsPerByte)) & 0xff);
  }
}

HeapProfiler::Record::Record(Tag tag, HeapProfiler* profiler)
    : tag_(tag), profiler_(profiler) {}

// Record
//
// Format:
//   u1 - TAG: denoting the type of the record
//   u4 - TIME: number of microseconds since the time stamp in the header
//   u4 - LENGTH: number of bytes that follow this u4 field and belong
//        to this record
//   [u1]* - BODY: as many bytes as specified in the above u4 field
HeapProfiler::Record::~Record() {
  if (profiler_) {
    profiler_->write8(tag());
    profiler_->write32(time());
    profiler_->write32(length());
    if (length() > 0) {
      profiler_->write(body(), length());
    }
  }
}

void HeapProfiler::Record::write(const byte* value, word size) {
  body_.write(value, size);
}

void HeapProfiler::Record::write8(uint8_t value) {
  body_.write(&value, sizeof(value));
}

void HeapProfiler::Record::write16(uint16_t value) {
  for (word i = 1; i >= 0; i--) {
    write8((value >> (i * kBitsPerByte)) & 0xff);
  }
}

void HeapProfiler::Record::write32(uint32_t value) {
  for (word i = 3; i >= 0; i--) {
    write8((value >> (i * kBitsPerByte)) & 0xff);
  }
}

void HeapProfiler::Record::write64(uint64_t value) {
  for (word i = 7; i >= 0; i--) {
    write8((value >> (i * kBitsPerByte)) & 0xff);
  }
}

void HeapProfiler::Record::writeObjectId(uword value) { write64(value); }

HeapProfiler::SubRecord::SubRecord(Subtag sub_tag, Record* record)
    : record_(record) {
  DCHECK(record_ != nullptr, "heap dump segment does not exist");
  record_->write8(sub_tag);
}

void HeapProfiler::SubRecord::beginInstanceDump(RawObject obj,
                                                uword stack_trace,
                                                uword num_bytes,
                                                uword layout_id) {
  // TODO(emacs): This is a hack that works around MAT expecting ClassLoader at
  // 0. Once we have modified MAT to dump ClassLoader at a different location
  // than 0, we should just dump SmallInt 0 normally.
  word id = obj.raw() == 0 ? 73 : obj.raw();
  writeObjectId(id);
  write32(stack_trace);
  writeObjectId(layout_id);
  write32(num_bytes);
}

void HeapProfiler::SubRecord::beginPrimitiveArrayDump(uword object_id,
                                                      uword stack_trace,
                                                      uword length,
                                                      BasicType type) {
  writeObjectId(object_id);
  write32(stack_trace);
  CHECK(length < kMaxUint32, "length %ld too big for Java length field",
        length);
  write32(static_cast<uint32_t>(length));
  write8(type);
}

void HeapProfiler::SubRecord::write(const uint8_t* value, intptr_t size) {
  record_->write(value, size);
}

void HeapProfiler::SubRecord::write8(uint8_t value) { record_->write8(value); }

void HeapProfiler::SubRecord::write16(uint16_t value) {
  record_->write16(value);
}

void HeapProfiler::SubRecord::write32(uint32_t value) {
  record_->write32(value);
}

void HeapProfiler::SubRecord::write64(uint64_t value) {
  record_->write64(value);
}

void HeapProfiler::SubRecord::writeObjectId(uword value) {
  record_->writeObjectId(value);
}

static void writeToFileStream(const void* data, word length, void* stream) {
  DCHECK(data != nullptr, "data must not be null");
  DCHECK(length > 0, "length must be positive");
  word fd = static_cast<int>(reinterpret_cast<word>(stream));
  int result = File::write(fd, data, length);
  CHECK(result == length, "could not write the whole chunk to disk");
}

class HeapProfilerRootVisitor : public PointerVisitor {
 public:
  HeapProfilerRootVisitor(HeapProfiler* profiler) : profiler_(profiler) {}
  void visitPointer(RawObject* pointer, PointerKind kind) {
    // TODO(emacs): This is a hack that works around MAT expecting ClassLoader
    // at 0. Once we have modified MAT to dump ClassLoader at a different
    // location than 0, we should just dump SmallInt 0 normally.
    RawObject obj = RawObject{pointer->raw() == 0 ? 73 : pointer->raw()};
    switch (kind) {
      case PointerKind::kRuntime:
      case PointerKind::kThread:
      case PointerKind::kUnknown:
        return profiler_->writeUnknownRoot(obj);
      case PointerKind::kHandle:
        return profiler_->writeHandleRoot(obj);
      case PointerKind::kStack:
        return profiler_->writeStackRoot(obj);
      case PointerKind::kApiHandle:
        return profiler_->writeApiHandleRoot(obj);
      case PointerKind::kLayout:
        return profiler_->writeStickyClassRoot(obj);
    }
  }

 protected:
  HeapProfiler* profiler_;

  DISALLOW_COPY_AND_ASSIGN(HeapProfilerRootVisitor);
};

class HeapProfilerObjectVisitor : public HeapObjectVisitor {
 public:
  HeapProfilerObjectVisitor(HeapProfiler* profiler) : profiler_(profiler) {}
  void visitHeapObject(RawHeapObject obj) {
    switch (obj.layoutId()) {
      case LayoutId::kLayout:
        return profiler_->writeClassDump(Layout::cast(obj));
      case LayoutId::kLargeInt:
        return profiler_->writeLargeInt(LargeInt::cast(obj));
      case LayoutId::kLargeBytes:
      case LayoutId::kMutableBytes:
        return profiler_->writeBytes(Bytes::cast(obj));
      case LayoutId::kFloat:
        return profiler_->writeFloat(Float::cast(obj));
      case LayoutId::kComplex:
        return profiler_->writeComplex(Complex::cast(obj));
      case LayoutId::kTuple:
      case LayoutId::kMutableTuple:
        return profiler_->writeObjectArray(Tuple::cast(obj));
      case LayoutId::kLargeStr:
        return profiler_->writeLargeStr(LargeStr::cast(obj));
      case LayoutId::kEllipsis:
        return profiler_->writeEllipsis(Ellipsis::cast(obj));
      default:
        CHECK(obj.isInstance(), "obj should be instance, but is %ld",
              obj.layoutId());
        return profiler_->writeInstanceDump(Instance::cast(obj));
    }
  }

 protected:
  HeapProfiler* profiler_;

  DISALLOW_COPY_AND_ASSIGN(HeapProfilerObjectVisitor);
};

RawObject heapDump(Thread* thread, const char* filename) {
  int fd = File::open(
      filename,
      File::kBinaryFlag | File::kCreate | File::kTruncate | File::kWriteOnly,
      0644);
  if (fd < 0) {
    int saved_errno = errno;
    return thread->raiseOSErrorFromErrno(saved_errno);
  }

  HeapProfiler profiler(thread, writeToFileStream, reinterpret_cast<void*>(fd));
  profiler.writeHeader();
  profiler.writeFakeStackTrace();

  {
    HeapProfiler::Record record(HeapProfiler::kHeapDumpSegment, &profiler);
    profiler.setRecord(&record);
    profiler.writeThreadRoot(thread);
    // java.lang.Class
    profiler.writeFakeClassDump(HeapProfiler::FakeClass::kJavaLangClass,
                                HeapProfiler::kJavaLangClass,
                                HeapProfiler::FakeClass::kJavaLangObject);
    // java.lang.ClassLoader
    profiler.writeFakeClassDump(HeapProfiler::FakeClass::kJavaLangClassLoader,
                                HeapProfiler::kJavaLangClassLoader,
                                HeapProfiler::FakeClass::kJavaLangObject);
    // java.lang.Object
    profiler.writeFakeClassDump(HeapProfiler::FakeClass::kJavaLangObject,
                                HeapProfiler::kJavaLangObject,
                                static_cast<HeapProfiler::FakeClass>(0x0));
    // java.lang.String
    profiler.writeFakeClassDump(HeapProfiler::FakeClass::kJavaLangString,
                                HeapProfiler::kJavaLangString,
                                HeapProfiler::FakeClass::kJavaLangObject);
    // byte[]
    profiler.writeFakeClassDump(HeapProfiler::FakeClass::kByteArray,
                                HeapProfiler::kByteArrayClassName,
                                HeapProfiler::FakeClass::kJavaLangObject);
    // double[]
    profiler.writeFakeClassDump(HeapProfiler::FakeClass::kDoubleArray,
                                HeapProfiler::kDoubleArrayClassName,
                                HeapProfiler::FakeClass::kJavaLangObject);
    // long[]
    profiler.writeFakeClassDump(HeapProfiler::FakeClass::kLongArray,
                                HeapProfiler::kLongArrayClassName,
                                HeapProfiler::FakeClass::kJavaLangObject);
    // java.lang.Object[]
    profiler.writeFakeClassDump(HeapProfiler::FakeClass::kObjectArray,
                                HeapProfiler::kObjectArrayClassName,
                                HeapProfiler::FakeClass::kJavaLangObject);

    Runtime* runtime = thread->runtime();

    HeapProfilerRootVisitor root_visitor(&profiler);
    runtime->visitRoots(&root_visitor);

    HeapProfilerObjectVisitor object_visitor(&profiler);
    runtime->heap()->visitAllObjects(&object_visitor);

    profiler.writeImmediates();
    profiler.clearRecord();
  }
  profiler.writeHeapDumpEnd();
  int result = File::close(fd);
  CHECK(result == 0, "could not close file '%s'", filename);
  return NoneType::object();
}

}  // namespace py
