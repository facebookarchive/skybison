# Execution model

Skybison is just the runtime, not a complete Python implementation. CPython
compiles Python code into *bytecode* that's typically stored in the `.pyc`
files. Unlike PyPy, we don't have a custom bytecode and we don't plan to have
one. In fact, we include a fork of CPython's compiler and ask it to compile the
code for us. Skybison is just trying to execute the bytecode faster while
preserving the behavior of CPython as closely as possible.

To understand Skybison, it's useful to understand how CPython works internally.
Check out the resources on the [CPython Developer
Guide](https://devguide.python.org/exploring/).

> NOTE: This document was written during early project planning stages. Parts
> of it may be out of date.

## CPython Bytecode

The [compileall module](https://docs.python.org/3/library/compileall.html) can
be used to compile a file or all files in a directory:

```
$ python3 -m compileall -b [FILE|DIR]
```

This will create `.pyc` files in the same directory. If you actually want to
see the bytecode, use the [dis
module](https://docs.python.org/3/library/dis.html):

```
$ python3 -m dis [FILE]
```

or from inside Python:

```
>>> import dis
>>> dis.dis("bar = foo()")
  1           0 LOAD_NAME                0 (foo)
              2 CALL_FUNCTION            0
              4 STORE_NAME               1 (bar)
              6 LOAD_CONST               0 (None)
              8 RETURN_VALUE
```

Complete list of bytecodes with descriptions [can be found
here](https://docs.python.org/3/library/dis.html#python-bytecode-instructions),
but this should only be used as an overview, not a specification of what the
bytecode does. To understand what exactly a bytecode does, read the actual
CPython implementation in
[Python/ceval.c](https://github.com/python/cpython/blob/master/Python/ceval.c).

In Skybison, the list of bytecodes can be found in `runtime/bytecode.h` - and
they are all mapped to interpreter functions implemented in
`runtime/interpreter.cpp`.

## Interpreter

Skybison has two implementations of its core interpreter loop (opcode dispatch
and evaluation):

* C++ Interpreter (see `runtime/interpreter.cpp`)
* Dynamically-Generated x64 Assembly Interpreter (see `runtime/interpreter-gen-x64.cpp`)

The assembly interpreter contains optimized versions of **some** opcodes; the
rest simply call back into the C++ interpreter. We can also execute the the C++
interpreter on its own (for non-x64 platforms).

In addition to hand-optimizing the native code generated for each opcode
handler, the assembly interpreter also allows us to have regularly-sized opcode
handlers, spaced such that the address of a handler can be computed with a base
address and the opcode's value. A few special pseudo-handlers are at negative
offsets from the base address, which are used to handle control flow such as
exceptions and returning.

The assembly interpreter is dynamically generated at runtime startup; for
background on this technique you can read the paper "[Generation of Virtual
Machine Code at
Startup](https://pdfs.semanticscholar.org/316a/9f4f2e614226b3e613a229a7e3a3f44b1351.pdf)".

There is another important behavioral distinction between the two interpreters
in how they manage and execute call frames, which is described below.

## Frame Management

An interpreted virtual machine needs to maintain two execution stacks - one
used by the natively executing interpreter for its own calls, and the one that
represents the stack of the virtual machine.

CPython accomplishes this by maintaining a virtual Python stack using a linked
list of `Frame` objects. Since Python provides multiple ways to inspect the
current state of the interpreter, internal objects like Frame or Thread have to
be treated as other managed objects - and Python code can retrieve them using
functions like
[inspect.currentframe](https://docs.python.org/3/library/inspect.html#inspect.currentframe)
or
[threading.current_thread](https://docs.python.org/3/library/threading.html#threading.current_thread).
However, this is inefficient: frames are inspected rarely but incur consistent
allocation overhead, and the calling convention does not match the native
machine's.

Skybison's approach is to allocate an "alternate" stack, created in
`Thread::Thread()`, which is used as the managed stack, and is laid out similar
to the machine stack. The assembly interpreter runs directly on this stack (the
machine's stack pointer registers point into this stack), allowing call /
return idioms to use native instructions.

The C++ interpreter also knows about the managed stack (and understands the
frame layout), but it executes on the native stack ([see this Quip for
details](https://fb.quip.com/VzrCAqXJ7fZn)). When we need to execute C++ code
(such as a Python function or method which is implemented in C++) we have to
jump back to executing on the native stack, incurring a performance penalty
(need to save and restore machine registers).

## Exceptions

Except for bytecode handlers in the interpreter, the procedure for raising an
exception is to set a pending exception on the current `Thread` and return an
`Error` object. In the vast majority of cases, this looks something like this:

```cpp
if (value < 0) {
  return thread->raiseWithFmt(LayoutId::kValueError, "value cannot be negative");
}
```

Consequently, if your code calls a function that could raise an exception, you
must check for and forward any `Error` return values:

```cpp
Object result(&scope, someFunction());
if (result.isError()) return *result;
```

Note that you should **return** the `Error` object **from the callee**, rather
than `Error::error()`. Although the various `Error` types are currently
singletons, we have plans to encode information about the type of the exception
into the `Error` object at some point. This will allow us to perform parts of
the unwinding process with only the `Error` object, avoiding loads from the
current `Thread`.

### In the Interpreter

In the bytecode handler functions (`Interpreter::doLoadFast`, etc.), the first
step is the same: set the appropriate thread-local state with
`Thread::raise*()`. Then, instead of returning an `Error` object, return
`Continue::UNWIND`. This begins the actual unwinding process, including finding
and jumping to an `except:` handler when appropriate.

### In Tests

To test that your function throws the right exception, use the
`testing::raisedWithStr()` predicate, like so:

```cpp
EXPECT_TRUE(raisedWithStr(runBuiltin(FooBuiltins::bar, some_obj),
                          LayoutId::kTypeError, "Argument 1 must be int, str given"));
```

In addition to being more compact than manually inspecting all the relevant
thread-local state, using `raisedWithStr()` will make your test future-proof
for when we encode meaning into the `Error` object.
