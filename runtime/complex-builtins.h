#pragma once

#include "objects.h"

namespace py {

class Thread;

word complexHash(RawObject value);

void initializeComplexType(Thread* thread);

}  // namespace py
