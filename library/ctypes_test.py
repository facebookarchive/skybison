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

    def test_ctypes_uint16_creation_stores_uint16(self):
        self.assertEqual(ctypes.c_uint16().value, 0)
        self.assertEqual(ctypes.c_uint16(10).value, 10)
        self.assertEqual(ctypes.c_uint16(True).value, 1)

    def test_ctypes_uint16_value_is_modifiable(self):
        uint = ctypes.c_uint16()
        self.assertEqual(uint.value, 0)
        uint.value = 12345
        self.assertEqual(uint.value, 12345)
        with self.assertRaises(TypeError) as ctx:
            uint.value = 1.0
        self.assertEqual(str(ctx.exception), "int expected instead of float")

    def test_ctypes_uint16_with_wrong_type_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:
            ctypes.c_uint16("1")
        self.assertEqual(str(ctx.exception), "an integer is required (got type str)")

    def test_ctypes_uint16_with_float_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:
            ctypes.c_uint16(1.0)
        self.assertEqual(str(ctx.exception), "int expected instead of float")

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
