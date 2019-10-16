#!/usr/bin/env python3


# These values are injected by our boot process. flake8 has no knowledge about
# their definitions and will complain without these circular assignments.
_patch = _patch  # noqa: F821


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
