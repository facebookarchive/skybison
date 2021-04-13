#!/usr/bin/env python3
import array
import unittest
from unittest.mock import Mock

from test_support import supports_38_feature


class ArrayTest(unittest.TestCase):
    def test_new_with_bad_typecode_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            array.array.__new__(array.array, "c")
        self.assertEqual(
            str(context.exception),
            "bad typecode (must be b, B, u, h, H, i, I, l, L, q, Q, f or d)",
        )

    def test_new_with_non_str_typecode_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            array.array.__new__(array.array, b"not-string")
        self.assertIn("must be a unicode character, not bytes", str(context.exception))

    def test_new_with_long_str_typecode_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            array.array.__new__(array.array, "too")
        self.assertIn("must be a unicode character, not str", str(context.exception))

    def test_new_with_str_array_init_and_non_str_typecode_raises_type_error(self):
        unicode_array = array.array("u")

        with self.assertRaises(TypeError) as context:
            array.array.__new__(array.array, "c", unicode_array)
        self.assertEqual(
            str(context.exception),
            "cannot use a unicode array to initialize an array with typecode 'c'",
        )

    def test_new_with_valid_typecode_returns_array(self):
        byte_array = array.array("b")
        self.assertEqual(byte_array.typecode, "b")

    def test_new_with_array_subtype_returns_subtype(self):
        class ArraySub(array.array):
            pass

        array_sub = array.array.__new__(ArraySub, "B")
        self.assertIsInstance(array_sub, ArraySub)
        self.assertEqual(array_sub.typecode, "B")

    def test_new_with_integer_typecode_and_non_number_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:
            array.array("i", ["not-a-number"])
        self.assertEqual(str(ctx.exception), "an integer is required (got type str)")

    def test_new_with_integer_typecode_and_float_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:
            array.array("Q", [2.0])
        self.assertEqual(str(ctx.exception), "array item must be integer")

    def test_new_with_integer_typecode_and_too_large_value_raises_overflow_error(self):
        with self.assertRaises(OverflowError) as ctx:
            array.array("b", [128])
        self.assertIn("greater than maximum", str(ctx.exception))

    def test_new_with_float_typecode_and_non_number_raises_type_error(self):
        with self.assertRaises(TypeError) as ctx:
            array.array("f", ["1"])
        self.assertEqual(str(ctx.exception), "must be real number, not str")

    def test_new_with_list_stores_each_number_in_the_array(self):
        int_array = array.array("i", [1, 2, 3])
        self.assertEqual(int_array[0], 1)
        self.assertEqual(int_array[1], 2)
        self.assertEqual(int_array[2], 3)

    def test_new_with_tuple_stores_each_number_in_the_array(self):
        int_array = array.array("d", (1.0, 2.0, 4.0))
        self.assertEqual(int_array[0], 1.0)
        self.assertEqual(int_array[1], 2.0)
        self.assertEqual(int_array[2], 4.0)

    def test_new_with_iterable_stores_each_number_in_the_array(self):
        class Iterator:
            def __iter__(self):
                return [1, 2, 3].__iter__()

        int_array = array.array("b", Iterator())
        self.assertEqual(int_array[0], 1)
        self.assertEqual(int_array[1], 2)
        self.assertEqual(int_array[2], 3)
        with self.assertRaises(IndexError) as ctx:
            int_array[3]
        self.assertEqual(str(ctx.exception), "array index out of range")

    def test_new_with_float_typecode_and_list_calls_dunder_float_on_elements(self):
        class FloatSub(float):
            pass

        class DunderFloat:
            def __float__(self):
                return 4.0

        float_array = array.array("f", [1.0, FloatSub(2.0), DunderFloat()])
        self.assertEqual(float_array[0], 1.0)
        self.assertEqual(type(float_array[0]), float)
        self.assertEqual(float_array[1], 2.0)
        self.assertEqual(type(float_array[1]), float)
        self.assertEqual(float_array[2], 4.0)
        self.assertEqual(type(float_array[2]), float)

    @supports_38_feature
    def test_new_with_integer_typecode_and_list_calls_dunder_index_on_elements(self):
        class IntSub(int):
            pass

        class DunderIndex:
            def __index__(self):
                return 8

        int_array = array.array("i", [1, IntSub(4), DunderIndex()])
        self.assertEqual(int_array[0], 1)
        self.assertEqual(type(int_array[0]), int)
        self.assertEqual(int_array[1], 4)
        self.assertEqual(type(int_array[1]), int)
        self.assertEqual(int_array[2], 8)
        self.assertEqual(type(int_array[2]), int)

    def test_append_does_not_iterate_over_argument(self):
        int_array = array.array("i")
        with self.assertRaises(TypeError) as ctx:
            int_array.append([0, 1])
        self.assertEqual(str(ctx.exception), "an integer is required (got type list)")

    def test_append_grows_array(self):
        int_array = array.array("i")
        self.assertEqual(int_array.__len__(), 0)
        int_array.append(1)
        self.assertEqual(int_array.__len__(), 1)
        int_array.append(2)
        self.assertEqual(int_array.__len__(), 2)
        self.assertEqual(int_array[0], 1)
        self.assertEqual(int_array[1], 2)

    def test_extend_with_iterable_can_resize_past_length_hint(self):
        class Iterator:
            def __length_hint__(self):
                return 1

            def __iter__(self):
                return [1, 2, 3].__iter__()

        int_array = array.array("b")
        int_array.extend(Iterator())
        self.assertEqual(int_array[0], 1)
        self.assertEqual(int_array[1], 2)
        self.assertEqual(int_array[2], 3)
        with self.assertRaises(IndexError) as ctx:
            int_array[3]
        self.assertEqual(str(ctx.exception), "array index out of range")

    def test_extend_with_iterable_shorter_than_length_hint_resizes_to_iterable(self):
        class Iterator:
            def __length_hint__(self):
                return 6

            def __iter__(self):
                return [1, 2, 3].__iter__()

        int_array = array.array("b")
        int_array.extend(Iterator())
        self.assertEqual(int_array[0], 1)
        self.assertEqual(int_array[1], 2)
        self.assertEqual(int_array[2], 3)
        with self.assertRaises(IndexError) as ctx:
            int_array[3] = 4
        self.assertEqual(str(ctx.exception), "array assignment index out of range")

    def test_subclass_will_call_array_setitem_on_initialization(self):
        class ArraySub(array.array):
            __setitem__ = Mock(name="__setitem__", return_value=None)

        int_array = ArraySub("i", [3, 2, 1])
        self.assertEqual(int_array[0], 3)
        self.assertEqual(int_array[1], 2)
        self.assertEqual(int_array[2], 1)

        int_array[0] = 0
        int_array.__setitem__.assert_called_once()

    def test_getting_a_value_out_range_raises_index_error(self):
        int_array = array.array("i", [1, 2, 3])
        with self.assertRaises(IndexError) as ctx:
            int_array[5]
        self.assertEqual(str(ctx.exception), "array index out of range")

    def test_getting_a_negative_value_out_of_range_raises_index_error(self):
        int_array = array.array("i", [1, 2, 3])
        with self.assertRaises(IndexError) as ctx:
            int_array[-4]
        self.assertEqual(str(ctx.exception), "array index out of range")

    def test_getting_an_index_longer_than_word_raises_index_error(self):
        int_array = array.array("i", [1, 2, 3])
        with self.assertRaises(IndexError) as ctx:
            int_array[1 << 65]
        self.assertEqual(
            str(ctx.exception), "cannot fit 'int' into an index-sized integer"
        )

    def test_getting_a_non_integer_index_raises_type_error(self):
        int_array = array.array("i", [1, 2, 3])
        with self.assertRaises(TypeError) as ctx:
            int_array["not-int"]
        self.assertIn("integer", str(ctx.exception))

    def test_getitem_wth_dunder_index_calls_dunder_index(self):
        class DunderIndex:
            def __index__(self):
                return 0

        int_array = array.array("i", [1, 2, 3])
        self.assertEqual(int_array[DunderIndex()], 1)

    def test_setting_a_value_out_range_raises_index_error(self):
        int_array = array.array("i", [1, 2, 3])
        with self.assertRaises(IndexError) as ctx:
            int_array[5] = 1
        self.assertEqual(str(ctx.exception), "array assignment index out of range")

    def test_setting_a_negative_value_out_range_raises_index_error(self):
        int_array = array.array("i", [1, 2, 3])
        with self.assertRaises(IndexError) as ctx:
            int_array[-5] = 1
        self.assertEqual(str(ctx.exception), "array assignment index out of range")

    def test_setting_an_index_longer_than_word_raises_index_error(self):
        int_array = array.array("i", [1, 2, 3])
        with self.assertRaises(IndexError) as ctx:
            int_array[1 << 65] = 1
        self.assertEqual(
            str(ctx.exception), "cannot fit 'int' into an index-sized integer"
        )

    def test_setting_a_non_integer_index_raises_type_error(self):
        int_array = array.array("i", [1, 2, 3])
        with self.assertRaises(TypeError) as ctx:
            int_array["not-int"] = 2
        self.assertIn("integer", str(ctx.exception))

    def test_setitem_wth_dunder_index_calls_dunder_index(self):
        class DunderIndex:
            def __index__(self):
                return 0

        int_array = array.array("i", [1, 2, 3])
        int_array[DunderIndex()] = 10
        self.assertEqual(int_array[0], 10)


if __name__ == "__main__":
    unittest.main()
