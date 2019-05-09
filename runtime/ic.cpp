#include "ic.h"

#include "bytearray-builtins.h"
#include "bytecode.h"
#include "runtime.h"
#include "utils.h"

namespace python {

// The canonical list of bytecode ops that have a _CACHED variant.
#define CACHED_OPS(X)                                                          \
  X(STORE_ATTR)                                                                \
  X(LOAD_ATTR)

static Bytecode cachedOp(Bytecode bc) {
  switch (bc) {
#define CASE(OP)                                                               \
  case OP:                                                                     \
    return OP##_CACHED;
    CACHED_OPS(CASE)
#undef CASE
    default:
      return bc;
  }
}

static bool hasCachedOp(Bytecode bc) {
  switch (bc) {
#define CASE(OP)                                                               \
  case OP:                                                                     \
    return true;
    CACHED_OPS(CASE)
#undef CASE
    default:
      return false;
  }
}

static bool isCachedOp(Bytecode bc) {
  switch (bc) {
#define CASE(OP)                                                               \
  case OP##_CACHED:                                                            \
    return true;
    CACHED_OPS(CASE)
#undef CASE
    default:
      return false;
  }
}

void icRewriteBytecode(Thread* thread, const Function& function) {
  HandleScope scope(thread);
  word num_caches = 0;

  // Scan bytecode to figure out how many cache slots we need.
  Bytes bytecode(&scope, RawCode::cast(function.code()).code());
  word bytecode_length = bytecode.length();
  for (word i = 0; i < bytecode_length; i += Frame::kCodeUnitSize) {
    Bytecode bc = static_cast<Bytecode>(bytecode.byteAt(i));
    DCHECK(!isCachedOp(bc), "Rewritten bytecode passed to icRewriteBytecode()");
    if (hasCachedOp(bc)) {
      num_caches++;
    }
  }

  Runtime* runtime = thread->runtime();
  Tuple original_arguments(&scope, runtime->newTuple(num_caches));

  // Rewrite bytecode.
  ByteArray result(&scope, runtime->newByteArray());
  runtime->byteArrayEnsureCapacity(thread, result, bytecode_length);
  result.setNumItems(bytecode_length);
  for (word i = 0, bucket = 0; i < bytecode_length;) {
    word begin = i;
    Bytecode bc = static_cast<Bytecode>(bytecode.byteAt(i++));
    int32_t arg = bytecode.byteAt(i++);
    while (bc == Bytecode::EXTENDED_ARG) {
      bc = static_cast<Bytecode>(bytecode.byteAt(i++));
      arg = (arg << kBitsPerByte) | bytecode.byteAt(i++);
    }

    if (hasCachedOp(bc)) {
      // Replace opcode arg with a cache index and zero EXTENDED_ARG args.
      CHECK(bucket < 256,
            "more than 256 entries may require bytecode stretching");
      for (word j = begin; j < i - 2; j += 2) {
        result.byteAtPut(j, static_cast<byte>(Bytecode::EXTENDED_ARG));
        result.byteAtPut(j + 1, 0);
      }
      result.byteAtPut(i - 2, static_cast<byte>(cachedOp(bc)));
      result.byteAtPut(i - 1, static_cast<byte>(bucket));

      // Remember original arg.
      original_arguments.atPut(bucket, SmallInt::fromWord(arg));
      bucket++;
    } else {
      for (word j = begin; j < i; j++) {
        result.byteAtPut(j, bytecode.byteAt(j));
      }
    }
  }

  function.setCaches(runtime->newTuple(num_caches * kIcPointersPerCache));
  function.setRewrittenBytecode(byteArrayAsBytes(thread, runtime, result));
  function.setOriginalArguments(*original_arguments);
}

word icFind(const Tuple& cache, word index, LayoutId layout_id) {
  RawSmallInt layout_id_int =
      RawSmallInt::fromWord(static_cast<word>(layout_id));
  for (word i = index * kIcPointersPerCache, end = i + kIcPointersPerCache;
       i < end; i += kIcPointersPerEntry) {
    RawObject layout = cache.at(i + kIcEntryKeyOffset);
    if (layout.isNoneType() || layout == layout_id_int) {
      return i;
    }
  }
  return -1;
}

void icUpdate(const Tuple& caches, word entry_offset, LayoutId layout_id,
              RawObject to_cache) {
  RawSmallInt layout_int = RawSmallInt::fromWord(static_cast<word>(layout_id));
  DCHECK(caches.at(entry_offset + kIcEntryKeyOffset).isNoneType() ||
             caches.at(entry_offset + kIcEntryKeyOffset) == layout_int,
         "invalid cache slot");
  caches.atPut(entry_offset + kIcEntryKeyOffset, layout_int);
  caches.atPut(entry_offset + kIcEntryValueOffset, to_cache);
}

}  // namespace python
