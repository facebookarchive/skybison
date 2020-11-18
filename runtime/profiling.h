#pragma once

namespace py {

class Thread;

void profiling_call(Thread* thread);

void profiling_return(Thread* thread);

}  // namespace py
