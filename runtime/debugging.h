#pragma once

#include <iosfwd>

#include "handles.h"
#include "objects.h"

typedef struct _object PyObject;

namespace py {

std::ostream& dumpExtendedCode(std::ostream& os, RawCode value,
                               const char* indent);
std::ostream& dumpExtendedFunction(std::ostream& os, RawFunction value);
std::ostream& dumpExtendedInstance(std::ostream& os, RawInstance value);
std::ostream& dumpExtendedLayout(std::ostream& os, RawLayout value,
                                 const char* indent);
std::ostream& dumpExtendedType(std::ostream& os, RawType value);
std::ostream& dumpExtended(std::ostream& os, RawObject value);

std::ostream& operator<<(std::ostream& os, CastError value);

std::ostream& operator<<(std::ostream& os, RawBool value);
std::ostream& operator<<(std::ostream& os, RawBoundMethod value);
std::ostream& operator<<(std::ostream& os, RawBytearray value);
std::ostream& operator<<(std::ostream& os, RawBytes value);
std::ostream& operator<<(std::ostream& os, RawCode value);
std::ostream& operator<<(std::ostream& os, RawDict value);
std::ostream& operator<<(std::ostream& os, RawError value);
std::ostream& operator<<(std::ostream& os, RawFloat value);
std::ostream& operator<<(std::ostream& os, RawFunction value);
std::ostream& operator<<(std::ostream& os, RawInt value);
std::ostream& operator<<(std::ostream& os, RawLargeInt value);
std::ostream& operator<<(std::ostream& os, RawLargeStr value);
std::ostream& operator<<(std::ostream& os, RawLayout value);
std::ostream& operator<<(std::ostream& os, RawList value);
std::ostream& operator<<(std::ostream& os, RawModule value);
std::ostream& operator<<(std::ostream& os, RawMutableTuple value);
std::ostream& operator<<(std::ostream& os, RawNoneType value);
std::ostream& operator<<(std::ostream& os, RawObject value);
std::ostream& operator<<(std::ostream& os, RawSmallInt value);
std::ostream& operator<<(std::ostream& os, RawSmallStr value);
std::ostream& operator<<(std::ostream& os, RawStr value);
std::ostream& operator<<(std::ostream& os, RawTuple value);
std::ostream& operator<<(std::ostream& os, RawType value);
std::ostream& operator<<(std::ostream& os, RawValueCell value);
template <typename T>
std::ostream& operator<<(std::ostream& os, const Handle<T>& value) {
  return os << *value;
}

std::ostream& operator<<(std::ostream& os, Frame* frame);
std::ostream& operator<<(std::ostream& os, Thread* thread);

void dump(RawObject object);
void dump(const Object& object);
void dump(Frame* frame);
void dumpPendingException(Thread* thread);
void dumpSingleFrame(Frame* frame);
void dumpTraceback();
void dump(PyObject* obj);

void initializeDebugging();

}  // namespace py
