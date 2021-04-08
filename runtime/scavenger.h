#pragma once

#include "objects.h"

namespace py {

class Runtime;

RawObject scavenge(Runtime* runtime);

void scavengeImmortalize(Runtime* runtime);

}  // namespace py
