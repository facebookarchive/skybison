# Welcome to Skybison!

Skybison is experimental performance-oriented greenfield implementation of
Python 3.8. It contains a number of performance optimizations, including: small
objects; a moving GC; hidden classes; bytecode inline caching; type-specialized
bytecode; an experimental template JIT.

## Is this supported?

No.

Even though the project is no longer under active development internally, we've
made Skybison publicly available in the hopes that people might find its
history and internals interesting, or maybe even useful. It has not been
polished or documented for anyone else's use.

We cannot commit to fixing external bug reports or reviewing pull requests.
That said, if you have experience in dynamic language runtimes and have ideas
to make Skybison faster; or if you work on CPython and want to use Skybison as
inspiration for improvements in CPython, feel free to open a Pull Request.

Instagram is now developing
[Cinder](https://github.com/facebookincubator/cinder/).

## How do I build it?

See [INSTALL.md](doc/INSTALL.md) for an overview. However, this project was
built in an internal development environment. We cannot guarantee that this
project will build or run on any arbitrary environment.

## What's here?

The project is structured in the following component directories:

### `./runtime`

The core of the runtime resides here. In it is all of the core types, core
modules, bytecode interpreter, and template JIT.

* `objects.[h|cpp]` - All object types. See
  [object-model.md](doc/object-model.md).
* `*-builtins.[h|cpp]` - Basic functionality of builtin types
* `*-module.[h|cpp]` - Implementation of built-in modules and types correlated
  to a module.
* `interpreter.[h|cpp]` - Implementation of all opcodes. See
  [execution-model.md](doc/execution-model.md).
* `runtime/interpreter-gen-x64.cpp` - Implementation of the assembly
  interpreter and template JIT.
* `heap.[h|cpp]` - Class that manages managed objects. See
  [garbage-collection.md](doc/garbage-collection.md).
* `runtime.[h|cpp]` - The Runtime. This is where everything starts.

### `./library`

The Python library code resides here. In it is a Skybison-specific set of
library files and their tests. It is meant to be overlaid on top of the CPython
standard library so that imports fall back on standard library code.

### `./capi` and `./ext`

Our C-API emulation layer resides here. Since any C code that interfaces with
Python's C-API expects `PyObject*` not to move, and the runtime has a moving
GC, we have built a layer of handle objects and C-API functions.

The structure inside this `./ext` is designed to mimic the CPython repo. For
example, CPython defines `PyLong_FromLong` in `Objects/longobject.c`. In
Skybison, this function should be implemented in `ext/Objects/longobject.cpp`.

### `./third-party`

We vendor several open-source projects for both testing and core functionality.
They all reside here. Included in this list is a version of CPython, which we
use for its Python and C library code.

### `./util`

Development tools and helper scripts for building and debugging.

### `./benchmarks`

Set of standard Python benchmarks with an accompanying test-runner.

### `./doc`

This folder contains documentation.

## Other notes

The repository has the name "pyro" sprinkled throughout the codebase. This is a
holdover from an old internal codename.

## License

Please see [LICENSE](LICENSE).
