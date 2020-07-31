#pragma once

#include "objects.h"

namespace py {

class Runtime;

RawObject scavenge(Runtime* runtime);

}  // namespace py
