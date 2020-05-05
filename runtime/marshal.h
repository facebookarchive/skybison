#pragma once

#include "globals.h"
#include "handles.h"

namespace py {

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

    void setBuiltinFunctions(const Function::Entry* builtin_functions,
                             word num_builtin_functions);

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
    const Function::Entry* builtin_functions_;
    word num_builtin_functions_;
    bool isRef_;

    const byte* start_;
    const byte* end_;
    int depth_;
    int length_;
    int pos_;

    static const int kBitsPerLongDigit = 15;

    DISALLOW_COPY_AND_ASSIGN(Reader);
    DISALLOW_HEAP_ALLOCATION();
  };

  DISALLOW_IMPLICIT_CONSTRUCTORS(Marshal);
};

}  // namespace py
