#pragma once

#include "globals.h"

namespace python {

class Object;

class Frame {
 public:
  // PyObject_VAR_HEAD
  Frame* f_back; /* previous frame, or NULL */
  Object* f_code; /* code segment */
  Object* f_builtins; /* builtin symbol table (PyDictObject) */
  Object* f_globals; /* global symbol table (PyDictObject) */
  Object* f_locals; /* local symbol table (any mapping) */
  Object** f_valuestack; /* points after the last local */
  /* Next free slot in f_valuestack.  Frame creation sets to f_valuestack.
     Frame evaluation usually NULLs it, but a frame that yields sets it
     to the current stack top. */
  Object** f_stacktop;
  Object* f_trace; /* Trace function */
  char f_trace_lines; /* Emit per-line trace events? */
  char f_trace_opcodes; /* Emit per-opcode trace events? */

  /* In a generator, we need to be able to swap between the exception
     state inside the generator and the exception state of the calling
     frame (which shouldn't be impacted when the generator "yields"
     from an except handler).
     These three fields exist exactly for that, and are unused for
     non-generator frames. See the save_exc_state and swap_exc_state
     functions in ceval.c for details of their use. */
  Object *f_exc_type, *f_exc_value, *f_exc_traceback;
  /* Borrowed reference to a generator, or NULL */
  Object* f_gen;

  int f_lasti; /* Last instruction if called */
  /* Call PyFrame_GetLineNumber() instead of reading this field
     directly.  As of 2.3 f_lineno is only valid when tracing is
     active (i.e. when f_trace is set).  At other times we use
     PyCode_Addr2Line to calculate the line from the current
     bytecode index. */
  int f_lineno; /* Current line number */
  int f_iblock; /* index in f_blockstack */
  char f_executing; /* whether the frame is still executing */
  // PyTryBlock f_blockstack[CO_MAXBLOCKS]; /* for try and loop blocks */
  Object* f_localsplus[1]; /* locals+stack, dynamically sized */

 private:
  DISALLOW_COPY_AND_ASSIGN(Frame);
};

} // namespace python
