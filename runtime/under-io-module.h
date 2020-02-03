#pragma once

#include "frame.h"
#include "globals.h"
#include "modules.h"
#include "objects.h"
#include "runtime.h"

namespace py {

RawObject FUNC(_io, _buffered_reader_clear_buffer)(Thread* thread, Frame* frame,
                                                   word nargs);
RawObject FUNC(_io, _buffered_reader_init)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_io, _buffered_reader_peek)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_io, _buffered_reader_read)(Thread* thread, Frame* frame,
                                           word nargs);
RawObject FUNC(_io, _buffered_reader_readline)(Thread* thread, Frame* frame,
                                               word nargs);
RawObject FUNC(_io, _StringIO_closed_guard)(Thread* thread, Frame* frame,
                                            word nargs);

class UnderIoModule {
 public:
  static void initialize(Thread* thread, const Module& module);

 private:
  static const BuiltinFunction kBuiltinFunctions[];
  static const BuiltinType kBuiltinTypes[];
};

class UnderIOBaseBuiltins : public Builtins<UnderIOBaseBuiltins, ID(_IOBase),
                                            LayoutId::kUnderIOBase> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class IncrementalNewlineDecoderBuiltins
    : public Builtins<IncrementalNewlineDecoderBuiltins,
                      ID(IncrementalNewlineDecoder),
                      LayoutId::kIncrementalNewlineDecoder> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class UnderRawIOBaseBuiltins
    : public Builtins<UnderRawIOBaseBuiltins, ID(_RawIOBase),
                      LayoutId::kUnderRawIOBase, LayoutId::kUnderIOBase> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);
};

class UnderBufferedIOBaseBuiltins
    : public Builtins<UnderBufferedIOBaseBuiltins, ID(_BufferedIOBase),
                      LayoutId::kUnderBufferedIOBase, LayoutId::kUnderIOBase> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);
};

class UnderBufferedIOMixinBuiltins
    : public Builtins<UnderBufferedIOMixinBuiltins, ID(_BufferedIOMixin),
                      LayoutId::kUnderBufferedIOMixin,
                      LayoutId::kUnderBufferedIOBase> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class BufferedRandomBuiltins
    : public Builtins<BufferedRandomBuiltins, ID(BufferedRandom),
                      LayoutId::kBufferedRandom,
                      LayoutId::kUnderBufferedIOMixin> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class BufferedReaderBuiltins
    : public Builtins<BufferedReaderBuiltins, ID(BufferedReader),
                      LayoutId::kBufferedReader,
                      LayoutId::kUnderBufferedIOMixin> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class BufferedWriterBuiltins
    : public Builtins<BufferedWriterBuiltins, ID(BufferedWriter),
                      LayoutId::kBufferedWriter,
                      LayoutId::kUnderBufferedIOMixin> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class BytesIOBuiltins
    : public Builtins<BytesIOBuiltins, ID(BytesIO), LayoutId::kBytesIO,
                      LayoutId::kUnderBufferedIOBase> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static const BuiltinAttribute kAttributes[];
};

class FileIOBuiltins
    : public Builtins<FileIOBuiltins, ID(FileIO), LayoutId::kFileIO,
                      LayoutId::kUnderRawIOBase> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class UnderTextIOBaseBuiltins
    : public Builtins<UnderTextIOBaseBuiltins, ID(_TextIOBase),
                      LayoutId::kUnderTextIOBase, LayoutId::kUnderIOBase> {};

class TextIOWrapperBuiltins
    : public Builtins<TextIOWrapperBuiltins, ID(TextIOWrapper),
                      LayoutId::kTextIOWrapper, LayoutId::kUnderTextIOBase> {
 public:
  static const BuiltinAttribute kAttributes[];
};

RawObject METH(StringIO, __init__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(StringIO, __next__)(Thread* thread, Frame* frame, word nargs);
RawObject METH(StringIO, getvalue)(Thread* thread, Frame* frame, word nargs);
RawObject METH(StringIO, read)(Thread* thread, Frame* frame, word nargs);
RawObject METH(StringIO, readline)(Thread* thread, Frame* frame, word nargs);
RawObject METH(StringIO, truncate)(Thread* thread, Frame* frame, word nargs);
RawObject METH(StringIO, write)(Thread* thread, Frame* frame, word nargs);

class StringIOBuiltins
    : public Builtins<StringIOBuiltins, ID(StringIO), LayoutId::kStringIO,
                      LayoutId::kUnderTextIOBase> {
 public:
  static const BuiltinAttribute kAttributes[];
  static const BuiltinMethod kBuiltinMethods[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(StringIOBuiltins);
};

}  // namespace py
