#!/usr/bin/env python3
import ctypes
import ctypes.util
import unittest

from test_support import pyro_only


class CtypesTests(unittest.TestCase):
    def test_addressof_non_cdata_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:
            ctypes.addressof("not-CData")
        self.assertEqual(str(ctx.exception), "invalid type")

    def test_addressof_mmap_returns_int_greater_than_zero(self):
        import mmap

        view = memoryview(mmap.mmap(-1, 2))
        uint = ctypes.c_uint16.from_buffer(view)
        addr = ctypes.addressof(uint)
        self.assertIsInstance(addr, int)
        self.assertGreater(addr, 0)

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

    def test_ctypes_ulonglong_creation_stores_long(self):
        self.assertEqual(ctypes.c_ulonglong().value, 0)
        self.assertEqual(ctypes.c_ulonglong(0xFF << 32).value, 0xFF << 32)

    def test_from_buffer_with_memory_reflects_memory(self):
        import mmap

        m = mmap.mmap(-1, 2)
        c = ctypes.c_uint16.from_buffer(memoryview(m))
        view = memoryview(m)
        view[0] = ord("f")
        view[1] = ord("b")
        self.assertEqual(c.value, (view[1] << 8) + view[0])

    def test_from_buffer_with_abstract_class_raises_type_error(self):
        import _ctypes

        with self.assertRaises(TypeError) as context:
            _ctypes._SimpleCData.from_buffer(b"as")
        self.assertEqual(str(context.exception), "abstract class")

    def test_from_buffer_with_readonly_buffer_raises_type_error(self):
        import mmap

        m = mmap.mmap(-1, 2, prot=mmap.PROT_READ)
        with self.assertRaises(TypeError) as context:
            ctypes.c_uint16.from_buffer(memoryview(m))
        self.assertEqual(str(context.exception), "underlying buffer is not writable")

    def test_from_buffer_with_negative_offset_raises_value_error(self):
        import mmap

        m = mmap.mmap(-1, 2)
        with self.assertRaises(ValueError) as context:
            ctypes.c_uint16.from_buffer(memoryview(m), -1)
        self.assertEqual(str(context.exception), "offset cannot be negative")

    def test_from_buffer_with_too_small_buffer_raises_value_error(self):
        import mmap

        m = mmap.mmap(-1, 1)
        with self.assertRaises(ValueError) as context:
            ctypes.c_uint16.from_buffer(memoryview(m))
        self.assertEqual(
            str(context.exception),
            "Buffer size too small (1 instead of at least 2 bytes)",
        )

    def test_from_buffer_with_different_sizes_read_correct_number_of_bytes(self):
        import mmap

        m = mmap.mmap(-1, 8)
        view = memoryview(m)
        view[0] = 0xFF
        view[3] = 0xFF
        short = ctypes.c_uint16.from_buffer(view)
        longlong = ctypes.c_ulonglong.from_buffer(view)
        self.assertEqual(short.value, 0xFF)
        self.assertEqual(longlong.value, (0xFF << 24) + 0xFF)

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

    def test_sizeof_non_cdata_type_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:
            ctypes.sizeof(TypeError)
        self.assertEqual(str(ctx.exception), "this type has no size")

    def test_sizeof_type_returns_size(self):
        self.assertEqual(ctypes.sizeof(ctypes.c_bool), 1)
        self.assertEqual(ctypes.sizeof(ctypes.c_uint16), 2)

    def test_sizeof_object_with_memory_returns_size_of_type(self):
        import mmap

        view = memoryview(mmap.mmap(-1, 6))
        uint = ctypes.c_uint16.from_buffer(view)
        self.assertEqual(ctypes.sizeof(uint), 2)


if __name__ == "__main__":
    unittest.main()
