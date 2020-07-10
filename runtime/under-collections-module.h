#pragma once

#include "globals.h"
#include "handles-decl.h"

namespace py {

class Thread;

// returns the index relative to the underlying tuple
word dequeIndex(const Deque& deque, word index);

void initializeUnderCollectionsTypes(Thread* thread);

}  // namespace py
