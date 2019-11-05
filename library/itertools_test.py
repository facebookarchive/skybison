#!/usr/bin/env python3
import itertools
import operator
import unittest
from unittest.mock import Mock


class ChainTests(unittest.TestCase):
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


class CountTests(unittest.TestCase):
    def test_count_with_int_returns_iterator(self):
        iterator = itertools.count(start=7, step=-2)
        list = [next(iterator) for _ in range(5)]
        self.assertEqual(list, [7, 5, 3, 1, -1])

    def test_count_with_non_number_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, "a number is required"):
            itertools.count(start="a", step=".")


class IsliceTests(unittest.TestCase):
    def test_too_few_arguments_raises_type_error(self):
        self.assertRaises(TypeError, itertools.islice, [])

    def test_too_many_arguments_raises_type_error(self):
        self.assertRaises(TypeError, itertools.islice, [], 1, 2, 3, 4)

    def test_single_slice_non_int_arg_raises_value_error(self):
        with self.assertRaises(ValueError) as ctx:
            itertools.islice([], [])
        self.assertEqual(
            str(ctx.exception),
            "Stop argument for islice() must be None or an integer: "
            "0 <= x <= sys.maxsize.",
        )

    def test_single_slice_neg_arg_raises_value_error(self):
        with self.assertRaises(ValueError) as ctx:
            itertools.islice([], -1)
        self.assertEqual(
            str(ctx.exception),
            "Stop argument for islice() must be None or an integer: "
            "0 <= x <= sys.maxsize.",
        )

    def test_slice_non_int_stop_raises_value_error(self):
        with self.assertRaises(ValueError) as ctx:
            itertools.islice([], 0, [])
        self.assertEqual(
            str(ctx.exception),
            "Stop argument for islice() must be None or an integer: "
            "0 <= x <= sys.maxsize.",
        )

    def test_slice_neg_one_stop_raises_value_error(self):
        with self.assertRaises(ValueError) as ctx:
            itertools.islice([], 0, -1)
        self.assertEqual(
            str(ctx.exception),
            "Stop argument for islice() must be None or an integer: "
            "0 <= x <= sys.maxsize.",
        )

    def test_slice_neg_two_stop_raises_value_error(self):
        with self.assertRaises(ValueError) as ctx:
            itertools.islice([], 0, -2)
        self.assertEqual(
            str(ctx.exception),
            "Indices for islice() must be None or an integer: 0 <= x <= sys.maxsize.",
        )

    def test_slice_non_int_start_raises_value_error(self):
        with self.assertRaises(ValueError) as ctx:
            itertools.islice([], [], 1)
        self.assertEqual(
            str(ctx.exception),
            "Indices for islice() must be None or an integer: 0 <= x <= sys.maxsize.",
        )

    def test_slice_neg_int_start_raises_value_error(self):
        with self.assertRaises(ValueError) as ctx:
            itertools.islice([], -1, 1)
        self.assertEqual(
            str(ctx.exception),
            "Indices for islice() must be None or an integer: 0 <= x <= sys.maxsize.",
        )

    def test_slice_non_int_step_raises_value_error(self):
        with self.assertRaises(ValueError) as ctx:
            itertools.islice([], 0, 1, [])
        self.assertEqual(
            str(ctx.exception), "Step for islice() must be a positive integer or None."
        )

    def test_slice_neg_int_step_raises_value_error(self):
        with self.assertRaises(ValueError) as ctx:
            itertools.islice([], 0, 1, -1)
        self.assertEqual(
            str(ctx.exception), "Step for islice() must be a positive integer or None."
        )

    def test_slice_with_none_stop_treats_stop_as_end(self):
        islice = itertools.islice([0, 1, 2, 3], None)
        self.assertEqual(next(islice), 0)
        self.assertEqual(next(islice), 1)
        self.assertEqual(next(islice), 2)
        self.assertEqual(next(islice), 3)
        with self.assertRaises(StopIteration):
            next(islice)

    def test_slice_with_zero_stop_immediately_stops(self):
        islice = itertools.islice([0, 1, 2, 3], 0)
        with self.assertRaises(StopIteration):
            next(islice)

    def test_slice_with_none_start_starts_at_zero(self):
        islice = itertools.islice([0, 1, 2, 3], None, 2)
        self.assertEqual(next(islice), 0)
        self.assertEqual(next(islice), 1)
        with self.assertRaises(StopIteration):
            next(islice)

    def test_slice_with_none_step_steps_by_one(self):
        islice = itertools.islice([0, 1, 2, 3], 1, 3, None)
        self.assertEqual(next(islice), 1)
        self.assertEqual(next(islice), 2)
        with self.assertRaises(StopIteration):
            next(islice)

    def test_slice_with_all_none_args_acts_as_the_given_iterable(self):
        islice = itertools.islice([0, 1, 2, 3], None, None, None)
        self.assertEqual(next(islice), 0)
        self.assertEqual(next(islice), 1)
        self.assertEqual(next(islice), 2)
        self.assertEqual(next(islice), 3)
        with self.assertRaises(StopIteration):
            next(islice)

    def test_slice_with_one_arg_stops_iterable_at_stop(self):
        islice = itertools.islice([0, 1, 2, 3], 2)
        self.assertEqual(next(islice), 0)
        self.assertEqual(next(islice), 1)
        with self.assertRaises(StopIteration):
            next(islice)

    def test_slice_with_one_arg_stops_at_end_of_iterable(self):
        islice = itertools.islice([0, 1, 2], 4)
        self.assertEqual(next(islice), 0)
        self.assertEqual(next(islice), 1)
        self.assertEqual(next(islice), 2)
        with self.assertRaises(StopIteration):
            next(islice)

    def test_slice_with_start_and_stop_respects_slice_bounds(self):
        islice = itertools.islice([0, 1, 2, 3], 1, 3)
        self.assertEqual(next(islice), 1)
        self.assertEqual(next(islice), 2)
        with self.assertRaises(StopIteration):
            next(islice)

    def test_slice_with_step_arg_respects_slice(self):
        islice = itertools.islice([0, 1, 2, 3, 4, 5, 6, 7, 8], 1, 6, 2)
        self.assertEqual(next(islice), 1)
        self.assertEqual(next(islice), 3)
        self.assertEqual(next(islice), 5)
        with self.assertRaises(StopIteration):
            next(islice)

    def test_slice_with_step_arg_ends_at_end_of_iterator(self):
        islice = itertools.islice([0, 1, 2, 3, 4], 1, 6, 2)
        self.assertEqual(next(islice), 1)
        self.assertEqual(next(islice), 3)
        with self.assertRaises(StopIteration):
            next(islice)

    def test_slice_calls_next_until_stop_is_reached(self):
        class RaisesAtFive:
            i = 0

            def __iter__(self):
                return self

            def __next__(self):
                if self.i < 5:
                    result = (0, 1, 2, 3, 4)[self.i]
                    self.i += 1
                    return result
                raise UserWarning(f"Called with {self.i}")

        islice = itertools.islice(RaisesAtFive(), 2, 6, 2)
        self.assertEqual(next(islice), 2)
        self.assertEqual(next(islice), 4)
        with self.assertRaises(UserWarning) as ctx:
            next(islice)
        self.assertEqual(str(ctx.exception), "Called with 5")


class PermutationsTests(unittest.TestCase):
    def test_too_few_arguments_raises_type_error(self):
        self.assertRaises(TypeError, itertools.permutations)

    def test_too_many_arguments_raises_type_error(self):
        self.assertRaises(TypeError, itertools.permutations, "1", "2", "3", "4")

    def test_non_int_r_type_error(self):
        self.assertRaises(TypeError, itertools.permutations, "1", 1.0)

    def test_empty_returns_single_empty_permutation(self):
        self.assertTupleEqual(tuple(itertools.permutations(())), ((),))

    def test_r_defaults_to_length(self):
        len1 = itertools.permutations("1")
        self.assertTrue(all(map(lambda x: len(x) == 1, len1)))

        len2 = itertools.permutations("12")
        self.assertTrue(all(map(lambda x: len(x) == 2, len2)))

        len3 = itertools.permutations("123")
        self.assertTrue(all(map(lambda x: len(x) == 3, len3)))

    def test_r_zero_returns_single_empty_permutation(self):
        self.assertTupleEqual(tuple(itertools.permutations("A", 0)), ((),))

    def test_r_gt_length_returns_stopped_iterator(self):
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


class AccumulateTests(unittest.TestCase):
    def test_accumulate_with_iterable_accumulates(self):
        self.assertTupleEqual(tuple(itertools.accumulate([1, 2, 3, 4])), (1, 3, 6, 10))

    def test_accumulate_with_func_arg_none_uses_addition(self):
        self.assertTupleEqual(
            tuple(itertools.accumulate([1, 2, 3, 4], None)), (1, 3, 6, 10)
        )

    def test_accumulate_with_func_arg_uses_func(self):
        self.assertTupleEqual(
            tuple(itertools.accumulate([1, 2, 3, 4], operator.mul)), (1, 2, 6, 24)
        )

    def test_accumulate_with_empty_iterable_returns_stopped_iterator(self):
        iterator = itertools.accumulate([])
        self.assertRaises(StopIteration, next, iterator)

    def test_accumulate_with_no_iterable_raises_type_error(self):
        self.assertRaises(TypeError, itertools.accumulate)

    def test_accumulate_dunder_next_with_non_callable_func_raises_type_error(self):
        iterator = itertools.accumulate([1, 2, 3, 4], "this is not a function")
        next(iterator)
        self.assertRaisesRegex(
            TypeError, "'str' object is not callable", next, iterator
        )


class GroupbyTests(unittest.TestCase):
    def test_groupby_returns_groups(self):
        it = itertools.groupby("AAAABBBCCD")
        self.assertEqual(next(it)[0], "A")
        self.assertEqual(next(it)[0], "B")
        self.assertEqual(next(it)[0], "C")
        self.assertEqual(next(it)[0], "D")

    def test_groupby_returns_all_elements_in_all_groups(self):
        it = itertools.groupby("AAAABBBCCD")
        group = next(it)[1]
        self.assertTupleEqual(tuple(group), ("A", "A", "A", "A"))
        group = next(it)[1]
        self.assertTupleEqual(tuple(group), ("B", "B", "B"))
        group = next(it)[1]
        self.assertTupleEqual(tuple(group), ("C", "C"))
        group = next(it)[1]
        self.assertEqual(next(group), "D")

    def test_groupby_raises_stopiteration_after_all_groups(self):
        it = itertools.groupby("AAAABBBCCD")
        for _i in range(4):
            next(it)
        self.assertRaises(StopIteration, next, it)

    def test_groupby_first_group_raises_stopiteration_after_all_elements(self):
        it = itertools.groupby("AAAABBBCCD")
        group = next(it)[1]
        for _i in range(4):
            next(group)
        self.assertRaises(StopIteration, next, group)

    def test_groupby_last_group_raises_stopiteration_after_all_elements(self):
        it = itertools.groupby("AAAABBBCCD")
        for _i in range(3):
            next(it)
        group = next(it)[1]
        next(group)
        self.assertRaises(StopIteration, next, group)

    def test_groupby_group_raises_stopiteration_after_advancing_groupby(self):
        it = itertools.groupby("AAAABBBCCD")
        group = next(it)[1]
        next(it)
        self.assertRaises(StopIteration, next, group)

    def test_groupby_handles_none_values(self):
        it = itertools.groupby([1, None, None, None])
        group = next(it)[1]
        self.assertEqual(next(group), 1)
        group = next(it)[1]
        self.assertTupleEqual(tuple(group), (None, None, None))

    def test_groupby_with_key_func_returns_keys_and_groups(self):
        def keyfunc(value):
            return 4 if value == 4 else 1

        it = itertools.groupby([1, 2, 3, 4], keyfunc)
        grouper = next(it)
        self.assertEqual(grouper[0], 1)
        self.assertTupleEqual(tuple(grouper[1]), (1, 2, 3))
        grouper = next(it)
        self.assertEqual(grouper[0], 4)
        self.assertEqual(next(grouper[1]), 4)


class FilterFalseTests(unittest.TestCase):
    def test_filterfalse_with_no_predicate_returns_false_values(self):
        it = itertools.filterfalse(None, range(10))
        self.assertTupleEqual(tuple(it), (0,))

    def test_filterfalse_with_predicate_returns_filtered_values(self):
        it = itertools.filterfalse(lambda x: x % 2, range(10))
        self.assertTupleEqual(tuple(it), (0, 2, 4, 6, 8))

    def test_filterfalse_with_no_sequence_raises_typeerror(self):
        with self.assertRaises(TypeError):
            itertools.filterfalse(None, None)


class TeeTests(unittest.TestCase):
    def test_default_n_retuns_two_iterators(self):
        its = itertools.tee([1, 2, 3, 4, 5])
        self.assertEqual(len(its), 2)

    def test_tee_returns_multiple_working_iterators(self):
        its = itertools.tee([2, 4, 6, 8, 10])
        self.assertTupleEqual(tuple(its[0]), (2, 4, 6, 8, 10))
        self.assertTupleEqual(tuple(its[1]), (2, 4, 6, 8, 10))

    def test_tee_with_long_iterable(self):
        its = itertools.tee(list(range(200)))
        self.assertTupleEqual(tuple(its[0]), tuple(range(200)))
        self.assertTupleEqual(tuple(its[1]), tuple(range(200)))

    def test_tee_with_n_equal_three_returns_three_working_iterators(self):
        its = itertools.tee(["A", "B"], 3)
        self.assertTupleEqual(tuple(its[0]), ("A", "B"))
        self.assertTupleEqual(tuple(its[1]), ("A", "B"))
        self.assertTupleEqual(tuple(its[2]), ("A", "B"))

    def test_tee_with_copyable_iterator_as_class_attribute(self):
        class CopyableRangeIterator:
            def __init__(self, n, i=0):
                self._n = n
                self._i = i

            def __iter__(self):
                return self

            def __next__(self):
                if self._i == self._n:
                    raise StopIteration
                value = self._i
                self._i += 1
                return value

            def __copy__(self):
                return self.__class__(self._n)

        its = itertools.tee(CopyableRangeIterator(2))
        self.assertTrue(isinstance(its[0], CopyableRangeIterator))
        self.assertTupleEqual(tuple(its[0]), (0, 1))
        self.assertTupleEqual(tuple(its[1]), (0, 1))

    def test_tee_with_copyable_iterator_as_instance_attribute(self):
        class CopyableRangeIterator:
            def __init__(self, n, i=0):
                self._n = n
                self._i = i

            def __iter__(self):
                return self

            def __next__(self):
                if self._i == self._n:
                    raise StopIteration
                value = self._i
                self._i += 1
                return value

        # This is a bit contrived
        it = CopyableRangeIterator(3)
        it.__copy__ = lambda: CopyableRangeIterator(it._n)
        its = itertools.tee(it)
        self.assertTrue(isinstance(its[0], CopyableRangeIterator))
        self.assertTupleEqual(tuple(its[0]), (0, 1, 2))
        self.assertTupleEqual(tuple(its[1]), (0, 1, 2))

    def test_tee_with_non_integer_n_raises_typeerror(self):
        with self.assertRaises(TypeError):
            itertools.tee([1, 2, 3, 4, 5], "2")

    def test_tee_with_n_lessthan_zero_raises_valueerror(self):
        with self.assertRaises(ValueError):
            itertools.tee([1, 2, 3, 4, 5], -2)

    def test_tee_with_n_equal_zero_returns_empty_tuple(self):
        self.assertEqual(itertools.tee([1, 2, 3, 4, 5], 0), ())


class DropWhileTests(unittest.TestCase):
    def test_dropwhile_passing_none_predicate_raises_typeerror(self):
        it = itertools.dropwhile(None, [1, 2, 3])
        self.assertRaises(TypeError, next, it)

    def test_dropwhile_passing_none_iterator_raises_typeerror(self):
        with self.assertRaises(TypeError):
            itertools.dropwhile(lambda x: x % 2, None)

    def test_dropwhile_returns_correct_elements_dropping_start(self):
        it = itertools.dropwhile(lambda x: x < 5, [1, 4, 6, 4, 1])
        self.assertTupleEqual(tuple(it), (6, 4, 1))

    def test_dropwhile_with_true_predicate_drops_all_elements(self):
        it = itertools.dropwhile(lambda x: True, [1, 4, 6, 4, 1])
        self.assertTupleEqual(tuple(it), ())

    def test_dropwhile_with_false_predicate_returns_all_elements(self):
        it = itertools.dropwhile(lambda x: False, [1, 4, 6, 4, 1])
        self.assertTupleEqual(tuple(it), (1, 4, 6, 4, 1))


class TakeWhileTests(unittest.TestCase):
    def test_takewhile_passing_none_predicate_raises_typeerror(self):
        it = itertools.takewhile(None, [1, 2, 3])
        self.assertRaises(TypeError, next, it)

    def test_takewhile_passing_none_iterator_raises_typeerror(self):
        with self.assertRaises(TypeError):
            itertools.takewhile(lambda x: x % 2, None)

    def test_takewhile_returns_correct_elements_dropping_end(self):
        it = itertools.takewhile(lambda x: x < 5, [1, 4, 6, 4, 1])
        self.assertTupleEqual(tuple(it), (1, 4))

    def test_takewhile_with_true_predicate_returns_all_elements(self):
        it = itertools.takewhile(lambda x: True, [1, 4, 6, 4, 1])
        self.assertTupleEqual(tuple(it), (1, 4, 6, 4, 1))

    def test_takewhile_with_false_predicate_drops_all_elements(self):
        it = itertools.takewhile(lambda x: False, [1, 4, 6, 4, 1])
        self.assertTupleEqual(tuple(it), ())


class StarMapTests(unittest.TestCase):
    def test_starmap_returns_arguments_mapped_onto_function(self):
        it = itertools.starmap(pow, [(2, 5), (3, 2), (10, 3)])
        self.assertTupleEqual(tuple(it), (32, 9, 1000))

    def test_starmap_handles_tuple_subclass(self):
        class C(tuple):
            pass

        points = [C((3, 4)), C((9, 10)), C((-2, -1))]
        self.assertTrue(isinstance(points[0], tuple))
        self.assertFalse(points[0] is tuple)

        it = itertools.starmap(lambda x, y: x ** 2 + y ** 2, points)
        self.assertTupleEqual(tuple(it), (25, 181, 5))

    def test_starmap_passing_empty_sequence_returns_empty_iterator(self):
        self.assertEqual(tuple(itertools.starmap(pow, [])), ())

    def test_starmap_passing_none_function_raises_typeerror(self):
        it = itertools.starmap(None, [(2, 5), (3, 2), (10, 3)])
        self.assertRaises(TypeError, next, it)

    def test_starmap_passing_none_iterable_raises_typeerror(self):
        with self.assertRaises(TypeError):
            itertools.starmap(pow, None)

    def test_starmap_passing_non_sequences_raises_typeerror(self):
        it = itertools.starmap(None, [1, 2])
        self.assertRaisesRegex(TypeError, "'int' object is not iterable", next, it)


class CombinationsTests(unittest.TestCase):
    def test_too_few_arguments_raises_type_error(self):
        self.assertRaises(TypeError, itertools.combinations)

    def test_too_many_arguments_raises_type_error(self):
        self.assertRaises(TypeError, itertools.combinations, "1", "2", "3", "4")

    def test_non_int_r_type_error(self):
        self.assertRaises(TypeError, itertools.combinations, "1", 1.0)

    def test_empty_returns_single_empty_combination(self):
        self.assertTupleEqual(tuple(itertools.combinations((), 0)), ((),))

    def test_r_zero_returns_single_empty_combination(self):
        self.assertTupleEqual(tuple(itertools.combinations("A", 0)), ((),))

    def test_r_gt_length_returns_stopped_iterator(self):
        self.assertTupleEqual(tuple(itertools.combinations("A", 2)), ())

    def test_r_lt_length_returns_items_with_length_r(self):
        result = tuple(itertools.combinations("Bam!", 2))
        self.assertTupleEqual(
            result,
            (("B", "a"), ("B", "m"), ("B", "!"), ("a", "m"), ("a", "!"), ("m", "!")),
        )

    def test_ordinary_iterable(self):
        result = tuple(itertools.combinations(range(4), 3))
        self.assertTupleEqual(result, ((0, 1, 2), (0, 1, 3), (0, 2, 3), (1, 2, 3)))


class CombinationsWithReplacementTests(unittest.TestCase):
    def test_too_few_arguments_raises_type_error(self):
        self.assertRaises(TypeError, itertools.combinations_with_replacement)

    def test_too_many_arguments_raises_type_error(self):
        self.assertRaises(
            TypeError, itertools.combinations_with_replacement, "1", "2", "3", "4"
        )

    def test_non_int_r_type_error(self):
        self.assertRaises(TypeError, itertools.combinations_with_replacement, "1", 1.0)

    def test_empty_returns_single_empty_combination(self):
        self.assertTupleEqual(
            tuple(itertools.combinations_with_replacement((), 0)), ((),)
        )

    def test_r_zero_returns_single_empty_combination(self):
        self.assertTupleEqual(
            tuple(itertools.combinations_with_replacement("A", 0)), ((),)
        )

    def test_r_gt_length_returns_items_with_length_r(self):
        self.assertTupleEqual(
            tuple(itertools.combinations_with_replacement("A", 2)), (("A", "A"),)
        )

    def test_r_positive_and_length_zero_returns_stopped_iterator(self):
        self.assertTupleEqual(tuple(itertools.combinations("A", 2)), ())

    def test_r_lt_length_returns_items_with_length_r(self):
        result = tuple(itertools.combinations_with_replacement("CBA", 2))
        self.assertTupleEqual(
            result,
            (("C", "C"), ("C", "B"), ("C", "A"), ("B", "B"), ("B", "A"), ("A", "A")),
        )

    def test_ordinary_iterable(self):
        result = tuple(itertools.combinations_with_replacement(range(4), 1))
        self.assertTupleEqual(result, ((0,), (1,), (2,), (3,)))


class CompressTests(unittest.TestCase):
    def test_empty_data_returns_stopped_iterator(self):
        self.assertEqual(tuple(itertools.compress([], [1, 0])), ())

    def test_empty_selectors_returns_stopped_iterator(self):
        self.assertEqual(tuple(itertools.compress("TYLER", [])), ())

    def test_numeric_selector_values(self):
        result = "".join(itertools.compress("T_Y_L_E_R", [1, 0, 1, 0, 1, 0, 1, 0, 1]))
        self.assertEqual(result, "TYLER")

    def test_boolean_selector_values(self):
        result = tuple(itertools.compress([2, 3, 4], [True, False, False]))
        self.assertTupleEqual(result, (2,))

    def test_other_selector_values(self):
        result = "".join(
            itertools.compress("FOUBAR", [None, [1], (), "hello", b"", {1: 2}])
        )
        self.assertEqual(result, "OBR")

    def test_data_length_gt_selector_length(self):
        result = tuple(itertools.compress([2, 3, 4, 5, 6], [1, 0, 1]))
        self.assertTupleEqual(result, (2, 4))

    def test_data_length_lt_selector_length(self):
        result = tuple(itertools.compress([2, 3, 4], [1, 0, 1, 1, 1, 1]))
        self.assertTupleEqual(result, (2, 4))

    def test_passing_non_iterable_data_raises_typeerror(self):
        self.assertRaises(TypeError, itertools.compress, None, [1, 2, 3])

    def test_passing_non_iterable_selectors_raises_typeerror(self):
        self.assertRaises(TypeError, itertools.compress, [1, 2, 3], None)


if __name__ == "__main__":
    unittest.main()
