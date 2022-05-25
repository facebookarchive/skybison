#include "compile-utils.h"

#include "handles.h"
#include "objects.h"
#include "runtime.h"
#include "str-builtins.h"
#include "thread.h"

namespace py {

RawObject compile(Thread* thread, const Object& source, const Object& filename,
                  SymbolId mode, word flags, int optimize) {
  HandleScope scope(thread);
  Runtime* runtime = thread->runtime();
  Object mode_str(&scope, runtime->symbols()->at(mode));
  Object flags_int(&scope, runtime->newInt(flags));
  Object optimize_int(&scope, SmallInt::fromWord(optimize));

  Object dunder_import(&scope, runtime->lookupNameInModule(thread, ID(builtins),
                                                           ID(__import__)));
  if (dunder_import.isErrorException()) return *dunder_import;
  Object compiler_name(&scope, runtime->symbols()->at(ID(_compiler)));
  Object import_result(
      &scope, Interpreter::call1(thread, dunder_import, compiler_name));
  if (import_result.isErrorException()) return *import_result;
  Object none(&scope, NoneType::object());
  return thread->invokeFunction6(ID(_compiler), ID(compile), source, filename,
                                 mode_str, flags_int, none, optimize_int);
}

RawObject mangle(Thread* thread, const Object& privateobj, const Str& ident) {
  Runtime* runtime = thread->runtime();
  // Only mangle names that start with two underscores, but do not end with
  // two underscores or contain a dot.
  word ident_length = ident.length();
  if (ident_length < 2 || ident.byteAt(0) != '_' || ident.byteAt(1) != '_' ||
      (ident.byteAt(ident_length - 2) == '_' &&
       ident.byteAt(ident_length - 1) == '_') ||
      strFindAsciiChar(ident, '.') >= 0) {
    return *ident;
  }

  if (!runtime->isInstanceOfStr(*privateobj)) {
    return *ident;
  }
  HandleScope scope(thread);
  Str privateobj_str(&scope, strUnderlying(*privateobj));
  word privateobj_length = privateobj_str.length();
  word begin = 0;
  while (begin < privateobj_length && privateobj_str.byteAt(begin) == '_') {
    begin++;
  }
  if (begin == privateobj_length) {
    return *ident;
  }

  word length0 = privateobj_length - begin;
  word length = length0 + ident_length + 1;
  MutableBytes result(&scope, runtime->newMutableBytesUninitialized(length));
  result.byteAtPut(0, '_');
  result.replaceFromWithStrStartAt(1, *privateobj_str, length0, begin);
  result.replaceFromWithStr(1 + length0, *ident, ident_length);
  return result.becomeStr();
}

}  // namespace py
