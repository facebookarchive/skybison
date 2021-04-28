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

    def test_array_subclass_is_instantiable(self):
        class ArraySub(ctypes.Array):
            _length_ = 2
            _type_ = ctypes.c_bool

        self.assertEqual(ctypes.sizeof(ArraySub()), 2)

    def test_array_without_length_raises_attribute_error(self):
        with self.assertRaises(AttributeError) as ctx:

            class ArraySub(ctypes.Array):
                _type_ = ctypes.c_bool

        self.assertEqual(str(ctx.exception), "class must define a '_length_' attribute")

    def test_array_with_non_int_length_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:

            class ArraySub(ctypes.Array):
                _length_ = 1.0
                _type_ = ctypes.c_bool

        self.assertEqual(
            str(ctx.exception), "The '_length_' attribute must be an integer"
        )

    def test_array_with_negative_length_raises_attribute_error(self):
        with self.assertRaises(ValueError) as ctx:

            class ArraySub(ctypes.Array):
                _length_ = -1
                _type_ = ctypes.c_bool

        self.assertEqual(
            str(ctx.exception), "The '_length_' attribute must not be negative"
        )

    def test_array_without_type_raises_attribute_error(self):
        with self.assertRaises(AttributeError) as ctx:

            class ArraySub(ctypes.Array):
                _length_ = 1

        self.assertEqual(str(ctx.exception), "class must define a '_type_' attribute")

    def test_array_with_non_class_type_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:

            class ArraySub(ctypes.Array):
                _length_ = 1
                _type_ = "b"

        self.assertEqual(str(ctx.exception), "_type_ must have storage info")

    def test_array_with_non_CData_type_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:

            class ArraySub(ctypes.Array):
                _length_ = 1
                _type_ = int

        self.assertEqual(str(ctx.exception), "_type_ must have storage info")

    def test_cfuncptr_dunder_call_no_args_c_int_restype_returns_int(self):
        lib_name = ctypes.util.find_library("python")
        lib = ctypes.CDLL(lib_name)
        recursion_limit = lib.Py_GetRecursionLimit
        self.assertIs(recursion_limit.restype, ctypes.c_int)
        result = recursion_limit()
        self.assertIs(type(result), int)
        self.assertEqual(result, 1000)

    def test_cfuncptr_dunder_call_no_args_c_char_p_restype_returns_bytes(self):
        lib_name = ctypes.util.find_library("python")
        lib = ctypes.CDLL(lib_name)
        get_platform = lib.Py_GetPlatform
        self.assertIs(get_platform.restype, ctypes.c_int)
        get_platform.restype = ctypes.c_char_p
        result = get_platform()
        self.assertIs(type(result), bytes)
        self.assertTrue(result in [b"linux", b"darwin"])

    def test_char_array_from_buffer_is_readable_and_writeable(self):
        import mmap

        m = mmap.mmap(-1, 6)
        array_type = ctypes.c_char * 6
        c = array_type.from_buffer(memoryview(m))
        view = memoryview(m)
        view[0] = ord("f")
        view[1] = ord("b")
        self.assertEqual(c.value, b"fb")
        c.value = b"foobar"
        self.assertEqual(c.value, b"foobar")
        self.assertEqual(view.tolist(), list(b"foobar"))

    def test_char_array_with_zero_initialization_rewrites_first_byte_memory(self):
        import mmap

        m = mmap.mmap(-1, 3)
        array_type = ctypes.c_char * 3
        view = memoryview(m)
        view[:3] = b"foo"
        c = array_type.from_buffer(memoryview(m))
        c.__init__(b"\0")
        self.assertEqual(c.value, b"")
        self.assertEqual(view.tolist(), list(b"\0oo"))

    def test_char_array_with_non_bytes_value_raises_type_error(self):
        c = (ctypes.c_char * 3)()
        with self.assertRaises(TypeError) as ctx:
            c.value = 1
        self.assertEqual(str(ctx.exception), "bytes expected instead of int instance")

    def test_char_array_with_too_long_value_raises_value_error(self):
        c = (ctypes.c_char * 3)()
        with self.assertRaises(ValueError) as ctx:
            c.value = b"foobar"
        self.assertEqual(str(ctx.exception), "byte string too long")

    def test_non_char_array_class_raises_attribute_error_on_value(self):
        array_type = ctypes.c_uint16 * 5
        with self.assertRaises(AttributeError) as ctx:
            array_type().value
        self.assertEqual(
            str(ctx.exception), "'c_ushort_Array_5' object has no attribute 'value'"
        )

    def test_ctypes_array_creation_with_type_returns_array_type(self):
        arr = ctypes.c_ubyte * 14
        self.assertEqual(arr._type_, ctypes.c_ubyte)
        self.assertEqual(arr._length_, 14)
        self.assertEqual(arr.__name__, "c_ubyte_Array_14")

    def test_ctypes_array_creation_cache_returns_same_instance_for_same_type(self):
        arr1 = ctypes.c_ubyte * 2
        arr2 = ctypes.c_ubyte * 2
        self.assertIs(arr1, arr2)

    # TODO(T47024191): Since inline caches hold strongrefs to their objects, array
    # types don't get collected on _gc().
    @unittest.skip("Disable until our inline caches store weakrefs")
    @pyro_only
    def test_ctypes_array_creation_cache_cleared_when_type_gced(self):
        from _ctypes import _array_from_ctype_cache

        from _builtins import _gc

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

    def test_memset_sets_memoryview_array(self):
        import mmap

        m = mmap.mmap(-1, 4)
        view = memoryview(m)
        view[:] = b"1234"
        array = (ctypes.c_char * 4).from_buffer(view)
        array_addr = ctypes.addressof(array)
        self.assertEqual(array_addr, ctypes.memset(array_addr, 1, 2))
        self.assertEqual(view.tolist(), list(b"\x01\x0134"))

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

    def test_sizeof_array_returns_length(self):
        self.assertEqual(ctypes.sizeof(ctypes.c_char * 5), 5)
        self.assertEqual(ctypes.sizeof(ctypes.c_uint16 * 20), 40)
        self.assertEqual(ctypes.sizeof(ctypes.c_uint16 * 5 * 5), 50)


if __name__ == "__main__":
    unittest.main()
