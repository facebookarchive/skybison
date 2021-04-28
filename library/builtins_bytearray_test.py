#!/usr/bin/env python3
import unittest
from array import array
from unittest.mock import Mock


class ByteArrayTests(unittest.TestCase):
    def test_dunder_add_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__add__' .* 'bytearray' object.* a 'str'",
            bytearray.__add__,
            "not a bytearray",
            bytearray(),
        )

    def test_dunder_contains_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.__contains__("not_bytearray", 123)

    def test_dunder_contains_with_element_in_bytearray_returns_true(self):
        self.assertTrue(bytearray(b"abc").__contains__(ord("a")))

    def test_dunder_contains_with_bytearray_in_bytearray_returns_true(self):
        self.assertTrue(bytearray(b"abcd").__contains__(bytearray(b"bc")))

    def test_dunder_contains_with_array_in_bytearray_returns_true(self):
        self.assertTrue(bytearray(b"abcd").__contains__(array("b", b"bc")))

    def test_dunder_contains_with_bytestring_in_bytearray_returns_true(self):
        self.assertTrue(bytearray(b"abcd").__contains__(b"bc"))
        self.assertTrue(bytearray(b"abcd").__contains__(b"abcd"))
        self.assertTrue(bytearray(b"abcd").__contains__(b""))

    def test_dunder_contains_with_memoryview_in_bytearray_returns_true(self):
        self.assertTrue(bytearray(b"abcd").__contains__(memoryview(b"bc")))

    def test_dunder_contains_with_element_not_in_bytearray_returns_false(self):
        self.assertFalse(bytearray(b"abc").__contains__(ord("z")))

    def test_dunder_contains_with_bytearray_not_in_bytearray_returns_false(self):
        self.assertFalse(bytearray(b"abcd").__contains__(bytearray(b"bd")))

    def test_dunder_contains_with_memoryview_not_in_bytearray_returns_false(self):
        self.assertFalse(bytearray(b"abcd").__contains__(memoryview(b"bd")))

    def test_dunder_contains_with_array_not_in_bytearray_returns_false(self):
        self.assertFalse(bytearray(b"abcd").__contains__(array("b", b"bd")))
        self.assertFalse(bytearray(b"abcd").__contains__(array("i", [ord("b")])))

    def test_dunder_contains_with_bytestring_not_in_bytearray_returns_false(self):
        self.assertFalse(bytearray(b"abcd").__contains__(b"bd"))
        self.assertFalse(bytearray(b"abcd").__contains__(b"abcde"))

    def test_dunder_contains_calls_dunder_index(self):
        class C:
            __index__ = Mock(name="__index__", return_value=ord("a"))

        c = C()
        self.assertTrue(bytearray(b"abc").__contains__(c))
        c.__index__.assert_called_once()

    def test_dunder_contains_ignores_errors_from_dunder_index_from_byteslike(self):
        class C(bytes):
            __index__ = Mock(name="__index__", side_effect=MemoryError("foo"))

        c = C(b"ab")
        self.assertTrue(bytearray(b"abc").__contains__(c))
        c.__index__.assert_called_once()

    def test_dunder_contains_calls_dunder_index_before_checking_byteslike(self):
        class C(bytes):
            __index__ = Mock(name="__index__", return_value=ord("a"))

        c = C(b"q")
        self.assertTrue(bytearray(b"abc").__contains__(c))
        c.__index__.assert_called_once()

    def test_dunder_contains_ignores_errors_from_dunder_index(self):
        class C:
            __index__ = Mock(name="__index__", side_effect=MemoryError("foo"))

        c = C()
        container = bytearray(b"abc")
        with self.assertRaises(TypeError):
            container.__contains__(c)
        c.__index__.assert_called_once()

    def test_delitem_with_out_of_bounds_index_raises_index_error(self):
        result = bytearray(b"")
        with self.assertRaises(IndexError) as context:
            del result[0]
        self.assertEqual(str(context.exception), "bytearray index out of range")

    def test_delitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        result = bytearray(b"01234")
        del result[C(0)]
        self.assertEqual(result, bytearray(b"1234"))

    def test_delitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        result = bytearray(b"01234")
        del result[C()]
        self.assertEqual(result, bytearray(b"0134"))

    def test_delslice_negative_indexes_removes_first_element(self):
        result = bytearray(b"01")
        del result[-2:-1]
        self.assertEqual(result, bytearray(b"1"))

    def test_delslice_negative_start_no_stop_removes_trailing_elements(self):
        result = bytearray(b"01")
        del result[-1:]
        self.assertEqual(result, bytearray(b"0"))

    def test_delslice_with_negative_step_deletes_every_other_element(self):
        result = bytearray(b"01234")
        del result[::-2]
        self.assertEqual(result, bytearray(b"13"))

    def test_delslice_with_start_and_negative_step_deletes_every_other_element(self):
        result = bytearray(b"01234")
        del result[1::-2]
        self.assertEqual(result, bytearray(b"0234"))

    def test_delslice_with_large_step_deletes_last_element(self):
        result = bytearray(b"01234")
        del result[4 :: 1 << 333]
        self.assertEqual(result, bytearray(b"0123"))

    def test_dunder_eq_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__eq__' .* 'bytearray' object.* a 'str'",
            bytearray.__eq__,
            "not a bytearray",
            bytearray(),
        )

    def test_dunder_ge_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__ge__' .* 'bytearray' object.* a 'str'",
            bytearray.__ge__,
            "not a bytearray",
            bytearray(),
        )

    def test_dunder_getitem_with_non_index_raises_type_error(self):
        ba = bytearray()
        with self.assertRaises(TypeError) as context:
            ba["not int or slice"]
        self.assertEqual(
            str(context.exception),
            "bytearray indices must be integers or slices, not str",
        )

    def test_dunder_getitem_with_large_int_raises_index_error(self):
        ba = bytearray()
        with self.assertRaises(IndexError) as context:
            ba[2 ** 63]
        self.assertEqual(
            str(context.exception), "cannot fit 'int' into an index-sized integer"
        )

    def test_dunder_getitem_positive_out_of_bounds_raises_index_error(self):
        ba = bytearray(b"foo")
        with self.assertRaises(IndexError) as context:
            ba[3]
        self.assertEqual(str(context.exception), "bytearray index out of range")

    def test_dunder_getitem_negative_out_of_bounds_raises_index_error(self):
        ba = bytearray(b"foo")
        with self.assertRaises(IndexError) as context:
            ba[-4]
        self.assertEqual(str(context.exception), "bytearray index out of range")

    def test_dunder_getitem_with_positive_int_returns_int(self):
        ba = bytearray(b"foo")
        self.assertEqual(ba[0], 102)
        self.assertEqual(ba[1], 111)
        self.assertEqual(ba[2], 111)

    def test_dunder_getitem_with_negative_int_returns_int(self):
        ba = bytearray(b"foo")
        self.assertEqual(ba[-3], 102)
        self.assertEqual(ba[-2], 111)
        self.assertEqual(ba[-1], 111)

    def test_dunder_getitem_with_int_subclass_returns_int(self):
        class N(int):
            pass

        ba = bytearray(b"foo")
        self.assertEqual(ba[N(0)], 102)
        self.assertEqual(ba[N(-2)], 111)

    def test_dunder_getitem_with_slice_returns_bytearray(self):
        ba = bytearray(b"hello world")
        result = ba[1:-1:3]
        self.assertIsInstance(result, bytearray)
        self.assertEqual(result, b"eoo")

    def test_dunder_gt_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__gt__' .* 'bytearray' object.* a 'str'",
            bytearray.__gt__,
            "not a bytearray",
            bytearray(),
        )

    def test_dunder_hash_is_none(self):
        self.assertIs(bytearray.__hash__, None)

    def test_dunder_iadd_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__iadd__' .* 'bytearray' object.* a 'str'",
            bytearray.__iadd__,
            "not a bytearray",
            bytearray(),
        )

    def test_dunder_imul_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__imul__' .* 'bytearray' object.* a 'str'",
            bytearray.__imul__,
            "not a bytearray",
            bytearray(),
        )

    def test_dunder_init_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__init__' .* 'bytearray' object.* a 'bytes'",
            bytearray.__init__,
            b"",
        )

    def test_dunder_init_no_args_clears_array(self):
        ba = bytearray(b"123")
        self.assertIsNone(ba.__init__())
        self.assertEqual(ba, b"")

    def test_dunder_init_with_encoding_without_source_raises_type_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(TypeError) as context:
            ba.__init__(encoding="utf-8")
        self.assertEqual(str(context.exception), "encoding without a string argument")

    def test_dunder_init_with_errors_without_source_raises_type_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(TypeError) as context:
            ba.__init__(errors="strict")
        self.assertEqual(str(context.exception), "errors without a string argument")

    def test_dunder_init_with_str_without_encoding_raises_type_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(TypeError) as context:
            ba.__init__("")
        self.assertEqual(str(context.exception), "string argument without an encoding")

    def test_dunder_init_with_ascii_str_copies_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__("hello", "ascii"))
        self.assertEqual(ba, b"hello")

    def test_dunder_init_with_invalid_unicode_propagates_surrogate_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(UnicodeEncodeError) as context:
            ba.__init__("hello\uac80world", "ascii")
        self.assertIn("ascii", str(context.exception))

    def test_dunder_init_with_ignore_errors_copies_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__("hello\uac80world", "ascii", "ignore"))
        self.assertEqual(ba, b"helloworld")

    def test_dunder_init_with_non_str_and_encoding_raises_type_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(TypeError) as context:
            ba.__init__(0, encoding="utf-8")
        self.assertEqual(str(context.exception), "encoding without a string argument")

    def test_dunder_init_with_non_str_and_errors_raises_type_error(self):
        ba = bytearray.__new__(bytearray)
        with self.assertRaises(TypeError) as context:
            ba.__init__(0, errors="ignore")
        self.assertEqual(str(context.exception), "errors without a string argument")

    def test_dunder_init_with_int_fills_with_null_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(5))
        self.assertEqual(ba, b"\x00\x00\x00\x00\x00")

    def test_dunder_init_with_int_subclass_fills_with_null_bytes(self):
        class N(int):
            pass

        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(N(3)))
        self.assertEqual(ba, b"\x00\x00\x00")

    def test_dunder_init_with_index_fills_with_null_bytes(self):
        class N:
            def __index__(self):
                return 3

        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(N()))
        self.assertEqual(ba, b"\x00\x00\x00")

    def test_dunder_init_with_bytes_copies_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(b"abc"))
        self.assertEqual(ba, b"abc")

    def test_dunder_init_with_other_bytearray_copies_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(bytearray(b"abc")))
        self.assertEqual(ba, b"abc")

    def test_dunder_init_with_self_empties(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__(ba))
        self.assertEqual(ba, b"")

    def test_dunder_init_with_iterable_copies_bytes(self):
        ba = bytearray.__new__(bytearray)
        self.assertIsNone(ba.__init__([100, 101, 102]))
        self.assertEqual(ba, b"def")

    def test_dunder_le_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__le__' .* 'bytearray' object.* a 'str'",
            bytearray.__le__,
            "not a bytearray",
            bytearray(),
        )

    def test_dunder_len_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__len__' .* 'bytearray' object.* a 'str'",
            bytearray.__len__,
            "not a bytearray",
        )

    def test_dunder_lt_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__lt__' .* 'bytearray' object.* a 'str'",
            bytearray.__lt__,
            "not a bytearray",
            bytearray(),
        )

    def test_dunder_mul_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__mul__' .* 'bytearray' object.* a 'str'",
            bytearray.__mul__,
            "not a bytearray",
            2,
        )

    def test_dunder_ne_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__ne__' .* 'bytearray' object.* a 'str'",
            bytearray.__ne__,
            "not a bytearray",
            bytearray(),
        )

    def test_dunder_repr_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__repr__' .* 'bytearray' object.* a 'str'",
            bytearray.__repr__,
            "not a bytearray",
        )

    def test_dunder_setitem_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.__setitem__(None, 1, 5)

    def test_dunder_setitem_with_non_int_value_raises_type_error(self):
        with self.assertRaises(TypeError):
            ba = bytearray(b"foo")
            bytearray.__setitem__(ba, 1, b"x")

    def test_dunder_setitem_with_int_key_sets_value_at_index(self):
        ba = bytearray(b"foo")
        bytearray.__setitem__(ba, 1, 1)
        self.assertEqual(ba, bytearray(b"f\01o"))

    def test_dunder_setitem_calls_dunder_index(self):
        class C:
            def __index__(self):
                raise UserWarning("foo")

        ba = bytearray(b"foo")
        with self.assertRaises(UserWarning):
            bytearray.__setitem__(ba, C(), 1)

    def test_dunder_setitem_slice_with_subclass_does_not_call_dunder_iter(self):
        class C(bytearray):
            def __iter__(self):
                raise UserWarning("foo")

            def __buffer__(self):
                raise UserWarning("bar")

        result = bytearray(b"foo")
        other = C(b"abc")
        result[1:] = other
        self.assertEqual(result, bytearray(b"fabc"))

    def test_dunder_setitem_slice_with_iterable_calls_dunder_iter(self):
        class C:
            def __iter__(self):
                raise UserWarning("foo")

        ba = bytearray(b"foo")
        it = C()
        with self.assertRaises(UserWarning):
            ba[1:] = it

    def test_dunder_setitem_slice_basic(self):
        result = bytearray(b"abcdefg")
        result[2:5] = b"CDE"
        self.assertEqual(result, bytearray(b"abCDEfg"))

    def test_dunder_setitem_slice_grow(self):
        result = bytearray(b"abcdefg")
        result[2:5] = b"CDEXYZ"
        self.assertEqual(result, bytearray(b"abCDEXYZfg"))

    def test_dunder_setitem_slice_shrink(self):
        result = bytearray(b"abcdefg")
        result[2:6] = b"CD"
        self.assertEqual(result, bytearray(b"abCDg"))

    def test_dunder_setitem_slice_iterable(self):
        result = bytearray(b"abcdefg")
        result[2:6] = (ord("x"), ord("y"), ord("z")).__iter__()
        self.assertEqual(result, bytearray(b"abxyzg"))

    def test_dunder_setitem_slice_self(self):
        result = bytearray(b"abcdefg")
        result[2:5] = result
        self.assertEqual(result, bytearray(b"ababcdefgfg"))

    def test_dunder_setitem_slice_rev_bounds(self):
        # Reverse ordered bounds, but step still +1
        result = bytearray(b"1234567890")
        result[5:2] = b"abcde"
        self.assertEqual(result, bytearray(b"12345abcde67890"))

    def test_dunder_setitem_slice_step(self):
        result = bytearray(b"0123456789xxxx")
        result[2:10:3] = b"abc"
        self.assertEqual(result, bytearray(b"01a34b67c9xxxx"))

    def test_dunder_setitem_slice_step_neg(self):
        result = bytearray(b"0123456789xxxxx")
        result[10:2:-3] = b"abc"
        self.assertEqual(result, bytearray(b"0123c56b89axxxx"))

    def test_dunder_setitem_slice_tuple(self):
        result = bytearray(b"0123456789xxxx")
        result[2:10:3] = (ord("a"), ord("b"), ord("c"))
        self.assertEqual(result, bytearray(b"01a34b67c9xxxx"))

    def test_dunder_setitem_slice_short_stop(self):
        result = bytearray(b"1234567890")
        result[:1] = b"000"
        self.assertEqual(result, bytearray(b"000234567890"))

    def test_dunder_setitem_slice_long_stop(self):
        result = bytearray(b"111")
        result[:1] = b"00000"
        self.assertEqual(result, bytearray(b"0000011"))

    def test_dunder_setitem_slice_short_step(self):
        result = bytearray(b"1234567890")
        result[::1] = b"000"
        self.assertEqual(result, bytearray(b"000"))

    def test_dunder_setitem_slice_appends_to_end(self):
        result = bytearray(b"abc")
        result[3:] = b"000"
        self.assertEqual(result, bytearray(b"abc000"))

    def test_dunder_setitem_slice_with_rhs_bigger_than_slice_length_raises_value_error(
        self,
    ):
        result = bytearray(b"0123456789xxxx")
        with self.assertRaises(ValueError) as context:
            result[2:10:3] = b"abcd"

        self.assertEqual(
            str(context.exception),
            "attempt to assign bytes of size 4 to extended slice of size 3",
        )

    def test_dunder_setitem_slice_with_rhs_shorter_than_slice_length_raises_value_error(
        self,
    ):
        result = bytearray(b"0123456789")
        with self.assertRaises(ValueError) as context:
            result[:8:2] = b"000"

        self.assertEqual(
            str(context.exception),
            "attempt to assign bytes of size 3 to extended slice of size 4",
        )

    def test_dunder_setitem_slice_with_non_iterable_rhs_raises_type_error(self):
        result = bytearray(b"abcdefg")
        with self.assertRaises(TypeError) as context:
            result[2:6] = 5

        self.assertEqual(
            str(context.exception),
            "can assign only bytes, buffers, or iterables of ints in range(0, 256)",
        )

    def test_append_appends_item_to_bytearray(self):
        result = bytearray()
        result.append(ord("a"))
        self.assertEqual(result, bytearray(b"a"))

    def test_append_with_index_calls_dunder_index(self):
        class Idx:
            def __index__(self):
                return ord("q")

        result = bytearray()
        result.append(Idx())
        self.assertEqual(result, bytearray(b"q"))

    def test_append_with_string_raises_type_error(self):
        result = bytearray()
        with self.assertRaises(TypeError) as context:
            result.append("a")
        self.assertEqual(
            "'str' object cannot be interpreted as an integer", str(context.exception)
        )

    def test_append_with_large_int_raises_value_error(self):
        result = bytearray()
        with self.assertRaises(ValueError) as context:
            result.append(256)
        self.assertEqual("byte must be in range(0, 256)", str(context.exception))

    def test_append_with_negative_int_raises_value_error(self):
        result = bytearray()
        with self.assertRaises(ValueError) as context:
            result.append(-10)
        self.assertEqual("byte must be in range(0, 256)", str(context.exception))

    def test_clear_with_non_bytearray_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'clear' .* 'bytearray' object.* a 'bytes'",
            bytearray.clear,
            b"",
        )

    def test_copy_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'copy' .* 'bytearray' object.* a 'bytes'",
            bytearray.copy,
            b"",
        )

    def test_copy_returns_new_object(self):
        array = bytearray(b"123")
        copy = array.copy()
        self.assertIsInstance(copy, bytearray)
        self.assertIsNot(copy, array)
        self.assertEqual(array, copy)

    def test_count_with_bytes_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'count' .* 'bytearray' object.* a 'bytes'",
            bytearray.count,
            b"",
            bytearray(),
        )

    def test_count_with_nonbyte_int_raises_value_error(self):
        haystack = bytearray()
        needle = 266
        with self.assertRaises(ValueError) as context:
            haystack.count(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    def test_count_with_string_raises_type_error(self):
        haystack = bytearray()
        needle = "133"
        with self.assertRaises(TypeError) as context:
            haystack.count(needle)
        self.assertEqual(
            str(context.exception),
            "argument should be integer or bytes-like object, not 'str'",
        )

    def test_count_with_non_number_index(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.count(needle), 1)

    def test_count_with_dunder_int_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            def __index__(self):
                return ord("a")

            __int__ = Desc()

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.count(needle), 1)

    def test_count_with_dunder_float_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            __float__ = Desc()

            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.count(needle), 1)

    def test_count_with_index_overflow_raises_overflow_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise OverflowError("not a byte!")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(OverflowError) as context:
            haystack.count(needle)
        self.assertEqual(str(context.exception), "not a byte!")

    def test_count_with_dunder_index_error_raises_type_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise MemoryError("not a byte!")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(MemoryError) as context:
            haystack.count(needle)
        self.assertEqual(str(context.exception), "not a byte!")

    def test_count_with_empty_sub_returns_length_minus_adjusted_start_plus_one(self):
        haystack = bytearray(b"abcde")
        needle = b""
        self.assertEqual(haystack.count(needle, -3), 4)

    def test_count_with_missing_returns_zero(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        self.assertEqual(haystack.count(needle), 0)

    def test_count_with_missing_stays_within_bounds(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        self.assertEqual(haystack.count(needle, None, 2), 0)

    def test_count_with_large_start_returns_zero(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"")
        self.assertEqual(haystack.count(needle, 10), 0)

    def test_count_with_negative_bounds_returns_count(self):
        haystack = bytearray(b"ababa")
        needle = b"a"
        self.assertEqual(haystack.count(needle, -4, -1), 1)

    def test_count_returns_non_overlapping_count(self):
        haystack = bytearray(b"0000000000")
        needle = bytearray(b"000")
        self.assertEqual(haystack.count(needle), 3)

    def test_endswith_with_bytes_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'endswith' .* 'bytearray' object.* a 'bytes'",
            bytearray.endswith,
            b"",
            bytearray(),
        )

    def test_endswith_with_list_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().endswith([])
        self.assertEqual(
            str(context.exception),
            "endswith first arg must be bytes or a tuple of bytes, not list",
        )

    def test_endswith_with_tuple_other_checks_each(self):
        haystack = bytearray(b"123")
        needle1 = (b"12", b"13", b"23", b"d")
        needle2 = (b"2", b"asd", b"122222")
        self.assertTrue(haystack.endswith(needle1))
        self.assertFalse(haystack.endswith(needle2))

    def test_endswith_with_end_searches_from_end(self):
        haystack = bytearray(b"12345")
        needle1 = bytearray(b"1")
        needle4 = b"34"
        self.assertFalse(haystack.endswith(needle1, 0))
        self.assertFalse(haystack.endswith(needle4, 1))
        self.assertTrue(haystack.endswith(needle1, 0, 1))
        self.assertTrue(haystack.endswith(needle4, 1, 4))

    def test_endswith_with_empty_returns_true_for_valid_bounds(self):
        haystack = bytearray(b"12345")
        self.assertTrue(haystack.endswith(bytearray()))
        self.assertTrue(haystack.endswith(b"", 5))
        self.assertTrue(haystack.endswith(bytearray(), -9, 1))

    def test_endswith_with_empty_returns_false_for_invalid_bounds(self):
        haystack = bytearray(b"12345")
        self.assertFalse(haystack.endswith(b"", 3, 2))
        self.assertFalse(haystack.endswith(bytearray(), 6))

    def test_extend_with_self_copies_data(self):
        array = bytearray(b"hello")
        self.assertIs(array.extend(array), None)
        self.assertEqual(array, b"hellohello")

    def test_extend_empty_with_bytes(self):
        array = bytearray(b"")
        self.assertIs(array.extend(b"hello"), None)
        self.assertEqual(array, b"hello")

    def test_extend_with_tuple_appends_to_end(self):
        array = bytearray(b"foo")
        self.assertIs(array.extend((42, 42, 42)), None)
        self.assertEqual(array, b"foo***")

    def test_extend_with_empty_string_is_noop(self):
        array = bytearray(b"foo")
        self.assertIs(array.extend(""), None)
        self.assertEqual(array, b"foo")

    def test_extend_with_nonempty_string_raises_type_error(self):
        array = bytearray(b"foo")
        self.assertRaises(TypeError, array.extend, "bar")

    def test_extend_with_non_int_raises_value_error(self):
        array = bytearray(b"foo")
        self.assertRaises(ValueError, array.extend, [256])
        self.assertRaises(ValueError, array.extend, (-1,))

    def test_extend_with_iterable_appends_to_end(self):
        duck = b"duck "
        goose = b"goose"
        array = bytearray()
        array.extend(map(int, duck * 2))
        array.extend(map(int, goose))
        self.assertEqual(array, b"duck duck goose")

    def test_find_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.find(b"", bytearray())

    def test_find_with_empty_sub_returns_start(self):
        haystack = bytearray(b"abc")
        needle = b""
        self.assertEqual(haystack.find(needle, 1), 1)

    def test_find_with_missing_returns_negative(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        self.assertEqual(haystack.find(needle), -1)

    def test_find_with_missing_stays_within_bounds(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        self.assertEqual(haystack.find(needle, None, 2), -1)

    def test_find_with_large_start_returns_negative(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"")
        self.assertEqual(haystack.find(needle, 10), -1)

    def test_find_with_negative_bounds_returns_index(self):
        haystack = bytearray(b"ababa")
        needle = b"a"
        self.assertEqual(haystack.find(needle, -4, -1), 2)

    def test_find_with_multiple_matches_returns_first_index_in_range(self):
        haystack = bytearray(b"abbabbabba")
        needle = bytearray(b"abb")
        self.assertEqual(haystack.find(needle, 1), 3)

    def test_find_with_nonbyte_int_raises_value_error(self):
        haystack = bytearray()
        needle = 266
        with self.assertRaises(ValueError):
            haystack.find(needle)

    def test_find_with_string_raises_type_error(self):
        haystack = bytearray()
        needle = "133"
        with self.assertRaises(TypeError):
            haystack.find(needle)

    def test_find_with_non_number_index(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    def test_find_with_dunder_int_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            def __index__(self):
                return ord("a")

            __int__ = Desc()

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    def test_find_with_dunder_float_calls_dunder_index(self):
        class Desc:
            def __get__(self, obj, type):
                raise NotImplementedError("called descriptor")

        class Idx:
            __float__ = Desc()

            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    def test_find_with_index_overflow_raises_overflow_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise OverflowError("not a byte!")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(OverflowError) as context:
            haystack.find(needle)
        self.assertEqual(str(context.exception), "not a byte!")

    def test_find_with_dunder_index_error_raises_type_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise MemoryError("not a byte!")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(MemoryError) as context:
            haystack.find(needle)
        self.assertEqual(str(context.exception), "not a byte!")

    def test_hex_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'hex' .* 'bytearray' object.* a 'str'",
            bytearray.hex,
            "not a bytearray",
        )

    def test_index_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.index(b"", bytearray())

    def test_index_with_subsequence_returns_first_in_range(self):
        haystack = bytearray(b"-a---a-aa")
        needle = ord("a")
        self.assertEqual(haystack.index(needle, 3), 5)

    def test_index_with_missing_raises_value_error(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        with self.assertRaises(ValueError) as context:
            haystack.index(needle)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_index_outside_of_bounds_raises_value_error(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        with self.assertRaises(ValueError) as context:
            haystack.index(needle, 0, 2)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_join_with_non_bytearray_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'join' .* 'bytearray' object.* a 'bytes'",
            bytearray.join,
            b"",
            bytearray(),
        )

    def test_ljust_without_growth_returns_copy(self):
        foo = bytearray(b"foo")
        self.assertEqual(foo.ljust(-1), foo)
        self.assertEqual(foo.ljust(0), foo)
        self.assertEqual(foo.ljust(1), foo)
        self.assertEqual(foo.ljust(2), foo)
        self.assertEqual(foo.ljust(3), foo)

        self.assertIsNot(foo.ljust(-1), foo)
        self.assertIsNot(foo.ljust(0), foo)
        self.assertIsNot(foo.ljust(1), foo)
        self.assertIsNot(foo.ljust(2), foo)
        self.assertIsNot(foo.ljust(3), foo)

    def test_ljust_with_custom_fillchar_returns_bytearray(self):
        orig = bytearray(b"ba")
        filled = bytearray(b"ba").ljust(7, b"@")
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"ba@@@@@")
        self.assertEqual(orig, b"ba")

    def test_ljust_pads_end_of_array(self):
        self.assertEqual(bytearray(b"abc").ljust(4), b"abc ")
        self.assertEqual(bytearray(b"abc").ljust(7), b"abc    ")

    def test_ljust_with_bytearray_fillchar_returns_bytearray(self):
        orig = bytearray(b"ba")
        fillchar = bytearray(b"@")
        filled = bytearray(b"ba").ljust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"ba@@@@@")
        self.assertEqual(orig, b"ba")

    def test_ljust_with_bytes_subclass_fillchar_returns_bytearray(self):
        class C(bytes):
            # access to character is done in native, so this is not called
            def __getitem__(self, key):
                return 0

        orig = bytearray(b"ba")
        fillchar = C(b"@")
        filled = bytearray(b"ba").ljust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"ba@@@@@")
        self.assertEqual(orig, b"ba")

    def test_ljust_with_bytearray_subclass_fillchar_returns_bytearray(self):
        class C(bytearray):
            # access to character is done in native, so this is not called
            def __getitem__(self, key):
                return 0

        orig = bytearray(b"ba")
        fillchar = C(b"@")
        filled = bytearray(b"ba").ljust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"ba@@@@@")
        self.assertEqual(orig, b"ba")

    def test_ljust_with_dunder_index_returns_bytearray(self):
        class C:
            def __index__(self):
                return 5

        orig = bytearray(b"ba")
        filled = bytearray(b"ba").ljust(C(), b"@")
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"ba@@@")
        self.assertEqual(orig, b"ba")

    def test_ljust_with_wrong_type_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().ljust(2, ord(" "))
        self.assertEqual(
            str(context.exception),
            "ljust() argument 2 must be a byte string of length 1, not int",
        )

    def test_ljust_with_wrong_length_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().ljust(2, b",,")
        self.assertEqual(
            str(context.exception),
            "ljust() argument 2 must be a byte string of length 1, not bytes",
        )

    def test_lower_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.lower("not a bytearray")

    def test_lower_empty_self_returns_new_bytearray(self):
        src = bytearray(b"")
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, src)

    def test_lower_all_lower_returns_new_bytearray(self):
        src = bytearray(b"abcdefghijklmnopqrstuvwxyz1234567890")
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, src)

    def test_lower_all_lower_and_non_alphanumeric_returns_new_bytearray(self):
        src = bytearray(b"abcdefghijklmnopqrstuvwxyz1234567890")
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, src)

    def test_lower_all_uppercase_returns_all_lowercase(self):
        src = bytearray(b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")
        dst = src.lower()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, bytearray(b"abcdefghijklmnopqrstuvwxyz"))

    def test_lower_mixed_case_returns_all_lowercase(self):
        src = bytearray(b"aBcDeFgHiJkLmNoPqRsTuVwXyZ")
        dst = src.lower()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, bytearray(b"abcdefghijklmnopqrstuvwxyz"))

    def test_lower_mixed_case_returns_all_lowercase(self):
        src = bytearray(b"a1!B2@c3#D4$e5%F6^g7&H8*i9(J0)")
        dst = src.lower()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, bytearray(b"a1!b2@c3#d4$e5%f6^g7&h8*i9(j0)"))

    def test_lstrip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().lstrip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_lstrip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(bytearray().lstrip(None), b"")
        self.assertEqual(bytearray(b"    ").lstrip(None), b"")
        self.assertEqual(bytearray(b"  hi  ").lstrip(), b"hi  ")

    def test_lstrip_with_byteslike_strips_bytes(self):
        self.assertEqual(bytearray().lstrip(b"123"), b"")
        self.assertEqual(bytearray(b"1aaa1").lstrip(bytearray()), b"1aaa1")
        self.assertEqual(bytearray(b"1 aaa1").lstrip(bytearray(b" 1")), b"aaa1")
        self.assertEqual(bytearray(b"hello").lstrip(b"ho"), b"ello")

    def test_lstrip_noop_returns_new_bytearray(self):
        arr = bytearray(b"abcd")
        result = arr.lstrip(b"e")
        self.assertIsInstance(result, bytearray)
        self.assertEqual(result, arr)
        self.assertIsNot(result, arr)

    def test_rfind_with_bytes_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'rfind' .* 'bytearray' object.* a 'bytes'",
            bytearray.rfind,
            b"",
            bytearray(),
        )

    def test_rfind_with_empty_sub_returns_end(self):
        haystack = bytearray(b"abc")
        needle = b""
        self.assertEqual(haystack.rfind(needle, 0, 2), 2)

    def test_rfind_with_missing_returns_negative(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        self.assertEqual(haystack.rfind(needle), -1)

    def test_rfind_with_missing_stays_within_bounds(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        self.assertEqual(haystack.rfind(needle, None, 2), -1)

    def test_rfind_with_large_start_returns_negative(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"")
        self.assertEqual(haystack.rfind(needle, 10), -1)

    def test_rfind_with_negative_bounds_returns_index(self):
        haystack = bytearray(b"ababa")
        needle = b"a"
        self.assertEqual(haystack.rfind(needle, -4, -1), 2)

    def test_rfind_with_multiple_matches_returns_last_index_in_range(self):
        haystack = bytearray(b"abbabbabba")
        needle = bytearray(b"abb")
        self.assertEqual(haystack.rfind(needle, 0, 7), 3)

    def test_rfind_with_nonbyte_int_raises_value_error(self):
        haystack = bytearray()
        needle = 266
        with self.assertRaises(ValueError) as context:
            haystack.rfind(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    def test_rfind_with_string_raises_type_error(self):
        haystack = bytearray()
        needle = "133"
        with self.assertRaises(TypeError) as context:
            haystack.rfind(needle)
        self.assertEqual(
            str(context.exception),
            "argument should be integer or bytes-like object, not 'str'",
        )

    def test_rfind_with_non_number_index(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.rfind(needle), 0)

    def test_rfind_with_dunder_int_calls_dunder_index(self):
        class Idx:
            def __int__(self):
                raise NotImplementedError("called __int__")

            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.rfind(needle), 0)

    def test_rfind_with_dunder_float_calls_dunder_index(self):
        class Idx:
            def __float__(self):
                raise NotImplementedError("called __float__")

            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        self.assertEqual(haystack.rfind(needle), 0)

    def test_rindex_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.rindex(b"", bytearray())

    def test_rindex_with_subsequence_returns_last_in_range(self):
        haystack = bytearray(b"-a-a--a--")
        needle = ord("a")
        self.assertEqual(haystack.rindex(needle, 1, 6), 3)

    def test_rindex_with_missing_raises_value_error(self):
        haystack = bytearray(b"abc")
        needle = b"d"
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_rindex_outside_of_bounds_raises_value_error(self):
        haystack = bytearray(b"abc")
        needle = bytearray(b"c")
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle, 0, 2)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_rjust_without_growth_returns_copy(self):
        foo = bytearray(b"foo")
        self.assertEqual(foo.rjust(-1), foo)
        self.assertEqual(foo.rjust(0), foo)
        self.assertEqual(foo.rjust(1), foo)
        self.assertEqual(foo.rjust(2), foo)
        self.assertEqual(foo.rjust(3), foo)

        self.assertIsNot(foo.rjust(-1), foo)
        self.assertIsNot(foo.rjust(0), foo)
        self.assertIsNot(foo.rjust(1), foo)
        self.assertIsNot(foo.rjust(2), foo)
        self.assertIsNot(foo.rjust(3), foo)

    def test_rjust_with_custom_fillchar_returns_bytearray(self):
        orig = bytearray(b"ba")
        filled = bytearray(b"ba").rjust(7, b"@")
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"@@@@@ba")
        self.assertEqual(orig, b"ba")

    def test_rjust_pads_beginning_of_array(self):
        self.assertEqual(bytearray(b"abc").rjust(4), b" abc")
        self.assertEqual(bytearray(b"abc").rjust(7), b"    abc")

    def test_rjust_with_bytearray_fillchar_returns_bytearray(self):
        orig = bytearray(b"ba")
        fillchar = bytearray(b"@")
        filled = bytearray(b"ba").rjust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"@@@@@ba")
        self.assertEqual(orig, b"ba")

    def test_rjust_with_bytes_subclass_fillchar_returns_bytearray(self):
        class C(bytes):
            # access to character is done in native, so this is not called
            def __getitem__(self, key):
                return 0

        orig = bytearray(b"ba")
        fillchar = C(b"@")
        filled = bytearray(b"ba").rjust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"@@@@@ba")
        self.assertEqual(orig, b"ba")

    def test_rjust_with_bytearray_subclass_fillchar_returns_bytearray(self):
        class C(bytearray):
            # access to character is done in native, so this is not called
            def __getitem__(self, key):
                return 0

        orig = bytearray(b"ba")
        fillchar = C(b"@")
        filled = bytearray(b"ba").rjust(7, fillchar)
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"@@@@@ba")
        self.assertEqual(orig, b"ba")

    def test_rjust_with_dunder_index_returns_bytearray(self):
        class C:
            def __index__(self):
                return 5

        orig = bytearray(b"ba")
        filled = bytearray(b"ba").rjust(C(), b"@")
        self.assertIsInstance(filled, bytearray)
        self.assertEqual(filled, b"@@@ba")
        self.assertEqual(orig, b"ba")

    def test_rjust_with_wrong_type_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().rjust(2, ord(" "))
        self.assertEqual(
            str(context.exception),
            "rjust() argument 2 must be a byte string of length 1, not int",
        )

    def test_rjust_with_wrong_length_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().rjust(2, b",,")
        self.assertEqual(
            str(context.exception),
            "rjust() argument 2 must be a byte string of length 1, not bytes",
        )

    def test_rstrip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().rstrip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_rstrip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(bytearray().rstrip(None), b"")
        self.assertEqual(bytearray(b"    ").rstrip(None), b"")
        self.assertEqual(bytearray(b"  hi  ").rstrip(), b"  hi")

    def test_rstrip_with_byteslike_strips_bytes(self):
        self.assertEqual(bytearray().rstrip(b"123"), b"")
        self.assertEqual(bytearray(b"1aaa1").rstrip(bytearray()), b"1aaa1")
        self.assertEqual(bytearray(b"1 aaa1").rstrip(bytearray(b" 1")), b"1 aaa")
        self.assertEqual(bytearray(b"hello").rstrip(b"lo"), b"he")

    def test_rstrip_noop_returns_new_bytearray(self):
        arr = bytearray(b"abcd")
        result = arr.rstrip(b"e")
        self.assertIsInstance(result, bytearray)
        self.assertEqual(result, arr)
        self.assertIsNot(result, arr)

    def test_split_with_non_bytearray_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'split' .* 'bytearray' object.* a 'bytes'",
            bytearray.split,
            b"foo bar",
            bytearray(),
        )

    def test_split_with_non_byteslike_sep_raises_type_error(self):
        b = bytearray(b"foo bar")
        sep = ""
        with self.assertRaises(TypeError) as context:
            b.split(sep)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_split_with_non_index_maxsplit_raises_type_error(self):
        b = bytearray(b"foo bar")
        with self.assertRaises(TypeError) as context:
            b.split(maxsplit=None)
        self.assertEqual(
            str(context.exception),
            "'NoneType' object cannot be interpreted as an integer",
        )

    def test_split_with_large_int_raises_overflow_error(self):
        b = bytearray(b"foo bar")
        with self.assertRaises(OverflowError) as context:
            b.split(maxsplit=2 ** 64)
        self.assertEqual(
            str(context.exception), "Python int too large to convert to C ssize_t"
        )

    def test_split_with_empty_sep_raises_value_error(self):
        b = bytearray(b"foo bar")
        with self.assertRaises(ValueError) as context:
            b.split(b"")
        self.assertEqual(str(context.exception), "empty separator")

    def test_split_empty_bytes_without_sep_returns_empty_list(self):
        b = bytearray(b"")
        self.assertEqual(b.split(), [])

    def test_split_empty_bytes_with_sep_returns_list_of_empty_bytes(self):
        b = bytearray(b"")
        self.assertEqual(b.split(b"a"), [b""])

    def test_split_without_sep_splits_whitespace(self):
        result = bytearray(b" foo bar  \t \nbaz\r\n   ").split()
        self.assertIsInstance(result[0], bytearray)
        self.assertEqual(result, [b"foo", b"bar", b"baz"])

    def test_split_with_none_sep_splits_whitespace_maxsplit_times(self):
        result = bytearray(b" foo bar  \t \nbaz\r\n   ").split(None, 2)
        self.assertIsInstance(result[0], bytearray)
        self.assertEqual(result, [b"foo", b"bar", b"baz\r\n   "])

    def test_split_by_byteslike_returns_list(self):
        b = bytearray(b"foo bar baz")
        self.assertEqual(b.split(b" "), [b"foo", b"bar", b"baz"])
        self.assertEqual(b.split(bytearray(b"o")), [b"f", b"", b" bar baz"])
        self.assertEqual(b.split(b"ba"), [b"foo ", b"r ", b"z"])
        self.assertEqual(b.split(b"not found"), [b])

    def test_startswith_with_bytes_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'startswith' .* 'bytearray' object.* a 'bytes'",
            bytearray.startswith,
            b"",
            bytearray(),
        )

    def test_startswith_with_list_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().startswith([])
        self.assertEqual(
            str(context.exception),
            "startswith first arg must be bytes or a tuple of bytes, not list",
        )

    def test_startswith_with_tuple_other_checks_each(self):
        haystack = bytearray(b"123")
        needle1 = (b"12", b"13", b"23", b"d")
        needle2 = (b"2", b"asd", b"122222")
        self.assertTrue(haystack.startswith(needle1))
        self.assertFalse(haystack.startswith(needle2))

    def test_startswith_with_start_searches_from_start(self):
        haystack = bytearray(b"12345")
        needle1 = bytearray(b"1")
        needle4 = b"34"
        self.assertFalse(haystack.startswith(needle1, 1))
        self.assertFalse(haystack.startswith(needle4, 0))
        self.assertTrue(haystack.startswith(needle1, 0, 1))
        self.assertTrue(haystack.startswith(needle4, 2))

    def test_startswith_with_empty_returns_true_for_valid_bounds(self):
        haystack = bytearray(b"12345")
        self.assertTrue(haystack.startswith(bytearray()))
        self.assertTrue(haystack.startswith(b"", 5))
        self.assertTrue(haystack.startswith(bytearray(), -9, 1))

    def test_startswith_with_empty_returns_false_for_invalid_bounds(self):
        haystack = bytearray(b"12345")
        self.assertFalse(haystack.startswith(b"", 3, 2))
        self.assertFalse(haystack.startswith(bytearray(), 6))

    def test_strip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray().strip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_strip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(bytearray().strip(None), b"")
        self.assertEqual(bytearray(b"     ").strip(None), b"")
        self.assertEqual(bytearray(b"  h i  ").strip(), b"h i")

    def test_strip_with_byteslike_strips_bytes(self):
        self.assertEqual(bytearray().strip(b"123"), b"")
        self.assertEqual(bytearray(b"1aaa1").strip(bytearray()), b"1aaa1")
        self.assertEqual(bytearray(b"1 aaa1").strip(bytearray(b" 1")), b"aaa")
        self.assertEqual(bytearray(b"hello").strip(b"ho"), b"ell")

    def test_strip_noop_returns_new_bytearray(self):
        arr = bytearray(b"abcd")
        result = arr.strip(b"e")
        self.assertIsInstance(result, bytearray)
        self.assertEqual(result, arr)
        self.assertIsNot(result, arr)

    def test_translate_with_non_bytearray_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'translate' .* 'bytearray' object.* a 'str'",
            bytearray.translate,
            "not a bytearray",
            bytes(256),
        )

    def test_upper_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.upper("not a bytearray")

    def test_upper_empty_self_returns_new_bytearray(self):
        src = bytearray(b"")
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, src)

    def test_upper_all_upper_returns_new_bytearray(self):
        src = bytearray(b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")
        dst = src.upper()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(src, dst)

    def test_upper_all_upper_and_non_alphanumeric_returns_new_bytearray(self):
        src = bytearray(b"ABCDEFGHIJ1234567890!@#$%^&*()")
        dst = src.upper()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(src, dst)

    def test_upper_all_lower_returns_all_uppercase(self):
        src = bytearray(b"abcdefghijklmnopqrstuvwxyz")
        dst = src.upper()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")

    def test_upper_mixed_case_returns_all_uppercase(self):
        src = bytearray(b"aBcDeFgHiJkLmNoPqRsTuVwXyZ")
        dst = src.upper()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")

    def test_upper_mixed_case_returns_all_uppercase(self):
        src = bytearray(b"a1!B2@c3#D4$e5%F6^g7&H8*i9(J0)")
        dst = src.upper()
        self.assertIsInstance(dst, bytearray)
        self.assertEqual(dst, b"A1!B2@C3#D4$E5%F6^G7&H8*I9(J0)")

    def test_splitlines_with_non_bytearray_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.splitlines(None)

    def test_splitlines_with_float_keepends_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.splitlines(bytearray(b"hello"), 0.4)

    def test_splitlines_with_string_keepends_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytearray.splitlines(bytearray(b"hello"), "1")

    def test_splitlines_returns_list(self):
        self.assertEqual(bytearray.splitlines(bytearray(b""), False), [])
        self.assertEqual(bytearray.splitlines(bytearray(b"a"), 0), [bytearray(b"a")])
        exp = [bytearray(b"a"), bytearray(b"b"), bytearray(b"c")]
        self.assertEqual(bytearray.splitlines(bytearray(b"a\nb\rc")), exp)
        exp = [bytearray(b"a"), bytearray(b"b")]
        self.assertEqual(bytearray.splitlines(bytearray(b"a\r\nb\r\n")), exp)
        exp = [bytearray(b""), bytearray(b""), bytearray(b"")]
        self.assertEqual(bytearray.splitlines(bytearray(b"\n\r\n\r")), exp)
        self.assertEqual(
            bytearray.splitlines(bytearray(b"a\x0Bb")), [bytearray(b"a\x0Bb")]
        )

    def test_splitlines_with_keepend_returns_list(self):
        self.assertEqual(bytearray.splitlines(bytearray(b""), True), [])
        self.assertEqual(bytearray.splitlines(bytearray(b"a"), 1), [bytearray(b"a")])
        exp = [bytearray(b"a\n"), bytearray(b"b\r"), bytearray(b"c")]
        self.assertEqual(bytearray.splitlines(bytearray(b"a\nb\rc"), 1), exp)
        exp = [bytearray(b"a\r\n"), bytearray(b"b\r\n")]
        self.assertEqual(bytearray.splitlines(bytearray(b"a\r\nb\r\n"), 1), exp)
        exp = [bytearray(b"\n"), bytearray(b"\r\n"), bytearray(b"\r")]
        self.assertEqual(bytearray.splitlines(bytearray(b"\n\r\n\r"), 1), exp)

    def test_splitlines_returns_bytearray_list(self):
        ret = bytearray.splitlines(bytearray(b"one\ntwo\nthree"))
        self.assertIs(type(ret), list)
        self.assertIs(type(ret[0]), bytearray)
        self.assertIs(type(ret[1]), bytearray)
        self.assertIs(type(ret[2]), bytearray)


if __name__ == "__main__":
    unittest.main()
