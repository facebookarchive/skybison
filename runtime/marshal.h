#pragma once

#include "globals.h"
#include "handles.h"

namespace python {

class RawList;
class RawObject;
class Runtime;

class Marshal {
 public:
  class Reader {
   public:
    Reader(HandleScope* scope, Runtime* runtime, const char* buffer);

    double readBinaryFloat();

    byte readByte();

    int32 readLong();

    RawObject readObject();

    int16 readShort();

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

    RawObject doSetElements(int32 length, RawObject set);
    RawObject doTupleElements(int32 length);

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

}  // namespace python
