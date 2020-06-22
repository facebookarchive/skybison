#!/usr/bin/env python3
import unittest

from _collections import deque


class DequeTests(unittest.TestCase):
    def test_dunder_init_with_large_int_raises_overflow_error(self):
        with self.assertRaises(OverflowError):
            deque(maxlen=2 ** 63 + 1)

    def test_dunder_int_with_large_negative_int_raises_overflow_error(self):
        with self.assertRaises(OverflowError):
            deque(maxlen=-(2 ** 63) - 1)

    def test_dunder_init_with_no_maxlen_returns_none(self):
        result = deque()
        self.assertEqual(result.maxlen, None)

    def test_dunder_init_with_negative_maxlen_raises_value_error(self):
        with self.assertRaises(ValueError):
            deque(maxlen=(-1))

    def test_dunder_init_with_non_int_raises_type_error(self):
        with self.assertRaises(TypeError):
            deque(maxlen="string")

    def test_dunder_init_sets_maxlen(self):
        result = deque(maxlen=5)
        self.assertEqual(result.maxlen, 5)

    def test_dunder_new(self):
        result = deque.__new__(deque)
        self.assertTrue(isinstance(result, deque))
        self.assertEqual(result.maxlen, None)


if __name__ == "__main__":
    unittest.main()
