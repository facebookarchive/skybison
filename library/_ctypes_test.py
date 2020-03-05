#!/usr/bin/env python3
import _ctypes
import unittest


class UnderCtypesTests(unittest.TestCase):
    def test_ctypes_imports_cleanly(self):
        # Importing ctypes exercises a lot of the functionality in _ctypes
        # (e.g. type size sanity-checking)
        import ctypes

        self.assertIsNotNone(ctypes)

    def test_simple_c_data_class(self):
        class py_object(_ctypes._SimpleCData):
            _type_ = "O"

        self.assertEqual(py_object._type_, "O")

    def test_simple_c_data_class_with_no_type_raises_attribute_error(self):
        with self.assertRaises(AttributeError) as context:

            class CType(_ctypes._SimpleCData):
                pass

            self.assertEqual(
                str(context.exception), "class must define a '_type_' attribute"
            )

    def test_simple_c_data_class_with_nonstr_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            class CType(_ctypes._SimpleCData):
                _type_ = 1

            self.assertEqual(
                context.exception, "class must define a '_type_' string attribute"
            )

    def test_simple_c_data_class_with_long_str_type_raises_value_error(self):
        with self.assertRaises(ValueError) as context:

            class CType(_ctypes._SimpleCData):
                _type_ = "oo"

        self.assertEqual(
            str(context.exception),
            "class must define a '_type_' attribute which must be a string of length 1",
        )

    def test_simple_c_data_class_with_unsupported_type_raises_attribute_error(self):
        with self.assertRaises(AttributeError) as context:

            class CType(_ctypes._SimpleCData):
                _type_ = "A"

        self.assertEqual(
            str(context.exception),
            "class must define a '_type_' attribute which must be\n"
            + "a single character string containing one of 'cbBhHiIlLdfuzZqQPXOv?g'.",
        )

    def test_dlopen_with_none_returns_handle(self):
        handle = _ctypes.dlopen(None, _ctypes.RTLD_LOCAL)
        self.assertIsInstance(handle, int)
        self.assertNotEqual(handle, 0)

    def test_POINTER_with_type_returns_pointer(self):
        class c_byte(_ctypes._SimpleCData):
            _type_ = "b"

        p = _ctypes.POINTER(c_byte)
        self.assertEqual(p.__name__, "LP_c_byte")
        self.assertEqual(_ctypes._pointer_type_cache[c_byte], p)

    def test_POINTER_with_str_returns_pointer(self):
        name = "my_class"
        p = _ctypes.POINTER(name)
        self.assertEqual(p.__name__, "LP_my_class")
        self.assertEqual(_ctypes._pointer_type_cache[id(p)], p)

    def test_POINTER_with_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            _ctypes.POINTER(1)

        self.assertEqual(str(context.exception), "must be a ctypes type")

    def test_sizeof_with_type_returns_correct_size(self):
        class c_byte(_ctypes._SimpleCData):
            _type_ = "b"

        from struct import calcsize

        self.assertEqual(_ctypes.sizeof(c_byte), calcsize(c_byte._type_))


if __name__ == "__main__":
    unittest.main()
