#pragma once

namespace py {

class Thread;

void importAcquireLock(Thread* thread);
bool importReleaseLock(Thread* thread);

}  // namespace py
