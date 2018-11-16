#pragma once

#include "globals.h"
#include "handle.h"
#include "objects.h"

namespace python {

class Marshal {
public:
  class Reader {
   public:
    Reader(const char* buffer);

    byte ReadByte();

    int32 ReadLong();

    Object* ReadObject();

    int16 ReadShort();

    const byte* ReadString(int length);

    Object* ReadTypeString();
    Object* ReadTypeShortAscii();
    Object* ReadTypeSmallTuple();
    Object* ReadTypeTuple();
    Object* ReadTypeCode();
    Object* ReadTypeRef();

    Object* DoTupleElements(int length);

    int AddRef(Object* value);
    void SetRef(int index, Object* value);
    Object* GetRef(int index);

   private:

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

}  // namespace python
