#pragma once

#include "Globals.h"
#include "Utils.h"

namespace python {

class Frame;

class Thread {
 public:
  explicit Thread(int size) : size_(Utils::RoundUp(size, kPointerSize)) {
    start_ = ptr_ = new byte[size];
    end_ = start_ + size;
  }
  ~Thread() {
    delete[] start_;
  }

  Frame* pushFrame(Object *code);
  void popFrame(Frame* frame);

  Object* run(Object* object);

 private:
  int size_;
  byte* start_;
  byte* end_;
  byte* ptr_;

  DISALLOW_COPY_AND_ASSIGN(Thread);
};

}
