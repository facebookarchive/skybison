#!/usr/bin/env python3
"""The operator module. Also, utility functions the C-API uses to do the heavy
lifting."""


def countOf(seq, v):
    count = 0
    for item in seq:
        if item == v:
            count += 1
    return count


def indexOf(seq, v):
    idx = 0
    for item in seq:
        if item == v:
            return idx
        idx += 1
    return -1


def contains(seq, v):
    return v in seq
