#include "handles.h"

#include <cstring>

#include "visitor.h"

namespace python {

Handles::Handles()
    : size_(kInitialSize), scopes_(new HandleScope*[kInitialSize]), top_(0) {}

Handles::~Handles() {
  delete[] scopes_;
}

void Handles::grow() {
  word new_size = size_ * 2;
  auto new_scopes = new HandleScope*[new_size];
  memcpy(new_scopes, scopes_, size_ * sizeof(scopes_[0]));
  delete[] scopes_;
  size_ = new_size;
  scopes_ = new_scopes;
}

void Handles::visitPointers(PointerVisitor* visitor) {
  for (word i = top_ - 1; i >= 0; i--) {
    HandleScope* scope = scopes_[i];
    ObjectHandle* handle = scope->list();
    while (handle != nullptr) {
      visitor->visitPointer(handle->pointer());
      handle = handle->next();
    }
  }
}

} // namespace python
