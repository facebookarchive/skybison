#!/usr/bin/env python3
import mmap
import unittest


class MmapTest(unittest.TestCase):
    def test_new_with_wrong_class_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(list, -1, 1)
        self.assertIn("is not a sub", str(context.exception))

    def test_new_with_non_int_fileno_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, "not-int", 1)
        self.assertIn("str", str(context.exception))

    def test_new_with_non_int_length_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, "not-int")
        self.assertIn("str", str(context.exception))

    def test_new_with_non_int_flags_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, flags="not-int")
        self.assertIn("str", str(context.exception))

    def test_new_with_non_int_prot_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, prot="not-int")
        self.assertIn("str", str(context.exception))

    def test_new_with_non_int_access_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, access="not-int")
        self.assertIn("str", str(context.exception))

    def test_new_with_non_int_offset_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, offset="not-int")
        self.assertIn("str", str(context.exception))

    def test_new_with_negative_len_raises_overflow_error(self):
        with self.assertRaises(OverflowError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, -1)
        self.assertEqual(
            "memory mapped length must be positive", str(context.exception)
        )

    def test_new_with_negative_offset_raises_overflow_error(self):
        with self.assertRaises(OverflowError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, offset=-1)
        self.assertEqual(
            "memory mapped offset must be positive", str(context.exception)
        )

    def test_new_that_sets_both_access_and_flags_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, flags=-1, access=1)
        self.assertEqual(
            "mmap can't specify both access and flags, prot.", str(context.exception)
        )

    def test_new_that_sets_both_access_and_prot_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, prot=-1, access=1)
        self.assertEqual(
            "mmap can't specify both access and flags, prot.", str(context.exception)
        )

    def test_new_that_sets_invalid_access_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, access=-1)
        self.assertEqual("mmap invalid access parameter.", str(context.exception))

    def test_anonymous_mmap_can_be_closed(self):
        m = mmap.mmap(-1, 1)
        m.close()

    def test_prot_constants_are_all_different(self):
        self.assertNotEqual(mmap.PROT_EXEC, mmap.PROT_READ)
        self.assertNotEqual(mmap.PROT_READ, mmap.PROT_WRITE)
        self.assertNotEqual(mmap.PROT_WRITE, mmap.PROT_EXEC)


if __name__ == "__main__":
    unittest.main()
