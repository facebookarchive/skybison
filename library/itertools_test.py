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


class PermutationsTests(unittest.TestCase):
    def test_too_few_arguments_raises_type_error(self):
        self.assertRaises(TypeError, itertools.permutations)

    def test_too_many_arguments_raises_type_error(self):
        self.assertRaises(TypeError, itertools.permutations, "1", "2", "3", "4")

    def test_non_int_r_type_error(self):
        self.assertRaises(TypeError, itertools.permutations, "1", 1.0)

    def test_empty_returns_empty(self):
        self.assertTupleEqual(tuple(itertools.permutations(())), ((),))

    def test_r_defaults_to_length(self):
        len1 = itertools.permutations("1")
        self.assertTrue(all(map(lambda x: len(x) == 1, len1)))

        len2 = itertools.permutations("12")
        self.assertTrue(all(map(lambda x: len(x) == 2, len2)))

        len3 = itertools.permutations("123")
        self.assertTrue(all(map(lambda x: len(x) == 3, len3)))

    def test_r_zero_returns_stopped_iterator(self):
        self.assertTupleEqual(tuple(itertools.permutations("A", 0)), ((),))

    def test_r_gt_length_returns_empty(self):
        self.assertTupleEqual(tuple(itertools.permutations("A", 2)), ())

    def test_r_lt_length_returns_items_with_length_r(self):
        result = tuple(itertools.permutations("ABC", 2))
        self.assertTupleEqual(
            result,
            (("A", "B"), ("A", "C"), ("B", "A"), ("B", "C"), ("C", "A"), ("C", "B")),
        )

    def test_ordinary_iterable(self):
        result = tuple(itertools.permutations(range(3)))
        self.assertTupleEqual(
            result, ((0, 1, 2), (0, 2, 1), (1, 0, 2), (1, 2, 0), (2, 0, 1), (2, 1, 0))
        )


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
