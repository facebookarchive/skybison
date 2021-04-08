#!/usr/bin/env python3
import unittest


class ListTests(unittest.TestCase):
    def test_dunder_add_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__add__' .* 'list' object.* a 'tuple'",
            list.__add__,
            (),
            [],
        )
        self.assertRaisesRegex(
            TypeError, "can only concatenate list", list.__add__, [], ()
        )

    def test_dunder_add_with_empty_lists_returns_new_empty_list(self):
        orig = []
        result = list.__add__(orig, orig)
        self.assertEqual(result, [])
        self.assertIsNot(result, orig)

    def test_dunder_add_with_list_returns_new_list(self):
        orig = [1, 2, 3]
        self.assertEqual(list.__add__(orig, []), [1, 2, 3])
        self.assertEqual(list.__add__(orig, [4, 5]), [1, 2, 3, 4, 5])

        # orig is unchanged
        self.assertEqual(orig, [1, 2, 3])

    def test_dunder_eq_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__eq__' .* 'list' object.* a 'bool'",
            list.__eq__,
            False,
            [],
        )

    def test_dunder_eq_propagates_error(self):
        class C:
            def __eq__(self, other):
                raise ValueError

        with self.assertRaises(ValueError):
            list.__eq__([C()], [C()])

    def test_dunder_iadd_with_list_subclass_uses_iter(self):
        class C(list):
            def __iter__(self):
                return iter([1, 2, 3])

        orig = [1, 2, 3]
        other = C([4, 5])
        self.assertIs(list.__iadd__(orig, other), orig)
        self.assertEqual(orig, [1, 2, 3, 1, 2, 3])

    def test_dunder_iadd_with_iterable_appends_to_list(self):
        orig = []

        self.assertIs(list.__iadd__(orig, [1, 2, 3]), orig)
        self.assertIs(list.__iadd__(orig, set()), orig)
        self.assertIs(list.__iadd__(orig, "abc"), orig)
        self.assertIs(list.__iadd__(orig, (4, 5)), orig)

        # orig now contains all other elements
        self.assertEqual(orig, [1, 2, 3, "a", "b", "c", 4, 5])

    def test_dunder_imul_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__imul__' .* 'list' object.* a 'bool'",
            list.__imul__,
            False,
            3,
        )

    def test_dunder_imul_with_non_int_raises_type_error(self):
        result = []
        with self.assertRaises(TypeError):
            list.__imul__(result, "x")

    def test_dunder_imul_with_empty_returns_self(self):
        orig = []
        result = orig.__imul__(3)
        self.assertIs(result, orig)

    def test_dunder_imul_with_empty_self_and_zero_repeat_does_nothing(self):
        orig = []
        result = orig.__imul__(0)
        self.assertIs(result, orig)
        self.assertEqual(len(result), 0)

    def test_dunder_imul_with_zero_repeat_clears_list(self):
        orig = [1, 2, 3]
        result = orig.__imul__(0)
        self.assertIs(result, orig)
        self.assertEqual(len(result), 0)

    def test_dunder_imul_with_negative_repeat_clears_list(self):
        orig = [1, 2, 3]
        result = orig.__imul__(-3)
        self.assertIs(result, orig)
        self.assertEqual(len(result), 0)

    def test_dunder_imul_with_repeat_equals_one_returns_self(self):
        orig = [1, 2, 3]
        result = orig.__imul__(1)
        self.assertIs(result, orig)

    def test_dunder_imul_with_too_big_repeat_raises_overflow_error(self):
        orig = [1, 2, 3]
        self.assertRaisesRegex(
            OverflowError,
            "cannot fit 'int' into an index-sized integer",
            orig.__imul__,
            100 ** 100,
        )

    def test_dunder_imul_with_too_big_repeat_raises_memory_error(self):
        orig = [1, 2, 3] * 10000
        self.assertRaises(MemoryError, orig.__imul__, 0x7FFFFFFFFFFFFFF)

    def test_dunder_imul_with_repeat_repeats_contents(self):
        orig = [1, 2, 3]
        result = orig.__imul__(2)
        self.assertIs(result, orig)
        self.assertEqual(result, [1, 2, 3, 1, 2, 3])

    def test_dunder_init_with_list_returns_list(self):
        li = list.__new__(list)
        self.assertEqual(li, [])
        list.__init__(li, [4, "a"])
        self.assertEqual(li, [4, "a"])

    def test_dunder_init_with_tuple_returns_list(self):
        li = list.__new__(list)
        self.assertEqual(li, [])
        list.__init__(li, (5.5, (), 42))
        self.assertEqual(li, [5.5, (), 42])

    def test_dunder_init_with_generator_returns_list(self):
        li = list.__new__(list)
        self.assertEqual(li, [])
        list.__init__(li, range(3, -5, -2))
        self.assertEqual(li, [3, 1, -1, -3])

    def test_dunder_init_does_not_call_extend_or_append(self):
        class C(list):
            def extend(self, x):
                raise Exception("must not be called")

            def append(self, x):
                raise Exception("must not be called")

        c = C(range(4))
        self.assertEqual(c, [0, 1, 2, 3])

    def test_dunder_ne_returns_not_eq(self):
        orig = [1, 2, 3]
        self.assertTrue(orig.__ne__([1, 2]))
        self.assertFalse(orig.__ne__(orig))

    def test_dunder_ne_returns_not_implemented_if_wrong_types(self):
        orig = [1, 2, 3]
        self.assertIs(orig.__ne__(1), NotImplemented)
        self.assertIs(orig.__ne__({}), NotImplemented)

    def test_dunder_reversed_returns_reversed_iterator(self):
        orig = [1, 2, 3]
        rev_iter = orig.__reversed__()
        self.assertEqual(next(rev_iter), 3)
        self.assertEqual(next(rev_iter), 2)
        self.assertEqual(next(rev_iter), 1)
        with self.assertRaises(StopIteration):
            next(rev_iter)

    def test_dunder_setitem_with_int_sets_value_at_index(self):
        orig = [1, 2, 3]
        orig[1] = 4
        self.assertEqual(orig, [1, 4, 3])

    def test_dunder_setitem_with_negative_int_sets_value_at_adjusted_index(self):
        orig = [1, 2, 3]
        orig[-1] = 4
        self.assertEqual(orig, [1, 2, 4])

    def test_dunder_setitem_with_str_raises_type_error(self):
        orig = []
        with self.assertRaises(TypeError) as context:
            orig["not an index"] = "element"
        self.assertEqual(
            str(context.exception), "list indices must be integers or slices, not str"
        )

    def test_dunder_setitem_with_large_int_raises_index_error(self):
        orig = [1, 2, 3]
        with self.assertRaises(IndexError) as context:
            orig[2 ** 63] = 4
        self.assertEqual(
            str(context.exception), "cannot fit 'int' into an index-sized integer"
        )

    def test_dunder_setitem_with_positive_out_of_bounds_raises_index_error(self):
        orig = [1, 2, 3]
        with self.assertRaises(IndexError) as context:
            orig[3] = 4
        self.assertEqual(str(context.exception), "list assignment index out of range")

    def test_dunder_setitem_with_negative_out_of_bounds_raises_index_error(self):
        orig = [1, 2, 3]
        with self.assertRaises(IndexError) as context:
            orig[-4] = 4
        self.assertEqual(str(context.exception), "list assignment index out of range")

    def test_dunder_setitem_slice_with_empty_slice_clears_list(self):
        orig = [1, 2, 3]
        orig[:] = ()
        self.assertEqual(orig, [])

    def test_dunder_setitem_slice_with_same_size(self):
        orig = [1, 2, 3, 4, 5]
        orig[1:4] = ["B", "C", "D"]
        self.assertEqual(orig, [1, "B", "C", "D", 5])

    def test_dunder_setitem_slice_with_short_stop_grows(self):
        orig = [1, 2, 3, 4, 5]
        orig[:1] = "abc"
        self.assertEqual(orig, ["a", "b", "c", 2, 3, 4, 5])

    def test_dunder_setitem_slice_with_larger_slice_grows(self):
        orig = [1, 2, 3, 4, 5]
        orig[1:4] = ["B", "C", "D", "MORE", "ELEMENTS"]
        self.assertEqual(orig, [1, "B", "C", "D", "MORE", "ELEMENTS", 5])

    def test_dunder_setitem_slice_with_smaller_slice_shrinks(self):
        orig = [1, 2, 3, 4, 5]
        orig[1:4] = ["FEWER", "ELEMENTS"]
        self.assertEqual(orig, [1, "FEWER", "ELEMENTS", 5])

    def test_dunder_setitem_slice_with_tuple_sets_slice(self):
        orig = [1, 2, 3, 4, 5]
        orig[1:4] = ("a", 6, None)
        self.assertEqual(orig, [1, "a", 6, None, 5])

    def test_dunder_setitem_slice_with_self_copies(self):
        orig = [1, 2, 3, 4, 5]
        orig[1:4] = orig
        self.assertEqual(orig, [1, 1, 2, 3, 4, 5, 5])

    def test_dunder_setitem_slice_with_reversed_bounds_inserts_at_start(self):
        orig = [1, 2, 3, 4, 5]
        orig[3:2] = "ab"
        self.assertEqual(orig, [1, 2, 3, "a", "b", 4, 5])

    def test_dunder_setitem_slice_with_step_assigns_slice(self):
        orig = [1, 2, 3, 4, 5, 6]
        orig[::2] = "abc"
        self.assertEqual(orig, ["a", 2, "b", 4, "c", 6])

    def test_dunder_setitem_slice_with_negative_step_assigns_backwards(self):
        orig = [1, 2, 3, 4, 5, 6]
        orig[::-2] = "abc"
        self.assertEqual(orig, [1, "c", 3, "b", 5, "a"])

    def test_dunder_setitem_slice_extended_short_raises_value_error(self):
        orig = [1, 2, 3, 4, 5]
        with self.assertRaises(ValueError) as context:
            orig[::2] = ()
        self.assertEqual(
            str(context.exception),
            "attempt to assign sequence of size 0 to extended slice of size 3",
        )

    def test_dunder_setitem_slice_extended_long_raises_value_error(self):
        orig = [1, 2, 3, 4, 5]
        with self.assertRaises(ValueError) as context:
            orig[::2] = (1, 2, 3, 4)
        self.assertEqual(
            str(context.exception),
            "attempt to assign sequence of size 4 to extended slice of size 3",
        )

    def test_append_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError, "'append' .* 'list' object.* a 'NoneType'", list.append, None, 42
        )

    def test_dunder_setitem_slice_with_non_iterable_raises_type_error(self):
        orig = []
        with self.assertRaises(TypeError) as context:
            orig[:] = 1
        self.assertEqual(str(context.exception), "can only assign an iterable")

    def test_clear_with_non_list_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'clear' .* 'list' object.* a 'tuple'",
            list.clear,
            (),
        )

    def test_clear_with_empty_list_does_nothing(self):
        ls = []
        self.assertIsNone(ls.clear())
        self.assertEqual(len(ls), 0)

    def test_copy_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'copy' .* 'list' object.* a 'NoneType'",
            list.copy,
            None,
        )

    def test_count_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'count' .* 'list' object.* a 'NoneType'",
            list.count,
            None,
            1,
        )

    def test_count_with_item_returns_int_count(self):
        ls = [1, 2, 1, 3, 1, 2, 1, 4, 1, 2, 1, 3, 1, 2, 1]
        self.assertEqual(ls.count(1), 8)
        self.assertEqual(ls.count(2), 4)
        self.assertEqual(ls.count(3), 2)
        self.assertEqual(ls.count(4), 1)
        self.assertEqual(ls.count(5), 0)

    def test_count_calls_dunder_eq(self):
        class AlwaysEqual:
            def __eq__(self, other):
                return True

        class NeverEqual:
            def __eq__(self, other):
                return False

        a = AlwaysEqual()
        n = NeverEqual()
        a_list = [a, a, a, a, a]
        n_list = [n, n, n]
        self.assertEqual(a_list.count(a), 5)
        self.assertEqual(a_list.count(n), 5)
        self.assertEqual(n_list.count(a), 0)
        self.assertEqual(n_list.count(n), 3)
        self.assertEqual(n_list.count(NeverEqual()), 0)

    def test_count_does_not_use_dunder_getitem_or_dunder_iter(self):
        class Foo(list):
            def __getitem__(self, idx):
                raise NotImplementedError("__getitem__")

            def __iter__(self):
                raise NotImplementedError("__iter__")

        a = Foo([1, 2, 1, 2, 1])
        self.assertEqual(a.count(0), 0)
        self.assertEqual(a.count(1), 3)
        self.assertEqual(a.count(2), 2)

    def test_dunder_hash_is_none(self):
        self.assertIs(list.__hash__, None)

    def test_extend_list_returns_none(self):
        original = [1, 2, 3]
        copy = []
        self.assertIsNone(copy.extend(original))
        self.assertFalse(copy is original)
        self.assertEqual(copy, original)

    def test_extend_with_iterator_that_raises_partway_through_has_sideeffect(self):
        class C:
            def __init__(self):
                self.n = 0

            def __iter__(self):
                return self

            def __next__(self):
                if self.n > 4:
                    raise UserWarning("foo")
                self.n += 1
                return self.n

        result = [0]
        with self.assertRaises(UserWarning):
            result.extend(C())
        self.assertEqual(result, [0, 1, 2, 3, 4, 5])

    def test_delitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        ls = list(range(5))
        del ls[C(0)]
        self.assertEqual(ls, [1, 2, 3, 4])

    def test_delitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        ls = list(range(5))
        del ls[C()]
        self.assertEqual(ls, [0, 1, 3, 4])

    def test_delslice_negative_indexes_removes_first_element(self):
        a = [0, 1]
        del a[-2:-1]
        self.assertEqual(a, [1])

    def test_delslice_negative_start_no_stop_removes_trailing_elements(self):
        a = [0, 1]
        del a[-1:]
        self.assertEqual(a, [0])

    def test_delslice_with_negative_step_deletes_every_other_element(self):
        a = [0, 1, 2, 3, 4]
        del a[::-2]
        self.assertEqual(a, [1, 3])

    def test_delslice_with_start_and_negative_step_deletes_every_other_element(self):
        a = [0, 1, 2, 3, 4]
        del a[1::-2]
        self.assertEqual(a, [0, 2, 3, 4])

    def test_delslice_with_large_step_deletes_last_element(self):
        a = [0, 1, 2, 3, 4]
        del a[4 :: 1 << 333]
        self.assertEqual(a, [0, 1, 2, 3])

    def test_delslice_with_large_stop_deletes_to_end(self):
        a = [0, 1, 2, 3, 4]
        del a[2 : 1 << 333]
        self.assertEqual(a, [0, 1])

    def test_delslice_with_negative_large_stop_deletes_nothing(self):
        a = [0, 1, 2, 3, 4]
        del a[2 : -(1 << 333)]
        self.assertEqual(a, [0, 1, 2, 3, 4])

    def test_delslice_with_true_stop_deletes_to_one(self):
        a = [0, 1, 2, 3, 4]
        del a[:True]
        self.assertEqual(a, [1, 2, 3, 4])

    def test_delslice_with_false_stop_deletes_to_zero(self):
        a = [0, 1, 2, 3, 4]
        del a[:False]
        self.assertEqual(a, [0, 1, 2, 3, 4])

    def test_getitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        ls = list(range(5))
        self.assertEqual(ls[C(3)], 3)

    def test_getitem_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("foo")

        class C:
            __index__ = Desc()

        ls = list(range(5))
        with self.assertRaises(AttributeError) as context:
            ls[C()]
        self.assertEqual(str(context.exception), "foo")

    def test_getitem_with_string_raises_type_error(self):
        ls = list(range(5))
        with self.assertRaises(TypeError) as context:
            ls["3"]
        self.assertEqual(
            str(context.exception), "list indices must be integers or slices, not str"
        )

    def test_getitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        ls = list(range(5))
        self.assertEqual(ls[C()], 2)

    def test_getitem_returns_item(self):
        original = [1, 2, 3, 4, 5, 6]
        self.assertEqual(original[0], 1)
        self.assertEqual(original[3], 4)
        self.assertEqual(original[5], 6)
        original[0] = 6
        original[5] = 1
        self.assertEqual(original[0], 6)
        self.assertEqual(original[3], 4)
        self.assertEqual(original[5], 1)

    def test_getslice_with_valid_indices_returns_sublist(self):
        ls = list(range(5))
        self.assertEqual(ls[2:-1:1], [2, 3])

    def test_getslice_with_negative_start_returns_trailing(self):
        ls = list(range(5))
        self.assertEqual(ls[-2:], [3, 4])

    def test_getslice_with_positive_stop_returns_leading(self):
        ls = list(range(5))
        self.assertEqual(ls[:2], [0, 1])

    def test_getslice_with_negative_stop_returns_all_but_trailing(self):
        ls = list(range(5))
        self.assertEqual(ls[:-2], [0, 1, 2])

    def test_getslice_with_positive_step_returns_forwards_list(self):
        ls = list(range(5))
        self.assertEqual(ls[::2], [0, 2, 4])

    def test_getslice_with_negative_step_returns_backwards_list(self):
        ls = list(range(5))
        self.assertEqual(ls[::-2], [4, 2, 0])

    def test_getslice_with_large_negative_start_returns_copy(self):
        ls = list(range(5))
        self.assertEqual(ls[-10:], ls)

    def test_getslice_with_large_positive_start_returns_empty(self):
        ls = list(range(5))
        self.assertEqual(ls[10:], [])

    def test_getslice_with_large_negative_start_returns_empty(self):
        ls = list(range(5))
        self.assertEqual(ls[:-10], [])

    def test_getslice_with_large_positive_start_returns_copy(self):
        ls = list(range(5))
        self.assertEqual(ls[:10], ls)

    def test_getslice_with_identity_slice_returns_copy(self):
        ls = list(range(5))
        copy = ls[::]
        self.assertEqual(copy, ls)
        self.assertIsNot(copy, ls)

    def test_getslice_with_none_slice_copies_list(self):
        original = [1, 2, 3]
        copy = original[:]
        self.assertEqual(len(copy), 3)
        self.assertEqual(copy[0], 1)
        self.assertEqual(copy[1], 2)
        self.assertEqual(copy[2], 3)

    def test_getslice_with_start_stop_step_returns_list(self):
        original = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        sliced = original[1:2:3]
        self.assertEqual(sliced, [2])

    def test_getslice_with_start_step_returns_list(self):
        original = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        sliced = original[1::3]
        self.assertEqual(sliced, [2, 5, 8])

    def test_getslice_with_start_stop_negative_step_returns_list(self):
        original = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        sliced = original[8:2:-2]
        self.assertEqual(sliced, [9, 7, 5])

    def test_getslice_with_start_stop_step_returns_empty_list(self):
        original = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]
        sliced = original[8:2:2]
        self.assertEqual(sliced, [])

    def test_getslice_in_list_comprehension(self):
        original = [1, 2, 3]
        result = [item[:] for item in [original] * 2]
        self.assertIsNot(result, original)
        self.assertEqual(len(result), 2)
        r1, r2 = result
        self.assertEqual(len(r1), 3)
        r11, r12, r13 = r1
        self.assertEqual(r11, 1)
        self.assertEqual(r12, 2)
        self.assertEqual(r13, 3)

    def test_gt_with_elem_gt_and_same_size_returns_false(self):
        class SubSet(set):
            def __gt__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1])]
        self.assertFalse(list.__gt__(a, b))

    def test_gt_with_elem_gt_and_diff_size_returns_true(self):
        class SubSet(set):
            def __gt__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1, 2])]
        self.assertTrue(list.__gt__(a, b))

    def test_gt_with_elem_eq_gt_and_diff_size_returns_false(self):
        class SubSet(set):
            def __eq__(self, other):
                return True

            def __gt__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1, 2])]
        self.assertFalse(list.__gt__(a, b))

    def test_gt_longer_rhs_with_elem_eq_gt_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return False

            def __gt__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1]), 2]
        self.assertTrue(list.__gt__(a, b))

    def test_gt_with_bigger_list_returns_true(self):
        a = [1, 2, 4]
        b = [1, 2, 3]
        self.assertTrue(list.__gt__(a, b))

    def test_gt_with_equal_lists_returns_false(self):
        a = [1, 2, 3]
        b = [1, 2, 3]
        self.assertFalse(list.__gt__(a, b))

    def test_gt_with_identity_lists_returns_false(self):
        a = [1, 2, 3]
        self.assertFalse(list.__gt__(a, a))

    def test_gt_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__gt__' .* 'list' object.* a 'bool'",
            list.__gt__,
            False,
            [],
        )

    def test_gt_with_shorter_lhs_but_bigger_elem_returns_true(self):
        a = [1, 2, 3]
        b = [1, 1, 1, 1]
        self.assertTrue(list.__gt__(a, b))

    def test_gt_with_shorter_lhs_returns_false(self):
        a = [1, 2]
        b = [1, 2, 3]
        self.assertFalse(list.__gt__(a, b))

    def test_gt_with_shorter_rhs_but_bigger_elem_returns_false(self):
        a = [1, 1, 1, 1]
        b = [1, 2, 3]
        self.assertFalse(list.__gt__(a, b))

    def test_gt_with_shorter_rhs_returns_true(self):
        a = [1, 2, 3, 4]
        b = [1, 2, 3]
        self.assertTrue(list.__gt__(a, b))

    def test_gt_with_smaller_list_returns_false(self):
        a = [1, 2, 2]
        b = [1, 2, 3]
        self.assertFalse(list.__gt__(a, b))

    def test_index_with_non_list_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'index' .* 'list' object.* a 'int'",
            list.index,
            1,
            2,
        )

    def test_index_with_none_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            [].index(1, None)
        self.assertEqual(
            str(context.exception),
            "slice indices must be integers or have an __index__ method",
        )

    def test_index_with_none_stop_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            [].index(1, 2, None)
        self.assertEqual(
            str(context.exception),
            "slice indices must be integers or have an __index__ method",
        )

    def test_index_with_negative_searches_relative_to_end(self):
        ls = list(range(10))
        self.assertEqual(ls.index(4, -7, -3), 4)

    def test_index_searches_from_left(self):
        ls = [1, 2, 1, 2, 1, 2, 1]
        self.assertEqual(ls.index(1, 1, -1), 2)

    def test_index_outside_of_bounds_raises_value_error(self):
        ls = list(range(10))
        with self.assertRaises(ValueError) as context:
            self.assertEqual(ls.index(4, 5), 4)
        self.assertEqual(str(context.exception), "4 is not in list")

    def test_index_calls_dunder_eq(self):
        class AlwaysEqual:
            def __eq__(self, other):
                return True

        class NeverEqual:
            def __eq__(self, other):
                return False

        a = AlwaysEqual()
        n = NeverEqual()
        a_list = [a, a, a, a, a]
        n_list = [n, n, n]
        self.assertEqual(a_list.index(a), 0)
        self.assertEqual(a_list.index(n, 1), 1)
        self.assertEqual(n_list.index(n, 2, 3), 2)

    def test_insert_with_non_list_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'insert' .* 'list' object.* a 'tuple'",
            list.insert,
            (),
            1,
            None,
        )

    def test_ge_with_elem_ge_and_same_size_returns_true(self):
        class SubSet(set):
            def __ge__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1])]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_elem_ge_and_diff_size_returns_true(self):
        class SubSet(set):
            def __ge__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_elem_eq_ge_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return True

            def __ge__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_longer_lhs_with_elem_eq_ge_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return False

            def __ge__(self, other):
                return True

        a = [SubSet([1, 2]), 2]
        b = [SubSet([1])]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_bigger_list_returns_true(self):
        a = [1, 2, 4]
        b = [1, 2, 3]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_equal_lists_returns_true(self):
        a = [1, 2, 3]
        b = [1, 2, 3]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_identity_lists_returns_true(self):
        a = [1, 2, 3]
        self.assertTrue(list.__ge__(a, a))

    def test_ge_with_longer_lhs_but_smaller_elem_returns_false(self):
        a = [1, 1, 1, 1]
        b = [1, 2, 3]
        self.assertFalse(list.__ge__(a, b))

    def test_ge_with_longer_lhs_returns_true(self):
        a = [1, 2, 3, 4]
        b = [1, 2, 3]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_longer_rhs_but_smaller_elem_returns_true(self):
        a = [1, 2, 3]
        b = [1, 1, 1, 1]
        self.assertTrue(list.__ge__(a, b))

    def test_ge_with_longer_rhs_returns_false(self):
        a = [1, 2]
        b = [1, 2, 3]
        self.assertFalse(list.__ge__(a, b))

    def test_ge_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__ge__' .* 'list' object.* a 'bool'",
            list.__ge__,
            False,
            [],
        )

    def test_le_with_elem_le_and_same_size_returns_true(self):
        class SubSet(set):
            def __le__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1])]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_elem_le_and_diff_size_returns_true(self):
        class SubSet(set):
            def __le__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_elem_eq_le_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return True

            def __le__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertTrue(list.__le__(a, b))

    def test_le_longer_lhs_with_elem_eq_le_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return False

            def __le__(self, other):
                return True

        a = [SubSet([1, 2]), 2]
        b = [SubSet([1])]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_bigger_list_returns_false(self):
        a = [1, 2, 4]
        b = [1, 2, 3]
        self.assertFalse(list.__le__(a, b))

    def test_le_with_equal_lists_returns_true(self):
        a = [1, 2, 3]
        b = [1, 2, 3]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_identity_lists_returns_true(self):
        a = [1, 2, 3]
        self.assertTrue(list.__le__(a, a))

    def test_le_with_longer_lhs_but_smaller_elem_returns_true(self):
        a = [1, 1, 1, 1]
        b = [1, 2, 3]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_longer_lhs_returns_false(self):
        a = [1, 2, 3, 4]
        b = [1, 2, 3]
        self.assertFalse(list.__le__(a, b))

    def test_le_with_longer_rhs_but_smaller_elem_returns_false(self):
        a = [1, 2, 3]
        b = [1, 1, 1, 1]
        self.assertFalse(list.__le__(a, b))

    def test_le_with_longer_rhs_returns_true(self):
        a = [1, 2]
        b = [1, 2, 3]
        self.assertTrue(list.__le__(a, b))

    def test_le_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__le__' .* 'list' object.* a 'bool'",
            list.__le__,
            False,
            [],
        )

    def test_le_with_smaller_list_returns_true(self):
        a = [1, 2, 2]
        b = [1, 2, 3]
        self.assertTrue(list.__le__(a, b))

    def test_lt_with_elem_lt_and_same_size_returns_false(self):
        class SubSet(set):
            def __lt__(self, other):
                return True

        a = [SubSet([1])]
        b = [SubSet([1])]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_with_elem_lt_and_diff_size_returns_true(self):
        class SubSet(set):
            def __lt__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertTrue(list.__lt__(a, b))

    def test_lt_with_elem_eq_lt_and_diff_size_returns_false(self):
        class SubSet(set):
            def __eq__(self, other):
                return True

            def __lt__(self, other):
                return True

        a = [SubSet([1, 2])]
        b = [SubSet([1])]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_longer_lhs_with_elem_eq_lt_and_diff_size_returns_true(self):
        class SubSet(set):
            def __eq__(self, other):
                return False

            def __lt__(self, other):
                return True

        a = [SubSet([1, 2]), 2]
        b = [SubSet([1])]
        self.assertTrue(list.__lt__(a, b))

    def test_lt_with_bigger_list_returns_false(self):
        a = [1, 2, 4]
        b = [1, 2, 3]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_with_equal_lists_returns_false(self):
        a = [1, 2, 3]
        b = [1, 2, 3]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_with_identity_lists_returns_false(self):
        a = [1, 2, 3]
        self.assertFalse(list.__lt__(a, a))

    def test_lt_with_longer_lhs_but_smaller_elem_returns_true(self):
        a = [1, 1, 1, 1]
        b = [1, 2, 3]
        self.assertTrue(list.__lt__(a, b))

    def test_lt_with_longer_lhs_returns_false(self):
        a = [1, 2, 3, 4]
        b = [1, 2, 3]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_with_longer_rhs_but_smaller_elem_returns_false(self):
        a = [1, 2, 3]
        b = [1, 1, 1, 1]
        self.assertFalse(list.__lt__(a, b))

    def test_lt_with_longer_rhs_returns_true(self):
        a = [1, 2]
        b = [1, 2, 3]
        self.assertTrue(list.__lt__(a, b))

    def test_lt_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__lt__' .* 'list' object.* a 'bool'",
            list.__lt__,
            False,
            [],
        )

    def test_lt_with_smaller_list_returns_true(self):
        a = [1, 2, 2]
        b = [1, 2, 3]
        self.assertTrue(list.__lt__(a, b))

    def test_pop_with_non_list_raises_type_error(self):
        self.assertRaises(TypeError, list.pop, None)

    def test_pop_with_non_index_index_raises_type_error(self):
        self.assertRaises(TypeError, list.pop, [], "idx")

    def test_pop_with_empty_list_raises_index_error(self):
        self.assertRaises(IndexError, list.pop, [])

    def test_pop_with_no_args_pops_from_end_of_list(self):
        original = [1, 2, 3]
        self.assertEqual(original.pop(), 3)
        self.assertEqual(original, [1, 2])

    def test_pop_with_positive_in_bounds_arg_pops_from_list(self):
        original = [1, 2, 3]
        self.assertEqual(original.pop(1), 2)
        self.assertEqual(original, [1, 3])

    def test_pop_with_positive_out_of_bounds_arg_raises_index_error(self):
        original = [1, 2, 3]
        self.assertRaises(IndexError, list.pop, original, 10)

    def test_pop_with_negative_in_bounds_arg_pops_from_list(self):
        original = [1, 2, 3]
        self.assertEqual(original.pop(-1), 3)
        self.assertEqual(original, [1, 2])

    def test_pop_with_negative_out_of_bounds_arg_raises_index_error(self):
        original = [1, 2, 3]
        self.assertRaises(IndexError, list.pop, original, -10)

    def test_repr_returns_string(self):
        self.assertEqual(list.__repr__([]), "[]")
        self.assertEqual(list.__repr__([42]), "[42]")
        self.assertEqual(list.__repr__([1, 2, "hello"]), "[1, 2, 'hello']")

        class M(type):
            def __repr__(cls):
                return "<M instance>"

        class C(metaclass=M):
            def __repr__(self):
                return "<C instance>"

        self.assertEqual(list.__repr__([C, C()]), "[<M instance>, <C instance>]")

    def test_repr_with_recursive_list_prints_ellipsis(self):
        ls = []
        ls.append(ls)
        self.assertEqual(list.__repr__(ls), "[[...]]")

    def test_setslice_with_empty_slice_grows_list(self):
        grows = []
        grows[:] = [1, 2, 3]
        self.assertEqual(grows, [1, 2, 3])

    def test_setslice_with_list_subclass_calls_dunder_iter(self):
        class C(list):
            def __iter__(self):
                return ["a", "b", "c"].__iter__()

        grows = []
        grows[:] = C()
        self.assertEqual(grows, ["a", "b", "c"])

    def test_sort_with_big_list(self):
        ls = list(range(0, 1000))
        reversed_ls = list(reversed(ls))
        reversed_ls.sort()
        self.assertEqual(ls, reversed_ls)

    def test_sort_with_non_list_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'sort' .* 'list' object.* a 'NoneType'",
            list.sort,
            None,
        )

    def test_sort_with_small_integers(self):
        ls = [6, 5, 4, 3, 2, 1]
        ls.sort()
        self.assertEqual(ls, [1, 2, 3, 4, 5, 6])

    def test_sort_with_non_integers(self):
        ls = ["world", "hello", "magnificent"]
        ls.sort()
        self.assertEqual(ls, ["hello", "magnificent", "world"])

    def test_sort_with_key(self):
        ls = [1, 2, 3, 4, 5, 6]
        ls.sort(key=lambda x: -x)
        self.assertEqual(ls, [6, 5, 4, 3, 2, 1])

    def test_sort_with_key_handles_duplicates(self):
        ls = [1, 1, 2, 2]
        ls.sort(key=lambda x: -x)
        self.assertEqual(ls, [2, 2, 1, 1])

    def test_sort_with_key_handles_noncomparable_item(self):
        class C:
            def __init__(self, val):
                self.val = val

            def __eq__(self, other):
                return self.val == other.val

        ls = [C(i % 2) for i in range(4)]
        ls.sort(key=lambda x: x.val)
        self.assertEqual(ls, [C(0), C(0), C(1), C(1)])


if __name__ == "__main__":
    unittest.main()
