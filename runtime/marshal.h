#pragma once

#include "globals.h"
#include "handles.h"
#include "modules.h"

namespace py {

// Magic number also used by `library/_frozen_importlib_external.py`.
// This uses a "Y\n" suffix to be different from the "\r\n" used in CPython.
const int32_t kPycMagic = 1005 | (int32_t{'Y'} << 16) | (int32_t{'\n'} << 24);

class RawList;
class RawObject;
class Runtime;

class Marshal {
 public:
  class Reader {
   public:
    // TODO(T38902583): Generalize Marshal::Reader to take a Bytes or buffer
    // protocol object
    Reader(HandleScope* scope, Thread* thread, View<byte> buffer);

    RawObject readPycHeader(const Str& filename);

    void setBuiltinFunctions(const BuiltinFunction* builtin_functions,
                             word num_builtin_functions,
                             const IntrinsicFunction* intrinsic_functions,
                             word num_intrinsic_functions);

    double readBinaryFloat();

    byte readByte();

    int32_t readLong();

    RawObject readObject();

    int16_t readShort();

    const byte* readBytes(int length);

    RawObject readTypeString();
    RawObject readTypeAscii();
    RawObject readTypeAsciiInterned();
    RawObject readTypeUnicode();
    RawObject readTypeShortAscii();
    RawObject readTypeShortAsciiInterned();
    RawObject readTypeSmallTuple();
    RawObject readTypeTuple();
    RawObject readTypeSet();
    RawObject readTypeFrozenSet();
    RawObject readTypeCode();
    RawObject readTypeRef();

    word numRefs();
    RawObject getRef(word index);

   private:
    word addRef(const Object& value);
    void setRef(word index, RawObject value);

    RawObject readStr(word length);
    RawObject readAndInternStr(word length);
    RawObject readLongObject();

    RawObject doSetElements(int32_t length, const SetBase& set);
    RawObject doTupleElements(int32_t length);

    Thread* thread_;
    Runtime* runtime_;
    List refs_;
    const BuiltinFunction* builtin_functions_ = nullptr;
    word num_builtin_functions_ = 0;
    const IntrinsicFunction* intrinsic_functions_ = nullptr;
    word num_intrinsic_functions_ = 0;
    bool isRef_;

    const byte* start_;
    const byte* end_;
    int length_;
    int pos_;

    static const int kBitsPerLongDigit = 15;

    DISALLOW_COPY_AND_ASSIGN(Reader);
    DISALLOW_HEAP_ALLOCATION();
  };

  DISALLOW_IMPLICIT_CONSTRUCTORS(Marshal);
};

}  // namespace py
