#!/usr/bin/env python3
import array
import unittest


class ArrayTest(unittest.TestCase):
    def test_new_with_bad_typecode_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            array.array.__new__(array.array, "c")
        self.assertEqual(
            str(context.exception),
            "bad typecode (must be b, B, u, h, H, i, I, l, L, q, Q, f or d)",
        )

    def test_new_with_non_str_typecode_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            array.array.__new__(array.array, b"not-string")
        self.assertIn("must be a unicode character, not bytes", str(context.exception))

    def test_new_with_long_str_typecode_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            array.array.__new__(array.array, "too")
        self.assertIn("must be a unicode character, not str", str(context.exception))

    def test_new_with_str_array_init_and_non_str_typecode_raises_type_error(self):
        unicode_array = array.array("u")

        with self.assertRaises(TypeError) as context:
            array.array.__new__(array.array, "c", unicode_array)
        self.assertEqual(
            str(context.exception),
            "cannot use a unicode array to initialize an array with typecode 'c'",
        )

    def test_new_with_valid_typecode_returns_array(self):
        byte_array = array.array("b")
        self.assertEqual(byte_array.typecode, "b")

    def test_new_with_array_subtype_returns_subtype(self):
        class ArraySub(array.array):
            pass

        array_sub = array.array.__new__(ArraySub, "B")
        self.assertIsInstance(array_sub, ArraySub)
        self.assertEqual(array_sub.typecode, "B")


if __name__ == "__main__":
    unittest.main()
