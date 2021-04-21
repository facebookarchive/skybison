#pragma once

#include "objects.h"

namespace py {

class Runtime;
class Scavenger;

bool isWhiteObject(Scavenger* scavenger, RawHeapObject object);

RawObject scavenge(Runtime* runtime);

void scavengeImmortalize(Runtime* runtime);

}  // namespace py
