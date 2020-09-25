#include "traceback-builtins.h"

#include "builtins.h"
#include "globals.h"
#include "os.h"
#include "type-builtins.h"

namespace py {

static const BuiltinAttribute kTracebackAttributes[] = {
    {ID(_traceback__next), RawTraceback::kNextOffset, AttributeFlags::kHidden},
    {ID(_traceback__function), RawTraceback::kFunctionOffset,
     AttributeFlags::kHidden},
    {ID(tb_lasti), RawTraceback::kLastiOffset, AttributeFlags::kReadOnly},
    {ID(_traceback__lineno), RawTraceback::kLinenoOffset,
     AttributeFlags::kHidden},
};

void initializeTracebackType(Thread* thread) {
  addBuiltinType(thread, ID(traceback), LayoutId::kTraceback,
                 /*superclass_id=*/LayoutId::kObject, kTracebackAttributes);
}

static void writeCStr(const MutableBytes& dst, word* index, const char* src) {
  word length = std::strlen(src);
  dst.replaceFromWithAll(*index, {reinterpret_cast<const byte*>(src), length});
  *index += length;
}

static void writeStr(const MutableBytes& dst, word* index, const Str& src) {
  word length = src.length();
  dst.replaceFromWithStr(*index, *src, length);
  *index += length;
}

static word tracebackWriteLine(Thread* thread, const Traceback& traceback,
                               const MutableBytes& dst, bool determine_size) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();

  Function function(&scope, traceback.function());
  Object code_obj(&scope, function.code());
  Object name_obj(&scope, function.name());
  word index = 0;

  if (!code_obj.isCode()) {
    if (runtime->isInstanceOfStr(*name_obj)) {
      Str name(&scope, strUnderlying(*name_obj));
      if (determine_size) {
        return std::strlen("  File <unknown>, in ") + name.length() + 1;
      }
      writeCStr(dst, &index, "  File <unknown>, in ");
      writeStr(dst, &index, name);
      writeCStr(dst, &index, "\n");
      return index;
    }
    if (determine_size) {
      return std::strlen("  File <unknown>, in <invalid name>\n");
    }
    writeCStr(dst, &index, "  File <unknown>, in <invalid name>\n");
    return index;
  }

  Code code(&scope, *code_obj);
  Object filename_obj(&scope, code.filename());
  if (runtime->isInstanceOfStr(*filename_obj)) {
    Str filename(&scope, *filename_obj);
    if (determine_size) {
      index += std::strlen("  File ") + filename.length();
    } else {
      writeCStr(dst, &index, "  File ");
      writeStr(dst, &index, filename);
    }
  } else if (determine_size) {
    index += std::strlen("  File <unknown>");
  } else {
    writeCStr(dst, &index, "  File <unknown>");
  }

  word lineno = -1;
  Object lineno_obj(&scope, traceback.lineno());
  if (!lineno_obj.isNoneType()) {
    lineno = SmallInt::cast(*lineno_obj).value();
  } else if (!code.isNative() && code.lnotab().isBytes()) {
    word lasti = SmallInt::cast(traceback.lasti()).value();
    lineno = code.offsetToLineNum(lasti);
    traceback.setLineno(SmallInt::fromWord(lineno));
  }
  if (lineno >= 0) {
    char buf[kUwordDigits10 + 8];
    word size = std::snprintf(buf, sizeof(buf), ", line %ld", lineno);
    DCHECK_BOUND(size, kUwordDigits10 + 7);
    if (determine_size) {
      index += size;
    } else {
      writeCStr(dst, &index, buf);
    }
  }

  if (runtime->isInstanceOfStr(*name_obj)) {
    Str name(&scope, strUnderlying(*name_obj));
    if (determine_size) {
      index += std::strlen(", in ") + name.length() + 1;
    } else {
      writeCStr(dst, &index, ", in ");
      writeStr(dst, &index, name);
      writeCStr(dst, &index, "\n");
    }
  } else if (code.isNative()) {
    void* addr = Int::cast(code.code()).asCPtr();

    char ptr_buf[2 * kWordSize + 25];
    word ptr_len = std::snprintf(ptr_buf, sizeof(ptr_buf),
                                 "<native function at %p (", addr);
    DCHECK_BOUND(ptr_len, 2 * kWordSize + 24);

    char name_buf[128];
    word name_size = std::strlen(name_buf);
    word name_len = OS::sharedObjectSymbolName(addr, name_buf, name_size);

    if (determine_size) {
      if (name_len < 0) {
        name_len = std::strlen("no symbol found");
      }
      index += ptr_len + name_len + std::strlen(")>\n");
    } else {
      writeCStr(dst, &index, ptr_buf);
      if (name_len < 0) {
        writeCStr(dst, &index, "no symbol found");
      } else if (name_len < name_size) {
        writeCStr(dst, &index, name_buf);
      } else {
        char* heap_buf = reinterpret_cast<char*>(std::malloc(name_len + 1));
        word new_len = OS::sharedObjectSymbolName(addr, heap_buf, name_len + 1);
        DCHECK(name_len == new_len, "unexpected number of bytes written");
        writeCStr(dst, &index, heap_buf);
      }
      writeCStr(dst, &index, ")>\n");
    }
  } else if (determine_size) {
    index += std::strlen("<invalid name>\n");
  } else {
    writeCStr(dst, &index, "<invalid name>\n");
  }
  return index;
}

RawObject tracebackWrite(Thread* thread, const Traceback& traceback,
                         const Object& file) {
  // TODO(T75801091): check sys.tracebacklimit and recursion
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Str line(&scope,
           runtime->newStrFromCStr("Traceback (most recent call last):\n"));
  Object result(&scope, thread->invokeMethod2(file, ID(write), line));
  if (result.isErrorException()) return *result;

  Traceback current(&scope, *traceback);
  MutableBytes buffer(&scope, runtime->emptyMutableBytes());
  for (;;) {
    word size = tracebackWriteLine(thread, current, buffer, true);
    buffer = runtime->newMutableBytesUninitialized(size);
    tracebackWriteLine(thread, current, buffer, false);
    line = buffer.becomeStr();
    result = thread->invokeMethod2(file, ID(write), line);
    if (result.isErrorException()) return *result;

    RawObject next = current.next();
    if (next.isNoneType()) {
      return NoneType::object();
    }

    current = next;
    buffer = runtime->emptyMutableBytes();
  }
}

}  // namespace py
