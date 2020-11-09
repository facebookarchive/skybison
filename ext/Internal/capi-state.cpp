#include "capi-state.h"

#include <new>

#include "capi-handles.h"
#include "dict-builtins.h"
#include "globals.h"
#include "handles-decl.h"
#include "handles.h"
#include "runtime.h"
#include "thread.h"
#include "visitor.h"

namespace py {

static const word kInitialCachesCapacity = 128;
static const word kInitialHandlesCapacity = 256;

void capiStateVisit(CAPIState* state, PointerVisitor* visitor) {
  state->handles_.visit(visitor);
  ApiHandle::visitReferences(&state->handles_, visitor);
  state->caches_.visit(visitor);
}

void finalizeCAPIState(Runtime* runtime) { runtime->capiState()->~CAPIState(); }

void initializeCAPIState(Runtime* runtime) {
  CAPIState* state = runtime->capiState();
  new (state) CAPIState;
  state->caches_.initialize(runtime, kInitialCachesCapacity);
  state->handles_.initialize(runtime, kInitialHandlesCapacity);
}

word numTrackedApiHandles(Runtime* runtime) {
  return capiHandles(runtime)->numItems();
}

}  // namespace py
