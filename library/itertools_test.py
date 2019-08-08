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


class ProductTests(unittest.TestCase):
    def test_empty_returns_stopped_iterable(self):
        self.assertTupleEqual(tuple(itertools.product("")), ())
        self.assertTupleEqual(tuple(itertools.product("ab", "")), ())
        self.assertTupleEqual(tuple(itertools.product("", "ab", "")), ())
        self.assertTupleEqual(tuple(itertools.product("ab", "", "12")), ())

    def test_no_args_returns_empty(self):
        self.assertTupleEqual(tuple(itertools.product()), ((),))

    def test_not_an_integer_raises_type_error(self):
        with self.assertRaises(TypeError):
            itertools.product("ab", repeat=None)

    def test_repeat_zero_returns_empty(self):
        self.assertTupleEqual(tuple(itertools.product("", repeat=0)), ((),))
        self.assertTupleEqual(tuple(itertools.product("ab", repeat=0)), ((),))
        self.assertTupleEqual(tuple(itertools.product("ab", "12", repeat=0)), ((),))

    def test_repeat_returns_repeated_iterables(self):
        self.assertTupleEqual(
            tuple(itertools.product("a", "1", repeat=2)), (("a", "1", "a", "1"),)
        )

    def test_different_lengths(self):
        self.assertTupleEqual(
            tuple(itertools.product("a", "12")), (("a", "1"), ("a", "2"))
        )
        self.assertTupleEqual(
            tuple(itertools.product("ab", "1")), (("a", "1"), ("b", "1"))
        )
        self.assertTupleEqual(
            tuple(itertools.product("ab", "1", "!")), (("a", "1", "!"), ("b", "1", "!"))
        )


if __name__ == "__main__":
    unittest.main()
