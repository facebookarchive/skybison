#pragma once

#include <iosfwd>

#include "handles.h"
#include "objects.h"

namespace python {

std::ostream& dumpExtendedCode(std::ostream& os, RawCode value,
                               const char* indent);
std::ostream& dumpExtendedFunction(std::ostream& os, RawFunction value);
std::ostream& dumpExtendedHeapObject(std::ostream& os, RawHeapObject value);
std::ostream& dumpExtended(std::ostream& os, RawObject value);

std::ostream& operator<<(std::ostream& os, CastError value);

std::ostream& operator<<(std::ostream& os, RawBool value);
std::ostream& operator<<(std::ostream& os, RawBoundMethod value);
std::ostream& operator<<(std::ostream& os, RawByteArray value);
std::ostream& operator<<(std::ostream& os, RawBytes value);
std::ostream& operator<<(std::ostream& os, RawCode value);
std::ostream& operator<<(std::ostream& os, RawDict value);
std::ostream& operator<<(std::ostream& os, RawError value);
std::ostream& operator<<(std::ostream& os, RawFloat value);
std::ostream& operator<<(std::ostream& os, RawFunction value);
std::ostream& operator<<(std::ostream& os, RawInt value);
std::ostream& operator<<(std::ostream& os, RawLargeInt value);
std::ostream& operator<<(std::ostream& os, RawLargeStr value);
std::ostream& operator<<(std::ostream& os, RawList value);
std::ostream& operator<<(std::ostream& os, RawModule value);
std::ostream& operator<<(std::ostream& os, RawNoneType value);
std::ostream& operator<<(std::ostream& os, RawObject value);
std::ostream& operator<<(std::ostream& os, RawSmallInt value);
std::ostream& operator<<(std::ostream& os, RawSmallStr value);
std::ostream& operator<<(std::ostream& os, RawStr value);
std::ostream& operator<<(std::ostream& os, RawTuple value);
std::ostream& operator<<(std::ostream& os, RawType value);
template <typename T>
std::ostream& operator<<(std::ostream& os, const Handle<T>& value) {
  return os << *value;
}

std::ostream& operator<<(std::ostream& os, Frame* frame);

void dump(RawObject object);
void dump(const Object& object);
void dump(Frame* frame);
void dumpSingleFrame(Frame* frame);

void initializeDebugging();

}  // namespace python
