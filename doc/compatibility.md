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

- Type dictionaries must only have `str` keys that do not override `__eq__` or
  `__hash__`. This Example will raise a `TypeError`:
```python
>>> class A:
...   locals()[500] = 1
...
TypeError: '_type_init' requires a 'str' object but got 'int'
```

- Object attributes must only have `str` names that do not override `__eq__` or
  `__hash__`. Identities of attribute names may not preserved.

- `str.__add__` with a non-str other returns `NotImplemented` in PyRo and PyPy,
  but raises a `TypeError` in CPython. This is because PyRo and PyPy do not
  share CPython's notions of different types of slots (eg sequence vs number)
  and that would make it very tricky to support this type of behavior.

Interpreter
-----------

- Changing `__builtins__` has an immediate effect, as opposed to CPython which
  caches the value and only updates the cache when calling across module
  boundaries. We generally do not recommend to reassign `__builtins__` and may
  further limit the ability to do so in the future.

C-API
-----

- `PyCFunction_Type` and `PyCFunction_Check` are not supported;
  `PyCFunction_New` works and returns something that behaves similar.


Runtime Inspection
------------------

- `sys._getframe()` always returns a new object whereas CPython returns an
  identical object for the same frame:
```python
>>> sys._getframe(0) is sys._getframe(0)
False
```
