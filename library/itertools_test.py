#!/usr/bin/env python3
import itertools
import unittest


class ItertoolsTests(unittest.TestCase):
    def test_count_with_int_returns_iterator(self):
        iterator = itertools.count(start=7, step=-2)
        list = [next(iterator) for _ in range(5)]
        self.assertEqual(list, [7, 5, 3, 1, -1])

    def test_count_with_non_number_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, "a number is required"):
            itertools.count(start="a", step=".")


if __name__ == "__main__":
    unittest.main()
