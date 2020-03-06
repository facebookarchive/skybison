#!/usr/bin/env python3
import ctypes
import unittest

from test_support import pyro_only


class CtypesTests(unittest.TestCase):
    def test_ctypes_array_creation_with_type_returns_array_type(self):
        arr = ctypes.c_ubyte * 14
        self.assertEqual(arr._type_, ctypes.c_ubyte)
        self.assertEqual(arr._length_, 14)
        self.assertEqual(arr.__name__, "c_ubyte_Array_14")

    def test_ctypes_array_creation_cache_returns_same_instance_for_same_type(self):
        arr1 = ctypes.c_ubyte * 2
        arr2 = ctypes.c_ubyte * 2
        self.assertIs(arr1, arr2)

    @pyro_only
    def test_ctypes_array_creation_cache_cleared_when_type_gced(self):
        from _builtins import _gc
        from _ctypes import _array_from_ctype_cache

        ctypes.c_ubyte * 2
        self.assertGreater(len(_array_from_ctype_cache), 0)
        _gc()
        self.assertEqual(len(_array_from_ctype_cache), 0)


if __name__ == "__main__":
    unittest.main()
