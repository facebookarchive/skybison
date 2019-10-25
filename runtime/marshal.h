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
    Reader(HandleScope* scope, Runtime* runtime, const char* buffer);

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
    word addRef(RawObject value);
    void setRef(word index, RawObject value);

    RawObject readStr(word length);
    RawObject readAndInternStr(word length);
    RawObject readLongObject();

    RawObject doSetElements(int32_t length, RawObject set_obj);
    RawObject doTupleElements(int32_t length);

    Runtime* runtime_;
    List refs_;
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
