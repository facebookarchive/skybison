#pragma once

#include "frame.h"
#include "globals.h"
#include "objects.h"
#include "runtime.h"
#include "thread.h"

namespace py {

RawObject tupleContains(Thread* thread, const Tuple& tuple,
                        const Object& value);

// Return the next item from the iterator, or Error if there are no items left.
RawObject tupleIteratorNext(Thread* thread, const TupleIterator& iter);

// Return a slice of the given tuple.
RawObject tupleSlice(Thread* thread, const Tuple& tuple, word start, word stop,
                     word step);

RawObject tupleHash(Thread* thread, const Tuple& tuple);

void initializeTupleTypes(Thread* thread);

}  // namespace py
