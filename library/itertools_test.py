#!/usr/bin/env python3
import itertools
import unittest
from unittest.mock import Mock


class ChainTest(unittest.TestCase):
    def test_chain_with_no_iterables_returns_stoped_iterator(self):
        self.assertTupleEqual(tuple(itertools.chain()), ())

    def test_chain_with_empty_iterables_returns_stoped_iterator(self):
        self.assertTupleEqual(tuple(itertools.chain([])), ())
        self.assertTupleEqual(tuple(itertools.chain([], [])), ())
        self.assertTupleEqual(tuple(itertools.chain([], [], [])), ())

    def test_chain_with_one_iterable(self):
        self.assertTupleEqual(tuple(itertools.chain("1")), ("1",))
        self.assertTupleEqual(tuple(itertools.chain("12")), ("1", "2"))
        self.assertTupleEqual(tuple(itertools.chain("123")), ("1", "2", "3"))

    def test_chain_with_many_iterables(self):
        self.assertTupleEqual(tuple(itertools.chain("1", "")), ("1",))
        self.assertTupleEqual(tuple(itertools.chain("1", "2")), ("1", "2"))
        self.assertTupleEqual(tuple(itertools.chain("12", "")), ("1", "2"))
        self.assertTupleEqual(tuple(itertools.chain("12", "3")), ("1", "2", "3"))
        self.assertTupleEqual(tuple(itertools.chain("12", "34")), ("1", "2", "3", "4"))
        self.assertTupleEqual(tuple(itertools.chain("", "1")), ("1",))
        self.assertTupleEqual(tuple(itertools.chain("", "1", "")), ("1",))
        self.assertTupleEqual(tuple(itertools.chain("1", "23")), ("1", "2", "3"))
        self.assertTupleEqual(tuple(itertools.chain("1", "23", "")), ("1", "2", "3"))
        self.assertTupleEqual(tuple(itertools.chain("1", "2", "3")), ("1", "2", "3"))

    def test_from_iterable(self):
        self.assertTupleEqual(tuple(itertools.chain.from_iterable([])), ())
        self.assertTupleEqual(tuple(itertools.chain.from_iterable([""])), ())
        self.assertTupleEqual(tuple(itertools.chain.from_iterable(["1"])), ("1",))
        self.assertTupleEqual(tuple(itertools.chain.from_iterable(["12"])), ("1", "2"))
        self.assertTupleEqual(
            tuple(itertools.chain.from_iterable(["12", ""])), ("1", "2")
        )
        self.assertTupleEqual(
            tuple(itertools.chain.from_iterable(["12", "3"])), ("1", "2", "3")
        )
        self.assertTupleEqual(
            tuple(itertools.chain.from_iterable(["12", "34"])), ("1", "2", "3", "4")
        )
        self.assertTupleEqual(
            tuple(itertools.chain.from_iterable(["12", "34", ""])), ("1", "2", "3", "4")
        )
        self.assertTupleEqual(
            tuple(itertools.chain.from_iterable(["12", "34", "5"])),
            ("1", "2", "3", "4", "5"),
        )
        self.assertTupleEqual(
            tuple(itertools.chain.from_iterable(["12", "34", "56"])),
            ("1", "2", "3", "4", "5", "6"),
        )


class CycleTests(unittest.TestCase):
    def test_dunder_init_with_seq_does_calls_dunder_iter(self):
        class C:
            __iter__ = Mock(name="__iter__", return_value=[].__iter__())

        instance = C()
        itertools.cycle(instance)
        C.__iter__.assert_called_once()

    def test_dunder_init_with_seq_dunder_iter_returning_non_iter_raises_type_error(
        self
    ):
        class C:
            def __iter__(self):
                return 5

        instance = C()
        with self.assertRaisesRegex(
            TypeError, "iter\\(\\) returned non-iterator of type 'int'"
        ):
            itertools.cycle(instance)

    def test_dunder_iter_yields_self(self):
        result = itertools.cycle([])
        self.assertIs(result.__iter__(), result)

    def test_dunder_next_iterates_through_sequence(self):
        result = itertools.cycle([1, 2, 3])
        self.assertEqual(next(result), 1)
        self.assertEqual(next(result), 2)
        self.assertEqual(next(result), 3)

    def test_dunder_next_cycles_through_sequence(self):
        result = itertools.cycle([1, 2, 3])
        self.assertEqual(next(result), 1)
        self.assertEqual(next(result), 2)
        self.assertEqual(next(result), 3)
        self.assertEqual(next(result), 1)
        self.assertEqual(next(result), 2)
        self.assertEqual(next(result), 3)
        self.assertEqual(next(result), 1)


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


class RepeatTests(unittest.TestCase):
    def test_dunder_init_with_non_int_times_raises_type_error(self):
        self.assertRaises(TypeError, itertools.repeat, 5, "not_an_int")

    def test_dunder_init_with_negative_times_repeats_zero_times(self):
        iterator = itertools.repeat(5, -1)
        self.assertEqual(tuple(iterator), ())

    def test_dunder_init_with_zero_times_repeats_zero_times(self):
        iterator = itertools.repeat(5, 0)
        self.assertEqual(tuple(iterator), ())

    def test_dunder_init_with_three_times_repeats_three_times(self):
        iterator = itertools.repeat(5, 3)
        self.assertEqual(next(iterator), 5)
        self.assertEqual(next(iterator), 5)
        self.assertEqual(next(iterator), 5)
        self.assertRaises(StopIteration, next, iterator)

    def test_dunder_init_with_no_times_repeats(self):
        iterator = itertools.repeat(5)
        self.assertEqual(next(iterator), 5)
        self.assertEqual(next(iterator), 5)
        self.assertEqual(next(iterator), 5)


class ZipLongestTests(unittest.TestCase):
    def test_dunder_init_with_no_seqs_returns_empty_iterator(self):
        iterator = itertools.zip_longest()
        self.assertEqual(tuple(iterator), ())

    def test_dunder_init_calls_dunder_iter_on_seqs(self):
        class C:
            __iter__ = Mock(name="__iter__", return_value=().__iter__())

        itertools.zip_longest(C(), C())
        self.assertEqual(C.__iter__.call_count, 2)

    def test_dunder_next_fills_with_none(self):
        right = itertools.zip_longest([], [1, 2, 3])
        self.assertEqual(list(right), [(None, 1), (None, 2), (None, 3)])

        left = itertools.zip_longest([1, 2, 3], [])
        self.assertEqual(list(left), [(1, None), (2, None), (3, None)])

    def test_dunder_next_fills_with_fill_value(self):
        right = itertools.zip_longest("ab", [1, 2, 3], fillvalue="X")
        self.assertEqual(list(right), [("a", 1), ("b", 2), ("X", 3)])


if __name__ == "__main__":
    unittest.main()
