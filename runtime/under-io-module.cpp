#include "under-io-module.h"

#include "builtins.h"
#include "bytes-builtins.h"
#include "byteslike.h"
#include "file.h"
#include "frame.h"
#include "globals.h"
#include "int-builtins.h"
#include "modules.h"
#include "object-builtins.h"
#include "objects.h"
#include "os.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"
#include "type-builtins.h"
#include "unicode.h"

namespace py {

RawObject FUNC(_io, _BytesIO_guard)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBytesIO(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(BytesIO));
  }
  return NoneType::object();
}

RawObject FUNC(_io, _BytesIO_closed_guard)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfBytesIO(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(BytesIO));
  }
  BytesIO self(&scope, *self_obj);
  if (self.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  return NoneType::object();
}

RawObject FUNC(_io, _BytesIO_seek)(Thread* thread, Arguments args) {
  HandleScope scope(thread);

  Runtime* runtime = thread->runtime();
  Object offset_obj(&scope, args.get(1));
  if (!runtime->isInstanceOfInt(*offset_obj)) {
    return Unbound::object();
  }

  Object whence_obj(&scope, args.get(2));
  if (!runtime->isInstanceOfInt(*whence_obj)) {
    return Unbound::object();
  }

  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBytesIO(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(BytesIO));
  }
  BytesIO self(&scope, *self_obj);
  if (self.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }

  Int offset_int(&scope, intUnderlying(*offset_obj));
  word offset = offset_int.asWordSaturated();
  if (!SmallInt::isValid(offset)) {
    return thread->raiseWithFmt(
        LayoutId::kOverflowError,
        "cannot fit offset into an index-sized integer");
  }
  Int whence_int(&scope, intUnderlying(*whence_obj));
  word whence = whence_int.asWordSaturated();
  word result;
  switch (whence) {
    case 0:
      if (offset < 0) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "Negative seek value %d", offset);
      }
      self.setPos(offset);
      return SmallInt::fromWord(offset);
    case 1:
      result = Utils::maximum(word{0}, self.pos() + offset);
      self.setPos(result);
      return SmallInt::fromWord(result);
    case 2:
      result = Utils::maximum(
          word{0}, MutableBytes::cast(self.buffer()).length() + offset);
      self.setPos(result);
      return SmallInt::fromWord(result);
    default:
      if (SmallInt::isValid(whence)) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "Invalid whence (%w, should be 0, 1 or 2)",
                                    whence);
      }
      return thread->raiseWithFmt(LayoutId::kOverflowError,
                                  "Python int too large to convert to C long");
  }
}

RawObject FUNC(_io, _BytesIO_truncate)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytesIO(*self)) {
    return thread->raiseRequiresType(self, ID(BytesIO));
  }
  BytesIO bytes_io(&scope, *self);
  if (bytes_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  Object size_obj(&scope, args.get(1));
  word size;
  if (size_obj.isNoneType()) {
    size = bytes_io.pos();
  } else {
    if (size_obj.isError()) return *size_obj;
    Int size_int(&scope, intUnderlying(*size_obj));
    // Allow SmallInt, Bool, and subclasses of Int containing SmallInt or Bool
    if (!size_int.isSmallInt() && !size_int.isBool()) {
      return thread->raiseWithFmt(LayoutId::kOverflowError,
                                  "cannot fit '%T' into an index-sized integer",
                                  &size_int);
    }
    size = size_int.asWord();
    if (size < 0) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "negative size value %d", size);
    }
  }
  if (size < bytes_io.numItems()) {
    bytes_io.setNumItems(size);
    bytes_io.setPos(size);
  }
  return SmallInt::fromWord(size);
}

RawObject FUNC(_io, _StringIO_closed_guard)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStringIO(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(StringIO));
  }
  StringIO self(&scope, *self_obj);
  if (self.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  return NoneType::object();
}

RawObject FUNC(_io, _StringIO_seek)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object offset_obj(&scope, args.get(1));
  Object whence_obj(&scope, args.get(2));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfInt(*offset_obj) ||
      !runtime->isInstanceOfInt(*whence_obj)) {
    return Unbound::object();
  }
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfStringIO(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(StringIO));
  }
  StringIO self(&scope, *self_obj);
  if (self.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  word offset = intUnderlying(*offset_obj).asWordSaturated();
  if (!SmallInt::isValid(offset)) {
    return thread->raiseWithFmt(
        LayoutId::kOverflowError,
        "cannot fit offset into an index-sized integer");
  }
  if (!runtime->isInstanceOfInt(*whence_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Invalid whence (should be 0, 1 or 2)");
  }
  word whence = intUnderlying(*whence_obj).asWordSaturated();
  switch (whence) {
    case 0:
      if (offset < 0) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "Negative seek position %d", offset);
      }
      self.setPos(offset);
      return *offset_obj;
    case 1:
      if (offset != 0) {
        return thread->raiseWithFmt(LayoutId::kOSError,
                                    "Can't do nonzero cur-relative seeks");
      }
      return SmallInt::fromWord(self.pos());
    case 2: {
      if (offset != 0) {
        return thread->raiseWithFmt(LayoutId::kOSError,
                                    "Can't do nonzero end-relative seeks");
      }
      word new_pos = MutableBytes::cast(self.buffer()).length();
      self.setPos(new_pos);
      return SmallInt::fromWord(new_pos);
    }
    default:
      if (SmallInt::isValid(whence)) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "Invalid whence (%w, should be 0, 1 or 2)",
                                    whence);
      } else {
        return thread->raiseWithFmt(
            LayoutId::kOverflowError,
            "Python int too large to convert to C long");
      }
  }
}

static RawObject initReadBuf(Thread* thread,
                             const BufferedReader& buffered_reader) {
  HandleScope scope(thread);
  word buffer_size = buffered_reader.bufferSize();
  MutableBytes read_buf(
      &scope, thread->runtime()->newMutableBytesUninitialized(buffer_size));
  buffered_reader.setReadBuf(*read_buf);
  buffered_reader.setReadPos(0);
  buffered_reader.setBufferNumBytes(0);
  return *read_buf;
}

// If there is no buffer allocated yet, allocate one. If there are remaining
// bytes in the buffer, move them to position 0; Set buffer read position to 0.
static RawObject rewindOrInitReadBuf(Thread* thread,
                                     const BufferedReader& buffered_reader) {
  HandleScope scope(thread);
  Object read_buf_obj(&scope, buffered_reader.readBuf());
  word read_pos = buffered_reader.readPos();
  if (read_pos > 0) {
    MutableBytes read_buf(&scope, *read_buf_obj);
    word buffer_num_bytes = buffered_reader.bufferNumBytes();
    read_buf.replaceFromWithStartAt(0, *read_buf, buffer_num_bytes - read_pos,
                                    read_pos);
    buffered_reader.setBufferNumBytes(buffer_num_bytes - read_pos);
    buffered_reader.setReadPos(0);
    return *read_buf;
  }
  if (read_buf_obj.isNoneType()) {
    return initReadBuf(thread, buffered_reader);
  }
  return *read_buf_obj;
}

// Perform one read operation to re-fill the buffer.
static RawObject fillBuffer(Thread* thread, const Object& raw_file,
                            const MutableBytes& buffer,
                            word* buffer_num_bytes) {
  HandleScope scope(thread);
  word buffer_size = buffer.length();
  word wanted = buffer_size - *buffer_num_bytes;
  Object wanted_int(&scope, SmallInt::fromWord(wanted));
  Object result_obj(&scope,
                    thread->invokeMethod2(raw_file, ID(read), wanted_int));
  if (result_obj.isError()) {
    if (result_obj.isErrorException()) return *result_obj;
    if (result_obj.isErrorNotFound()) {
      if (raw_file.isNoneType()) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "raw stream has been detached");
      }
      Object name(&scope, thread->runtime()->symbols()->at(ID(read)));
      return objectRaiseAttributeError(thread, raw_file, name);
    }
  }
  if (result_obj.isNoneType()) return NoneType::object();

  Runtime* runtime = thread->runtime();
  Bytes bytes(&scope, Bytes::empty());
  word length;
  if (runtime->isInstanceOfBytes(*result_obj)) {
    bytes = bytesUnderlying(*result_obj);
    length = bytes.length();
  } else if (runtime->isInstanceOfBytearray(*result_obj)) {
    Bytearray byte_array(&scope, *result_obj);
    bytes = byte_array.items();
    length = byte_array.numItems();
  } else if (runtime->isByteslike(*result_obj)) {
    UNIMPLEMENTED("byteslike");
  } else {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "read() should return bytes");
  }
  if (length == 0) return Bytes::empty();
  if (length > wanted && wanted != -1) {
    UNIMPLEMENTED("read() returned too many bytes");
  }
  buffer.replaceFromWithBytes(*buffer_num_bytes, *bytes, length);
  *buffer_num_bytes += length;
  return Unbound::object();
}

// Helper function for read requests that are bigger (or close to) than the size
// of the buffer.
static RawObject readBig(Thread* thread, const BufferedReader& buffered_reader,
                         word num_bytes) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  word available = buffered_reader.bufferNumBytes() - buffered_reader.readPos();
  DCHECK(num_bytes == kMaxWord || num_bytes > available,
         "num_bytes should be big");

  // TODO(T59000373): We could specialize this to avoid the intermediate
  // allocations when the size of the result is known and `readinto` is
  // available.

  word length = available;
  Object chunks(&scope, NoneType::object());
  Object chunk(&scope, NoneType::object());
  Object raw_file(&scope, buffered_reader.underlying());
  Bytes bytes(&scope, Bytes::empty());
  for (;;) {
    word wanted = (num_bytes == kMaxWord) ? 32 * kKiB : num_bytes - available;
    Object wanted_int(&scope, SmallInt::fromWord(wanted));
    Object result_obj(&scope,
                      thread->invokeMethod2(raw_file, ID(read), wanted_int));
    if (result_obj.isError()) {
      if (result_obj.isErrorException()) return *result_obj;
      if (result_obj.isErrorNotFound()) {
        if (raw_file.isNoneType()) {
          return thread->raiseWithFmt(LayoutId::kValueError,
                                      "raw stream has been detached");
        }
        Object name(&scope, runtime->symbols()->at(ID(read)));
        return objectRaiseAttributeError(thread, raw_file, name);
      }
    }
    if (result_obj.isNoneType()) {
      if (length == 0) return NoneType::object();
      break;
    }

    word chunk_length;
    if (runtime->isInstanceOfBytes(*result_obj)) {
      bytes = bytesUnderlying(*result_obj);
      chunk = *bytes;
      chunk_length = bytes.length();
    } else if (runtime->isInstanceOfBytearray(*result_obj)) {
      Bytearray byte_array(&scope, *result_obj);
      bytes = byte_array.items();
      chunk = *byte_array;
      chunk_length = byte_array.numItems();
    } else if (runtime->isByteslike(*result_obj)) {
      UNIMPLEMENTED("byteslike");
    } else {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "read() should return bytes");
    }

    if (chunk_length == 0) {
      if (length == 0) return *chunk;
      break;
    }
    if (chunk_length > wanted) {
      UNIMPLEMENTED("read() returned too many bytes");
    }

    if (chunks.isNoneType()) {
      chunks = runtime->newList();
    }
    List list(&scope, *chunks);
    runtime->listAdd(thread, list, chunk);

    length += chunk_length;
    if (num_bytes != kMaxWord) {
      num_bytes -= chunk_length;
      if (num_bytes <= 0) break;
    }
  }

  MutableBytes result(&scope, runtime->newMutableBytesUninitialized(length));
  word idx = 0;
  if (available > 0) {
    result.replaceFromWithStartAt(idx,
                                  MutableBytes::cast(buffered_reader.readBuf()),
                                  available, buffered_reader.readPos());
    idx += available;
    buffered_reader.setReadPos(0);
    buffered_reader.setBufferNumBytes(0);
  }
  if (!chunks.isNoneType()) {
    List list(&scope, *chunks);
    for (word i = 0, num_items = list.numItems(); i < num_items; i++) {
      chunk = list.at(i);
      word chunk_length;
      if (chunk.isBytes()) {
        bytes = *chunk;
        chunk_length = bytes.length();
      } else {
        Bytearray byte_array(&scope, *chunk);
        bytes = byte_array.items();
        chunk_length = byte_array.numItems();
      }
      result.replaceFromWithBytes(idx, *bytes, chunk_length);
      idx += chunk_length;
    }
  }
  DCHECK(idx == length, "mismatched length");
  return result.becomeImmutable();
}

RawObject FUNC(_io, _buffered_reader_clear_buffer)(Thread* thread,
                                                   Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBufferedReader(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(BufferedReader));
  }
  BufferedReader self(&scope, *self_obj);
  self.setReadPos(0);
  self.setBufferNumBytes(0);
  return NoneType::object();
}

RawObject FUNC(_io, _buffered_reader_init)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBufferedReader(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(BufferedReader));
  }
  BufferedReader self(&scope, *self_obj);

  Int buffer_size_obj(&scope, intUnderlying(args.get(1)));
  if (!buffer_size_obj.isSmallInt() && !buffer_size_obj.isBool()) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit value into an index-sized integer");
  }
  word buffer_size = buffer_size_obj.asWord();
  DCHECK(buffer_size > 0, "invalid buffer size");

  self.setBufferSize(buffer_size);
  self.setReadPos(0);
  self.setBufferNumBytes(0);
  // readBuf() starts out as `None` and is initialized lazily so patterns like
  // just doing a single `read()` on the whole buffered reader will not even
  // bother allocating the read buffer. There may however be already a
  // `_read_buf` allocated previously when `_init` is used to clear the buffer
  // as part of `seek`.
  if (!self.readBuf().isNoneType() &&
      MutableBytes::cast(self.readBuf()).length() != buffer_size) {
    return thread->raiseWithFmt(LayoutId::kValueError, "length mismatch");
  }
  return NoneType::object();
}

RawObject FUNC(_io, _buffered_reader_peek)(Thread* thread, Arguments args) {
  // TODO(T58490915): Investigate what thread safety guarantees python has,
  // and add locking code as necessary.
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBufferedReader(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(BufferedReader));
  }
  BufferedReader self(&scope, *self_obj);

  Object num_bytes_obj(&scope, args.get(1));
  // TODO(T59004416) Is there a way to push intFromIndex() towards managed?
  Object num_bytes_int_obj(&scope, intFromIndex(thread, num_bytes_obj));
  if (num_bytes_int_obj.isErrorException()) return *num_bytes_int_obj;
  Int num_bytes_int(&scope, intUnderlying(*num_bytes_int_obj));
  if (!num_bytes_int.isSmallInt() && !num_bytes_int.isBool()) {
    return thread->raiseWithFmt(LayoutId::kOverflowError,
                                "cannot fit value into an index-sized integer");
  }
  word num_bytes = num_bytes_int.asWord();

  word buffer_num_bytes = self.bufferNumBytes();
  word read_pos = self.readPos();
  Object read_buf_obj(&scope, self.readBuf());
  word available = buffer_num_bytes - read_pos;
  if (num_bytes <= 0 || num_bytes > available) {
    // Perform a lightweight "reset" of the read buffer that does not move data
    // around.
    if (read_buf_obj.isNoneType()) {
      read_buf_obj = initReadBuf(thread, self);
    } else if (available == 0) {
      buffer_num_bytes = 0;
      read_pos = 0;
      self.setReadPos(0);
      self.setBufferNumBytes(0);
    }
    // Attempt a single read to fill the buffer.
    MutableBytes read_buf(&scope, *read_buf_obj);
    Object raw_file(&scope, self.underlying());
    Object fill_result(
        &scope, fillBuffer(thread, raw_file, read_buf, &buffer_num_bytes));
    if (fill_result.isErrorException()) return *fill_result;
    self.setBufferNumBytes(buffer_num_bytes);
    available = buffer_num_bytes - read_pos;
  }

  Bytes read_buf(&scope, *read_buf_obj);
  return bytesSubseq(thread, read_buf, read_pos, available);
}

RawObject FUNC(_io, _buffered_reader_read)(Thread* thread, Arguments args) {
  // TODO(T58490915): Investigate what thread safety guarantees python has,
  // and add locking code as necessary.
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBufferedReader(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(BufferedReader));
  }
  BufferedReader self(&scope, *self_obj);

  Object num_bytes_obj(&scope, args.get(1));
  word num_bytes;
  if (num_bytes_obj.isNoneType()) {
    num_bytes = kMaxWord;
  } else {
    // TODO(T59004416) Is there a way to push intFromIndex() towards managed?
    Object num_bytes_int_obj(&scope, intFromIndex(thread, num_bytes_obj));
    if (num_bytes_int_obj.isErrorException()) return *num_bytes_int_obj;
    Int num_bytes_int(&scope, intUnderlying(*num_bytes_int_obj));
    if (!num_bytes_int.isSmallInt() && !num_bytes_int.isBool()) {
      return thread->raiseWithFmt(
          LayoutId::kOverflowError,
          "cannot fit value into an index-sized integer");
    }
    num_bytes = num_bytes_int.asWord();
    if (num_bytes == -1) {
      num_bytes = kMaxWord;
    } else if (num_bytes < 0) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "read length must be non-negative or -1");
    }
  }

  word buffer_num_bytes = self.bufferNumBytes();
  word read_pos = self.readPos();

  word available = buffer_num_bytes - read_pos;
  DCHECK(available >= 0, "invalid state");
  if (num_bytes <= available) {
    word new_read_pos = read_pos + num_bytes;
    self.setReadPos(new_read_pos);
    Bytes read_buf(&scope, self.readBuf());
    return bytesSubseq(thread, read_buf, read_pos, num_bytes);
  }

  Object raw_file(&scope, self.underlying());
  if (num_bytes == kMaxWord) {
    Object readall_result(&scope, thread->invokeMethod1(raw_file, ID(readall)));
    if (readall_result.isErrorException()) return *readall_result;
    if (!readall_result.isErrorNotFound()) {
      Bytes bytes(&scope, Bytes::empty());
      word bytes_length;
      if (readall_result.isNoneType()) {
        if (available == 0) return NoneType::object();
        bytes_length = 0;
      } else if (runtime->isInstanceOfBytes(*readall_result)) {
        bytes = bytesUnderlying(*readall_result);
        bytes_length = bytes.length();
      } else if (runtime->isInstanceOfBytearray(*readall_result)) {
        Bytearray byte_array(&scope, *readall_result);
        bytes = byte_array.items();
        bytes_length = byte_array.numItems();
      } else if (runtime->isByteslike(*readall_result)) {
        UNIMPLEMENTED("byteslike");
      } else {
        return thread->raiseWithFmt(LayoutId::kTypeError,
                                    "readall() should return bytes");
      }
      word length = bytes_length + available;
      if (length == 0) return Bytes::empty();
      MutableBytes result(&scope,
                          runtime->newMutableBytesUninitialized(length));
      word idx = 0;
      if (available > 0) {
        result.replaceFromWithStartAt(idx, MutableBytes::cast(self.readBuf()),
                                      available, read_pos);
        idx += available;
        self.setReadPos(0);
        self.setBufferNumBytes(0);
      }
      if (bytes_length > 0) {
        result.replaceFromWithBytes(idx, *bytes, bytes_length);
        idx += bytes_length;
      }
      DCHECK(idx == length, "length mismatch");
      return result.becomeImmutable();
    }
  }

  // Use alternate reading code for big requests where buffering would not help.
  // (This is also used for the num_bytes==kMaxWord (aka "readall") case when
  // the file object does not provide a "readall" method.
  word buffer_size = self.bufferSize();
  if (num_bytes > (buffer_size / 2)) {
    return readBig(thread, self, num_bytes);
  }

  // Fill buffer until we have enough bytes available.
  MutableBytes read_buf(&scope, rewindOrInitReadBuf(thread, self));
  buffer_num_bytes = self.bufferNumBytes();
  Object fill_result(&scope, NoneType::object());
  do {
    fill_result = fillBuffer(thread, raw_file, read_buf, &buffer_num_bytes);
    if (fill_result.isErrorException()) return *fill_result;
    if (!fill_result.isUnbound()) {
      if (buffer_num_bytes == 0) return *fill_result;
      break;
    }
  } while (buffer_num_bytes < num_bytes);

  word length = Utils::minimum(buffer_num_bytes, num_bytes);
  self.setBufferNumBytes(buffer_num_bytes);
  self.setReadPos(length);
  Bytes read_buf_bytes(&scope, *read_buf);
  return bytesSubseq(thread, read_buf_bytes, 0, length);
}

RawObject FUNC(_io, _buffered_reader_readline)(Thread* thread, Arguments args) {
  // TODO(T58490915): Investigate what thread safety guarantees Python has,
  // and add locking code as necessary.
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfBufferedReader(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(BufferedReader));
  }
  BufferedReader self(&scope, *self_obj);

  Object max_line_bytes_obj(&scope, args.get(1));
  word max_line_bytes = kMaxWord;
  if (!max_line_bytes_obj.isNoneType()) {
    // TODO(T59004416) Is there a way to push intFromIndex() towards managed?
    Object max_line_bytes_int_obj(&scope,
                                  intFromIndex(thread, max_line_bytes_obj));
    if (max_line_bytes_int_obj.isErrorException()) {
      return *max_line_bytes_int_obj;
    }
    Int max_line_bytes_int(&scope, intUnderlying(*max_line_bytes_int_obj));
    if (!max_line_bytes_int.isSmallInt() && !max_line_bytes_int.isBool()) {
      return thread->raiseWithFmt(
          LayoutId::kOverflowError,
          "cannot fit value into an index-sized integer");
    }
    max_line_bytes = max_line_bytes_int.asWord();
    if (max_line_bytes == -1) {
      max_line_bytes = kMaxWord;
    } else if (max_line_bytes < 0) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "read length must be non-negative or -1");
    }
  }

  word buffer_num_bytes = self.bufferNumBytes();
  word read_pos = self.readPos();
  word available = buffer_num_bytes - read_pos;
  if (available > 0) {
    MutableBytes read_buf(&scope, self.readBuf());
    word line_end = -1;
    word scan_length = available;
    if (available >= max_line_bytes) {
      scan_length = max_line_bytes;
      line_end = read_pos + max_line_bytes;
    } else {
      max_line_bytes -= available;
    }
    word newline_index = read_buf.findByte('\n', read_pos, scan_length);
    if (newline_index >= 0) {
      line_end = newline_index + 1;
    }
    if (line_end >= 0) {
      self.setReadPos(line_end);
      Bytes read_buf_bytes(&scope, *read_buf);
      return bytesSubseq(thread, read_buf_bytes, read_pos, line_end - read_pos);
    }
  }

  MutableBytes read_buf(&scope, rewindOrInitReadBuf(thread, self));
  buffer_num_bytes = self.bufferNumBytes();
  word buffer_size = self.bufferSize();

  Object raw_file(&scope, self.underlying());
  Object fill_result(&scope, NoneType::object());
  Object chunks(&scope, NoneType::object());
  word line_end = -1;
  // Outer loop in case for case where a line is longer than a single buffer. In
  // that case we will collect the pieces in the `chunks` list.
  for (;;) {
    // Fill buffer until we find a newline character or filled up the whole
    // buffer.
    do {
      word old_buffer_num_bytes = buffer_num_bytes;
      fill_result = fillBuffer(thread, raw_file, read_buf, &buffer_num_bytes);
      if (fill_result.isErrorException()) return *fill_result;
      if (!fill_result.isUnbound()) {
        if (buffer_num_bytes == 0 && chunks.isNoneType()) return *fill_result;
        line_end = buffer_num_bytes;
        break;
      }

      word scan_start = old_buffer_num_bytes;
      word scan_length = buffer_num_bytes - old_buffer_num_bytes;
      if (scan_length >= max_line_bytes) {
        scan_length = max_line_bytes;
        line_end = scan_start + max_line_bytes;
      } else {
        max_line_bytes -= buffer_num_bytes - old_buffer_num_bytes;
      }
      word newline_index = read_buf.findByte('\n', scan_start, scan_length);
      if (newline_index >= 0) {
        line_end = newline_index + 1;
        break;
      }
    } while (line_end < 0 && buffer_num_bytes < buffer_size);

    if (line_end < 0) {
      // The line is longer than the buffer: Add the current buffer to the
      // chunks list, create a fresh one and repeat scan loop.
      if (chunks.isNoneType()) {
        chunks = runtime->newList();
      }
      List list(&scope, *chunks);
      runtime->listAdd(thread, list, read_buf);

      // Create a fresh buffer and retry.
      read_buf = initReadBuf(thread, self);
      buffer_num_bytes = 0;
      continue;
    }
    break;
  }

  word length = line_end;
  if (!chunks.isNoneType()) {
    List list(&scope, *chunks);
    for (word i = 0, num_items = list.numItems(); i < num_items; i++) {
      length += MutableBytes::cast(list.at(i)).length();
    }
  }
  MutableBytes result(&scope, runtime->newMutableBytesUninitialized(length));
  word idx = 0;
  if (!chunks.isNoneType()) {
    List list(&scope, *chunks);
    Bytes chunk(&scope, Bytes::empty());
    for (word i = 0, num_items = list.numItems(); i < num_items; i++) {
      chunk = list.at(i);
      word chunk_length = chunk.length();
      result.replaceFromWithBytes(idx, *chunk, chunk_length);
      idx += chunk_length;
    }
  }
  result.replaceFromWith(idx, *read_buf, line_end);
  DCHECK(idx + line_end == length, "length mismatch");
  self.setReadPos(line_end);
  self.setBufferNumBytes(buffer_num_bytes);
  return result.becomeImmutable();
}

RawObject FUNC(_io, _TextIOWrapper_attached_guard)(Thread* thread,
                                                   Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfTextIOWrapper(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(TextIOWrapper));
  }
  TextIOWrapper self(&scope, *self_obj);
  if (self.detached()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "underlying buffer has been detached");
  }
  return NoneType::object();
}

RawObject FUNC(_io, _TextIOWrapper_attached_closed_guard)(Thread* thread,
                                                          Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTextIOWrapper(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(TextIOWrapper));
  }
  TextIOWrapper self(&scope, *self_obj);
  if (self.detached()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "underlying buffer has been detached");
  }
  Object buffer_obj(&scope, self.buffer());
  if (runtime->isInstanceOfBufferedReader(*buffer_obj)) {
    BufferedReader buffer(&scope, *buffer_obj);
    if (buffer.closed()) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "I/O operation on closed file.");
    }
    return NoneType::object();
  }

  if (runtime->isInstanceOfBufferedWriter(*buffer_obj)) {
    BufferedWriter buffer(&scope, *buffer_obj);
    if (!buffer.closed()) {
      return NoneType::object();
    }
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  // TODO(T61927696): Add closed check support for other types of buffer
  return Unbound::object();
}

RawObject FUNC(_io,
               _TextIOWrapper_attached_closed_seekable_guard)(Thread* thread,
                                                              Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfTextIOWrapper(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(TextIOWrapper));
  }
  TextIOWrapper self(&scope, *self_obj);
  if (self.detached()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "underlying buffer has been detached");
  }
  Object buffer_obj(&scope, self.buffer());
  if (runtime->isInstanceOfBufferedReader(*buffer_obj)) {
    BufferedReader buffer(&scope, *buffer_obj);
    if (buffer.closed()) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "I/O operation on closed file.");
    }
    // TODO(T61927696): change this when TextIOWrapper.seekable() returns bool
    Object seekable_obj(&scope, self.seekable());
    if (seekable_obj.isNoneType() || seekable_obj == Bool::falseObj()) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "underlying stream is not seekable");
    }
    return NoneType::object();
  }

  if (runtime->isInstanceOfBufferedWriter(*buffer_obj)) {
    BufferedWriter buffer(&scope, *buffer_obj);
    if (!buffer.closed()) {
      // TODO(T61927696): change this when TextIOWrapper.seekable() returns bool
      Object seekable_obj(&scope, self.seekable());
      if (seekable_obj.isNoneType() || seekable_obj == Bool::falseObj()) {
        return thread->raiseWithFmt(LayoutId::kValueError,
                                    "underlying stream is not seekable");
      }
      return NoneType::object();
    }
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }

  // TODO(T61927696): Add closed check support for other types of buffer
  return Unbound::object();
}

// Copy the bytes of a UTF-8 encoded string with no surrogates to the write
// buffer (a Bytearray) of underlying Bufferedwriter of TextIOWrapper
// If the length of write buffer will be larger than
// BufferedWriter.bufferSize(), return Unbound to escape to managed code and
// call BufferedWriter.write()
// If the newline is "\r\n", return Unbound to use managed code
// If text_io.lineBuffering() or haslf or "\r" in text, return Unbound to
// managed code to use flush()
// TODO(T61927696): Implement native version of BufferedWriter._flush_unlocked()
// with FileIO as BufferedWriter.raw. With that function, we can do flush in
// here.
RawObject FUNC(_io, _TextIOWrapper_write_UTF8)(Thread* thread, Arguments args) {
  HandleScope scope(thread);

  Object text_obj(&scope, args.get(1));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStr(*text_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "write() argument must be str, not %T",
                                &text_obj);
  }

  Object self_obj(&scope, args.get(0));
  if (!runtime->isInstanceOfTextIOWrapper(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(TextIOWrapper));
  }
  TextIOWrapper text_io(&scope, *self_obj);
  if (text_io.detached()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "underlying buffer has been detached");
  }

  Object buffer_obj(&scope, text_io.buffer());
  if (!buffer_obj.isBufferedWriter()) {
    return Unbound::object();
  }
  BufferedWriter buffer(&scope, text_io.buffer());
  if (buffer.closed()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "I/O operation on closed file.");
  }

  if (Str::cast(text_io.encoding()) != SmallStr::fromCStr("UTF-8")) {
    return Unbound::object();
  }
  Str writenl(&scope, text_io.writenl());

  // Only allow writenl to be cr or lf in this short cut
  if (!text_io.writetranslate() || writenl == SmallStr::fromCStr("\r\n")) {
    return Unbound::object();
  }

  Str text(&scope, strUnderlying(*text_obj));
  word text_len = text.length();

  Bytearray write_buffer(&scope, buffer.writeBuf());
  word old_len = write_buffer.numItems();
  word new_len = old_len + text_len;
  runtime->bytearrayEnsureCapacity(thread, write_buffer, new_len);
  MutableBytes write_buffer_bytes(&scope, write_buffer.items());
  write_buffer_bytes.replaceFromWithStr(old_len, *text, text_len);
  write_buffer.setNumItems(new_len);

  int32_t codepoint;
  word num_bytes;
  bool hasnl = false;

  if (writenl == SmallStr::fromCStr("\n")) {
    for (word offset = 0; offset < text_len;) {
      codepoint = text.codePointAt(offset, &num_bytes);
      if (Unicode::isSurrogate(codepoint)) {
        write_buffer.downsize(old_len);
        return Unbound::object();
      }
      if (num_bytes == 1) {
        if (text.byteAt(offset) == '\n' || text.byteAt(offset) == '\r') {
          hasnl = true;
        }
      }
      offset += num_bytes;
    }
  } else {
    for (word offset = 0; offset < text_len;) {
      codepoint = text.codePointAt(offset, &num_bytes);
      if (Unicode::isSurrogate(codepoint)) {
        write_buffer.downsize(old_len);
        return Unbound::object();
      }
      if (num_bytes == 1) {
        if (text.byteAt(offset) == '\n') {
          hasnl = true;
          write_buffer_bytes.byteAtPut(offset + old_len, byte{'\r'});
          offset += num_bytes;
          continue;
        }
        if (text.byteAt(offset) == '\r') {
          hasnl = true;
          offset += num_bytes;
          continue;
        }
      }
      offset += num_bytes;
    }
  }

  if (text_io.lineBuffering() && hasnl) {
    // TODO(T61927696): Implement native support for
    // BufferedWriter._flush_unlocked to do flush here
    Object flush_result(&scope, thread->invokeMethod1(buffer, ID(flush)));
    if (flush_result.isErrorException()) return *flush_result;
    text_io.setTelling(text_io.seekable());
  }

  text_io.setDecodedChars(Str::empty());
  text_io.setSnapshot(NoneType::object());
  Object decoder_obj(&scope, text_io.decoder());
  if (!decoder_obj.isNoneType()) {
    Object reset_result(&scope, thread->invokeMethod1(decoder_obj, ID(reset)));
    if (reset_result.isErrorException()) return *reset_result;
  }

  return SmallInt::fromWord(text_len);
}

static const BuiltinAttribute kUnderIOBaseAttributes[] = {
    {ID(_closed), RawUnderIOBase::kClosedOffset},
};

static const BuiltinAttribute kIncrementalNewlineDecoderAttributes[] = {
    {ID(_errors), RawIncrementalNewlineDecoder::kErrorsOffset},
    {ID(_translate), RawIncrementalNewlineDecoder::kTranslateOffset},
    {ID(_decoder), RawIncrementalNewlineDecoder::kDecoderOffset},
    {ID(_seennl), RawIncrementalNewlineDecoder::kSeennlOffset},
    {ID(_pendingcr), RawIncrementalNewlineDecoder::kPendingcrOffset},
};

static const BuiltinAttribute kUnderBufferedIOMixinAttributes[] = {
    {ID(_raw), RawUnderBufferedIOMixin::kUnderlyingOffset},
};

static const BuiltinAttribute kBufferedRandomAttributes[] = {
    {ID(buffer_size), RawBufferedRandom::kBufferSizeOffset},
    {ID(_reader), RawBufferedRandom::kReaderOffset},
    {ID(_write_buf), RawBufferedRandom::kWriteBufOffset},
    {ID(_write_lock), RawBufferedRandom::kWriteLockOffset},
};

static const BuiltinAttribute kBufferedReaderAttributes[] = {
    {ID(_buffer_size), RawBufferedReader::kBufferSizeOffset,
     AttributeFlags::kReadOnly},
    {ID(_buffered_reader__read_buf), RawBufferedReader::kReadBufOffset,
     AttributeFlags::kHidden},
    {ID(_read_pos), RawBufferedReader::kReadPosOffset,
     AttributeFlags::kReadOnly},
    {ID(_buffer_num_bytes), RawBufferedReader::kBufferNumBytesOffset,
     AttributeFlags::kReadOnly},
};

static const BuiltinAttribute kBufferedWriterAttributes[] = {
    {ID(buffer_size), RawBufferedWriter::kBufferSizeOffset},
    {ID(_write_buf), RawBufferedWriter::kWriteBufOffset},
    {ID(_write_lock), RawBufferedWriter::kWriteLockOffset},
};

static const BuiltinAttribute kBytesIOAttributes[] = {
    {ID(_buffer), RawBytesIO::kBufferOffset},
    {ID(_BytesIO__num_items), RawBytesIO::kNumItemsOffset,
     AttributeFlags::kReadOnly},
    {ID(_pos), RawBytesIO::kPosOffset},
    {ID(__dict__), RawBytesIO::kDictOffset},
};

static void bytesIOEnsureCapacity(Thread* thread, const BytesIO& bytes_io,
                                  word min_capacity) {
  DCHECK_BOUND(min_capacity, SmallInt::kMaxValue);
  HandleScope scope(thread);
  MutableBytes curr_buffer(&scope, bytes_io.buffer());
  word curr_capacity = curr_buffer.length();
  if (min_capacity <= curr_capacity) return;
  word new_capacity = Runtime::newCapacity(curr_capacity, min_capacity);
  MutableBytes new_buffer(
      &scope, thread->runtime()->newMutableBytesUninitialized(new_capacity));
  new_buffer.replaceFromWith(0, *curr_buffer, curr_capacity);
  new_buffer.replaceFromWithByte(curr_capacity, 0,
                                 new_capacity - curr_capacity);
  bytes_io.setBuffer(*new_buffer);
}

RawObject METH(BytesIO, __init__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfBytesIO(*self)) {
    return thread->raiseRequiresType(self, ID(BytesIO));
  }
  BytesIO bytes_io(&scope, *self);
  Object initial_bytes(&scope, args.get(1));
  if (initial_bytes.isNoneType() || initial_bytes == Bytes::empty()) {
    bytes_io.setBuffer(runtime->emptyMutableBytes());
    bytes_io.setNumItems(0);
    bytes_io.setPos(0);
    bytes_io.setClosed(false);
    return NoneType::object();
  }

  Byteslike byteslike(&scope, thread, *initial_bytes);
  if (!byteslike.isValid()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &initial_bytes);
  }
  word byteslike_length = byteslike.length();
  MutableBytes buffer(&scope,
                      runtime->newMutableBytesUninitialized(byteslike_length));
  buffer.replaceFromWithByteslike(0, byteslike, byteslike_length);
  bytes_io.setBuffer(*buffer);
  bytes_io.setClosed(false);
  bytes_io.setNumItems(byteslike_length);
  bytes_io.setPos(0);
  return NoneType::object();
}

RawObject METH(BytesIO, getvalue)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfBytesIO(*self)) {
    return thread->raiseRequiresType(self, ID(BytesIO));
  }
  BytesIO bytes_io(&scope, *self);
  if (bytes_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  Bytes buffer(&scope, bytes_io.buffer());
  word num_items = bytes_io.numItems();
  return runtime->bytesCopyWithSize(thread, buffer, num_items);
}

RawObject METH(BytesIO, read)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfBytesIO(*self)) {
    return thread->raiseRequiresType(self, ID(BytesIO));
  }
  BytesIO bytes_io(&scope, *self);
  if (bytes_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }

  Object size_obj(&scope, args.get(1));
  MutableBytes buffer(&scope, bytes_io.buffer());

  word size;
  word buffer_len = bytes_io.numItems();
  word pos = bytes_io.pos();
  if (size_obj.isNoneType()) {
    size = buffer_len;
  } else {
    size_obj = intFromIndex(thread, size_obj);
    if (size_obj.isError()) return *size_obj;
    // Allow SmallInt, Bool, and subclasses of Int containing SmallInt or Bool
    Int size_int(&scope, intUnderlying(*size_obj));
    if (size_obj.isLargeInt()) {
      return thread->raiseWithFmt(LayoutId::kOverflowError,
                                  "cannot fit '%T' into an index-sized integer",
                                  &size_int);
    }
    if (size_int.asWord() < 0) {
      size = buffer_len;
    } else {
      size = size_int.asWord();
    }
  }
  if (buffer_len <= pos) {
    return Bytes::empty();
  }
  word new_pos = Utils::minimum(buffer_len, pos + size);
  bytes_io.setPos(new_pos);
  Bytes result(&scope, *buffer);
  return bytesSubseq(thread, result, pos, new_pos - pos);
}

RawObject METH(BytesIO, write)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfBytesIO(*self)) {
    return thread->raiseRequiresType(self, ID(BytesIO));
  }
  BytesIO bytes_io(&scope, *self);
  if (bytes_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }

  Object value_obj(&scope, args.get(1));
  Byteslike value(&scope, thread, *value_obj);
  if (!value.isValid()) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "a bytes-like object is required, not '%T'",
                                &value_obj);
  }

  word pos = bytes_io.pos();
  word value_length = value.length();
  word new_pos = pos + value_length;
  bytesIOEnsureCapacity(thread, bytes_io, new_pos);

  MutableBytes::cast(bytes_io.buffer())
      .replaceFromWithByteslike(pos, value, value_length);
  if (new_pos > bytes_io.numItems()) {
    bytes_io.setNumItems(new_pos);
  }
  bytes_io.setPos(new_pos);
  return SmallInt::fromWord(value_length);
}

static const BuiltinAttribute kFileIOAttributes[] = {
    {ID(_fd), RawFileIO::kFdOffset},
    {ID(name), RawFileIO::kNameOffset},
    {ID(_created), RawFileIO::kCreatedOffset},
    {ID(_readable), RawFileIO::kReadableOffset},
    {ID(_writable), RawFileIO::kWritableOffset},
    {ID(_appending), RawFileIO::kAppendingOffset},
    {ID(_seekable), RawFileIO::kSeekableOffset},
    {ID(_closefd), RawFileIO::kCloseFdOffset},
};

static const word kDefaultBufferSize = 1 * kKiB;  // bytes

RawObject METH(FileIO, readall)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfFileIO(*self)) {
    return thread->raiseRequiresType(self, ID(FileIO));
  }
  FileIO file_io(&scope, *self);
  if (file_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  Object fd_obj(&scope, file_io.fd());
  DCHECK(fd_obj.isSmallInt(), "fd must be small int");
  int fd = SmallInt::cast(*fd_obj).value();
  // If there is OSError from File::seek or File::size, error will not be
  // thrown. This case is handled by the for loop below.
  word pos = File::seek(fd, 0, 1);
  word end = File::size(fd);
  word buffer_size = kDefaultBufferSize;

  if (end > 0 && pos >= 0 && end >= pos) {
    buffer_size = end - pos + 1;
  }
  // OSError from getting File::seek or File::size, or end < pos
  // read buffer by buffer
  Bytearray result_array(&scope, runtime->newBytearray());
  word read_size;
  word total_len = 0;
  for (;;) {
    read_size = buffer_size;
    runtime->bytearrayEnsureCapacity(thread, result_array,
                                     total_len + buffer_size);
    byte* dst = reinterpret_cast<byte*>(
        MutableBytes::cast(result_array.items()).address());
    word result_len = File::read(fd, dst + total_len, read_size);
    if (result_len < 0) {
      return thread->raiseOSErrorFromErrno(-result_len);
    }
    total_len += result_len;
    // From the glibc manual: "If read returns at least one character, there
    // is no way you can tell whether end-of-file was reached. But if you did
    // reach the end, the next read will return zero."
    // Therefore, we can't stop when the result_len is less than read_len, as
    // we still don't know if there's more input that we're blocked on.
    if (result_len == 0) {
      if (total_len == 0) {
        return Bytes::empty();
      }
      // TODO(T70612758): Find a way to shorten the MutableBytes object without
      // extra allocation
      Bytes result_bytes(
          &scope, MutableBytes::cast(result_array.items()).becomeImmutable());
      MutableBytes result(&scope,
                          runtime->newMutableBytesUninitialized(total_len));
      dst = reinterpret_cast<byte*>(result.address());
      result_bytes.copyTo(dst, total_len);
      return result.becomeImmutable();
    }
    result_array.setNumItems(total_len);
    if (total_len == buffer_size) {
      buffer_size *= 2;
    }
  }
}

static RawObject readintoBytesAddress(Thread* thread, const int fd, byte* dst,
                                      const word dst_len) {
  if (dst_len == 0) {
    return SmallInt::fromWord(0);
  }
  word result = File::read(fd, dst, dst_len);
  if (result < 0) return thread->raiseOSErrorFromErrno(-result);
  return SmallInt::fromWord(result);
}

RawObject METH(FileIO, readinto)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfFileIO(*self)) {
    return thread->raiseRequiresType(self, ID(FileIO));
  }
  FileIO file_io(&scope, *self);
  if (file_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  Object dst_obj(&scope, args.get(1));
  if (!runtime->isByteslike(*dst_obj) && !runtime->isInstanceOfMmap(*dst_obj)) {
    return thread->raiseWithFmt(LayoutId::kTypeError,
                                "Expected bytes-like object, not %T", &dst_obj);
  }

  int fd = SmallInt::cast(file_io.fd()).value();
  if (runtime->isInstanceOfBytearray(*dst_obj)) {
    Bytearray dst_array(&scope, *dst_obj);
    return readintoBytesAddress(
        thread, fd,
        reinterpret_cast<byte*>(
            MutableBytes::cast(dst_array.items()).address()),
        dst_array.numItems());
  }
  if (dst_obj.isArray()) {
    Array array(&scope, *dst_obj);
    return readintoBytesAddress(
        thread, fd,
        reinterpret_cast<byte*>(MutableBytes::cast(array.buffer()).address()),
        array.length());
  }
  if (dst_obj.isMemoryView()) {
    MemoryView dst_memoryview(&scope, *dst_obj);
    dst_obj = dst_memoryview.buffer();
    if (runtime->isInstanceOfBytes(*dst_obj)) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError, "Expected read-write bytes-like object, not %T",
          &dst_memoryview);
    }
    Pointer dst_ptr(&scope, *dst_obj);
    return readintoBytesAddress(
        thread, fd, reinterpret_cast<byte*>(dst_ptr.cptr()), dst_ptr.length());
  }
  if (dst_obj.isMmap()) {
    Mmap dst_mmap(&scope, *dst_obj);
    if (!dst_mmap.isWritable()) {
      return thread->raiseWithFmt(
          LayoutId::kTypeError, "Expected read-write bytes-like object, not %T",
          &dst_mmap);
    }
    Pointer dst_ptr(&scope, dst_mmap.data());
    return readintoBytesAddress(
        thread, fd, reinterpret_cast<byte*>(dst_ptr.cptr()), dst_ptr.length());
  }
  // Bytes, not valid arguments for readinto
  return thread->raiseWithFmt(LayoutId::kTypeError,
                              "Expected read-write bytes-like object, not %T",
                              &dst_obj);
}

static const BuiltinAttribute kStringIOAttributes[] = {
    {ID(_buffer), RawStringIO::kBufferOffset},
    {ID(_pos), RawStringIO::kPosOffset},
    {ID(_readnl), RawStringIO::kReadnlOffset},
    {ID(_readtranslate), RawStringIO::kReadtranslateOffset},
    {ID(_readuniversal), RawStringIO::kReaduniversalOffset},
    {ID(_seennl), RawStringIO::kSeennlOffset},
    {ID(_writenl), RawStringIO::kWritenlOffset},
    {ID(_writetranslate), RawStringIO::kWritetranslateOffset},
    {ID(__dict__), RawStringIO::kDictOffset},
};

enum NewlineFound { kLF = 0x1, kCR = 0x2, kCRLF = 0x4 };

static RawObject stringIOWrite(Thread* thread, const StringIO& string_io,
                               const Str& value) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  if (*value == Str::empty()) {
    return SmallInt::fromWord(0);
  }

  Str writenl(&scope, string_io.writenl());
  bool long_writenl = writenl.length() == 2;
  byte first_writenl_char = writenl.byteAt(0);
  bool has_write_translate =
      string_io.hasWritetranslate() && first_writenl_char != '\n';
  word original_val_len = value.length();
  word val_len = original_val_len;

  // If write_translate is true, read_translate is false
  // Contrapositively, if read_translate is true, write_translate is false
  // Therefore we don't have to worry about their interactions with each other
  if (has_write_translate && long_writenl) {
    val_len += value.occurrencesOf(SmallStr::fromCStr("\n"));
  }

  word start = string_io.pos();
  word new_len = start + val_len;
  bool has_read_translate = string_io.hasReadtranslate();
  if (has_read_translate) {
    new_len -= value.occurrencesOf(SmallStr::fromCStr("\r\n"));
  }

  MutableBytes buffer(&scope, string_io.buffer());
  word old_len = buffer.length();
  if (old_len < new_len) {
    MutableBytes new_buffer(&scope,
                            runtime->newMutableBytesUninitialized(new_len));
    new_buffer.replaceFromWith(0, *buffer, old_len);
    new_buffer.replaceFromWithByte(old_len, 0, new_len - old_len);
    string_io.setBuffer(*new_buffer);
    buffer = *new_buffer;
  }

  if (has_read_translate) {
    word new_seen_nl = Int::cast(string_io.seennl()).asWord();
    for (word str_i = 0, byte_i = start; str_i < val_len; ++str_i, ++byte_i) {
      byte ch = value.byteAt(str_i);
      if (ch == '\r') {
        if (val_len > str_i + 1 && value.byteAt(str_i + 1) == '\n') {
          new_seen_nl |= NewlineFound::kCRLF;
          buffer.byteAtPut(byte_i, '\n');
          str_i++;
          continue;
        }
        new_seen_nl |= NewlineFound::kCR;
        buffer.byteAtPut(byte_i, '\n');
        continue;
      }
      if (ch == '\n') {
        new_seen_nl |= NewlineFound::kLF;
      }
      buffer.byteAtPut(byte_i, ch);
    }
    string_io.setSeennl(SmallInt::fromWord(new_seen_nl));
  } else if (has_write_translate) {
    for (word str_i = 0, byte_i = start; str_i < original_val_len;
         ++str_i, ++byte_i) {
      byte ch = value.byteAt(str_i);
      if (ch == '\n') {
        buffer.byteAtPut(byte_i, first_writenl_char);
        if (long_writenl) {
          buffer.byteAtPut(++byte_i, writenl.byteAt(1));
        }
        continue;
      }
      buffer.byteAtPut(byte_i, ch);
    }
  } else {
    buffer.replaceFromWithStr(start, *value, val_len);
  }
  string_io.setPos(new_len);
  return SmallInt::fromWord(original_val_len);
}

static bool isValidStringIONewline(const Object& newline) {
  if (newline == SmallStr::empty()) return true;
  if (newline == SmallStr::fromCodePoint('\n')) return true;
  if (newline == SmallStr::fromCodePoint('\r')) return true;
  return newline == SmallStr::fromCStr("\r\n");
}

RawObject METH(StringIO, __init__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfStringIO(*self)) {
    return thread->raiseRequiresType(self, ID(StringIO));
  }
  Object newline(&scope, args.get(2));
  if (newline != NoneType::object()) {
    if (!runtime->isInstanceOfStr(*newline)) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "newline must be str or None, not %T",
                                  &newline);
    }
    newline = strUnderlying(*newline);
    if (!isValidStringIONewline(newline)) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "illegal newline value: %S", &newline);
    }
  }
  StringIO string_io(&scope, *self);
  string_io.setBuffer(runtime->emptyMutableBytes());
  string_io.setClosed(false);
  string_io.setPos(0);
  string_io.setReadnl(*newline);
  string_io.setSeennl(SmallInt::fromWord(0));
  if (newline == NoneType::object()) {
    string_io.setReadtranslate(true);
    string_io.setReaduniversal(true);
    string_io.setWritetranslate(false);
    string_io.setWritenl(SmallStr::fromCodePoint('\n'));
  } else if (newline == Str::empty()) {
    string_io.setReadtranslate(false);
    string_io.setReaduniversal(true);
    string_io.setWritetranslate(false);
    string_io.setWritenl(SmallStr::fromCodePoint('\n'));
  } else {
    string_io.setReadtranslate(false);
    string_io.setReaduniversal(false);
    string_io.setWritetranslate(true);
    string_io.setWritenl(*newline);
  }

  Object initial_value_obj(&scope, args.get(1));
  if (initial_value_obj != NoneType::object()) {
    if (!runtime->isInstanceOfStr(*initial_value_obj)) {
      return thread->raiseWithFmt(LayoutId::kTypeError,
                                  "initial_value must be str or None, not %T",
                                  &initial_value_obj);
    }
    Str initial_value(&scope, strUnderlying(*initial_value_obj));
    stringIOWrite(thread, string_io, initial_value);
    string_io.setPos(0);
  }
  return NoneType::object();
}

static word stringIOReadline(Thread* thread, const StringIO& string_io,
                             word size) {
  HandleScope scope(thread);
  MutableBytes buffer(&scope, string_io.buffer());
  word buf_len = buffer.length();
  word start = string_io.pos();
  if (start >= buf_len) {
    return -1;
  }
  bool has_read_universal = string_io.hasReaduniversal();
  bool has_read_translate = string_io.hasReadtranslate();
  Object newline_obj(&scope, string_io.readnl());
  if (has_read_translate) {
    newline_obj = SmallStr::fromCodePoint('\n');
  }
  Str newline(&scope, *newline_obj);
  if (size < 0 || (size + start) > buf_len) {
    size = buf_len - start;
  }
  word i = start;

  if (has_read_universal) {
    const byte crlf[] = {'\r', '\n'};
    i = buffer.indexOfAny(crlf, start);
    // when this condition is met, either '\r' or '\n' is found
    if (buf_len > i) {
      // ch is the '\n' or '\r'
      byte ch = buffer.byteAt(i++);
      if (ch == '\r') {
        if (buf_len > i && buffer.byteAt(i) == '\n') {
          i++;
        }
      }
    }
  } else {
    byte first_nl_byte = newline.byteAt(0);
    while (i < start + size) {
      word index = buffer.findByte(first_nl_byte, i, (size + start - i));
      if (index == -1) {
        i += (size + start - i);
        break;
      }
      i = index + 1;
      if (buf_len >= (i + newline.length() - 1)) {
        bool match = true;
        for (int j = 1; j < newline.length(); j++) {
          if (buffer.byteAt(i + j - 1) != newline.byteAt(j)) {
            match = false;
          }
        }
        if (match) {
          i += (newline.length() - 1);
          break;
        }
      }
    }
  }
  string_io.setPos(i);
  return i;
}

RawObject METH(StringIO, __next__)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStringIO(*self)) {
    return thread->raiseRequiresType(self, ID(StringIO));
  }
  StringIO string_io(&scope, *self);
  if (string_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  word start = string_io.pos();
  word end = stringIOReadline(thread, string_io, -1);
  if (end == -1) {
    return thread->raise(LayoutId::kStopIteration, NoneType::object());
  }
  Bytes result(&scope, string_io.buffer());
  result = bytesSubseq(thread, result, start, end - start);
  return result.becomeStr();
}

RawObject METH(StringIO, close)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self_obj(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStringIO(*self_obj)) {
    return thread->raiseRequiresType(self_obj, ID(StringIO));
  }
  StringIO self(&scope, *self_obj);
  self.setClosed(true);
  return NoneType::object();
}

RawObject METH(StringIO, getvalue)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfStringIO(*self)) {
    return thread->raiseRequiresType(self, ID(StringIO));
  }
  StringIO string_io(&scope, *self);
  if (string_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  Bytes buffer(&scope, string_io.buffer());
  buffer = runtime->bytesCopy(thread, buffer);
  return buffer.becomeStr();
}

RawObject METH(StringIO, read)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStringIO(*self)) {
    return thread->raiseRequiresType(self, ID(StringIO));
  }
  StringIO string_io(&scope, *self);
  if (string_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  Object size_obj(&scope, args.get(1));
  word size;
  if (size_obj.isNoneType()) {
    size = -1;
  } else {
    size_obj = intFromIndex(thread, size_obj);
    if (size_obj.isError()) return *size_obj;
    // TODO(T55084422): have a better abstraction for int to word conversion
    if (!size_obj.isSmallInt() && !size_obj.isBool()) {
      return thread->raiseWithFmt(
          LayoutId::kOverflowError,
          "cannot fit value into an index-sized integer");
    }
    size = Int::cast(*size_obj).asWord();
  }
  Bytes result(&scope, string_io.buffer());
  word start = string_io.pos();
  word end = result.length();
  if (start > end) {
    return Str::empty();
  }
  if (size < 0) {
    string_io.setPos(end);
    result = bytesSubseq(thread, result, start, end - start);
    return result.becomeStr();
  }
  word new_pos = Utils::minimum(end, start + size);
  string_io.setPos(new_pos);
  result = bytesSubseq(thread, result, start, new_pos - start);
  return result.becomeStr();
}

RawObject METH(StringIO, readline)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  if (!thread->runtime()->isInstanceOfStringIO(*self)) {
    return thread->raiseRequiresType(self, ID(StringIO));
  }
  StringIO string_io(&scope, *self);
  if (string_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  Object size_obj(&scope, args.get(1));
  word size;
  if (size_obj.isNoneType()) {
    size = -1;
  } else {
    size_obj = intFromIndex(thread, size_obj);
    if (size_obj.isError()) return *size_obj;
    // TODO(T55084422): have a better abstraction for int to word conversion
    if (!size_obj.isSmallInt() && !size_obj.isBool()) {
      return thread->raiseWithFmt(
          LayoutId::kOverflowError,
          "cannot fit value into an index-sized integer");
    }
    size = Int::cast(*size_obj).asWord();
  }
  word start = string_io.pos();
  word end = stringIOReadline(thread, string_io, size);
  if (end == -1) {
    return Str::empty();
  }
  Bytes result(&scope, string_io.buffer());
  result = bytesSubseq(thread, result, start, end - start);
  return result.becomeStr();
}

RawObject METH(StringIO, truncate)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Object self(&scope, args.get(0));
  Runtime* runtime = thread->runtime();
  if (!runtime->isInstanceOfStringIO(*self)) {
    return thread->raiseRequiresType(self, ID(StringIO));
  }
  StringIO string_io(&scope, *self);
  if (string_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  Object size_obj(&scope, args.get(1));
  word size;
  if (size_obj.isNoneType()) {
    size = string_io.pos();
  } else {
    size_obj = intFromIndex(thread, size_obj);
    if (size_obj.isError()) return *size_obj;
    // TODO(T55084422): have a better abstraction for int to word conversion
    if (!size_obj.isSmallInt() && !size_obj.isBool()) {
      return thread->raiseWithFmt(
          LayoutId::kOverflowError,
          "cannot fit value into an index-sized integer");
    }
    size = Int::cast(*size_obj).asWord();
    if (size < 0) {
      return thread->raiseWithFmt(LayoutId::kValueError,
                                  "Negative size value %d", size);
    }
  }
  MutableBytes buffer(&scope, string_io.buffer());
  if (size < buffer.length()) {
    MutableBytes new_buffer(&scope,
                            runtime->newMutableBytesUninitialized(size));
    new_buffer.replaceFromWith(0, *buffer, size);
    string_io.setBuffer(*new_buffer);
  }
  return SmallInt::fromWord(size);
}

RawObject METH(StringIO, write)(Thread* thread, Arguments args) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object self(&scope, args.get(0));
  if (!runtime->isInstanceOfStringIO(*self)) {
    return thread->raiseRequiresType(self, ID(StringIO));
  }
  StringIO string_io(&scope, *self);
  if (string_io.closed()) {
    return thread->raiseWithFmt(LayoutId::kValueError,
                                "I/O operation on closed file.");
  }
  Object value(&scope, args.get(1));
  if (!runtime->isInstanceOfStr(*value)) {
    return thread->raiseRequiresType(value, ID(str));
  }
  Str str(&scope, strUnderlying(*value));
  return stringIOWrite(thread, string_io, str);
}

static const BuiltinAttribute kTextIOWrapperAttributes[] = {
    {ID(_buffer), RawTextIOWrapper::kBufferOffset},
    {ID(_line_buffering), RawTextIOWrapper::kLineBufferingOffset},
    {ID(_encoding), RawTextIOWrapper::kEncodingOffset},
    {ID(_errors), RawTextIOWrapper::kErrorsOffset},
    {ID(_readuniversal), RawTextIOWrapper::kReaduniversalOffset},
    {ID(_readtranslate), RawTextIOWrapper::kReadtranslateOffset},
    {ID(_readnl), RawTextIOWrapper::kReadnlOffset},
    {ID(_writetranslate), RawTextIOWrapper::kWritetranslateOffset},
    {ID(_writenl), RawTextIOWrapper::kWritenlOffset},
    {ID(_encoder), RawTextIOWrapper::kEncoderOffset},
    {ID(_decoder), RawTextIOWrapper::kDecoderOffset},
    {ID(_decoded_chars), RawTextIOWrapper::kDecodedCharsOffset},
    {ID(_decoded_chars_used), RawTextIOWrapper::kDecodedCharsUsedOffset},
    {ID(_snapshot), RawTextIOWrapper::kSnapshotOffset},
    {ID(_seekable), RawTextIOWrapper::kSeekableOffset},
    {ID(_has_read1), RawTextIOWrapper::kHasRead1Offset},
    {ID(_b2cratio), RawTextIOWrapper::kB2cratioOffset},
    {ID(_telling), RawTextIOWrapper::kTellingOffset},
    {ID(mode), RawTextIOWrapper::kModeOffset},  // TODO(T54575279): remove
};

void initializeUnderIOTypes(Thread* thread) {
  addBuiltinType(thread, ID(_IOBase), LayoutId::kUnderIOBase,
                 /*superclass_id=*/LayoutId::kObject, kUnderIOBaseAttributes,
                 UnderIOBase::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(IncrementalNewlineDecoder),
                 LayoutId::kIncrementalNewlineDecoder,
                 /*superclass_id=*/LayoutId::kObject,
                 kIncrementalNewlineDecoderAttributes,
                 IncrementalNewlineDecoder::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(_RawIOBase), LayoutId::kUnderRawIOBase,
                 /*superclass_id=*/LayoutId::kUnderIOBase, kNoAttributes,
                 UnderRawIOBase::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(_BufferedIOBase), LayoutId::kUnderBufferedIOBase,
                 /*superclass_id=*/LayoutId::kUnderIOBase, kNoAttributes,
                 UnderBufferedIOBase::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(BytesIO), LayoutId::kBytesIO,
                 /*superclass_id=*/LayoutId::kUnderBufferedIOBase,
                 kBytesIOAttributes, BytesIO::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(_BufferedIOMixin), LayoutId::kUnderBufferedIOMixin,
                 /*superclass_id=*/LayoutId::kUnderBufferedIOBase,
                 kUnderBufferedIOMixinAttributes, UnderBufferedIOMixin::kSize,
                 /*basetype=*/true);

  addBuiltinType(thread, ID(BufferedRandom), LayoutId::kBufferedRandom,
                 /*superclass_id=*/LayoutId::kUnderBufferedIOMixin,
                 kBufferedRandomAttributes, BufferedRandom::kSize,
                 /*basetype=*/true);

  addBuiltinType(thread, ID(BufferedReader), LayoutId::kBufferedReader,
                 /*superclass_id=*/LayoutId::kUnderBufferedIOMixin,
                 kBufferedReaderAttributes, BufferedReader::kSize,
                 /*basetype=*/true);

  addBuiltinType(thread, ID(BufferedWriter), LayoutId::kBufferedWriter,
                 /*superclass_id=*/LayoutId::kUnderBufferedIOMixin,
                 kBufferedWriterAttributes, BufferedWriter::kSize,
                 /*basetype=*/true);

  addBuiltinType(thread, ID(FileIO), LayoutId::kFileIO,
                 /*superclass_id=*/LayoutId::kUnderRawIOBase, kFileIOAttributes,
                 FileIO::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(_TextIOBase), LayoutId::kUnderTextIOBase,
                 /*superclass_id=*/LayoutId::kUnderIOBase, kNoAttributes,
                 RawUnderTextIOBase::kSize, /*basetype=*/true);

  addBuiltinType(thread, ID(TextIOWrapper), LayoutId::kTextIOWrapper,
                 /*superclass_id=*/LayoutId::kUnderTextIOBase,
                 kTextIOWrapperAttributes, TextIOWrapper::kSize,
                 /*basetype=*/true);

  addBuiltinType(thread, ID(StringIO), LayoutId::kStringIO,
                 /*superclass_id=*/LayoutId::kUnderTextIOBase,
                 kStringIOAttributes, StringIO::kSize, /*basetype=*/true);
}

}  // namespace py
