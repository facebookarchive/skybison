#!/usr/bin/env python3
import unittest
from unittest.mock import Mock


class BytesTests(unittest.TestCase):
    def test_center_without_growth_returns_original_bytes(self):
        foo = b"foo"
        self.assertIs(foo.center(-1), foo)
        self.assertIs(foo.center(0), foo)
        self.assertIs(foo.center(1), foo)
        self.assertIs(foo.center(2), foo)
        self.assertIs(foo.center(3), foo)

    def test_center_with_both_odd_centers_bytes(self):
        self.assertEqual(b"abc".center(5), b" abc ")
        self.assertEqual(b"abc".center(7), b"  abc  ")

    def test_center_with_both_even_centers_bytes(self):
        self.assertEqual(b"".center(4), b"    ")
        self.assertEqual(b"abcd".center(8), b"  abcd  ")

    def test_center_with_odd_length_and_even_number_centers_bytes(self):
        self.assertEqual(b"foo".center(4), b"foo ")
        self.assertEqual(b"\t \n".center(6), b" \t \n  ")

    def test_center_with_even_length_and_odd_number_centers_bytes(self):
        self.assertEqual(b"food".center(5), b" food")
        self.assertEqual(b"\t  \n".center(7), b"  \t  \n ")

    def test_center_with_custom_fillchar_returns_bytes(self):
        self.assertEqual(b"ba".center(7, b"@"), b"@@@ba@@")

    def test_center_with_non_bytes_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".center(2, ord(" "))
        self.assertEqual(
            str(context.exception),
            "center() argument 2 must be a byte string of length 1, not int",
        )

    def test_center_with_wrong_length_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".center(2, b",,")
        self.assertEqual(
            str(context.exception),
            "center() argument 2 must be a byte string of length 1, not bytes",
        )

    def test_decode_finds_ascii(self):
        self.assertEqual(b"abc".decode("ascii"), "abc")

    def test_decode_finds_latin_1(self):
        self.assertEqual(b"abc\xE5".decode("latin-1"), "abc\xE5")

    def test_dunder_add_with_bytes_like_other_returns_bytes(self):
        self.assertEqual(b"123".__add__(bytearray(b"456")), b"123456")

    def test_dunder_add_with_non_bytes_like_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".__add__(2)
        self.assertEqual(str(context.exception), "can't concat int to bytes")

    def test_dunder_contains_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.__contains__("not_bytes", 123)

    def test_dunder_contains_with_element_in_bytes_returns_true(self):
        self.assertTrue(b"abc".__contains__(ord("a")))

    def test_dunder_contains_with_element_not_in_bytes_returns_false(self):
        self.assertFalse(b"abc".__contains__(ord("z")))

    def test_dunder_contains_calls_dunder_index(self):
        class C:
            __index__ = Mock(name="__index__", return_value=ord("a"))

        c = C()
        self.assertTrue(b"abc".__contains__(c))
        c.__index__.assert_called_once()

    def test_dunder_contains_calls_dunder_index_before_checking_byteslike(self):
        class C(bytearray):
            __index__ = Mock(name="__index__", return_value=ord("a"))

        c = C(b"q")
        self.assertTrue(b"abc".__contains__(c))
        c.__index__.assert_called_once()

    def test_dunder_contains_ignores_errors_from_dunder_index(self):
        class C:
            __index__ = Mock(name="__index__", side_effect=MemoryError("foo"))

        c = C()
        container = b"abc"
        with self.assertRaises(TypeError):
            container.__contains__(c)
        c.__index__.assert_called_once()

    def test_dunder_contains_with_single_byte_byteslike_returns_true(self):
        self.assertTrue(b"abc".__contains__(b"a"))
        self.assertTrue(b"abc".__contains__(bytearray(b"a")))

    def test_dunder_contains_with_single_byte_byteslike_returns_false(self):
        self.assertFalse(b"abc".__contains__(b"z"))
        self.assertFalse(b"abc".__contains__(bytearray(b"z")))

    def test_dunder_contains_with_byteslike_returns_true(self):
        self.assertTrue(b"foobar".__contains__(b"foo"))
        self.assertTrue(b"foobar".__contains__(bytearray(b"bar")))

    def test_dunder_contains_with_byteslike_returns_false(self):
        self.assertFalse(b"foobar".__contains__(b"baz"))
        self.assertFalse(b"foobar".__contains__(bytearray(b"baz")))

    def test_dunder_iter_returns_iterator(self):
        b = b"123"
        it = b.__iter__()
        self.assertTrue(hasattr(it, "__next__"))
        self.assertIs(iter(it), it)

    def test_dunder_new_with_str_without_encoding_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes("foo")

    def test_dunder_new_with_str_and_encoding_returns_bytes(self):
        self.assertEqual(bytes("foo", "ascii"), b"foo")

    def test_dunder_new_with_ignore_errors_returns_bytes(self):
        self.assertEqual(bytes("fo\x80o", "ascii", "ignore"), b"foo")

    def test_dunder_new_with_bytes_subclass_returns_bytes_subclass(self):
        class A(bytes):
            pass

        class B(bytes):
            pass

        class C:
            def __bytes__(self):
                return B()

        self.assertIsInstance(bytes.__new__(A, b""), A)
        self.assertIsInstance(bytes.__new__(B, 2), B)
        self.assertIsInstance(bytes.__new__(A, C()), A)

    def test_dunder_str_returns_same_as_dunder_repr(self):
        b = b"foobar\x80"
        b_str = b.__repr__()
        self.assertEqual(b_str, "b'foobar\\x80'")
        self.assertEqual(b_str, b.__str__())

    def test_count_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.count(bytearray(), b"")
        self.assertIn(
            "'count' requires a 'bytes' object but received a 'bytearray'",
            str(context.exception),
        )

    def test_count_with_nonbyte_int_raises_value_error(self):
        haystack = b""
        needle = 266
        with self.assertRaises(ValueError) as context:
            haystack.count(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    def test_count_with_string_raises_type_error(self):
        haystack = b""
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

        haystack = b"abc"
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

        haystack = b"abc"
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

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.count(needle), 1)

    def test_count_with_index_overflow_raises_overflow_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise OverflowError("not a byte!")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(OverflowError) as context:
            haystack.count(needle)
        self.assertEqual(str(context.exception), "not a byte!")

    def test_count_with_index_error_raises_type_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise TypeError("not a byte!")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.count(needle)
        self.assertEqual(str(context.exception), "not a byte!")

    def test_count_with_empty_sub_returns_length_minus_adjusted_start_plus_one(self):
        haystack = b"abcde"
        needle = bytearray()
        self.assertEqual(haystack.count(needle, -3), 4)

    def test_count_with_missing_returns_zero(self):
        haystack = b"abc"
        needle = b"d"
        self.assertEqual(haystack.count(needle), 0)

    def test_count_with_missing_stays_within_bounds(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.count(needle, None, 2), 0)

    def test_count_with_large_start_returns_zero(self):
        haystack = b"abc"
        needle = bytearray(b"")
        self.assertEqual(haystack.count(needle, 10), 0)

    def test_count_with_negative_bounds_returns_count(self):
        haystack = b"foobar"
        needle = bytearray(b"o")
        self.assertEqual(haystack.count(needle, -6, -1), 2)

    def test_count_returns_non_overlapping_count(self):
        haystack = b"abababab"
        needle = bytearray(b"aba")
        self.assertEqual(haystack.count(needle), 2)

    def test_endswith_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.endswith(bytearray(), b"")
        self.assertIn(
            "'endswith' requires a 'bytes' object but received a 'bytearray'",
            str(context.exception),
        )

    def test_endswith_with_list_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".endswith([])
        self.assertEqual(
            str(context.exception),
            "endswith first arg must be bytes or a tuple of bytes, not list",
        )

    def test_endswith_with_tuple_other_checks_each(self):
        haystack = b"123"
        needle1 = (b"12", b"13", b"23", b"d")
        needle2 = (b"2", b"asd", b"122222")
        self.assertTrue(haystack.endswith(needle1))
        self.assertFalse(haystack.endswith(needle2))

    def test_endswith_with_end_searches_from_end(self):
        haystack = b"12345"
        needle1 = bytearray(b"1")
        needle4 = b"34"
        self.assertFalse(haystack.endswith(needle1, 0))
        self.assertFalse(haystack.endswith(needle4, 1))
        self.assertTrue(haystack.endswith(needle1, 0, 1))
        self.assertTrue(haystack.endswith(needle4, 1, 4))

    def test_endswith_with_empty_returns_true_for_valid_bounds(self):
        haystack = b"12345"
        self.assertTrue(haystack.endswith(bytearray()))
        self.assertTrue(haystack.endswith(b"", 5))
        self.assertTrue(haystack.endswith(bytearray(), -9, 1))

    def test_endswith_with_empty_returns_false_for_invalid_bounds(self):
        haystack = b"12345"
        self.assertFalse(haystack.endswith(b"", 3, 2))
        self.assertFalse(haystack.endswith(bytearray(), 6))

    def test_find_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.find(bytearray(), b"")

    def test_find_with_empty_sub_returns_start(self):
        haystack = b"abc"
        needle = bytearray()
        self.assertEqual(haystack.find(needle, 1), 1)

    def test_find_with_missing_returns_negative(self):
        haystack = b"abc"
        needle = b"d"
        self.assertEqual(haystack.find(needle), -1)

    def test_find_with_missing_stays_within_bounds(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.find(needle, None, 2), -1)

    def test_find_with_large_start_returns_negative(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.find(needle, 10), -1)

    def test_find_with_negative_bounds_returns_index(self):
        haystack = b"foobar"
        needle = bytearray(b"o")
        self.assertEqual(haystack.find(needle, -6, -1), 1)

    def test_find_with_multiple_matches_returns_first_index_in_range(self):
        haystack = b"abbabbabba"
        needle = bytearray(b"abb")
        self.assertEqual(haystack.find(needle, 1), 3)

    def test_find_with_nonbyte_int_raises_value_error(self):
        haystack = b""
        needle = 266
        with self.assertRaises(ValueError):
            haystack.find(needle)

    def test_find_with_int_returns_index(self):
        haystack = b"123"
        self.assertEqual(haystack.find(ord("1")), 0)
        self.assertEqual(haystack.find(ord("2")), 1)
        self.assertEqual(haystack.find(ord("3")), 2)

    def test_find_with_string_raises_type_error(self):
        haystack = b""
        needle = "133"
        with self.assertRaises(TypeError):
            haystack.find(needle)

    def test_find_with_non_number_index(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = b"abc"
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

        haystack = b"abc"
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

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.find(needle), 0)

    def test_find_with_index_overflow_raises_overflow_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise OverflowError("not a byte!")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(OverflowError) as context:
            haystack.find(needle)
        self.assertEqual(str(context.exception), "not a byte!")

    def test_find_with_index_error_raises_type_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise TypeError("not a byte!")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.find(needle)
        self.assertEqual(str(context.exception), "not a byte!")

    def test_fromhex_returns_bytes_instance(self):
        self.assertEqual(bytes.fromhex("1234 ab AB"), b"\x124\xab\xab")

    def test_fromhex_ignores_spaces(self):
        self.assertEqual(bytes.fromhex("ab cc deff"), b"\xab\xcc\xde\xff")

    def test_fromhex_with_trailing_spaces_returns_bytes(self):
        self.assertEqual(bytes.fromhex("ABCD  "), b"\xab\xcd")

    def test_fromhex_with_number_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.fromhex(1234)

    def test_fromhex_with_bad_byte_groupings_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            bytes.fromhex("abc d")
        self.assertEqual(
            str(context.exception),
            "non-hexadecimal number found in fromhex() arg at position 3",
        )

    def test_fromhex_with_dangling_nibble_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            bytes.fromhex("AB AB C")
        self.assertEqual(
            str(context.exception),
            "non-hexadecimal number found in fromhex() arg at position 7",
        )

    def test_fromhex_with_non_ascii_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            bytes.fromhex("édcb")
        self.assertEqual(
            str(context.exception),
            "non-hexadecimal number found in fromhex() arg at position 0",
        )

    def test_fromhex_with_non_hex_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            bytes.fromhex("0123abcdefgh")
        self.assertEqual(
            str(context.exception),
            "non-hexadecimal number found in fromhex() arg at position 10",
        )

    def test_fromhex_with_bytes_subclass_returns_subclass_instance(self):
        class C(bytes):
            __init__ = Mock(name="__init__", return_value=None)

        c = C.fromhex("1111")
        self.assertIs(c.__class__, C)
        c.__init__.assert_called_once()
        self.assertEqual(c, b"\x11\x11")

    def test_index_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.index(bytearray(), b"")

    def test_index_with_subsequence_returns_first_in_range(self):
        haystack = b"-a---a-aa"
        needle = ord("a")
        self.assertEqual(haystack.index(needle, 3), 5)

    def test_index_with_missing_raises_value_error(self):
        haystack = b"abc"
        needle = b"d"
        with self.assertRaises(ValueError) as context:
            haystack.index(needle)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_index_outside_of_bounds_raises_value_error(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        with self.assertRaises(ValueError) as context:
            haystack.index(needle, 0, 2)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_iteration_returns_ints(self):
        expected = [97, 98, 99, 100]
        index = 0
        for val in b"abcd":
            self.assertEqual(val, expected[index])
            index += 1
        self.assertEqual(index, len(expected))

    def test_join_with_non_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.join(1, [])
        self.assertIn(
            "'join' requires a 'bytes' object but received a 'int'",
            str(context.exception),
        )

    def test_lower_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.lower("not a bytes")

    def test_lower_empty_self_returns_self(self):
        src = b""
        dst = src.lower()
        self.assertIs(src, dst)

    def test_lower_all_lower_returns_new_bytes(self):
        src = b"abcdefghijklmnopqrstuvwxyz"
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytes)
        self.assertEqual(src, dst)

    def test_lower_all_lower_and_non_alphanumeric_returns_self(self):
        src = b"abcdefghijklmnopqrstuvwxyz1234567890"
        dst = src.lower()
        self.assertIsNot(src, dst)
        self.assertIsInstance(dst, bytes)
        self.assertEqual(src, dst)

    def test_lower_all_uppercase_returns_all_lowercase(self):
        src = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        dst = src.lower()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"abcdefghijklmnopqrstuvwxyz")

    def test_lower_mixed_case_returns_all_lowercase(self):
        src = b"aBcDeFgHiJkLmNoPqRsTuVwXyZ"
        dst = src.lower()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"abcdefghijklmnopqrstuvwxyz")

    def test_lower_mixed_case_returns_all_lowercase(self):
        src = b"a1!B2@c3#D4$e5%F6^g7&H8*i9(J0)"
        dst = src.lower()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"a1!b2@c3#d4$e5%f6^g7&h8*i9(j0)")

    def test_ljust_without_growth_returns_original_bytes(self):
        foo = b"foo"
        self.assertIs(foo.ljust(-1), foo)
        self.assertIs(foo.ljust(0), foo)
        self.assertIs(foo.ljust(1), foo)
        self.assertIs(foo.ljust(2), foo)
        self.assertIs(foo.ljust(3), foo)

    def test_ljust_pads_end_of_bytes(self):
        self.assertEqual(b"abc".ljust(4), b"abc ")
        self.assertEqual(b"abc".ljust(7), b"abc    ")

    def test_ljust_with_custom_fillchar_returns_bytes(self):
        self.assertEqual(b"ba".ljust(7, b"@"), b"ba@@@@@")

    def test_ljust_with_non_bytes_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".ljust(2, ord(" "))
        self.assertEqual(
            str(context.exception),
            "ljust() argument 2 must be a byte string of length 1, not int",
        )

    def test_ljust_with_wrong_length_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".ljust(2, b",,")
        self.assertEqual(
            str(context.exception),
            "ljust() argument 2 must be a byte string of length 1, not bytes",
        )

    def test_ljust_with_swapped_width_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".ljust(b",", 2)
        self.assertEqual(
            str(context.exception), "'bytes' object cannot be interpreted as an integer"
        )

    def test_lstrip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".lstrip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_lstrip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(b"".lstrip(None), b"")
        self.assertEqual(b"      ".lstrip(None), b"")
        self.assertEqual(b"  hi  ".lstrip(), b"hi  ")

    def test_lstrip_with_byteslike_strips_bytes(self):
        self.assertEqual(b"".lstrip(b"123"), b"")
        self.assertEqual(b"1aaa1".lstrip(bytearray()), b"1aaa1")
        self.assertEqual(b"1 aaa1".lstrip(bytearray(b" 1")), b"aaa1")
        self.assertEqual(b"hello".lstrip(b"eho"), b"llo")

    def test_replace(self):
        test = b"mississippi"
        self.assertEqual(test.replace(b"i", b"a"), b"massassappa")
        self.assertEqual(test.replace(b"i", b"vv"), b"mvvssvvssvvppvv")
        self.assertEqual(test.replace(b"ss", b"x"), b"mixixippi")
        self.assertEqual(test.replace(bytearray(b"ss"), bytearray(b"x")), b"mixixippi")
        self.assertEqual(test.replace(b"i", bytearray()), b"msssspp")
        self.assertEqual(test.replace(bytearray(b"i"), b""), b"msssspp")

    def test_replace_with_count(self):
        test = b"mississippi"
        self.assertEqual(test.replace(b"i", b"a", 0), b"mississippi")
        self.assertEqual(test.replace(b"i", b"a", 2), b"massassippi")

    def test_replace_int_error(self):
        with self.assertRaises(TypeError) as context:
            b"a b".replace(b"r", 4)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'int'"
        )

    def test_replace_str_error(self):
        with self.assertRaises(TypeError) as context:
            b"a b".replace(b"r", "s")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_replace_float_count_error(self):
        with self.assertRaises(TypeError) as context:
            b"a b".replace(b"r", b"b", 4.2)
        self.assertEqual(str(context.exception), "integer argument expected, got float")

    def test_replace_str_count_error(self):
        test = b"a b"
        self.assertRaises(TypeError, test.replace, "r", b"", "hello")

    def test_rfind_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.rfind(bytearray(), b"")
        self.assertIn(
            "'rfind' requires a 'bytes' object but received a 'bytearray'",
            str(context.exception),
        )

    def test_rfind_with_empty_sub_returns_end(self):
        haystack = b"abc"
        needle = bytearray()
        self.assertEqual(haystack.rfind(needle), 3)

    def test_rfind_with_missing_returns_negative(self):
        haystack = b"abc"
        needle = b"d"
        self.assertEqual(haystack.rfind(needle), -1)

    def test_rfind_with_missing_stays_within_bounds(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.rfind(needle, None, 2), -1)

    def test_rfind_with_large_start_returns_negative(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        self.assertEqual(haystack.rfind(needle, 10), -1)

    def test_rfind_with_negative_bounds_returns_index(self):
        haystack = b"foobar"
        needle = bytearray(b"o")
        self.assertEqual(haystack.rfind(needle, -6, -1), 2)

    def test_rfind_with_multiple_matches_returns_last_index_in_range(self):
        haystack = b"abbabbabba"
        needle = bytearray(b"abb")
        self.assertEqual(haystack.rfind(needle), 6)

    def test_rfind_with_nonbyte_int_raises_value_error(self):
        haystack = b""
        needle = 266
        with self.assertRaises(ValueError) as context:
            haystack.rfind(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    def test_rfind_with_string_raises_type_error(self):
        haystack = b""
        needle = "133"
        with self.assertRaises(TypeError) as context:
            haystack.rfind(needle)
        self.assertEqual(
            str(context.exception),
            "argument should be integer or bytes-like object, not 'str'",
        )

    def test_rfind_with_non_number_index_calls_dunder_index(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.rfind(needle), 0)

    def test_rfind_with_dunder_int_calls_dunder_index(self):
        class Idx:
            def __int__(self):
                raise NotImplementedError("called __int__")

            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.rfind(needle), 0)

    def test_rfind_with_dunder_float_calls_dunder_index(self):
        class Idx:
            def __float__(self):
                raise NotImplementedError("called __float__")

            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        self.assertEqual(haystack.rfind(needle), 0)

    def test_rindex_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.rindex(bytearray(), b"")

    def test_rindex_with_subsequence_returns_last_in_range(self):
        haystack = b"-a-aa----a--"
        needle = ord("a")
        self.assertEqual(haystack.rindex(needle, 2, 8), 4)

    def test_rindex_with_missing_raises_value_error(self):
        haystack = b"abc"
        needle = b"d"
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_rindex_outside_of_bounds_raises_value_error(self):
        haystack = b"abc"
        needle = bytearray(b"c")
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle, 0, 2)
        self.assertEqual(str(context.exception), "subsection not found")

    def test_rjust_without_growth_returns_original_bytes(self):
        foo = b"foo"
        self.assertIs(foo.rjust(-1), foo)
        self.assertIs(foo.rjust(0), foo)
        self.assertIs(foo.rjust(1), foo)
        self.assertIs(foo.rjust(2), foo)
        self.assertIs(foo.rjust(3), foo)

    def test_rjust_pads_beginning_of_bytes(self):
        self.assertEqual(b"abc".rjust(4), b" abc")
        self.assertEqual(b"abc".rjust(7), b"    abc")

    def test_rjust_with_custom_fillchar_returns_bytes(self):
        self.assertEqual(b"ba".rjust(7, b"@"), b"@@@@@ba")

    def test_rjust_with_non_bytes_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".rjust(2, ord(" "))
        self.assertEqual(
            str(context.exception),
            "rjust() argument 2 must be a byte string of length 1, not int",
        )

    def test_rjust_with_wrong_length_fillchar_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".rjust(2, b",,")
        self.assertEqual(
            str(context.exception),
            "rjust() argument 2 must be a byte string of length 1, not bytes",
        )

    def test_rstrip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".rstrip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_rstrip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(b"".rstrip(None), b"")
        self.assertEqual(b"      ".rstrip(None), b"")
        self.assertEqual(b"  hi  ".rstrip(), b"  hi")

    def test_rstrip_with_byteslike_strips_bytes(self):
        self.assertEqual(b"".rstrip(b"123"), b"")
        self.assertEqual(b"1aaa1".rstrip(bytearray()), b"1aaa1")
        self.assertEqual(b"1aa a1".rstrip(bytearray(b" 1")), b"1aa a")
        self.assertEqual(b"hello".rstrip(b"lo"), b"he")

    def test_split_with_non_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.split("foo bar")
        self.assertIn(
            "'split' requires a 'bytes' object but received a 'str'",
            str(context.exception),
        )

    def test_split_with_non_byteslike_sep_raises_type_error(self):
        b = b"foo bar"
        sep = ""
        with self.assertRaises(TypeError) as context:
            b.split(sep)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_split_with_non_index_maxsplit_raises_type_error(self):
        b = b"foo bar"
        with self.assertRaises(TypeError) as context:
            b.split(maxsplit=None)
        self.assertEqual(
            str(context.exception),
            "'NoneType' object cannot be interpreted as an integer",
        )

    def test_split_with_large_int_raises_overflow_error(self):
        b = b"foo bar"
        with self.assertRaises(OverflowError) as context:
            b.split(maxsplit=2 ** 64)
        self.assertEqual(
            str(context.exception), "Python int too large to convert to C ssize_t"
        )

    def test_split_with_empty_sep_raises_value_error(self):
        b = b"foo bar"
        with self.assertRaises(ValueError) as context:
            b.split(bytearray())
        self.assertEqual(str(context.exception), "empty separator")

    def test_split_empty_bytes_without_sep_returns_empty_list(self):
        b = b""
        self.assertEqual(b.split(), [])

    def test_split_empty_bytes_with_sep_returns_list_of_empty_bytes(self):
        b = b""
        self.assertEqual(b.split(b"a"), [b""])

    def test_split_without_sep_splits_whitespace(self):
        b = b"foo bar"
        self.assertEqual(b.split(), [b"foo", b"bar"])
        b = b" foo bar  \t \nbaz\r\n   "
        self.assertEqual(b.split(), [b"foo", b"bar", b"baz"])

    def test_split_with_none_sep_splits_whitespace_maxsplit_times(self):
        b = b" foo bar  \t \nbaz\r\n   "
        self.assertEqual(b.split(None, 2), [b"foo", b"bar", b"baz\r\n   "])

    def test_split_by_byteslike_returns_list(self):
        b = b"foo bar baz"
        self.assertEqual(b.split(b" "), [b"foo", b"bar", b"baz"])
        self.assertEqual(b.split(bytearray(b"o")), [b"f", b"", b" bar baz"])
        self.assertEqual(b.split(b"ba"), [b"foo ", b"r ", b"z"])
        self.assertEqual(b.split(b"not found"), [b])

    def test_splitlines_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.splitlines(None)

    def test_splitlines_with_float_keepends_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.splitlines(b"hello", 0.4)

    def test_splitlines_with_string_keepends_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.splitlines(b"hello", "1")

    def test_splitlines_returns_list(self):
        self.assertEqual(bytes.splitlines(b"", False), [])
        self.assertEqual(bytes.splitlines(b"a", 0), [b"a"])
        self.assertEqual(bytes.splitlines(b"a\nb\rc"), [b"a", b"b", b"c"])
        self.assertEqual(bytes.splitlines(b"a\r\nb\r\n"), [b"a", b"b"])
        self.assertEqual(bytes.splitlines(b"\n\r\n\r"), [b"", b"", b""])
        self.assertEqual(bytes.splitlines(b"a\x0Bb"), [b"a\x0Bb"])

    def test_splitlines_with_keepend_returns_list(self):
        self.assertEqual(bytes.splitlines(b"", True), [])
        self.assertEqual(bytes.splitlines(b"a", 1), [b"a"])
        self.assertEqual(bytes.splitlines(b"a\nb\rc", 1), [b"a\n", b"b\r", b"c"])
        self.assertEqual(bytes.splitlines(b"a\r\nb\r\n", 1), [b"a\r\n", b"b\r\n"])
        self.assertEqual(bytes.splitlines(b"\n\r\n\r", 1), [b"\n", b"\r\n", b"\r"])

    def test_startswith_with_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytes.startswith(bytearray(), b"")
        self.assertIn(
            "'startswith' requires a 'bytes' object but received a 'bytearray'",
            str(context.exception),
        )

    def test_startswith_with_list_other_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".startswith([])
        self.assertEqual(
            str(context.exception),
            "startswith first arg must be bytes or a tuple of bytes, not list",
        )

    def test_startswith_with_tuple_other_checks_each(self):
        haystack = b"123"
        needle1 = (b"13", b"23", b"12", b"d")
        needle2 = (b"2", b"asd", b"122222")
        self.assertTrue(haystack.startswith(needle1))
        self.assertFalse(haystack.startswith(needle2))

    def test_startswith_with_start_searches_from_start(self):
        haystack = b"12345"
        needle1 = bytearray(b"1")
        needle4 = b"34"
        self.assertFalse(haystack.startswith(needle1, 1))
        self.assertFalse(haystack.startswith(needle4, 0))
        self.assertTrue(haystack.startswith(needle1, 0))
        self.assertTrue(haystack.startswith(needle4, 2))

    def test_startswith_with_empty_returns_true_for_valid_bounds(self):
        haystack = b"12345"
        self.assertTrue(haystack.startswith(bytearray()))
        self.assertTrue(haystack.startswith(b"", 5))
        self.assertTrue(haystack.startswith(bytearray(), -9, 1))

    def test_startswith_with_empty_returns_false_for_invalid_bounds(self):
        haystack = b"12345"
        self.assertFalse(haystack.startswith(b"", 3, 2))
        self.assertFalse(haystack.startswith(bytearray(), 6))

    def test_strip_with_non_byteslike_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            b"".strip("")
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_strip_with_none_or_default_strips_ascii_space(self):
        self.assertEqual(b"".strip(None), b"")
        self.assertEqual(b"    ".strip(None), b"")
        self.assertEqual(b"  hi  ".strip(), b"hi")

    def test_strip_with_byteslike_strips_bytes(self):
        self.assertEqual(b"".strip(b"123"), b"")
        self.assertEqual(b"1aaa1".strip(bytearray()), b"1aaa1")
        self.assertEqual(b"1 aaa1".strip(bytearray(b" 1")), b"aaa")
        self.assertEqual(b"hello".strip(b"ho"), b"ell")

    def test_upper_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes.upper("not a bytes")

    def test_upper_empty_self_returns_self(self):
        src = b""
        dst = src.upper()
        self.assertIs(src, dst)

    def test_upper_all_upper_returns_new_bytes(self):
        src = b"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        dst = src.upper()
        self.assertIsInstance(dst, bytes)
        self.assertIsNot(src, dst)
        self.assertEqual(src, dst)

    def test_upper_all_upper_and_non_alphanumeric_returns_new_bytes(self):
        src = b"ABCDEFGHIJ1234567890!@#$%^&*()"
        dst = src.upper()
        self.assertIsInstance(dst, bytes)
        self.assertIsNot(src, dst)
        self.assertEqual(src, dst)

    def test_upper_all_lower_returns_all_uppercase(self):
        src = b"abcdefghijklmnopqrstuvwxyz"
        dst = src.upper()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")

    def test_upper_mixed_case_returns_all_uppercase(self):
        src = b"aBcDeFgHiJkLmNoPqRsTuVwXyZ"
        dst = src.upper()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"ABCDEFGHIJKLMNOPQRSTUVWXYZ")

    def test_upper_mixed_case_returns_all_uppercase(self):
        src = b"a1!B2@c3#D4$e5%F6^g7&H8*i9(J0)"
        dst = src.upper()
        self.assertIsInstance(dst, bytes)
        self.assertEqual(dst, b"A1!B2@C3#D4$E5%F6^G7&H8*I9(J0)")


if __name__ == "__main__":
    unittest.main()
