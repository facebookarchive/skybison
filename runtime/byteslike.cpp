#include "byteslike.h"

#include "array-module.h"
#include "handles.h"
#include "runtime.h"
#include "unicode.h"

namespace py {

Byteslike::Byteslike(HandleScope* scope, Thread* thread, RawObject object)
    : d_{} {
  if (object.isSmallBytes()) {
    RawSmallBytes small_bytes = SmallBytes::cast(object);
    initWithSmallData(small_bytes, small_bytes.length());
    return;
  }
  if (object.isLargeBytes()) {
    RawLargeBytes bytes = LargeBytes::cast(object);
    initWithLargeBytes(scope, bytes, bytes.length());
    return;
  }
  if (object.isMemoryView()) {
    RawMemoryView memory_view = MemoryView::cast(object);
    RawObject buffer = memory_view.buffer();
    word length = memory_view.length();
    word start = memory_view.start();
    if (buffer.isLargeBytes()) {
      if (start != 0) {
        UNIMPLEMENTED("non-zero start on DataArray not supported yet");
      }
      initWithLargeBytes(scope, LargeBytes::cast(buffer), length);
      return;
    }
    if (buffer.isPointer()) {
      byte* data = static_cast<byte*>(Pointer::cast(buffer).cptr()) + start;
      initWithMemory(data, length);
      return;
    }
    if (buffer.isSmallBytes()) {
      initWithSmallData(SmallBytes::cast(buffer), length);
      d_.small.reference += start;
      return;
    }
    UNIMPLEMENTED("TODO memoryview from C extension object?");
  }
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfBytearray(object)) {
    RawBytearray bytearray = object.rawCast<RawBytearray>();
    initWithLargeBytes(scope, MutableBytes::cast(bytearray.items()),
                       bytearray.numItems());
    return;
  }
  if (runtime->isInstanceOfBytes(object)) {
    RawObject bytes = object.rawCast<RawUserBytesBase>().value();
    if (bytes.isImmediateObjectNotSmallInt()) {
      RawSmallBytes small_bytes = SmallBytes::cast(bytes);
      initWithSmallData(small_bytes, small_bytes.length());
      return;
    }
    RawLargeBytes large_bytes = LargeBytes::cast(bytes);
    initWithLargeBytes(scope, large_bytes, large_bytes.length());
    return;
  }
  if (runtime->isInstanceOfArray(object)) {
    RawArray array = object.rawCast<RawArray>();
    word length = arrayByteLength(array);
    initWithLargeBytes(scope, MutableBytes::cast(array.buffer()), length);
    return;
  }
  DCHECK(!runtime->isByteslike(object), "expected non-byteslike");
  d_.handle.object = Error::error();
  next_ = nullptr;
}

inline void Byteslike::initWithLargeBytes(HandleScope* scope,
                                          RawLargeBytes bytes, word length) {
  static_assert(sizeof(Byteslike) == sizeof(Object) + sizeof(length_),
                "size mismatch");

  DCHECK_BOUND(length, bytes.length());
  Handles* handles = scope->handles();
  d_.handle.object = bytes;
  d_.handle.handles = handles;
  Handle<RawObject>* as_handle = reinterpret_cast<Handle<RawObject>*>(this);
  next_ = handles->push(as_handle);
  length_ = length;
}

inline void Byteslike::initWithMemory(byte* data, word length) {
  // Add `kHeapObjectTag` to the pointer. This mirrors the way references into
  // the managed heap work (compare with `RawHeapObject::fromAddress`) so we can
  // use the same code to access non-managed and managed memory.
  d_.reference = reinterpret_cast<uword>(data) + Object::kHeapObjectTag;
  next_ = nullptr;
  length_ = length;
}

inline void Byteslike::initWithSmallData(RawSmallBytes bytes, word length) {
  d_.small.small_storage = bytes;
  const byte* data = smallDataData(&d_.small.small_storage);
  // Add `kHeapObjectTag` to the pointer. This mirrors the way references into
  // the managed heap work (compare with `RawHeapObject::fromAddress`) so we can
  // use the same code to access non-managed and managed memory.
  d_.small.reference = reinterpret_cast<uword>(data) + Object::kHeapObjectTag;
  next_ = nullptr;
  length_ = length;
}

RawObject byteslikeReprSmartQuotes(Thread* thread, const Byteslike& byteslike) {
  // Precalculate the length of the result to minimize allocation.
  word length = byteslike.length();
  word num_single_quotes = 0;
  bool has_double_quotes = false;
  word result_length = length + 3;  // b''
  for (word i = 0; i < length; i++) {
    byte current = byteslike.byteAt(i);
    switch (current) {
      case '\'':
        num_single_quotes++;
        break;
      case '"':
        has_double_quotes = true;
        break;
      case '\t':
      case '\n':
      case '\r':
      case '\\':
        result_length++;
        break;
      default:
        if (!ASCII::isPrintable(current)) {
          result_length += 3;
        }
    }
  }

  byte delimiter = '\'';
  if (num_single_quotes > 0) {
    if (has_double_quotes) {
      result_length += num_single_quotes;
    } else {
      delimiter = '"';
    }
  }

  return thread->runtime()->byteslikeRepr(thread, byteslike, result_length,
                                          delimiter);
}

}  // namespace py
