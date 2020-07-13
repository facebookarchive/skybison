#!/usr/bin/env python3
import unittest

from _collections import deque


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

    def test_dunder_new(self):
        result = deque.__new__(deque)
        self.assertTrue(isinstance(result, deque))
        self.assertEqual(result.maxlen, None)

    def test_append_adds_elements(self):
        result = deque()
        self.assertEqual(len(result), 0)
        result.append(1)
        result.append(2)
        result.append(3)
        self.assertEqual(len(result), 3)

    def test_appendleft_adds_elements_to_left(self):
        result = deque()
        self.assertEqual(len(result), 0)
        result.appendleft(1)
        result.appendleft(2)
        result.appendleft(3)
        self.assertEqual(len(result), 3)

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


if __name__ == "__main__":
    unittest.main()
