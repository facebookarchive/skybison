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

- A dictionary passed to the `globals` parameter of `builtins.exec`,
  `builtins.eval`, `PyEval_EvalCode` is not updated while the code is
  being executed, but will be updated only upon completion of the call.
  Note that this does not affect passing `<some_module>.__dict__` for
  globals; `__dict__` of a module returns a specialized proxy object
  which leads to module attributes being updated immediately as the
  code is running.

- Similarly, a dictionary passed to the `globals` parameter of the function
  constructor (`types.FunctionType`), will have its contents copied to a
  specialized proxy object inside a module. This means that any changes to
  globals from within the function will not reflect in the dictionary
  passed-in, but instead in the function's module proxy object. The following
  test will fail:

        def _f():
            global foo
            foo = "baz"

        d = {"foo": "bar"}
        result = types.FunctionType(_f.__code__, d, name="hi")
        self.assertEqual(d["foo"], "baz")

C-API
-----

- `PyCFunction_Type` and `PyCFunction_Check` are not supported;
  `PyCFunction_New` works and returns something that behaves similar.

- `PyEval_EvalCode`: See `builtins.exec`.

Runtime Inspection
------------------

- `sys._getframe()` always returns a new object whereas CPython returns an
  identical object for the same frame:
```python
>>> sys._getframe(0) is sys._getframe(0)
False
```
