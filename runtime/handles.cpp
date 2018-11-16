#include "handles.h"

#include "visitor.h"

namespace python {

Handles::Handles() { scopes_.reserve(kInitialSize); }

void Handles::visitPointers(PointerVisitor* visitor) {
  for (auto const& scope : scopes_) {
    Handle<RawObject>* handle = scope->list();
    while (handle != nullptr) {
      visitor->visitPointer(handle->pointer());
      handle = handle->next();
    }
  }
}

}  // namespace python
