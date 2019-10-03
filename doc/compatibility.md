CPython Compatibility
=====================

This runtime intends to be a drop-in replacement for CPython. Occasionally, it
is advantageous for performance to deviate in small ways. These deviations
should not affect the majority of programs and it should be easy to adapt the
rest.

This document describes the differences.  Note that it does not cover features
that are planned but not implemented yet.


Built-in Types
--------------

- `module.__dict__` is always a dictionary and initialized in `module.__new__`;
  CPython initializes it with `None` and only sets it to a dictionary in
  `module.__init__`.


Interpreter
-----------

- Changing `__builtins__` has an immediate effect, as opposed to CPython which
  caches the value and only updates the cache when calling across module
  boundaries. We generally do not recommend to reassign `__builtins__` and may
  further limit the ability to do so in the future.
