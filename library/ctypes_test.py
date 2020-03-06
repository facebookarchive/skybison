#!/usr/bin/env python3
import ctypes
import ctypes.util
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

    def test_loaded_library_has_function_attrs(self):
        libc_name = ctypes.util.find_library("c")
        libc = ctypes.CDLL(libc_name)
        self.assertTrue(hasattr(libc, "abort"))
        self.assertTrue(hasattr(libc, "printf"))
        self.assertFalse(hasattr(libc, "DoesNotExist"))
        self.assertIsNotNone(libc.abort)
        self.assertIsNotNone(libc.printf)

    def test_import_uuid(self):
        # importing uuid exercises ctypes, make sure it doesn't break
        import uuid

        self.assertIsNotNone(uuid)


if __name__ == "__main__":
    unittest.main()
