#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"

namespace python {

class UnderIoModule : public ModuleBase<UnderIoModule, SymbolId::kUnderIo> {
 public:
  static RawObject underReadFile(Thread* thread, Frame* frame, word nargs);
  static RawObject underReadBytes(Thread* thread, Frame* frame, word nargs);

  static const BuiltinMethod kBuiltinMethods[];
  static const BuiltinType kBuiltinTypes[];
  static const char* const kFrozenData;
};

class UnderIOBaseBuiltins
    : public Builtins<UnderIOBaseBuiltins, SymbolId::kUnderIOBase,
                      LayoutId::kUnderIOBase> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class IncrementalNewlineDecoderBuiltins
    : public Builtins<IncrementalNewlineDecoderBuiltins,
                      SymbolId::kIncrementalNewlineDecoder,
                      LayoutId::kIncrementalNewlineDecoder> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class UnderRawIOBaseBuiltins
    : public Builtins<UnderRawIOBaseBuiltins, SymbolId::kUnderRawIOBase,
                      LayoutId::kUnderRawIOBase, LayoutId::kUnderIOBase> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);
};

class UnderBufferedIOBaseBuiltins
    : public Builtins<
          UnderBufferedIOBaseBuiltins, SymbolId::kUnderBufferedIOBase,
          LayoutId::kUnderBufferedIOBase, LayoutId::kUnderRawIOBase> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);
};

class UnderBufferedIOMixinBuiltins
    : public Builtins<
          UnderBufferedIOMixinBuiltins, SymbolId::kUnderBufferedIOMixin,
          LayoutId::kUnderBufferedIOMixin, LayoutId::kUnderBufferedIOBase> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class BufferedReaderBuiltins
    : public Builtins<BufferedReaderBuiltins, SymbolId::kBufferedReader,
                      LayoutId::kBufferedReader,
                      LayoutId::kUnderBufferedIOMixin> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class BytesIOBuiltins
    : public Builtins<BytesIOBuiltins, SymbolId::kBytesIO, LayoutId::kBytesIO,
                      LayoutId::kUnderBufferedIOBase> {
 public:
  static void postInitialize(Runtime* runtime, const Type& new_type);

  static const BuiltinAttribute kAttributes[];
};

class FileIOBuiltins
    : public Builtins<FileIOBuiltins, SymbolId::kFileIO, LayoutId::kFileIO,
                      LayoutId::kUnderRawIOBase> {
 public:
  static const BuiltinAttribute kAttributes[];
};

class UnderTextIOBaseBuiltins
    : public Builtins<UnderTextIOBaseBuiltins, SymbolId::kUnderTextIOBase,
                      LayoutId::kUnderTextIOBase, LayoutId::kUnderIOBase> {};

class TextIOWrapperBuiltins
    : public Builtins<TextIOWrapperBuiltins, SymbolId::kTextIOWrapper,
                      LayoutId::kTextIOWrapper, LayoutId::kUnderTextIOBase> {
 public:
  static const BuiltinAttribute kAttributes[];
};

}  // namespace python
