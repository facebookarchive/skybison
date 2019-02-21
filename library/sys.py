#!/usr/bin/env python3
"""The sys module"""


def exit(code=0):
    raise SystemExit(code)


path_hooks = []


path_importer_cache = {}


# TODO(T39224400): Implement flags as a structsequence
class FlagsStructSeq:
    def __init__(self):
        self.verbose = 0


flags = FlagsStructSeq()
