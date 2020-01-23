#!/usr/bin/env python3

from _builtins import _patch


@_patch
def callgrind_dump_stats(description=None):
    pass


@_patch
def callgrind_start_instrumentation():
    pass


@_patch
def callgrind_stop_instrumentation():
    pass


@_patch
def callgrind_zero_stats():
    pass
