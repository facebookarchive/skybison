#pragma once

#include "globals.h"
#include "handles.h"

namespace python {

class List;
class Object;
class Runtime;

class Marshal {
 public:
  class Reader {
   public:
    Reader(HandleScope* scope, Runtime* runtime, const char* buffer);

    double readBinaryFloat();

    byte readByte();

    int32 readLong();

    Object* readObject();

    int16 readShort();

    const byte* readBytes(int length);

    Object* readTypeString();
    Object* readTypeAscii();
    Object* readTypeAsciiInterned();
    Object* readTypeUnicode();
    Object* readTypeShortAscii();
    Object* readTypeShortAsciiInterned();
    Object* readTypeSmallTuple();
    Object* readTypeTuple();
    Object* readTypeCode();
    Object* readTypeRef();

    Object* doTupleElements(int length);

    word addRef(Object* value);
    void setRef(word index, Object* value);
    Object* getRef(word index);
    word numRefs();

   private:
    Object* readString(word length);
    Object* readAndInternString(word length);

    Runtime* runtime_;
    Handle<List> refs_;
    bool isRef_;

    const byte* start_;
    const byte* end_;
    int depth_;
    int length_;
    int pos_;

    DISALLOW_COPY_AND_ASSIGN(Reader);
    DISALLOW_HEAP_ALLOCATION();
  };

  DISALLOW_IMPLICIT_CONSTRUCTORS(Marshal);
};

}  // namespace python
