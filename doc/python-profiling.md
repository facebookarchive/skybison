# `_profiler` Module

Skybison now comes with a built-in python profiler that counts executed opcodes
and produces call data in callgrind format that can be read with
qcachegrind/kcachegrind (see [cpp-profiling](cpp-profiling.md) for Tips about
qcachegrind).

Note: Counting opcodes isn't necessarily the best measure for performance... we
plan to add other metrics later.

Basic Usage
========

Profiling is enable form sourcecode:
```
import _profiler

_profiler.install()
# Code Here will be profiled, you can dump intermediate state at any time:

_profiler.dump_callgrind("phase1.cg")      # dumps phase1.cg file, and resets counters
...
_profiler.dump_callgrind("phase2.cg")
...
```

Note: `_profiler.install()` also currently has a limitation where it does not
always affect outer callframes and may miss opcodes there. It is recommended to
only call it at the module level to avoid this problem (and not inside
functions). Calling `dump_callgrind` in fine anywhere.
