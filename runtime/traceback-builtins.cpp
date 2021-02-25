#include "traceback-builtins.h"

#include "builtins.h"
#include "globals.h"
#include "os.h"
#include "str-builtins.h"
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

static const word kTracebackLimit = 1000;         // PyTraceback_LIMIT
static const word kTracebackRecursiveCutoff = 3;  // TB_RECURSIVE_CUTOFF

void initializeTracebackType(Thread* thread) {
  addBuiltinType(thread, ID(traceback), LayoutId::kTraceback,
                 /*superclass_id=*/LayoutId::kObject, kTracebackAttributes,
                 Traceback::kSize, /*basetype=*/false);
}

static RawObject lineRepeated(Thread* thread, word count) {
  count -= kTracebackRecursiveCutoff;
  if (count == 1) {
    return thread->runtime()->newStrFromCStr(
        "  [Previous line repeated 1 more time]\n");
  }
  return thread->runtime()->newStrFromFmt(
      "  [Previous line repeated %w more times]\n", count);
}

static RawObject sourceLine(Thread* thread, const Object& filename,
                            const Object& lineno_obj) {
  if (!filename.isStr() || !lineno_obj.isSmallInt()) {
    return NoneType::object();
  }

  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object linecache(&scope, runtime->symbols()->at(ID(linecache)));
  if (runtime->findModule(linecache).isErrorNotFound()) {
    linecache =
        thread->invokeFunction1(ID(builtins), ID(__import__), linecache);
    if (linecache.isErrorException()) {
      thread->clearPendingException();
      return NoneType::object();
    }
  }
  Object line_obj(&scope, thread->invokeFunction2(ID(linecache), ID(getline),
                                                  filename, lineno_obj));
  if (line_obj.isErrorException()) {
    return *line_obj;
  }

  CHECK(line_obj.isStr(), "got a non-str line");
  Str line(&scope, *line_obj);
  line = strStripSpace(thread, line);
  word length = line.length();
  if (length == 0) {
    return NoneType::object();
  }

  MutableBytes result(&scope,
                      runtime->newMutableBytesUninitialized(length + 5));
  result.replaceFromWithByte(0, ' ', 4);
  result.replaceFromWithStr(4, *line, length);
  result.byteAtPut(length + 4, '\n');
  return result.becomeStr();
}

static RawObject tracebackFilename(Thread* thread, const Traceback& traceback) {
  HandleScope scope(thread);
  Object code(&scope, Function::cast(traceback.function()).code());
  if (!code.isCode()) {
    return NoneType::object();
  }
  Object name(&scope, Code::cast(*code).filename());
  if (thread->runtime()->isInstanceOfStr(*name)) {
    return strUnderlying(*name);
  }
  return NoneType::object();
}

static RawObject tracebackFunctionName(Thread* thread,
                                       const Traceback& traceback) {
  HandleScope scope(thread);
  Function function(&scope, traceback.function());
  Object name(&scope, function.name());
  Runtime* runtime = thread->runtime();
  if (runtime->isInstanceOfStr(*name)) {
    return strUnderlying(*name);
  }
  Object code_obj(&scope, function.code());
  if (!code_obj.isCode()) {
    return NoneType::object();
  }
  Code code(&scope, *code_obj);
  if (!code.isNative()) {
    return NoneType::object();
  }

  void* addr = Int::cast(code.code()).asCPtr();
  char name_buf[128];
  word name_size = std::strlen(name_buf);
  word name_len = OS::sharedObjectSymbolName(addr, name_buf, name_size);

  if (name_len < 0) {
    return runtime->newStrFromFmt("<native function at %p (no symbol found)>",
                                  addr);
  }
  if (name_len < name_size) {
    return runtime->newStrFromFmt("<native function at %p (%s)>", addr,
                                  name_buf);
  }
  unique_c_ptr<char> heap_buf(
      reinterpret_cast<char*>(std::malloc(name_len + 1)));
  word new_len = OS::sharedObjectSymbolName(addr, heap_buf.get(), name_len + 1);
  DCHECK(name_len == new_len, "unexpected number of bytes written");
  return runtime->newStrFromFmt("<native function at %p (%s)>", addr,
                                heap_buf.get());
}

static RawObject tracebackLineno(Thread* thread, const Traceback& traceback) {
  HandleScope scope(thread);
  Object lineno(&scope, traceback.lineno());
  if (lineno.isSmallInt()) {
    return *lineno;
  }
  Object code_obj(&scope, Function::cast(traceback.function()).code());
  if (!code_obj.isCode()) {
    return NoneType::object();
  }
  Code code(&scope, *code_obj);
  if (code.isNative() || !code.lnotab().isBytes()) {
    return NoneType::object();
  }
  word lasti = SmallInt::cast(traceback.lasti()).value();
  Object result(&scope, SmallInt::fromWord(code.offsetToLineNum(lasti)));
  traceback.setLineno(*result);
  return *result;
}

static void writeCStr(const MutableBytes& dst, word* index, const char* src) {
  word length = std::strlen(src);
  dst.replaceFromWithAll(*index, {reinterpret_cast<const byte*>(src), length});
  *index += length;
}

static void writeStr(const MutableBytes& dst, word* index, RawStr src) {
  word length = src.length();
  dst.replaceFromWithStr(*index, src, length);
  *index += length;
}

static word tracebackWriteLine(const Object& filename, const Object& lineno,
                               const Object& function_name,
                               const MutableBytes& dst, bool determine_size) {
  word index = 0;

  if (filename.isStr()) {
    if (determine_size) {
      index += std::strlen("  File \"") + Str::cast(*filename).length() + 1;
    } else {
      writeCStr(dst, &index, "  File \"");
      writeStr(dst, &index, Str::cast(*filename));
      writeCStr(dst, &index, "\"");
    }
  } else if (determine_size) {
    index += std::strlen("  File \"<unknown>\"");
  } else {
    writeCStr(dst, &index, "  File \"<unknown>\"");
  }

  if (lineno.isSmallInt()) {
    char buf[kUwordDigits10 + 9];
    word size = std::snprintf(buf, sizeof(buf), ", line %ld",
                              SmallInt::cast(*lineno).value());
    DCHECK_BOUND(size, kUwordDigits10 + 8);
    if (determine_size) {
      index += size;
    } else {
      writeCStr(dst, &index, buf);
    }
  }

  if (function_name.isStr()) {
    if (determine_size) {
      index += std::strlen(", in ") + Str::cast(*function_name).length() + 1;
    } else {
      writeCStr(dst, &index, ", in ");
      writeStr(dst, &index, Str::cast(*function_name));
      writeCStr(dst, &index, "\n");
    }
  } else if (determine_size) {
    index += std::strlen(", in <invalid name>\n");
  } else {
    writeCStr(dst, &index, ", in <invalid name>\n");
  }

  return index;
}

RawObject tracebackWrite(Thread* thread, const Traceback& traceback,
                         const Object& file) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object limit_obj(
      &scope, runtime->lookupNameInModule(thread, ID(sys), ID(tracebacklimit)));

  word limit = kTracebackLimit;
  if (!limit_obj.isErrorNotFound() && runtime->isInstanceOfInt(*limit_obj)) {
    limit = intUnderlying(*limit_obj).asWordSaturated();
    if (limit <= 0) {
      return NoneType::object();
    }
  }

  Str line(&scope,
           runtime->newStrFromCStr("Traceback (most recent call last):\n"));
  Object result(&scope, thread->invokeMethod2(file, ID(write), line));
  if (result.isErrorException()) return *result;

  word depth = 0;
  Object tb(&scope, *traceback);
  while (tb.isTraceback()) {
    depth++;
    tb = Traceback::cast(*tb).next();
  }

  Traceback current(&scope, *traceback);
  while (depth > limit) {
    depth--;
    current = current.next();
  }

  MutableBytes buffer(&scope, runtime->emptyMutableBytes());
  Object filename(&scope, NoneType::object());
  Object function_name(&scope, NoneType::object());
  Object lineno(&scope, NoneType::object());
  Object last_filename(&scope, NoneType::object());
  Object last_function_name(&scope, NoneType::object());
  Object last_lineno(&scope, NoneType::object());
  Object next(&scope, NoneType::object());
  for (word count = 0;;) {
    filename = tracebackFilename(thread, current);
    lineno = tracebackLineno(thread, current);
    function_name = tracebackFunctionName(thread, current);
    bool filename_changed =
        last_filename.isNoneType() || filename.isNoneType() ||
        !Str::cast(*last_filename).equals(Str::cast(*filename));
    bool lineno_changed = last_lineno.isNoneType() || lineno != last_lineno;
    bool function_name_changed =
        last_function_name.isNoneType() || function_name.isNoneType() ||
        !Str::cast(*last_function_name).equals(Str::cast(*function_name));
    if (filename_changed || lineno_changed || function_name_changed) {
      if (count > kTracebackRecursiveCutoff) {
        line = lineRepeated(thread, count);
        result = thread->invokeMethod2(file, ID(write), line);
        if (result.isErrorException()) return *result;
      }
      last_filename = *filename;
      last_lineno = *lineno;
      last_function_name = *function_name;
      count = 0;
    }

    count++;
    if (count <= kTracebackRecursiveCutoff) {
      word size =
          tracebackWriteLine(filename, lineno, function_name, buffer, true);
      buffer = runtime->newMutableBytesUninitialized(size);
      tracebackWriteLine(filename, lineno, function_name, buffer, false);
      line = buffer.becomeStr();
      result = thread->invokeMethod2(file, ID(write), line);
      if (result.isErrorException()) return *result;

      result = sourceLine(thread, filename, lineno);
      if (result.isErrorException()) return *result;
      if (result.isStr()) {
        result = thread->invokeMethod2(file, ID(write), result);
        if (result.isErrorException()) return *result;
      }

      result = runtime->handlePendingSignals(thread);
      if (result.isErrorException()) return *result;
    }

    next = current.next();
    if (next.isNoneType()) {
      if (count > kTracebackRecursiveCutoff) {
        line = lineRepeated(thread, count);
        result = thread->invokeMethod2(file, ID(write), line);
        if (result.isErrorException()) return *result;
      }
      return NoneType::object();
    }

    current = *next;
    buffer = runtime->emptyMutableBytes();
  }
}

}  // namespace py
