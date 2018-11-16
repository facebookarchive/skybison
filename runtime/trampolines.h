#pragma once

namespace python {

class Frame;
class Object;
class Thread;

/**
 * An entry point into a function.
 *
 * A FunctionTrampoline is responsible for ensuring that the arguments on the
 * stack are in the canonical form, allocating a stack frame, calling into the
 * function, and cleaning up after itself.
 */
using FunctionTrampoline = Object* (*)(Thread*, Frame*, int argc);

// Decode a trampoline from a SmallInteger
FunctionTrampoline trampolineFromObject(Object* object);

// Encode a trampoline as a SmallInteger
Object* trampolineToObject(FunctionTrampoline trampoline);

// Entry point for an interpreted function with no defaults invoked via
// CALL_FUNCTION
Object* interpreterTrampoline(Thread* thread, Frame* previousFrame, int argc);

// Entry point for an interpreted function with no defaults invoked via
// CALL_FUNCTION_KW
Object* interpreterTrampolineKw(Thread* thread, Frame* previousFrame, int argc);

// Aborts immediately when called
Object* unimplementedTrampoline(Thread* thread, Frame* previousFrame, int argc);

} // namespace python
