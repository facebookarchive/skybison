#pragma once

#include "globals.h"
#include "handles.h"
#include "objects.h"

namespace py {

// A generic handle allowing uniform use of bytes-like objects, which are
// objects that implement the buffer protocol in CPython.
class Byteslike {
 public:
  // Initialize handle. It is allowed to pass a non-byteslike object and then
  // use `isValid()` to test. The other methods must only be called when a
  // byteslike object was used in the constructor.
  Byteslike(HandleScope* scope, Thread* thread, RawObject object);
  ~Byteslike();

  uword address() const;

  byte byteAt(word index) const;

  bool isValid() const;

  word length() const;

 private:
  void initWithLargeBytes(HandleScope* scope, RawLargeBytes bytes, word length);
  void initWithMemory(byte* data, word length);
  void initWithSmallData(RawSmallBytes bytes, word length);

  union {
    uword reference;
    struct {
      RawObject object;
      Handles* handles;
    } handle;
    struct {
      uword reference;
      RawSmallBytes small_storage;
    } small;
  } d_;
  Handle<RawObject>* next_;
  word length_;
};

inline Byteslike::~Byteslike() {
  if (next_ != nullptr) {
    d_.handle.handles->pop(next_);
  }
}

inline uword Byteslike::address() const {
  return d_.reference - RawObject::kHeapObjectTag;
}

inline byte Byteslike::byteAt(word index) const {
  DCHECK_INDEX(index, length());
  return *reinterpret_cast<byte*>(address() + index);
}

inline bool Byteslike::isValid() const {
  return !d_.handle.object.isErrorError();
}

inline word Byteslike::length() const { return length_; }

}  // namespace py
