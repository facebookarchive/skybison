#pragma once

#include "globals.h"

namespace python {

class Object;
class Runtime;

class Marshal {
 public:
  class Reader {
   public:
    Reader(Runtime* runtime, const char* buffer);

    byte readByte();

    int32 readLong();

    Object* readObject();

    int16 readShort();

    const byte* readString(int length);

    Object* readTypeString();
    Object* readTypeShortAscii();
    Object* readTypeSmallTuple();
    Object* readTypeTuple();
    Object* readTypeCode();
    Object* readTypeRef();

    Object* doTupleElements(int length);

    int addRef(Object* value);
    void setRef(int index, Object* value);
    Object* getRef(int index);

   private:
    Runtime* runtime_;

    bool isRef_;
    Object* refs_;

    const byte* start_;
    const byte* end_;
    int depth_;
    int length_;
    int pos_;

    DISALLOW_COPY_AND_ASSIGN(Reader);
  };

  DISALLOW_IMPLICIT_CONSTRUCTORS(Marshal);
};

} // namespace python
