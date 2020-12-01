#!/usr/bin/env python3
import unittest
from _collections import _deque_iterator, _deque_reverse_iterator, deque


class DequeIteratorTests(unittest.TestCase):
    def test_dunder_length_hint_returns_remaining(self):
        d = deque(range(10))

        it = _deque_iterator(d)
        self.assertEqual(it.__length_hint__(), 10)
        next(it)
        self.assertEqual(it.__length_hint__(), 9)

        it = _deque_iterator(d, 6)
        self.assertEqual(it.__length_hint__(), 4)

    def test_dunder_new_with_bad_args_raises_type_error(self):
        d = deque()
        self.assertRaises(TypeError, _deque_iterator.__new__, 1)
        self.assertRaises(TypeError, _deque_iterator.__new__, int)
        self.assertRaises(TypeError, _deque_iterator.__new__, _deque_iterator, [])
        self.assertRaises(TypeError, _deque_iterator.__new__, _deque_iterator, d, "")

    def test_dunder_new_without_index_sets_index_zero(self):
        d = deque(range(10))
        it = _deque_iterator(d)
        self.assertEqual(list(it), list(d))

    def test_dunder_new_with_index_truncates_to_deque_len(self):
        d = deque(range(10))

        it = _deque_iterator(d, -(2 ** 63))
        self.assertEqual(list(it), list(d))

        it = _deque_iterator(d, -1)
        self.assertEqual(list(it), list(d))

        it = _deque_iterator(d, 0)
        self.assertEqual(list(it), list(d))

        it = _deque_iterator(d, 3)
        self.assertEqual(list(it), list(range(3, 10)))

        it = _deque_iterator(d, 10)
        self.assertEqual(list(it), [])

        it = _deque_iterator(d, 12)
        self.assertEqual(list(it), [])

        it = _deque_iterator(d, 2 ** 63 - 1)
        self.assertEqual(list(it), [])

    def test_dunder_new_with_large_int_index_raises_overflow_error(self):
        d = deque(range(10))
        self.assertRaises(OverflowError, _deque_iterator, d, 2 ** 63)
        self.assertRaises(OverflowError, _deque_iterator, d, -(2 ** 63) - 1)

    def test_iter_returns_self(self):
        d = deque(range(10))
        it = _deque_iterator(d)
        self.assertIs(iter(it), it)

    def test_next_after_deque_append_raises_runtime_error(self):
        d = deque(range(10))
        it = _deque_iterator(d)
        d.append(10)
        self.assertRaises(RuntimeError, next, it)

    def test_next_after_deque_pop_raises_runtime_error(self):
        d = deque(range(10))
        it = _deque_iterator(d)
        d.pop()
        self.assertRaises(RuntimeError, next, it)


class DequeReverseIteratorTests(unittest.TestCase):
    def test_dunder_length_hint_returns_remaining(self):
        d = deque(range(10))

        it = _deque_reverse_iterator(d)
        self.assertEqual(it.__length_hint__(), 10)
        next(it)
        self.assertEqual(it.__length_hint__(), 9)

        it = _deque_reverse_iterator(d, 6)
        self.assertEqual(it.__length_hint__(), 4)

    def test_dunder_new_with_bad_args_raises_type_error(self):
        d = deque()
        self.assertRaises(TypeError, _deque_reverse_iterator.__new__, 1)
        self.assertRaises(TypeError, _deque_reverse_iterator.__new__, int)
        self.assertRaises(
            TypeError, _deque_reverse_iterator.__new__, _deque_reverse_iterator, []
        )
        self.assertRaises(
            TypeError, _deque_reverse_iterator.__new__, _deque_reverse_iterator, d, ""
        )

    def test_dunder_new_without_index_sets_index_zero(self):
        d = deque(range(10))
        it = _deque_reverse_iterator(d)
        self.assertEqual(list(it), list(range(9, -1, -1)))

    def test_dunder_new_with_index_truncates_to_deque_len(self):
        d = deque(range(10))
        rev = list(range(9, -1, -1))

        it = _deque_reverse_iterator(d, -(2 ** 63))
        self.assertEqual(list(it), rev)

        it = _deque_reverse_iterator(d, -1)
        self.assertEqual(list(it), rev)

        it = _deque_reverse_iterator(d, 0)
        self.assertEqual(list(it), rev)

        it = _deque_reverse_iterator(d, 3)
        self.assertEqual(list(it), rev[3:])

        it = _deque_reverse_iterator(d, 10)
        self.assertEqual(list(it), [])

        it = _deque_reverse_iterator(d, 12)
        self.assertEqual(list(it), [])

        it = _deque_reverse_iterator(d, 2 ** 63 - 1)
        self.assertEqual(list(it), [])

    def test_dunder_new_with_large_int_index_raises_overflow_error(self):
        d = deque(range(10))
        self.assertRaises(OverflowError, _deque_reverse_iterator, d, 2 ** 63)
        self.assertRaises(OverflowError, _deque_reverse_iterator, d, -(2 ** 63) - 1)

    def test_iter_returns_self(self):
        d = deque(range(10))
        it = _deque_reverse_iterator(d)
        self.assertIs(iter(it), it)

    def test_next_after_deque_append_raises_runtime_error(self):
        d = deque(range(10))
        it = _deque_reverse_iterator(d)
        d.append(10)
        self.assertRaises(RuntimeError, next, it)

    def test_next_after_deque_pop_raises_runtime_error(self):
        d = deque(range(10))
        it = _deque_reverse_iterator(d)
        d.pop()
        self.assertRaises(RuntimeError, next, it)


class DequeTests(unittest.TestCase):
    def test_dunder_getitem_returns_item_at_index(self):
        result = deque()
        result.append(1)
        result.appendleft(0)
        result.append(2)
        result.appendleft(-1)
        self.assertEqual(result.__getitem__(0), -1)
        self.assertEqual(result.__getitem__(1), 0)
        self.assertEqual(result.__getitem__(2), 1)
        self.assertEqual(result.__getitem__(3), 2)

    def test_dunder_getitem_with_index_too_small_raises_index_error(self):
        result = deque()
        with self.assertRaises(IndexError) as context:
            result.__getitem__(-1)
        self.assertEqual(str(context.exception), "deque index out of range")

    def test_dunder_getitem_with_index_too_large_raises_index_error(self):
        result = deque()
        result.append(1)
        result.append(2)
        result.append(3)
        with self.assertRaises(IndexError) as context:
            result.__getitem__(3)
        self.assertEqual(str(context.exception), "deque index out of range")

    def test_dunder_getitem_with_negative_index_indexes_from_end(self):
        result = deque()
        result.append(1)
        result.appendleft(0)
        result.append(2)
        result.appendleft(-1)
        self.assertEqual(result.__getitem__(-1), 2)

    def test_dunder_getitem_with_non_int_raises_type_error(self):
        result = deque([1, 2, 3])
        with self.assertRaises(TypeError):
            result.__getitem__("3")

    def test_dunder_getitem_with_slice_raises_type_error(self):
        result = deque()
        result.append(1)
        result.append(2)
        result.append(3)
        self.assertRaises(TypeError, result.__getitem__, slice(0, 2))

    def test_dunder_init_adds_elements_from_iterable(self):
        result = deque([1, 2, 3])
        self.assertEqual(len(result), 3)
        self.assertEqual(1, result[0])
        self.assertEqual(2, result[1])
        self.assertEqual(3, result[2])

    def test_dunder_init_clears_existing_deque(self):
        d = deque([1, 2, 3])
        self.assertEqual(len(d), 3)

        d.__init__(d)
        self.assertEqual(len(d), 0)

    def test_dunder_init_with_large_int_raises_overflow_error(self):
        with self.assertRaises(OverflowError):
            deque(maxlen=2 ** 63 + 1)

    def test_dunder_init_with_large_negative_int_raises_overflow_error(self):
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

    def test_dunder_iter_returns_deque_iterator(self):
        d = deque(range(10))
        self.assertIsInstance(d.__iter__(), _deque_iterator)

    def test_dunder_new(self):
        result = deque.__new__(deque)
        self.assertTrue(isinstance(result, deque))
        self.assertEqual(result.maxlen, None)

    def test_dunder_reversed_returns_deque__reverse_iterator(self):
        d = deque(range(10))
        self.assertIsInstance(d.__reversed__(), _deque_reverse_iterator)

    def test_append_adds_elements(self):
        result = deque()
        self.assertEqual(len(result), 0)
        result.append(1)
        result.append(2)
        result.append(3)
        self.assertEqual(len(result), 3)

    def test_append_over_maxlen_removes_element_from_left(self):
        result = deque(maxlen=3)
        result.append(1)
        result.append(2)
        result.append(3)
        result.append("foo")
        self.assertEqual(len(result), 3)
        self.assertEqual(result[0], 2)
        self.assertEqual(result[1], 3)
        self.assertEqual(result[2], "foo")

    def test_append_over_zero_maxlen_does_nothing(self):
        result = deque(maxlen=0)
        result.append(1)
        self.assertEqual(len(result), 0)

    def test_appendleft_adds_elements_to_left(self):
        result = deque()
        self.assertEqual(len(result), 0)
        result.appendleft(1)
        result.appendleft(2)
        result.appendleft(3)
        self.assertEqual(len(result), 3)

    def test_appendleft_over_maxlen_removes_element_from_right(self):
        result = deque(maxlen=3)
        result.appendleft(1)
        result.appendleft(2)
        result.appendleft(3)
        result.appendleft("foo")
        self.assertEqual(len(result), 3)
        self.assertEqual(result[0], "foo")
        self.assertEqual(result[1], 3)
        self.assertEqual(result[2], 2)

    def test_appendleft_over_zero_maxlen_does_nothing(self):
        result = deque(maxlen=0)
        result.appendleft(1)
        self.assertEqual(len(result), 0)

    def test_clear_removes_elements(self):
        result = deque()
        result.append(1)
        result.append(2)
        result.append(3)
        self.assertEqual(len(result), 3)
        result.clear()
        self.assertEqual(len(result), 0)

    def test_pop_from_empty_deque_raises_index_error(self):
        result = deque()
        self.assertRaises(IndexError, result.pop)

    def test_pop_removes_element_from_right(self):
        result = deque()
        result.append(1)
        result.append(2)
        result.append(3)
        result.pop()
        self.assertEqual(len(result), 2)

    def test_popleft_from_empty_deque_raises_index_error(self):
        result = deque()
        self.assertRaises(IndexError, result.popleft)

    def test_popleft_removes_element_from_left(self):
        result = deque()
        result.append(1)
        result.append(2)
        result.append(3)
        result.popleft()
        self.assertEqual(len(result), 2)

    def test_reverse_with_empty_deque_is_noop(self):
        d = deque()
        self.assertIsNone(d.reverse())
        self.assertEqual(list(d), [])

    def test_reverse_from_range_reverses_deque(self):
        d = deque(range(5))
        self.assertIsNone(d.reverse())
        self.assertEqual(list(d), list(range(4, -1, -1)))

    def test_reverse_with_longer_left_reverses_deque(self):
        d = deque()
        d.append(2)
        d.append(1)
        d.appendleft(3)
        d.appendleft(4)
        d.appendleft(5)
        self.assertIsNone(d.reverse())
        self.assertEqual(list(d), list(range(1, 6)))

    def test_reverse_with_longer_right_reverses_deque(self):
        d = deque()
        d.append(3)
        d.append(2)
        d.append(1)
        d.appendleft(4)
        d.appendleft(5)
        self.assertIsNone(d.reverse())
        self.assertEqual(list(d), list(range(1, 6)))


if __name__ == "__main__":
    unittest.main()
