#!/usr/bin/env python3
import unittest
import warnings


class BoundMethodTests(unittest.TestCase):
    def test_bound_method_dunder_func(self):
        class Foo:
            def foo(self):
                pass

        self.assertIs(Foo.foo, Foo().foo.__func__)

    def test_bound_method_dunder_self(self):
        class Foo:
            def foo(self):
                pass

        f = Foo()
        self.assertIs(f, f.foo.__self__)

    def test_bound_method_doc(self):
        class Foo:
            def foo(self):
                "This is the docstring of foo"
                pass

        self.assertEqual(Foo().foo.__doc__, "This is the docstring of foo")
        self.assertIs(Foo.foo.__doc__, Foo().foo.__doc__)

    def test_bound_method_readonly_attributes(self):
        class Foo:
            def foo(self):
                "This is the docstring of foo"
                pass

        f = Foo().foo
        with self.assertRaises(AttributeError):
            f.__func__ = abs

        with self.assertRaises(AttributeError):
            f.__self__ = int

        with self.assertRaises(AttributeError):
            f.__doc__ = "hey!"


class ByteArrayTests(unittest.TestCase):
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

    def test_clear_with_non_bytearray_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.clear(b"")
        self.assertIn(
            "'clear' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
        )

    def test_endswith_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.endswith(b"", bytearray())
        self.assertIn(
            "'endswith' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
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

    def test_find_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(TypeError):
            haystack.find(needle)

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

    def test_find_with_index_overflow_raises_value_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise OverflowError("not a byte!")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(ValueError) as context:
            haystack.find(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

    def test_find_with_dunder_index_error_raises_type_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise MemoryError("not a byte!")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.find(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
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
        with self.assertRaises(TypeError) as context:
            bytearray.join(b"", [])
        self.assertIn(
            "'join' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
        )

    def test_rfind_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            bytearray.rfind(b"", bytearray())
        self.assertIn(
            "'rfind' requires a 'bytearray' object but received a 'bytes'",
            str(context.exception),
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
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_rfind_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = bytearray(b"abc")
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.rfind(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

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


class BytesTests(unittest.TestCase):
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

    def test_find_with_string_raises_type_error(self):
        haystack = b""
        needle = "133"
        with self.assertRaises(TypeError):
            haystack.find(needle)

    def test_find_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(TypeError):
            haystack.find(needle)

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

    def test_find_with_index_overflow_raises_value_error(self):
        class Idx:
            def __float__(self):
                return 0.0

            def __index__(self):
                raise OverflowError("not a byte!")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(ValueError) as context:
            haystack.find(needle)
        self.assertEqual(str(context.exception), "byte must be in range(0, 256)")

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
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

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
            str(context.exception), "a bytes-like object is required, not 'str'"
        )

    def test_rfind_with_non_number_index_raises_type_error(self):
        class Idx:
            def __index__(self):
                return ord("a")

        haystack = b"abc"
        needle = Idx()
        with self.assertRaises(TypeError) as context:
            haystack.rfind(needle)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'Idx'"
        )

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


class ChrTests(unittest.TestCase):
    def test_returns_string(self):
        self.assertEqual(chr(101), "e")
        self.assertEqual(chr(42), "*")
        self.assertEqual(chr(0x1F40D), "\U0001f40d")

    def test_with_int_subclass_returns_string(self):
        class C(int):
            pass

        self.assertEqual(chr(C(122)), "z")

    def test_with_unicode_max_returns_string(self):
        import sys

        self.assertEqual(ord(chr(sys.maxunicode)), sys.maxunicode)

    def test_with_unicode_max_plus_one_raises_value_error(self):
        import sys

        with self.assertRaises(ValueError) as context:
            chr(sys.maxunicode + 1)
        self.assertIn("chr() arg not in range", str(context.exception))

    def test_with_negative_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            chr(-1)
        self.assertIn("chr() arg not in range", str(context.exception))

    def test_non_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            chr(None)
        self.assertEqual(
            str(context.exception), "an integer is required (got type NoneType)"
        )

    def test_with_large_int_raises_overflow_error(self):
        with self.assertRaises(OverflowError) as context:
            chr(123456789012345678901234567890)
        self.assertEqual(
            str(context.exception), "Python int too large to convert to C long"
        )


class ClassMethodTests(unittest.TestCase):
    def test_dunder_abstractmethod_with_missing_attr_returns_false(self):
        def foo():
            pass

        method = classmethod(foo)
        self.assertIs(method.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_false_attr_returns_false(self):
        def foo():
            pass

        foo.__isabstractmethod__ = False
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        prop = property(foo)
        self.assertIs(prop.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_abstract_returns_true(self):
        def foo():
            pass

        foo.__isabstractmethod__ = ["random", "values"]
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        method = classmethod(foo)
        self.assertIs(method.__isabstractmethod__, True)


class DelattrTests(unittest.TestCase):
    def test_non_str_as_name_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            delattr("str instance", None)
        self.assertIn(
            "attribute name must be string, not 'NoneType'", str(context.exception)
        )

    def test_delete_non_existing_instance_attribute_raises_attribute_error(self):
        class C:
            fld = 4

        c = C()
        with self.assertRaises(AttributeError):
            delattr(c, "fld")

    def test_calls_dunder_delattr_and_returns_none_if_successful(self):
        class C:
            def __init__(self):
                self.fld = ""

            def __delattr__(self, name):
                self.fld = name
                return "unused value"

        c = C()
        self.assertIs(delattr(c, "passed to __delattr__"), None)
        self.assertEqual(c.fld, "passed to __delattr__")

    def test_passes_exception_raised_by_dunder_delattr(self):
        class C:
            def __delattr__(self, name):
                raise UserWarning("delattr failed")

        c = C()
        with self.assertRaises(UserWarning) as context:
            delattr(c, "foo")
        self.assertIn("delattr failed", str(context.exception))

    def test_deletes_existing_instance_attribute(self):
        class C:
            def __init__(self):
                self.fld = 0

        c = C()
        self.assertTrue(hasattr(c, "fld"))
        delattr(c, "fld")
        self.assertFalse(hasattr(c, "fld"))

    def test_accepts_str_subclass_as_name(self):
        class C:
            def __init__(self):
                self.fld = 0

        class S(str):
            pass

        c = C()
        self.assertTrue(hasattr(c, "fld"))
        delattr(c, S("fld"))
        self.assertFalse(hasattr(c, "fld"))


class DictTests(unittest.TestCase):
    def test_clear_with_non_dict_raises_type_error(self):
        with self.assertRaises(TypeError):
            dict.clear(None)

    def test_clear_removes_all_elements(self):
        d = {"a": 1}
        self.assertEqual(dict.clear(d), None)
        self.assertEqual(d.__len__(), 0)
        self.assertNotIn("a", d)

    def test_copy_with_non_dict_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            dict.copy(None)
        self.assertIn(
            "'copy' requires a 'dict' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_update_with_malformed_sequence_elt_raises_type_error(self):
        with self.assertRaises(ValueError):
            dict.update({}, [("a",)])

    def test_update_with_no_params_does_nothing(self):
        d = {"a": 1}
        d.update()
        self.assertEqual(len(d), 1)

    def test_update_with_mapping_adds_elements(self):
        d = {"a": 1}
        d.update([("a", "b"), ("c", "d")])
        self.assertIn("a", d)
        self.assertIn("c", d)
        self.assertEqual(d["a"], "b")
        self.assertEqual(d["c"], "d")

    def test_dunder_delitem_with_none_dunder_hash(self):
        class C:
            __hash__ = None

        with self.assertRaises(TypeError):
            dict.__delitem__({}, C())

    def test_dunder_delitem_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        with self.assertRaises(UserWarning):
            dict.__delitem__({}, C())

    def test_update_with_tuple_keys_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        class D:
            def keys(self):
                return (C(),)

            def __getitem__(self, key):
                return "foo"

        with self.assertRaises(UserWarning):
            dict.update({}, D())

    def test_update_with_list_keys_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        class D:
            def keys(self):
                return [C()]

            def __getitem__(self, key):
                return "foo"

        with self.assertRaises(UserWarning):
            dict.update({}, D())

    def test_update_with_iter_keys_propagates_exceptions_from_dunder_hash(self):
        class C:
            def __hash__(self):
                raise UserWarning("foo")

        class D:
            def keys(self):
                return [C()].__iter__()

            def __getitem__(self, key):
                return "foo"

        with self.assertRaises(UserWarning):
            dict.update({}, D())


class ExceptionTests(unittest.TestCase):
    def test_maybe_unbound_attributes(self):
        exc = BaseException()
        exc2 = BaseException()
        self.assertIs(exc.__cause__, None)
        self.assertIs(exc.__context__, None)
        self.assertIs(exc.__traceback__, None)

        # Test setter for __cause__.
        self.assertRaises(TypeError, setattr, exc, "__cause__", 123)
        exc.__cause__ = exc2
        self.assertIs(exc.__cause__, exc2)
        exc.__cause__ = None
        self.assertIs(exc.__cause__, None)

        # Test setter for __context__.
        self.assertRaises(TypeError, setattr, exc, "__context__", 456)
        exc.__context__ = exc2
        self.assertIs(exc.__context__, exc2)
        exc.__context__ = None
        self.assertIs(exc.__context__, None)

        # Test setter for __traceback__.
        self.assertRaises(TypeError, setattr, "__traceback__", "some string")
        # TODO(bsimmers): Set a real traceback once we support them.
        exc.__traceback__ = None
        self.assertIs(exc.__traceback__, None)

    def test_context_chaining(self):
        inner_exc = None
        outer_exc = None
        try:
            try:
                raise RuntimeError("whoops")
            except RuntimeError as exc:
                inner_exc = exc
                raise TypeError("darn")
        except TypeError as exc:
            outer_exc = exc

        self.assertIsInstance(inner_exc, RuntimeError)
        self.assertIsInstance(outer_exc, TypeError)
        self.assertIs(outer_exc.__context__, inner_exc)
        self.assertIsNone(inner_exc.__context__)

    def test_context_chaining_cycle_avoidance(self):
        exc0 = None
        exc1 = None
        exc2 = None
        exc3 = None
        try:
            try:
                try:
                    try:
                        raise RuntimeError("inner")
                    except RuntimeError as exc:
                        exc0 = exc
                        raise RuntimeError("middle")
                except RuntimeError as exc:
                    exc1 = exc
                    raise RuntimeError("outer")
            except RuntimeError as exc:
                exc2 = exc
                # The __context__ link between exc1 and exc0 should be broken
                # by this raise.
                raise exc0
        except RuntimeError as exc:
            exc3 = exc

        self.assertIs(exc3, exc0)
        self.assertIs(exc3.__context__, exc2)
        self.assertIs(exc2.__context__, exc1)
        self.assertIs(exc1.__context__, None)


class FloatTests(unittest.TestCase):
    def test_dunder_divmod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__divmod__(1, 1.0)

    def test_dunder_divmod_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__divmod__(1.0, "1"), NotImplemented)

    def test_dunder_divmod_with_zero_denominator_raises_zero_division_error(self):
        with self.assertRaises(ZeroDivisionError):
            float.__divmod__(1.0, 0.0)

    def test_dunder_divmod_with_negative_zero_denominator_raises_zero_division_error(
        self
    ):
        with self.assertRaises(ZeroDivisionError):
            float.__divmod__(1.0, -0.0)

    def test_dunder_divmod_with_zero_numerator(self):
        floordiv, remainder = float.__divmod__(0.0, 4.0)
        self.assertEqual(floordiv, 0.0)
        self.assertEqual(remainder, 0.0)

    def test_dunder_divmod_with_positive_denominator_positive_numerator(self):
        floordiv, remainder = float.__divmod__(3.25, 1.0)
        self.assertEqual(floordiv, 3.0)
        self.assertEqual(remainder, 0.25)

    def test_dunder_divmod_with_negative_denominator_positive_numerator(self):
        floordiv, remainder = float.__divmod__(-3.25, 1.0)
        self.assertEqual(floordiv, -4.0)
        self.assertEqual(remainder, 0.75)

    def test_dunder_divmod_with_negative_denominator_negative_numerator(self):
        floordiv, remainder = float.__divmod__(-3.25, -1.0)
        self.assertEqual(floordiv, 3.0)
        self.assertEqual(remainder, -0.25)

    def test_dunder_divmod_with_positive_denominator_negative_numerator(self):
        floordiv, remainder = float.__divmod__(3.25, -1.0)
        self.assertEqual(floordiv, -4.0)
        self.assertEqual(remainder, -0.75)

    def test_dunder_divmod_with_nan_denominator(self):
        import math

        floordiv, remainder = float.__divmod__(3.25, float("nan"))
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_nan_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("nan"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_negative_nan_denominator(self):
        import math

        floordiv, remainder = float.__divmod__(3.25, float("-nan"))
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_negative_nan_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("-nan"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_inf_denominator(self):
        floordiv, remainder = float.__divmod__(3.25, float("inf"))
        self.assertEqual(floordiv, 0.0)
        self.assertEqual(remainder, 3.25)

    def test_dunder_divmod_with_inf_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("inf"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_negative_inf_denominator(self):
        floordiv, remainder = float.__divmod__(3.25, float("-inf"))
        self.assertEqual(floordiv, -1.0)
        self.assertEqual(remainder, -float("inf"))

    def test_dunder_divmod_with_negative_inf_numerator(self):
        import math

        floordiv, remainder = float.__divmod__(float("-inf"), 1.0)
        self.assertTrue(math.isnan(floordiv))
        self.assertTrue(math.isnan(remainder))

    def test_dunder_divmod_with_big_numerator(self):
        floordiv, remainder = float.__divmod__(1e200, 1.0)
        self.assertEqual(floordiv, 1e200)
        self.assertEqual(remainder, 0.0)

    def test_dunder_divmod_with_big_denominator(self):
        floordiv, remainder = float.__divmod__(1.0, 1e200)
        self.assertEqual(floordiv, 0.0)
        self.assertEqual(remainder, 1.0)

    def test_dunder_divmod_with_negative_zero_numerator(self):
        floordiv, remainder = float.__divmod__(-0.0, 4.0)
        self.assertTrue(str(floordiv) == "-0.0")
        self.assertEqual(remainder, 0.0)

    def test_dunder_floordiv_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__floordiv__(1, 1.0)

    def test_dunder_floordiv_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__floordiv__(1.0, "1"), NotImplemented)

    def test_dunder_floordiv_with_zero_denominator_raises_zero_division_error(self):
        with self.assertRaises(ZeroDivisionError):
            float.__floordiv__(1.0, 0.0)

    def test_dunder_floordiv_returns_floor_quotient(self):
        self.assertEqual(float.__floordiv__(3.25, -1.0), -4.0)

    def test_dunder_mod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__mod__(1, 1.0)

    def test_dunder_mod_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__mod__(1.0, "1"), NotImplemented)

    def test_dunder_mod_with_zero_denominator_raises_zero_division_error(self):
        with self.assertRaises(ZeroDivisionError):
            float.__mod__(1.0, 0.0)

    def test_dunder_mod_returns_remainder(self):
        self.assertEqual(float.__mod__(3.25, -1.0), -0.75)

    def test_dunder_rdivmod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rdivmod__(1, 1.0)

    def test_dunder_rdivmod_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__rdivmod__(1.0, "1"), NotImplemented)

    def test_dunder_rdivmod_with_int_returns_same_result_as_divmod_with_reversed_args(
        self
    ):
        self.assertEqual(float.__rdivmod__(1.0, 3), float.__divmod__(float(3), 1.0))

    def test_dunder_rdivmod_returns_same_result_as_divmod_with_reversed_args(self):
        self.assertEqual(float.__rdivmod__(1.0, 3.25), float.__divmod__(3.25, 1.0))

    def test_repr_with_infinity_returns_string(self):
        self.assertEqual(float.__repr__(float("inf")), "inf")
        self.assertEqual(float.__repr__(-float("inf")), "-inf")

    def test_repr_with_nan_returns_nan(self):
        self.assertEqual(float.__repr__(float("nan")), "nan")
        self.assertEqual(float.__repr__(float("-nan")), "nan")

    def test_repr_returns_string_without_exponent(self):
        self.assertEqual(float.__repr__(0.0), "0.0")
        self.assertEqual(float.__repr__(-0.0), "-0.0")
        self.assertEqual(float.__repr__(1.0), "1.0")
        self.assertEqual(float.__repr__(-1.0), "-1.0")
        self.assertEqual(float.__repr__(42.5), "42.5")
        self.assertEqual(float.__repr__(1.234567891234567), "1.234567891234567")
        self.assertEqual(float.__repr__(-1.234567891234567), "-1.234567891234567")
        self.assertEqual(float.__repr__(9.99999999999999e15), "9999999999999990.0")
        self.assertEqual(float.__repr__(0.0001), "0.0001")

    def test_repr_returns_string_with_exponent(self):
        self.assertEqual(float.__repr__(1e16), "1e+16")
        self.assertEqual(float.__repr__(0.00001), "1e-05")
        self.assertEqual(float.__repr__(1e100), "1e+100")
        self.assertEqual(float.__repr__(1e-88), "1e-88")
        self.assertEqual(
            float.__repr__(1.23456789123456789e123), "1.2345678912345679e+123"
        )
        self.assertEqual(
            float.__repr__(-1.23456789123456789e-123), "-1.2345678912345678e-123"
        )

    def test_dunder_rfloordiv_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rfloordiv__(1, 1.0)

    def test_dunder_rfloordiv_with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__rfloordiv__(1.0, "1"), NotImplemented)

    def test_dunder_rfloordiv_with_int_returns_same_result_as_floordiv(self):
        self.assertEqual(float.__rfloordiv__(1.0, 3), float.__floordiv__(float(3), 1.0))

    def test_dunder_rfloordiv_returns_same_result_as_floordiv_for_float_other(self):
        self.assertEqual(float.__rfloordiv__(1.0, 3.25), float.__floordiv__(3.25, 1.0))

    def test_dunder_rmod_raises_type_error(self):
        with self.assertRaises(TypeError):
            float.__rmod__(1, 1.0)

    def test_dunder_rmod__with_non_number_as_other_returns_not_implemented(self):
        self.assertIs(float.__rmod__(1.0, "1"), NotImplemented)

    def test_dunder_rmod_with_int_returns_same_result_as_mod_with_reversed_args(self):
        self.assertEqual(float.__rmod__(1.0, 3), float.__mod__(float(3), 1.0))

    def test_dunder_rmod_returns_same_result_as_mod_for_float_other(self):
        self.assertEqual(float.__rmod__(1.0, 3.25), float.__mod__(3.25, 1.0))

    def test_dunder_round_with_one_arg_returns_int(self):
        self.assertEqual(float.__round__(0.0), 0)
        self.assertIsInstance(float.__round__(0.0), int)
        self.assertEqual(float.__round__(-0.0), 0)
        self.assertIsInstance(float.__round__(-0.0), int)
        self.assertEqual(float.__round__(1.0), 1)
        self.assertIsInstance(float.__round__(1.0), int)
        self.assertEqual(float.__round__(-1.0), -1)
        self.assertIsInstance(float.__round__(-1.0), int)
        self.assertEqual(float.__round__(42.42), 42)
        self.assertIsInstance(float.__round__(42.42), int)
        self.assertEqual(float.__round__(0.4), 0)
        self.assertEqual(float.__round__(0.5), 0)
        self.assertEqual(float.__round__(0.5000000000000001), 1)
        self.assertEqual(float.__round__(1.49), 1)
        self.assertEqual(float.__round__(1.5), 2)
        self.assertEqual(float.__round__(1.5000000000000001), 2)
        self.assertEqual(
            float.__round__(1.234567e200),
            123456699999999995062622360556161455756457158443485858665105941107312145749402909576243454437530421952327149599911208391362816498839992520580209467560546813973197632314335145381120371005964774514098176,  # noqa: B950
        )
        self.assertEqual(float.__round__(-13.4, None), -13)
        self.assertIsInstance(float.__round__(-13.4, None), int)

    def test_dunder_round_with_float_subclass_returns_int(self):
        class C(float):
            pass

        self.assertEqual(float.__round__(C(-7654321.7654321)), -7654322)

    def test_dunder_round_with_one_arg_raises_error(self):
        with self.assertRaises(ValueError) as context:
            float.__round__(float("nan"))
        self.assertEqual(str(context.exception), "cannot convert float NaN to integer")
        with self.assertRaises(OverflowError) as context:
            float.__round__(float("inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )
        with self.assertRaises(OverflowError) as context:
            float.__round__(float("-inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )

    def test_dunder_round_with_two_args_returns_float(self):
        self.assertEqual(float.__round__(0.0, 0), 0.0)
        self.assertIsInstance(float.__round__(0.0, 0), float)
        self.assertEqual(float.__round__(-0.0, 1), 0.0)
        self.assertIsInstance(float.__round__(-0.0, 1), float)
        self.assertEqual(float.__round__(1.0, 0), 1.0)
        self.assertIsInstance(float.__round__(1.0, 0), float)
        self.assertEqual(float.__round__(-77441.7, -2), -77400.0)
        self.assertIsInstance(float.__round__(-77441.7, -2), float)

        self.assertEqual(float.__round__(12.34567, -(1 << 200)), 0.0)
        self.assertEqual(float.__round__(12.34567, -50), 0.0)
        self.assertEqual(float.__round__(12.34567, -2), 0.0)
        self.assertEqual(float.__round__(12.34567, -1), 10.0)
        self.assertEqual(float.__round__(12.34567, 0), 12.0)
        self.assertEqual(float.__round__(12.34567, 1), 12.3)
        self.assertEqual(float.__round__(12.34567, 2), 12.35)
        self.assertEqual(float.__round__(12.34567, 3), 12.346)
        self.assertEqual(float.__round__(12.34567, 4), 12.3457)
        self.assertEqual(float.__round__(12.34567, 50), 12.34567)
        self.assertEqual(float.__round__(12.34567, 1 << 200), 12.34567)

        self.assertEqual(float("inf"), float("inf"))
        self.assertEqual(float("-inf"), -float("inf"))

        float_max = 1.7976931348623157e308
        self.assertEqual(float.__round__(float_max, -309), 0.0)
        self.assertEqual(float.__round__(float_max, -303), 1.79769e308)

    def test_dunder_round_with_two_args_returns_nan(self):
        import math

        self.assertTrue(math.isnan(float.__round__(float("nan"), 2)))

    def test_dunder_round_with_two_args_raises_error(self):
        float_max = 1.7976931348623157e308
        with self.assertRaises(OverflowError) as context:
            float.__round__(float_max, -308)
        self.assertEqual(str(context.exception), "rounded value too large to represent")

    def test_dunder_trunc_returns_int(self):
        self.assertEqual(float.__trunc__(0.0), 0)
        self.assertEqual(float.__trunc__(-0.0), 0)
        self.assertEqual(float.__trunc__(1.0), 1)
        self.assertEqual(float.__trunc__(-1.0), -1)
        self.assertEqual(float.__trunc__(42.12345), 42)
        self.assertEqual(float.__trunc__(1.6069380442589903e60), 1 << 200)
        self.assertEqual(float.__trunc__(1e-20), 0)
        self.assertEqual(float.__trunc__(-1e-20), 0)
        self.assertIsInstance(float.__trunc__(0.0), int)
        self.assertIsInstance(float.__trunc__(1.6069380442589903e60), int)
        self.assertIsInstance(float.__trunc__(1e-20), int)

    def test_dunder_trunc_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            float.__trunc__(float("nan"))
        self.assertEqual(str(context.exception), "cannot convert float NaN to integer")
        with self.assertRaises(OverflowError) as context:
            float.__trunc__(float("inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )
        with self.assertRaises(OverflowError) as context:
            float.__trunc__(float("-inf"))
        self.assertEqual(
            str(context.exception), "cannot convert float infinity to integer"
        )


class FrozensetTests(unittest.TestCase):
    def test_issuperset_with_non_frozenset_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.issuperset(None, frozenset())

    def test_issuperset_with_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            frozenset.issuperset(set(), frozenset())

    def test_issuperset_with_empty_sets_returns_true(self):
        self.assertTrue(frozenset.issuperset(frozenset(), frozenset()))
        self.assertTrue(frozenset.issuperset(frozenset(), set()))

    def test_issuperset_with_anyset_subset_returns_true(self):
        self.assertTrue(frozenset({1}).issuperset(set()))
        self.assertTrue(frozenset({1}).issuperset(frozenset()))

        self.assertTrue(frozenset({1}).issuperset({1}))
        self.assertTrue(frozenset({1}).issuperset(frozenset({1})))

        self.assertTrue(frozenset({1, 2}).issuperset({1}))
        self.assertTrue(frozenset({1, 2}).issuperset(frozenset({1})))

        self.assertTrue(frozenset({1, 2}).issuperset({1, 2}))
        self.assertTrue(frozenset({1, 2}).issuperset(frozenset({1, 2})))

        self.assertTrue(frozenset({1, 2, 3}).issuperset({1, 2}))
        self.assertTrue(frozenset({1, 2, 3}).issuperset(frozenset({1, 2})))

    def test_issuperset_with_iterable_subset_returns_true(self):
        self.assertTrue(frozenset({1}).issuperset([]))
        self.assertTrue(frozenset({1}).issuperset(range(1, 1)))

        self.assertTrue(frozenset({1}).issuperset([1]))
        self.assertTrue(frozenset({1}).issuperset(range(1, 2)))

        self.assertTrue(frozenset({1, 2}).issuperset([1]))
        self.assertTrue(frozenset({1, 2}).issuperset(range(1, 2)))

        self.assertTrue(frozenset({1, 2}).issuperset([1, 2]))
        self.assertTrue(frozenset({1, 2}).issuperset(range(1, 3)))

        self.assertTrue(frozenset({1, 2, 3}).issuperset([1, 2]))
        self.assertTrue(frozenset({1, 2, 3}).issuperset(range(1, 3)))

    def test_issuperset_with_superset_returns_false(self):
        self.assertFalse(frozenset({}).issuperset({1}))
        self.assertFalse(frozenset({}).issuperset(frozenset({1})))
        self.assertFalse(frozenset({}).issuperset([1]))
        self.assertFalse(frozenset({}).issuperset(range(1, 2)))

        self.assertFalse(frozenset({1}).issuperset({1, 2}))
        self.assertFalse(frozenset({1}).issuperset(frozenset({1, 2})))
        self.assertFalse(frozenset({1}).issuperset([1, 2]))
        self.assertFalse(frozenset({1}).issuperset(range(1, 3)))

        self.assertFalse(frozenset({1, 2}).issuperset({1, 2, 3}))
        self.assertFalse(frozenset({1, 2}).issuperset(frozenset({1, 2, 3})))
        self.assertFalse(frozenset({1, 2}).issuperset([1, 2, 3]))
        self.assertFalse(frozenset({1, 2}).issuperset(range(1, 4)))


class GeneratorTests(unittest.TestCase):
    def test_managed_stop_iteration(self):
        warnings.filterwarnings(
            action="ignore",
            category=DeprecationWarning,
            message="generator.*raised StopIteration",
            module=__name__,
        )

        def inner_gen():
            yield None
            raise StopIteration("hello")

        def outer_gen():
            val = yield from inner_gen()
            yield val

        g = outer_gen()
        self.assertIs(next(g), None)
        self.assertEqual(next(g), "hello")

    def test_gi_running(self):
        def gen():
            self.assertTrue(g.gi_running)
            yield 1
            self.assertTrue(g.gi_running)
            yield 2

        g = gen()
        self.assertFalse(g.gi_running)
        next(g)
        self.assertFalse(g.gi_running)
        next(g)
        self.assertFalse(g.gi_running)

    def test_gi_running_readonly(self):
        def gen():
            yield None

        g = gen()
        self.assertRaises(AttributeError, setattr, g, "gi_running", 1234)

    def test_running_gen_raises(self):
        def gen():
            self.assertRaises(ValueError, next, g)
            yield "done"

        g = gen()
        self.assertEqual(next(g), "done")

    class MyError(Exception):
        pass

    @staticmethod
    def simple_gen():
        yield 1
        yield 2

    @staticmethod
    def catching_gen():
        try:
            yield 1
        except GeneratorTests.MyError:
            yield "caught"

    @staticmethod
    def catching_returning_gen():
        try:
            yield 1
        except GeneratorTests.MyError:
            return "all done!"  # noqa

    @staticmethod
    def delegate_gen(g):
        r = yield from g
        yield r

    def test_throw(self):
        g = self.simple_gen()
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError())

    def test_throw_caught(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError()), "caught")

    def test_throw_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError), "caught")

    def test_throw_type_and_value(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(
            g.throw(GeneratorTests.MyError, GeneratorTests.MyError()), "caught"
        )

    def test_throw_uncaught_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(RuntimeError, g.throw, RuntimeError)

    def test_throw_finished(self):
        g = self.catching_returning_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(StopIteration, g.throw, GeneratorTests.MyError)

    def test_throw_two_values(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(
            TypeError, g.throw, GeneratorTests.MyError(), GeneratorTests.MyError()
        )

    def test_throw_bad_traceback(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(
            TypeError, g.throw, GeneratorTests.MyError, GeneratorTests.MyError(), 5
        )

    def test_throw_bad_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(TypeError, g.throw, 1234)

    def test_throw_not_started(self):
        g = self.simple_gen()
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError())
        self.assertRaises(StopIteration, next, g)

    def test_throw_stopped(self):
        g = self.simple_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(next(g), 2)
        self.assertRaises(StopIteration, next, g)
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError())

    def test_throw_yield_from(self):
        g = self.delegate_gen(self.simple_gen())
        self.assertEqual(next(g), 1)
        self.assertRaises(GeneratorTests.MyError, g.throw, GeneratorTests.MyError)

    def test_throw_yield_from_caught(self):
        g = self.delegate_gen(self.catching_gen())
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError), "caught")

    def test_throw_yield_from_finishes(self):
        g = self.delegate_gen(self.catching_returning_gen())
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(GeneratorTests.MyError), "all done!")

    def test_throw_yield_from_non_gen(self):
        g = self.delegate_gen([1, 2, 3, 4])
        self.assertEqual(next(g), 1)
        self.assertRaises(RuntimeError, g.throw, RuntimeError)

    def test_dunder_repr(self):
        def foo():
            yield 5

        self.assertTrue(
            foo()
            .__repr__()
            .startswith(
                "<generator object GeneratorTests.test_dunder_repr.<locals>.foo at "
            )
        )


class HashTests(unittest.TestCase):
    def test_hash_with_raising_dunder_hash_raises_type_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Foo:
            __hash__ = Desc()

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            hash(foo)
        self.assertEqual(str(context.exception), "unhashable type: 'Foo'")

    def test_hash_with_none_dunder_hash_raises_type_error(self):
        class Foo:
            __hash__ = None

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            hash(foo)
        self.assertEqual(str(context.exception), "unhashable type: 'Foo'")

    def test_hash_with_non_int_dunder_hash_raises_type_error(self):
        class Foo:
            def __hash__(self):
                return "not an int"

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            hash(foo)
        self.assertEqual(
            str(context.exception), "__hash__ method should return an integer"
        )

    def test_hash_with_int_subclass_dunder_hash_returns_int(self):
        class SubInt(int):
            pass

        class Foo:
            def __hash__(self):
                return SubInt(42)

        foo = Foo()
        result = hash(foo)
        self.assertEqual(42, result)
        self.assertEqual(type(42), int)


class IntTests(unittest.TestCase):
    def test_dunder_new_with_bool_class_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            int.__new__(bool, 0)
        self.assertEqual(
            str(context.exception), "int.__new__(bool) is not safe, use bool.__new__()"
        )

    def test_dunder_new_with_dunder_int_subclass_warns(self):
        class Num(int):
            pass

        class Foo:
            def __int__(self):
                return Num(42)

        foo = Foo()
        with self.assertWarns(DeprecationWarning):
            self.assertEqual(int(foo), 42)

    def test_dunder_new_uses_type_dunder_int(self):
        class Foo:
            def __int__(self):
                return 0

        foo = Foo()
        foo.__int__ = "not callable"
        self.assertEqual(int(foo), 0)

    def test_dunder_new_uses_type_dunder_trunc(self):
        class Foo:
            def __trunc__(self):
                return 0

        foo = Foo()
        foo.__trunc__ = "not callable"
        self.assertEqual(int(foo), 0)

    def test_dunder_new_with_raising_trunc_propagates_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Foo:
            __trunc__ = Desc()

        foo = Foo()
        with self.assertRaises(AttributeError) as context:
            int(foo)
        self.assertEqual(str(context.exception), "failed")

    def test_dunder_new_with_base_without_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            int(base=8)

    def test_dunder_new_with_bool_returns_int(self):
        self.assertIs(int(False), 0)
        self.assertIs(int(True), 1)

    def test_dunder_new_with_bytearray_returns_int(self):
        self.assertEqual(int(bytearray(b"23")), 23)
        self.assertEqual(int(bytearray(b"-23"), 8), -0o23)
        self.assertEqual(int(bytearray(b"abc"), 16), 0xABC)
        self.assertEqual(int(bytearray(b"0xabc"), 0), 0xABC)

    def test_dunder_new_with_bytes_returns_int(self):
        self.assertEqual(int(b"-23"), -23)
        self.assertEqual(int(b"23", 8), 0o23)
        self.assertEqual(int(b"abc", 16), 0xABC)
        self.assertEqual(int(b"0xabc", 0), 0xABC)

    def test_dunder_new_with_empty_bytearray_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(bytearray())

    def test_dunder_new_with_empty_bytes_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(b"")

    def test_dunder_new_with_empty_str_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("")

    def test_dunder_new_with_int_returns_int(self):
        self.assertEqual(int(23), 23)

    def test_dunder_new_with_int_and_base_raises_type_error(self):
        with self.assertRaises(TypeError):
            int(4, 5)

    def test_dunder_new_with_invalid_base_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("0", 1)

    def test_dunder_new_with_invalid_chars_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("&2*")

    def test_dunder_new_with_invalid_digits_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(b"789", 6)

    def test_dunder_new_with_str_returns_int(self):
        self.assertEqual(int("23"), 23)
        self.assertEqual(int("-23", 8), -0o23)
        self.assertEqual(int("-abc", 16), -0xABC)
        self.assertEqual(int("0xabc", 0), 0xABC)

    def test_dunder_new_with_zero_args_returns_zero(self):
        self.assertIs(int(), 0)

    def test_dunder_pow_with_zero_returns_one(self):
        self.assertEqual(int.__pow__(4, 0), 1)

    def test_dunder_pow_with_one_returns_self(self):
        self.assertEqual(int.__pow__(4, 1), 4)

    def test_dunder_pow_with_two_squares_number(self):
        self.assertEqual(int.__pow__(4, 2), 16)

    def test_dunder_pow_with_mod_equals_one_returns_zero(self):
        self.assertEqual(int.__pow__(4, 2, 1), 0)

    def test_dunder_pow_with_negative_power_and_mod_raises_value_error(self):
        with self.assertRaises(ValueError):
            int.__pow__(4, -2, 1)

    def test_dunder_pow_with_mod(self):
        self.assertEqual(int.__pow__(4, 8, 10), 6)

    def test_dunder_pow_with_negative_base_calls_float_dunder_pow(self):
        self.assertLess((int.__pow__(2, -1) - 0.5).__abs__(), 0.00001)

    def test_dunder_pow_with_non_int_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            int.__pow__(None, 1, 1)

    def test_dunder_pow_with_non_int_power_returns_not_implemented(self):
        self.assertEqual(int.__pow__(1, None), NotImplemented)

    def test_from_bytes_returns_int(self):
        self.assertEqual(int.from_bytes(b"\xca\xfe", "little"), 0xFECA)

    def test_from_bytes_with_kwargs_returns_int(self):
        self.assertEqual(
            int.from_bytes(bytes=b"\xca\xfe", byteorder="big", signed=True), -13570
        )

    def test_from_bytes_with_bytes_convertible_returns_int(self):
        class C:
            def __bytes__(self):
                return b"*"

        i = C()
        self.assertEqual(int.from_bytes(i, "big"), 42)

    def test_from_bytes_with_invalid_bytes_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, "cannot convert 'str' object to bytes"):
            int.from_bytes("not a bytes object", "big")

    def test_from_bytes_with_invalid_byteorder_raises_value_error(self):
        with self.assertRaisesRegex(
            ValueError, "byteorder must be either 'little' or 'big'"
        ):
            int.from_bytes(b"*", byteorder="medium")

    def test_from_bytes_with_invalid_byteorder_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "from_bytes\\(\\) argument 2 must be str, not int"
        ):
            int.from_bytes(b"*", byteorder=42)

    def test_from_bytes_uses_type_dunder_bytes(self):
        class Foo:
            def __bytes__(self):
                return b"*"

        foo = Foo()
        foo.__bytes__ = lambda: b"123"
        self.assertEqual(int.from_bytes(foo, "big"), 42)


class IsInstanceTests(unittest.TestCase):
    def test_isinstance_with_same_types_returns_true(self):
        self.assertIs(isinstance(1, int), True)

    def test_isinstance_with_subclass_returns_true(self):
        self.assertIs(isinstance(False, int), True)

    def test_isinstance_with_superclass_returns_false(self):
        self.assertIs(isinstance(2, bool), False)

    def test_isinstance_with_type_and_metaclass_returns_true(self):
        self.assertIs(isinstance(list, type), True)

    def test_isinstance_with_type_returns_true(self):
        self.assertIs(isinstance(type, type), True)

    def test_isinstance_with_object_type_returns_true(self):
        self.assertIs(isinstance(object, object), True)

    def test_isinstance_with_int_type_returns_false(self):
        self.assertIs(isinstance(int, int), False)

    def test_isinstance_with_unrelated_types_returns_false(self):
        self.assertIs(isinstance(int, (dict, bytes, str)), False)

    def test_isinstance_with_superclass_tuple_returns_true(self):
        self.assertIs(isinstance(True, (int, "bad - not a type")), True)

    def test_isinstance_with_non_type_superclass_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            isinstance(4, "bad - not a type")
        self.assertEqual(
            str(context.exception),
            "isinstance() arg 2 must be a type or tuple of types",
        )

    def test_isinstance_with_non_type_in_tuple_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            isinstance(5, ("bad - not a type", int))
        self.assertEqual(
            str(context.exception),
            "isinstance() arg 2 must be a type or tuple of types",
        )

    def test_isinstance_with_multiple_inheritance_returns_true(self):
        class A:
            pass

        class B(A):
            pass

        class C(A):
            pass

        class D(B, C):
            pass

        d = D()

        # D() is an instance of all specified superclasses
        self.assertIs(isinstance(d, A), True)
        self.assertIs(isinstance(d, B), True)
        self.assertIs(isinstance(d, C), True)
        self.assertIs(isinstance(d, D), True)

        # D() is not an instance of builtin types except object
        self.assertIs(isinstance(d, object), True)
        self.assertIs(isinstance(d, list), False)

        # D is an instance type, but D() is not
        self.assertIs(isinstance(D, type), True)
        self.assertIs(isinstance(d, type), False)

    def test_isinstance_with_type_checks_instance_type_and_dunder_class(self):
        class A(int):
            __class__ = list

        a = A()
        self.assertIs(isinstance(a, int), True)
        self.assertIs(isinstance(a, list), True)

    def test_isinstance_with_nontype_checks_dunder_bases_and_dunder_class(self):
        class A:
            __bases__ = ()

        a = A()

        class B:
            __bases__ = (a,)

        b = B()

        class C(int):
            __class__ = b
            __bases__ = (int,)

        c = C()
        self.assertIs(isinstance(c, a), True)
        self.assertIs(isinstance(c, b), True)
        self.assertIs(isinstance(c, c), False)

    def test_isinstance_with_non_tuple_dunder_bases_raises_type_error(self):
        class A:
            __bases__ = 5

        with self.assertRaises(TypeError) as context:
            isinstance(5, A())
        self.assertEqual(
            str(context.exception),
            "isinstance() arg 2 must be a type or tuple of types",
        )

    def test_isinstance_calls_custom_instancecheck_true(self):
        class Meta(type):
            def __instancecheck__(cls, obj):
                return [1]

        class A(metaclass=Meta):
            pass

        self.assertIs(isinstance(0, A), True)

    def test_isinstance_calls_custom_instancecheck_false(self):
        class Meta(type):
            def __instancecheck__(cls, obj):
                return None

        class A(metaclass=Meta):
            pass

        class B(A):
            pass

        self.assertIs(isinstance(A(), A), True)
        self.assertIs(isinstance(B(), A), False)

    def test_isinstance_with_raising_instancecheck_propagates_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Meta(type):
            __instancecheck__ = Desc()

        class A(metaclass=Meta):
            pass

        with self.assertRaises(AttributeError) as context:
            isinstance(2, A)
        self.assertEqual(str(context.exception), "failed")


class IsSubclassTests(unittest.TestCase):
    def test_issubclass_with_same_types_returns_true(self):
        self.assertIs(issubclass(int, int), True)

    def test_issubclass_with_subclass_returns_true(self):
        self.assertIs(issubclass(bool, int), True)

    def test_issubclass_with_superclass_returns_false(self):
        self.assertIs(issubclass(int, bool), False)

    def test_issubclass_with_unrelated_types_returns_false(self):
        self.assertIs(issubclass(int, (dict, bytes, str)), False)

    def test_issubclass_with_superclass_tuple_returns_true(self):
        self.assertIs(issubclass(bool, (int, "bad - not a type")), True)

    def test_issubclass_with_non_type_subclass_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            issubclass("bad - not a type", str)
        self.assertEqual(str(context.exception), "issubclass() arg 1 must be a class")

    def test_issubclass_with_non_type_superclass_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            issubclass(int, "bad - not a type")
        self.assertEqual(
            str(context.exception),
            "issubclass() arg 2 must be a class or tuple of classes",
        )

    def test_issubclass_with_non_type_in_tuple_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            issubclass(bool, ("bad - not a type", int))
        self.assertEqual(
            str(context.exception),
            "issubclass() arg 2 must be a class or tuple of classes",
        )

    def test_issubclass_with_non_type_subclass_uses_bases(self):
        class A:
            __bases__ = (list,)

        self.assertIs(issubclass(A(), list), True)

    def test_issubclass_calls_custom_subclasscheck_true(self):
        class Meta(type):
            def __subclasscheck__(cls, subclass):
                return 1

        class A(metaclass=Meta):
            pass

        self.assertIs(issubclass(list, A), True)

    def test_issubclass_calls_custom_subclasscheck_false(self):
        class Meta(type):
            def __subclasscheck__(cls, subclass):
                return []

        class A(metaclass=Meta):
            pass

        class B(A):
            pass

        self.assertIs(issubclass(B, A), False)

    def test_issubclass_with_raising_subclasscheck_propagates_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Meta(type):
            __subclasscheck__ = Desc()

        class A(metaclass=Meta):
            pass

        with self.assertRaises(AttributeError) as context:
            issubclass(bool, A)
        self.assertEqual(str(context.exception), "failed")


class IterTests(unittest.TestCase):
    def test_iter_with_no_dunder_iter_raises_type_error(self):
        class C:
            pass

        c = C()

        with self.assertRaises(TypeError) as context:
            iter(c)

        self.assertEqual(str(context.exception), "'C' object is not iterable")

    def test_iter_with_dunder_iter_calls_dunder_iter(self):
        class C:
            def __iter__(self):
                raise UserWarning("foo")

        c = C()

        with self.assertRaises(UserWarning) as context:
            iter(c)

        self.assertEqual(str(context.exception), "foo")

    def test_iter_with_raising_descriptor_dunder_iter_raises_type_error(self):
        dunder_get_called = False

        class Desc:
            def __get__(self, obj, type):
                nonlocal dunder_get_called
                dunder_get_called = True
                raise UserWarning("foo")

        class C:
            __iter__ = Desc()

        c = C()

        with self.assertRaises(TypeError) as context:
            iter(c)

        self.assertEqual(str(context.exception), "'C' object is not iterable")
        self.assertTrue(dunder_get_called)

    def test_iter_with_none_dunder_iter_raises_type_error(self):
        class Foo:
            __iter__ = None

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            iter(foo)
        self.assertEqual(str(context.exception), "'Foo' object is not iterable")


class LenTests(unittest.TestCase):
    def test_len_without_class_dunder_len_raises_type_error(self):
        class Foo:
            pass

        foo = Foo()
        foo.__len__ = lambda: 0
        with self.assertRaises(TypeError) as context:
            len(foo)
        self.assertEqual(str(context.exception), "object of type 'Foo' has no len()")

    def test_len_without_non_int_dunder_len_raises_type_error(self):
        class Foo:
            def __len__(self):
                return "not an int"

        foo = Foo()
        with self.assertRaises(TypeError) as context:
            len(foo)
        self.assertEqual(
            str(context.exception), "'str' object cannot be interpreted as an integer"
        )

    def test_len_with_dunder_len_returns_int(self):
        class Foo:
            def __len__(self):
                return 5

        self.assertEqual(len(Foo()), 5)

    def test_len_with_list_returns_list_length(self):
        self.assertEqual(len([1, 2, 3]), 3)

    def test_len_with_non_container_raises_type_error(self):
        self.assertRaises(TypeError, len, 1)


class ListTests(unittest.TestCase):
    def test_dunder_eq_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.__eq__(False, [])
        self.assertIn(
            "'__eq__' requires a 'list' object but received a 'bool'",
            str(context.exception),
        )

    def test_copy_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.copy(None)
        self.assertIn(
            "'copy' requires a 'list' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_count_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.count(None, 1)
        self.assertIn(
            "'count' requires a 'list' object but received a 'NoneType'",
            str(context.exception),
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

    def test_index_with_non_list_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.index(1, 2)
        self.assertIn(
            "'index' requires a 'list' object but received a 'int'",
            str(context.exception),
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

    def test_sort_with_non_list_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            list.sort(None)
        self.assertIn(
            "'sort' requires a 'list' object but received a 'NoneType'",
            str(context.exception),
        )


class LocalsTests(unittest.TestCase):
    def test_returns_local_vars(self):
        def foo():
            a = 4
            b = 5
            return locals()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        self.assertEqual(result["b"], 5)

    def test_returns_free_vars(self):
        def foo():
            a = 4

            def bar():
                nonlocal a
                a = 5
                return locals()

            return bar()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 1)
        self.assertEqual(result["a"], 5)

    def test_returns_cell_vars(self):
        def foo():
            a = 4

            def bar(b):
                return a + b

            return locals()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        from types import FunctionType

        self.assertIsInstance(result["bar"], FunctionType)


class LongRangeIteratorTests(unittest.TestCase):
    def test_dunder_iter_returns_self(self):
        large_int = 2 ** 123
        it = iter(range(large_int))
        self.assertEqual(iter(it), it)

    def test_dunder_length_hint_returns_pending_length(self):
        large_int = 2 ** 123
        it = iter(range(large_int))
        self.assertEqual(it.__length_hint__(), large_int)
        it.__next__()
        self.assertEqual(it.__length_hint__(), large_int - 1)

    def test_dunder_next_returns_ints(self):
        large_int = 2 ** 123
        it = iter(range(large_int))
        for i in [0, 1, 2, 3]:
            self.assertEqual(it.__next__(), i)


class ModuleTest(unittest.TestCase):
    def test_dunder_dir_returns_newly_created_list_object(self):
        from types import ModuleType

        mymodule = ModuleType("mymodule")
        self.assertEqual(type(mymodule.__dir__()), list)
        self.assertIsNot(mymodule.__dir__(), mymodule.__dir__())

    def test_dunder_dir_returns_list_containing_module_attributes(self):
        from types import ModuleType

        mymodule = ModuleType("mymodule")
        mymodule.x = 40
        mymodule.y = 50
        result = mymodule.__dir__()
        self.assertIn("x", result)
        self.assertIn("y", result)

    def test_dunder_dir_returns_list_containing_added_module_attributes(self):
        from types import ModuleType

        mymodule = ModuleType("mymodule")
        self.assertNotIn("z", mymodule.__dir__())

        mymodule.z = 60
        self.assertIn("z", mymodule.__dir__())

    def test_dunder_dir_returns_list_not_containing_deleted_module_attributes(self):
        from types import ModuleType

        mymodule = ModuleType("mymodule")
        mymodule.x = 40
        self.assertIn("x", mymodule.__dir__())

        del mymodule.x
        self.assertNotIn("x", mymodule.__dir__())


class NextTests(unittest.TestCase):
    def test_next_with_raising_dunder_next_propagates_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Foo:
            __next__ = Desc()

        foo = Foo()
        with self.assertRaises(AttributeError) as context:
            next(foo)
        self.assertEqual(str(context.exception), "failed")


class ObjectTests(unittest.TestCase):
    def test_dunder_subclasshook_returns_not_implemented(self):
        self.assertIs(object.__subclasshook__(), NotImplemented)
        self.assertIs(object.__subclasshook__(int), NotImplemented)

    def test_dunder_class_on_instance_returns_type(self):
        class Foo:
            pass

        class Bar(Foo):
            pass

        class Hello(Bar, list):
            pass

        self.assertIs([].__class__, list)
        self.assertIs(Foo().__class__, Foo)
        self.assertIs(Bar().__class__, Bar)
        self.assertIs(Hello().__class__, Hello)
        self.assertIs(Foo.__class__, type)
        self.assertIs(super(Bar, Bar()).__class__, super)


class PrintTests(unittest.TestCase):
    class MyStream:
        def __init__(self):
            self.buf = ""

        def write(self, text):
            self.buf += text
            return len(text)

        def flush(self):
            raise UserWarning("foo")

    def test_print_writes_to_stream(self):
        stream = PrintTests.MyStream()
        print("hello", file=stream)
        self.assertEqual(stream.buf, "hello\n")

    def test_print_returns_none(self):
        stream = PrintTests.MyStream()
        self.assertIs(print("hello", file=stream), None)

    def test_print_writes_end(self):
        stream = PrintTests.MyStream()
        print("hi", end="ho", file=stream)
        self.assertEqual(stream.buf, "hiho")

    def test_print_with_no_sep_defaults_to_space(self):
        stream = PrintTests.MyStream()
        print("hello", "world", file=stream)
        self.assertEqual(stream.buf, "hello world\n")

    def test_print_writes_none(self):
        stream = PrintTests.MyStream()
        print(None, file=stream)
        self.assertEqual(stream.buf, "None\n")

    def test_print_with_none_file_prints_to_sys_stdout(self):
        stream = PrintTests.MyStream()
        import sys

        orig_stdout = sys.stdout
        sys.stdout = stream
        print("hello", file=None)
        self.assertEqual(stream.buf, "hello\n")
        sys.stdout = orig_stdout

    def test_print_with_none_stdout_does_nothing(self):
        import sys

        orig_stdout = sys.stdout
        sys.stdout = None
        print("hello", file=None)
        sys.stdout = orig_stdout

    def test_print_with_deleted_stdout_raises_runtime_error(self):
        import sys

        orig_stdout = sys.stdout
        del sys.stdout
        with self.assertRaises(RuntimeError):
            print("hello", file=None)
        sys.stdout = orig_stdout

    def test_print_with_flush_calls_file_flush(self):
        stream = PrintTests.MyStream()
        with self.assertRaises(UserWarning):
            print("hello", file=stream, flush=True)
        self.assertEqual(stream.buf, "hello\n")

    def test_print_calls_dunder_str(self):
        class C:
            def __str__(self):
                raise UserWarning("foo")

        stream = PrintTests.MyStream()
        c = C()
        with self.assertRaises(UserWarning):
            print(c, file=stream)


class PropertyTests(unittest.TestCase):
    def test_dunder_abstractmethod_with_missing_attr_returns_false(self):
        def foo():
            pass

        prop = property(foo)
        self.assertIs(prop.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_false_attr_returns_false(self):
        def foo():
            pass

        foo.__isabstractmethod__ = False
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        prop = property(foo)
        self.assertIs(prop.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_abstract_getter_returns_true(self):
        def foo():
            pass

        foo.__isabstractmethod__ = b"random non-empty value"
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        prop = property(foo)
        self.assertIs(prop.__isabstractmethod__, True)

    def test_dunder_abstractmethod_with_abstract_setter_returns_true(self):
        def foo():
            pass

        foo.__isabstractmethod__ = True
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        prop = property(fset=foo)
        self.assertIs(prop.__isabstractmethod__, True)

    def test_dunder_abstractmethod_with_abstract_deleter_returns_true(self):
        def foo():
            pass

        foo.__isabstractmethod__ = (42, "non-empty tuple")
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        prop = property(fdel=foo)
        self.assertIs(prop.__isabstractmethod__, True)


class RangeTests(unittest.TestCase):
    def test_dunder_eq_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__eq__(1, 2)

    def test_dunder_eq_with_non_range_other_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__eq__(1), NotImplemented)

    def test_dunder_eq_same_returns_true(self):
        r = range(10)
        self.assertTrue(r == r)

    def test_dunder_eq_both_empty_returns_true(self):
        r1 = range(0)
        r2 = range(4, 3, 2)
        r3 = range(2, 5, -1)
        self.assertTrue(r1 == r2)
        self.assertTrue(r1 == r3)
        self.assertTrue(r2 == r3)

    def test_dunder_eq_different_start_returns_false(self):
        r1 = range(1, 10, 3)
        r2 = range(2, 10, 3)
        self.assertFalse(r1 == r2)

    def test_dunder_eq_different_stop_returns_true(self):
        r1 = range(0, 10, 3)
        r2 = range(0, 11, 3)
        self.assertTrue(r1 == r2)

    def test_dunder_eq_different_step_length_one_returns_true(self):
        r1 = range(0, 4, 10)
        r2 = range(0, 4, 11)
        self.assertTrue(r1 == r2)

    def test_dunder_eq_different_step_returns_false(self):
        r1 = range(0, 14, 10)
        r2 = range(0, 14, 11)
        self.assertFalse(r1 == r2)

    def test_dunder_ge_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__ge__(1, 2)

    def test_dunder_ge_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__ge__(r), NotImplemented)

    def test_dunder_getitem_with_non_range_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__getitem__(1, 2)

    def test_dunder_getitem_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise ValueError("foo")

        r = range(5)
        self.assertEqual(r[C(3)], 3)

    def test_dunder_getitem_with_raising_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("foo")

        class C:
            __index__ = Desc()

        r = range(5)
        with self.assertRaises(AttributeError) as context:
            r[C()]
        self.assertEqual(str(context.exception), "foo")

    def test_dunder_getitem_with_string_raises_type_error(self):
        r = range(5)
        with self.assertRaises(TypeError) as context:
            r["3"]
        self.assertEqual(
            str(context.exception), "range indices must be integers or slices, not str"
        )

    def test_dunder_getitem_with_dunder_index_calls_dunder_index(self):
        class C:
            def __index__(self):
                return 2

        r = range(5)
        self.assertEqual(r[C()], 2)

    def test_dunder_getitem_index_too_small_raises_index_error(self):
        r = range(5)
        with self.assertRaises(IndexError) as context:
            r[-6]
        self.assertEqual(str(context.exception), "range object index out of range")

    def test_dunder_getitem_index_too_large_raises_index_error(self):
        r = range(5)
        with self.assertRaises(IndexError) as context:
            r[5]
        self.assertEqual(str(context.exception), "range object index out of range")

    def test_dunder_getitem_negative_index_relative_to_end_value_error(self):
        r = range(5)
        self.assertEqual(r[-4], 1)

    def test_dunder_getitem_with_valid_indices_returns_sublist(self):
        r = range(5)
        self.assertEqual(r[2:-1:1], range(2, 4))

    def test_dunder_getitem_with_negative_start_returns_trailing(self):
        r = range(5)
        self.assertEqual(r[-2:], range(3, 5))

    def test_dunder_getitem_with_positive_stop_returns_leading(self):
        r = range(5)
        self.assertEqual(r[:2], range(2))

    def test_dunder_getitem_with_negative_stop_returns_all_but_trailing(self):
        r = range(5)
        self.assertEqual(r[:-2], range(3))

    def test_dunder_getitem_with_positive_step_returns_forwards_list(self):
        r = range(5)
        self.assertEqual(r[::2], range(0, 5, 2))

    def test_dunder_getitem_with_negative_step_returns_backwards_list(self):
        r = range(5)
        self.assertEqual(r[::-2], range(4, -1, -2))

    def test_dunder_getitem_with_large_negative_start_returns_copy(self):
        r = range(5)
        copy = r[-10:]
        self.assertEqual(copy, r)
        self.assertIsNot(copy, r)

    def test_dunder_getitem_with_large_positive_start_returns_empty(self):
        r = range(5)
        self.assertEqual(r[10:], range(0))

    def test_dunder_getitem_with_large_negative_start_returns_empty(self):
        r = range(5)
        self.assertEqual(r[:-10], range(0))

    def test_dunder_getitem_with_large_positive_start_returns_copy(self):
        r = range(5)
        copy = r[:10]
        self.assertEqual(copy, r)
        self.assertIsNot(copy, r)

    def test_dunder_getitem_with_identity_slice_returns_copy(self):
        r = range(5)
        copy = r[::]
        self.assertEqual(copy, r)
        self.assertIsNot(copy, r)

    def test_dunder_gt_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__gt__(1, 2)

    def test_dunder_gt_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__gt__(r), NotImplemented)

    def test_dunder_iter_returns_range_iterator(self):
        it = iter(range(100))
        self.assertEqual(type(it).__name__, "range_iterator")

    def test_dunder_iter_returns_longrange_iterator(self):
        it = iter(range(2 ** 63))
        self.assertEqual(type(it).__name__, "longrange_iterator")

    def test_dunder_le_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__le__(1, 2)

    def test_dunder_le_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__le__(r), NotImplemented)

    def test_dunder_lt_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__lt__(1, 2)

    def test_dunder_lt_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__lt__(r), NotImplemented)

    def test_dunder_ne_with_non_range_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            range.__ne__(1, 2)

    def test_dunder_ne_with_non_range_other_returns_not_implemented(self):
        r = range(100)
        self.assertIs(r.__ne__(1), NotImplemented)

    def test_dunder_ne_same_returns_false(self):
        r = range(10)
        self.assertFalse(r != r)

    def test_dunder_ne_both_empty_returns_false(self):
        r1 = range(0)
        r2 = range(4, 3, 2)
        r3 = range(2, 5, -1)
        self.assertFalse(r1 != r2)
        self.assertFalse(r1 != r3)
        self.assertFalse(r2 != r3)

    def test_dunder_ne_different_start_returns_true(self):
        r1 = range(1, 10, 3)
        r2 = range(2, 10, 3)
        self.assertTrue(r1 != r2)

    def test_dunder_ne_different_stop_returns_false(self):
        r1 = range(0, 10, 3)
        r2 = range(0, 11, 3)
        self.assertFalse(r1 != r2)

    def test_dunder_ne_different_step_length_one_returns_false(self):
        r1 = range(0, 4, 10)
        r2 = range(0, 4, 11)
        self.assertFalse(r1 != r2)

    def test_dunder_ne_different_step_returns_true(self):
        r1 = range(0, 14, 10)
        r2 = range(0, 14, 11)
        self.assertTrue(r1 != r2)

    def test_dunder_new_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            range.__new__(2, 1)
        self.assertEqual(
            str(context.exception), "range.__new__(X): X is not a type object (int)"
        )

    def test_dunder_new_with_str_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            range.__new__(str, 5)
        self.assertEqual(
            str(context.exception), "range.__new__(str): str is not a subtype of range"
        )

    def test_dunder_new_with_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            range("2")
        self.assertEqual(
            str(context.exception), "'str' object cannot be interpreted as an integer"
        )

    def test_dunder_new_with_zero_step_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            range(1, 2, 0)
        self.assertEqual(str(context.exception), "range() arg 3 must not be zero")

    def test_dunder_new_calls_dunder_index(self):
        class Foo:
            def __index__(self):
                return 10

        obj = range(10)
        self.assertEqual(obj.start, 0)
        self.assertEqual(obj.stop, 10)
        self.assertEqual(obj.step, 1)

    def test_dunder_new_stores_int_subclasses(self):
        class Foo(int):
            pass

        class Bar:
            def __index__(self):
                return Foo(10)

        warnings.filterwarnings(
            action="ignore",
            category=DeprecationWarning,
            message="__index__ returned non-int.*",
            module=__name__,
        )
        obj = range(Foo(2), Bar())
        self.assertEqual(type(obj.start), Foo)
        self.assertEqual(type(obj.stop), Foo)
        self.assertEqual(obj.start, 2)
        self.assertEqual(obj.stop, 10)
        self.assertEqual(obj.step, 1)

    def test_dunder_new_with_one_arg_sets_stop(self):
        obj = range(10)
        self.assertEqual(obj.start, 0)
        self.assertEqual(obj.stop, 10)
        self.assertEqual(obj.step, 1)

    def test_dunder_new_with_two_args_sets_start_and_stop(self):
        obj = range(10, 11)
        self.assertEqual(obj.start, 10)
        self.assertEqual(obj.stop, 11)
        self.assertEqual(obj.step, 1)

    def test_dunder_new_with_three_args_sets_all(self):
        obj = range(10, 11, 12)
        self.assertEqual(obj.start, 10)
        self.assertEqual(obj.stop, 11)
        self.assertEqual(obj.step, 12)

    def test_start_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).start = 2

    def test_step_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).step = 2

    def test_stop_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).stop = 2


class RangeIteratorTests(unittest.TestCase):
    def test_dunder_iter_returns_self(self):
        it = iter(range(100))
        self.assertEqual(iter(it), it)

    def test_dunder_length_hint_returns_pending_length(self):
        len = 100
        it = iter(range(len))
        self.assertEqual(it.__length_hint__(), len)
        it.__next__()
        self.assertEqual(it.__length_hint__(), len - 1)

    def test_dunder_next_returns_ints(self):
        it = iter(range(10, 5, -2))
        self.assertEqual(it.__next__(), 10)
        self.assertEqual(it.__next__(), 8)
        self.assertEqual(it.__next__(), 6)
        with self.assertRaises(StopIteration):
            it.__next__()


class ReversedTests(unittest.TestCase):
    def test_reversed_iterates_backwards_over_iterable(self):
        it = reversed([1, 2, 3])
        self.assertEqual(it.__next__(), 3)
        self.assertEqual(it.__next__(), 2)
        self.assertEqual(it.__next__(), 1)
        with self.assertRaises(StopIteration):
            it.__next__()

    def test_reversed_calls_dunder_reverse(self):
        class C:
            def __reversed__(self):
                return "foo"

        self.assertEqual(reversed(C()), "foo")

    def test_reversed_with_none_dunder_reverse_raises_type_error(self):
        class C:
            __reversed__ = None

        with self.assertRaises(TypeError) as context:
            reversed(C())
        self.assertEqual(str(context.exception), "'C' object is not reversible")

    def test_reversed_with_non_callable_dunder_reverse_raises_type_error(self):
        class C:
            __reversed__ = 1

        with self.assertRaises(TypeError) as context:
            reversed(C())
        self.assertEqual(str(context.exception), "'int' object is not callable")

    def test_reversed_length_hint(self):
        it = reversed([1, 2, 3])
        self.assertEqual(it.__length_hint__(), 3)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 2)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 1)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 0)


class RoundTests(unittest.TestCase):
    def test_round_calls_dunder_round(self):
        class Roundable:
            def __init__(self, value):
                self.value = value

            def __round__(self, ndigits="a default value"):
                return (self.value, ndigits)

        self.assertEqual(round(Roundable(12), 34), (12, 34))
        self.assertEqual(round(Roundable(56)), (56, "a default value"))

    def test_round_raises_type_error(self):
        class ClassWithoutDunderRound:
            pass

        with self.assertRaises(TypeError):
            round(ClassWithoutDunderRound())


class SeqTests(unittest.TestCase):
    def test_sequence_is_iterable(self):
        class A:
            def __getitem__(self, index):
                return [1, 2, 3][index]

        self.assertEqual([x for x in A()], [1, 2, 3])

    def test_sequence_iter_is_itself(self):
        class A:
            def __getitem__(self, index):
                return [1, 2, 3][index]

        a = iter(A())
        self.assertEqual(a, a.__iter__())

    def test_non_iter_non_sequence_with_iter_raises_type_error(self):
        class NonIter:
            pass

        with self.assertRaises(TypeError) as context:
            iter(NonIter())

        self.assertEqual(str(context.exception), "'NonIter' object is not iterable")

    def test_non_iter_non_sequence_with_for_raises_type_error(self):
        class NonIter:
            pass

        with self.assertRaises(TypeError) as context:
            [x for x in NonIter()]

        self.assertEqual(str(context.exception), "'NonIter' object is not iterable")

    def test_sequence_with_error_raising_iter_descriptor_raises_type_error(self):
        dunder_get_called = False

        class Desc:
            def __get__(self, obj, type):
                nonlocal dunder_get_called
                dunder_get_called = True
                raise UserWarning("Nope")

        class C:
            __iter__ = Desc()

        with self.assertRaises(TypeError) as context:
            [x for x in C()]

        self.assertEqual(str(context.exception), "'C' object is not iterable")
        self.assertTrue(dunder_get_called)

    def test_sequence_with_error_raising_getitem_descriptor_returns_iter(self):
        dunder_get_called = False

        class Desc:
            def __get__(self, obj, type):
                nonlocal dunder_get_called
                dunder_get_called = True
                raise UserWarning("Nope")

        class C:
            __getitem__ = Desc()

        i = iter(C())
        self.assertTrue(hasattr(i, "__next__"))
        self.assertFalse(dunder_get_called)


class SetTests(unittest.TestCase):
    def test_dunder_or_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            set.__or__(frozenset(), set())
        self.assertIn(
            "'__or__' requires a 'set' object but received a 'frozenset'",
            str(context.exception),
        )

    def test_difference_no_others_copies_self(self):
        a_set = {1, 2, 3}
        self.assertIsNot(set.difference(a_set), a_set)

    def test_difference_same_sets_returns_empty_set(self):
        a_set = {1, 2, 3}
        self.assertFalse(set.difference(a_set, a_set))

    def test_difference_two_sets_returns_difference(self):
        set1 = {1, 2, 3, 4, 5, 6, 7}
        set2 = {1, 3, 5, 7}
        self.assertEqual(set.difference(set1, set2), {2, 4, 6})

    def test_difference_many_sets_returns_difference(self):
        a_set = {1, 10, 100, 1000}
        self.assertEqual(set.difference(a_set, {10}, {100}, {1000}), {1})

    def test_discard_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.discard(None, 1)

    def test_discard_with_non_member_returns_none(self):
        self.assertIs(set.discard(set(), 1), None)

    def test_discard_with_member_removes_element(self):
        s = {1, 2, 3}
        self.assertIn(1, s)
        self.assertIs(set.discard(s, 1), None)
        self.assertNotIn(1, s)

    def test_remove_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.remove(None, 1)

    def test_remove_with_non_member_raises_key_error(self):
        with self.assertRaises(KeyError):
            set.remove(set(), 1)

    def test_remove_with_member_removes_element(self):
        s = {1, 2, 3}
        self.assertIn(1, s)
        set.remove(s, 1)
        self.assertNotIn(1, s)

    def test_inplace_with_non_set_raises_type_error(self):
        a = frozenset()
        self.assertRaises(TypeError, set.__ior__, a, set())

    def test_inplace_with_non_set_as_other_returns_unimplemented(self):
        a = set()
        result = set.__ior__(a, 1)
        self.assertEqual(len(a), 0)
        self.assertIs(result, NotImplemented)

    def test_inplace_or_modifies_self(self):
        a = set()
        b = {"foo"}
        result = set.__ior__(a, b)
        self.assertIs(result, a)
        self.assertEqual(len(a), 1)
        self.assertIn("foo", a)

    def test_sub_returns_difference(self):
        self.assertEqual(set.__sub__({1, 2}, set()), {1, 2})
        self.assertEqual(set.__sub__({1, 2}, {1}), {2})
        self.assertEqual(set.__sub__({1, 2}, {1, 2}), set())

    def test_sub_with_non_set_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.__sub__("not a set", set())
        with self.assertRaises(TypeError):
            set.__sub__("not a set", "also not a set")
        with self.assertRaises(TypeError):
            set.__sub__(frozenset(), set())

    def test_sub_with_non_set_other_returns_not_implemented(self):
        self.assertEqual(set.__sub__(set(), "not a set"), NotImplemented)

    def test_union_with_non_set_as_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            set.union(frozenset(), set())

    def test_union_with_set_returns_union(self):
        set1 = {1, 2}
        set2 = {2, 3}
        set3 = {4, 5}
        self.assertEqual(set.union(set1, set2, set3), {1, 2, 3, 4, 5})

    def test_union_with_self_returns_copy(self):
        a_set = {1, 2, 3}
        self.assertIsNot(set.union(a_set), a_set)
        self.assertIsNot(set.union(a_set, a_set), a_set)

    def test_union_with_iterable_contains_iterable_items(self):
        a_set = {1, 2}
        a_dict = {2: True, 3: True}
        self.assertEqual(set.union(a_set, a_dict), {1, 2, 3})

    def test_union_with_custom_iterable(self):
        class C:
            def __init__(self, start, end):
                self.pos = start
                self.end = end

            def __iter__(self):
                return self

            def __next__(self):
                if self.pos == self.end:
                    raise StopIteration
                result = self.pos
                self.pos += 1
                return result

        self.assertEqual(set.union(set(), C(1, 3), C(6, 9)), {1, 2, 6, 7, 8})


class StaticMethodTests(unittest.TestCase):
    def test_dunder_abstractmethod_with_missing_attr_returns_false(self):
        def foo():
            pass

        method = staticmethod(foo)
        self.assertIs(method.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_false_attr_returns_false(self):
        def foo():
            pass

        foo.__isabstractmethod__ = False
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        method = staticmethod(foo)
        self.assertIs(method.__isabstractmethod__, False)

    def test_dunder_abstractmethod_with_abstract_returns_true(self):
        def foo():
            pass

        foo.__isabstractmethod__ = 10  # non-zero is True
        with self.assertRaises(AttributeError):
            type(foo).__isabstractmethod__
        method = staticmethod(foo)
        self.assertIs(method.__isabstractmethod__, True)


class StrTests(unittest.TestCase):
    def test_dunder_new_with_raising_dunder_str_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise AttributeError("failed")

        class Foo:
            __str__ = Desc()

        foo = Foo()
        with self.assertRaises(AttributeError) as context:
            str(foo)
        self.assertEqual(str(context.exception), "failed")

    def test_capitalize_with_non_str_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.capitalize(1)
        self.assertIn(
            "'capitalize' requires a 'str' object but received a 'int'",
            str(context.exception),
        )

    def test_count_with_non_str_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.count(None, "foo")

    def test_count_with_non_str_other_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.count("foo", None)

    def test_count_calls_dunder_index_on_start(self):
        class C:
            def __index__(self):
                raise UserWarning("foo")

        c = C()
        with self.assertRaises(UserWarning):
            str.count("foo", "bar", c, 100)

    def test_count_calls_dunder_index_on_end(self):
        class C:
            def __index__(self):
                raise UserWarning("foo")

        c = C()
        with self.assertRaises(UserWarning):
            str.count("foo", "bar", 0, c)

    def test_count_returns_number_of_occurrences(self):
        self.assertEqual("foo".count("o"), 2)

    def test_format_single_open_curly_brace_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.format("{")
        self.assertEqual(
            str(context.exception), "Single '{' encountered in format string"
        )

    def test_format_single_close_curly_brace_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.format("}")
        self.assertEqual(
            str(context.exception), "Single '}' encountered in format string"
        )

    def test_format_index_out_of_args_raises_index_error(self):
        with self.assertRaises(IndexError) as context:
            str.format("{1}", 4)
        self.assertEqual(str(context.exception), "tuple index out of range")

    def test_format_not_existing_key_in_kwargs_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            str.format("{a}", b=4)
        self.assertEqual(str(context.exception), "'a'")

    def test_format_not_index_field_not_in_kwargs_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            str.format("{-1}", b=4)
        self.assertEqual(str(context.exception), "'-1'")

    def test_format_str_without_field_returns_itself(self):
        result = str.format("no field added")
        self.assertEqual(result, "no field added")

    def test_format_double_open_curly_braces_returns_single(self):
        result = str.format("{{")
        self.assertEqual(result, "{")

    def test_format_double_close_open_curly_braces_returns_single(self):
        result = str.format("}}")
        self.assertEqual(result, "}")

    def test_format_auto_index_field_returns_str_replaced_for_matching_args(self):
        result = str.format("a{}b{}c", 0, 1)
        self.assertEqual(result, "a0b1c")

    def test_format_auto_index_field_with_explicit_index_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.format("a{}b{0}c", 0)
        self.assertEqual(
            str(context.exception),
            "cannot switch from automatic field numbering to manual "
            "field specification",
        )

    def test_format_auto_index_field_with_keyword_returns_formatted_str(self):
        result = str.format("a{}b{keyword}c{}d", 0, 1, keyword=888)
        self.assertEqual(result, "a0b888c1d")

    def test_format_explicit_index_fields_returns_formatted_str(self):
        result = str.format("a{2}b{1}c{0}d{1}e", 0, 1, 2)
        self.assertEqual(result, "a2b1c0d1e")

    def test_format_keyword_fields_returns_formatted_str(self):
        result = str.format("1{a}2{b}3{c}4{b}5", a="a", b="b", c="c")
        self.assertEqual(result, "1a2b3c4b5")

    def test_isalnum_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isalnum(None)
        self.assertIn(
            "'isalnum' requires a 'str' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_islower_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.islower(None)
        self.assertIn(
            "'islower' requires a 'str' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_isupper_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isupper(None)
        self.assertIn(
            "'isupper' requires a 'str' object but received a 'NoneType'",
            str(context.exception),
        )

    def test_join_with_raising_descriptor_dunder_iter_raises_type_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("foo")

        class C:
            __iter__ = Desc()

        with self.assertRaises(TypeError):
            str.join("", C())

    def test_join_with_non_string_in_items_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.join(",", ["hello", 1])
        self.assertEqual(
            str(context.exception), "sequence item 1: expected str instance, int found"
        )

    def test_join_with_non_string_separator_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.join(None, ["hello", "world"])
        self.assertTrue(
            str(context.exception).endswith(
                "'join' requires a 'str' object but received a 'NoneType'"
            )
        )

    def test_join_with_empty_items_returns_empty_string(self):
        result = str.join(",", ())
        self.assertEqual(result, "")

    def test_join_with_one_item_returns_item(self):
        result = str.join(",", ("foo",))
        self.assertEqual(result, "foo")

    def test_join_with_empty_separator_uses_empty_string(self):
        result = str.join("", ("1", "2", "3"))
        self.assertEqual(result, "123")

    def test_join_with_tuple_joins_elements(self):
        result = str.join(",", ("1", "2", "3"))
        self.assertEqual(result, "1,2,3")

    def test_join_with_tuple_subclass_calls_dunder_iter(self):
        class C(tuple):
            def __iter__(self):
                return ("a", "b", "c").__iter__()

        elements = C(("p", "q", "r"))
        result = "".join(elements)
        self.assertEqual(result, "abc")

    def test_splitlines_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.splitlines(None)

    def test_splitlines_with_float_keepends_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.splitlines("hello", 0.4)

    def test_splitlines_with_non_int_calls_dunder_int_non_bool(self):
        class C:
            def __int__(self):
                return 100

        self.assertEqual(str.splitlines("\n", C()), ["\n"])

    def test_splitlines_with_non_int_calls_dunder_int_true(self):
        class C:
            def __int__(self):
                return 1

        self.assertEqual(str.splitlines("\n", C()), ["\n"])

    def test_splitlines_with_non_int_calls_dunder_int_false(self):
        class C:
            def __int__(self):
                return 0

        self.assertEqual(str.splitlines("\n", C()), [""])

    def test_str_new_with_bytes_and_no_encoding_returns_str(self):
        decoded = str(b"abc")
        self.assertEqual(decoded, "b'abc'")

    def test_str_new_with_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str("", encoding="utf_8")

    def test_str_new_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            str(1, encoding="utf_8")

    def test_str_new_with_bytes_and_encoding_returns_decoded_str(self):
        decoded = str(b"abc", encoding="ascii")
        self.assertEqual(decoded, "abc")

    def test_str_new_with_no_object_and_encoding_returns_empty_string(self):
        self.assertEqual(str(encoding="ascii"), "")

    def test_str_new_with_class_without_dunder_str_returns_str(self):
        class A:
            def __repr__(self):
                return "test"

        self.assertEqual(str(A()), "test")

    def test_str_new_with_class_with_faulty_dunder_str_raises_type_error(self):
        with self.assertRaises(TypeError):

            class A:
                def __str__(self):
                    return 1

            str(A())

    def test_str_new_with_class_with_proper_duner_str_returns_str(self):
        class A:
            def __str__(self):
                return "test"

        self.assertEqual(str(A()), "test")


class StrModTests(unittest.TestCase):
    def test_empty_format_returns_empty_string(self):
        self.assertEqual("" % (), "")

    def test_simple_string_returns_string(self):
        self.assertEqual("foo bar (}" % (), "foo bar (}")

    def test_with_non_tuple_args_returns_string(self):
        self.assertEqual("%s" % "foo", "foo")
        self.assertEqual("%d" % 42, "42")
        self.assertEqual("%s" % {"foo": "bar"}, "{'foo': 'bar'}")

    def test_with_named_args_returns_string(self):
        self.assertEqual("%(foo)s %(bar)d" % {"foo": "ho", "bar": 42}, "ho 42")
        self.assertEqual("%()x" % {"": 123}, "7b")
        self.assertEqual(")%(((()) ()))s(" % {"((()) ())": 99}, ")99(")
        self.assertEqual("%(%s)s" % {"%s": -5}, "-5")

    def test_with_custom_mapping_returns_string(self):
        class C:
            def __getitem__(self, key):
                return "getitem called with " + key

        self.assertEqual("%(foo)s" % C(), "getitem called with foo")

    def test_with_custom_mapping_propagates_errors(self):
        with self.assertRaises(KeyError) as context:
            "%(foo)s" % {}
        self.assertEqual(str(context.exception), "'foo'")

        class C:
            def __getitem__(self, key):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            "%(foo)s" % C()

    def test_without_mapping_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "%(foo)s" % None
        self.assertEqual(str(context.exception), "format requires a mapping")
        with self.assertRaises(TypeError) as context:
            "%(foo)s" % "foobar"
        self.assertEqual(str(context.exception), "format requires a mapping")
        with self.assertRaises(TypeError) as context:
            "%(foo)s" % ("foobar",)
        self.assertEqual(str(context.exception), "format requires a mapping")

    def test_positional_after_named_arg_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "%(foo)s %s" % {"foo": "bar"}
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )

    def test_mix_named_and_tuple_args_returns_string(self):
        self.assertEqual("%s %(a)s" % {"a": 77}, "{'a': 77} 77")

    def test_mapping_in_tuple_returns_string(self):
        self.assertEqual("%s" % ({"foo": "bar"},), "{'foo': 'bar'}")

    def test_c_format_returns_string(self):
        self.assertEqual("%c" % ("x",), "x")
        self.assertEqual("%c" % ("\U0001f44d",), "\U0001f44d")
        self.assertEqual("%c" % (76,), "L")
        self.assertEqual("%c" % (0x1F40D,), "\U0001f40d")

    def test_c_format_with_non_int_returns_string(self):
        class C:
            def __index__(self):
                return 42

        self.assertEqual("%c" % (C(),), "*")

    def test_c_format_raises_overflow_error(self):
        import sys

        maxunicode_range = "range(0x%x)" % (sys.maxunicode + 1)
        with self.assertRaises(OverflowError) as context:
            "%c" % (sys.maxunicode + 1,)
        self.assertEqual(str(context.exception), f"%c arg not in {maxunicode_range}")
        with self.assertRaises(OverflowError) as context:
            "%c" % (-1,)
        self.assertEqual(str(context.exception), f"%c arg not in {maxunicode_range}")

    def test_c_format_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "%c" % (None,)
        self.assertEqual(str(context.exception), f"%c requires int or char")
        with self.assertRaises(TypeError) as context:
            "%c" % ("ab",)
        self.assertEqual(str(context.exception), f"%c requires int or char")
        with self.assertRaises(TypeError) as context:
            "%c" % (123456789012345678901234567890,)
        self.assertEqual(str(context.exception), f"%c requires int or char")

        class C:
            def __index__(self):
                raise UserWarning()

        with self.assertRaises(TypeError) as context:
            "%c" % (C(),)
        self.assertEqual(str(context.exception), f"%c requires int or char")

    def test_s_format_returns_string(self):
        self.assertEqual("%s" % ("foo",), "foo")

        class C:
            __repr__ = None

            def __str__(self):
                return "str called"

        self.assertEqual("%s" % (C(),), "str called")

    def test_s_format_propagates_errors(self):
        class C:
            def __str__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            "%s" % (C(),)

    def test_r_format_returns_string(self):
        self.assertEqual("%r" % (42,), "42")
        self.assertEqual("%r" % ("foo",), "'foo'")
        self.assertEqual(
            "%r" % ({"foo": "\U0001d4eb\U0001d4ea\U0001d4fb"},),
            "{'foo': '\U0001d4eb\U0001d4ea\U0001d4fb'}",
        )

        class C:
            def __repr__(self):
                return "repr called"

            __str__ = None

        self.assertEqual("%r" % (C(),), "repr called")

    def test_r_format_propagates_errors(self):
        class C:
            def __repr__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            "%r" % (C(),)

    def test_a_format_returns_string(self):
        self.assertEqual("%a" % (42,), "42")
        self.assertEqual("%a" % ("foo",), "'foo'")

        class C:
            def __repr__(self):
                return "repr called"

            __str__ = None

        self.assertEqual("%a" % (C(),), "repr called")
        # TODO(T39861344, T38702699): We should have a test with some non-ascii
        # characters here proving that they are escaped. Unfortunately
        # builtins.ascii() does not work in that case yet.

    def test_a_format_propagates_errors(self):
        class C:
            def __repr__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            "%a" % (C(),)

    def test_diu_format_returns_string(self):
        self.assertEqual("%d" % (0,), "0")
        self.assertEqual("%d" % (-1,), "-1")
        self.assertEqual("%d" % (42,), "42")
        self.assertEqual("%i" % (0,), "0")
        self.assertEqual("%i" % (-1,), "-1")
        self.assertEqual("%i" % (42,), "42")
        self.assertEqual("%u" % (0,), "0")
        self.assertEqual("%u" % (-1,), "-1")
        self.assertEqual("%u" % (42,), "42")

    def test_diu_format_with_largeint_returns_string(self):
        self.assertEqual(
            "%d" % (-123456789012345678901234567890), "-123456789012345678901234567890"
        )
        self.assertEqual(
            "%i" % (-123456789012345678901234567890), "-123456789012345678901234567890"
        )
        self.assertEqual(
            "%u" % (-123456789012345678901234567890), "-123456789012345678901234567890"
        )

    def test_diu_format_with_non_int_returns_string(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise UserWarning()

        self.assertEqual("%d" % (C(),), "42")
        self.assertEqual("%i" % (C(),), "42")
        self.assertEqual("%u" % (C(),), "42")

    def test_diu_format_raises_typeerrors(self):
        with self.assertRaises(TypeError) as context:
            "%d" % (None,)
        self.assertEqual(
            str(context.exception), "%d format: a number is required, not NoneType"
        )
        with self.assertRaises(TypeError) as context:
            "%i" % (None,)
        self.assertEqual(
            str(context.exception), "%i format: a number is required, not NoneType"
        )
        with self.assertRaises(TypeError) as context:
            "%u" % (None,)
        self.assertEqual(
            str(context.exception), "%u format: a number is required, not NoneType"
        )

    def test_diu_format_propagates_errors(self):
        class C:
            def __int__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            "%d" % (C(),)
        with self.assertRaises(UserWarning):
            "%i" % (C(),)
        with self.assertRaises(UserWarning):
            "%u" % (C(),)

    def test_diu_format_reraises_typerrors(self):
        class C:
            def __int__(self):
                raise TypeError("foobar")

        with self.assertRaises(TypeError) as context:
            "%d" % (C(),)
        self.assertEqual(
            str(context.exception), "%d format: a number is required, not C"
        )
        with self.assertRaises(TypeError) as context:
            "%i" % (C(),)
        self.assertEqual(
            str(context.exception), "%i format: a number is required, not C"
        )
        with self.assertRaises(TypeError) as context:
            "%u" % (C(),)
        self.assertEqual(
            str(context.exception), "%u format: a number is required, not C"
        )

    def test_xX_format_returns_string(self):
        self.assertEqual("%x" % (0,), "0")
        self.assertEqual("%x" % (-123,), "-7b")
        self.assertEqual("%x" % (42,), "2a")
        self.assertEqual("%X" % (0,), "0")
        self.assertEqual("%X" % (-123,), "-7B")
        self.assertEqual("%X" % (42,), "2A")

    def test_xX_format_with_largeint_returns_string(self):
        self.assertEqual(
            "%x" % (-123456789012345678901234567890), "-18ee90ff6c373e0ee4e3f0ad2"
        )
        self.assertEqual(
            "%X" % (-123456789012345678901234567890), "-18EE90FF6C373E0EE4E3F0AD2"
        )

    def test_xX_format_with_non_int_returns_string(self):
        class C:
            def __float__(self):
                return 3.3

            def __index__(self):
                return 77

        self.assertEqual("%x" % (C(),), "4d")
        self.assertEqual("%X" % (C(),), "4D")

    def test_xX_format_raises_typeerrors(self):
        with self.assertRaises(TypeError) as context:
            "%x" % (None,)
        self.assertEqual(
            str(context.exception), "%x format: an integer is required, not NoneType"
        )
        with self.assertRaises(TypeError) as context:
            "%X" % (None,)
        self.assertEqual(
            str(context.exception), "%X format: an integer is required, not NoneType"
        )

    def test_xX_format_propagates_errors(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            "%x" % (C(),)
        with self.assertRaises(UserWarning):
            "%X" % (C(),)

    def test_xX_format_reraises_typerrors(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise TypeError("foobar")

        with self.assertRaises(TypeError) as context:
            "%x" % (C(),)
        self.assertEqual(
            str(context.exception), "%x format: an integer is required, not C"
        )
        with self.assertRaises(TypeError) as context:
            "%X" % (C(),)
        self.assertEqual(
            str(context.exception), "%X format: an integer is required, not C"
        )

    def test_o_format_returns_string(self):
        self.assertEqual("%o" % (0,), "0")
        self.assertEqual("%o" % (-123,), "-173")
        self.assertEqual("%o" % (42,), "52")

    def test_o_format_with_largeint_returns_string(self):
        self.assertEqual(
            "%o" % (-123456789012345678901234567890),
            "-143564417755415637016711617605322",
        )

    def test_o_format_with_non_int_returns_string(self):
        class C:
            def __float__(self):
                return 3.3

            def __index__(self):
                return 77

        self.assertEqual("%o" % (C(),), "115")

    def test_o_format_raises_typeerrors(self):
        with self.assertRaises(TypeError) as context:
            "%o" % (None,)
        self.assertEqual(
            str(context.exception), "%o format: an integer is required, not NoneType"
        )

    def test_o_format_propagates_errors(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            "%o" % (C(),)

    def test_o_format_reraises_typerrors(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise TypeError("foobar")

        with self.assertRaises(TypeError) as context:
            "%o" % (C(),)
        self.assertEqual(
            str(context.exception), "%o format: an integer is required, not C"
        )

    def test_f_format_returns_string(self):
        self.assertEqual("%f" % (0.0,), "0.000000")
        self.assertEqual("%f" % (-0.0,), "-0.000000")
        self.assertEqual("%f" % (1.0,), "1.000000")
        self.assertEqual("%f" % (-1.0,), "-1.000000")
        self.assertEqual("%f" % (42.125), "42.125000")

        self.assertEqual("%f" % (1e3,), "1000.000000")
        self.assertEqual("%f" % (1e6,), "1000000.000000")
        self.assertEqual(
            "%f" % (1e40,), "10000000000000000303786028427003666890752.000000"
        )

    def test_F_format_returns_string(self):
        self.assertEqual("%F" % (42.125), "42.125000")

    def test_e_format_returns_string(self):
        self.assertEqual("%e" % (0.0,), "0.000000e+00")
        self.assertEqual("%e" % (-0.0,), "-0.000000e+00")
        self.assertEqual("%e" % (1.0,), "1.000000e+00")
        self.assertEqual("%e" % (-1.0,), "-1.000000e+00")
        self.assertEqual("%e" % (42.125), "4.212500e+01")

        self.assertEqual("%e" % (1e3,), "1.000000e+03")
        self.assertEqual("%e" % (1e6,), "1.000000e+06")
        self.assertEqual("%e" % (1e40,), "1.000000e+40")

    def test_E_format_returns_string(self):
        self.assertEqual("%E" % (1.0,), "1.000000E+00")

    def test_g_format_returns_string(self):
        self.assertEqual("%g" % (0.0,), "0")
        self.assertEqual("%g" % (-1.0,), "-1")
        self.assertEqual("%g" % (0.125,), "0.125")
        self.assertEqual("%g" % (3.5,), "3.5")

    def test_eEfFgG_format_with_inf_returns_string(self):
        self.assertEqual("%e" % (float("inf"),), "inf")
        self.assertEqual("%E" % (float("inf"),), "INF")
        self.assertEqual("%f" % (float("inf"),), "inf")
        self.assertEqual("%F" % (float("inf"),), "INF")
        self.assertEqual("%g" % (float("inf"),), "inf")
        self.assertEqual("%G" % (float("inf"),), "INF")

        self.assertEqual("%e" % (-float("inf"),), "-inf")
        self.assertEqual("%E" % (-float("inf"),), "-INF")
        self.assertEqual("%f" % (-float("inf"),), "-inf")
        self.assertEqual("%F" % (-float("inf"),), "-INF")
        self.assertEqual("%g" % (-float("inf"),), "-inf")
        self.assertEqual("%G" % (-float("inf"),), "-INF")

    def test_eEfFgG_format_with_nan_returns_string(self):
        self.assertEqual("%e" % (float("nan"),), "nan")
        self.assertEqual("%E" % (float("nan"),), "NAN")
        self.assertEqual("%f" % (float("nan"),), "nan")
        self.assertEqual("%F" % (float("nan"),), "NAN")
        self.assertEqual("%g" % (float("nan"),), "nan")
        self.assertEqual("%G" % (float("nan"),), "NAN")

        self.assertEqual("%e" % (float("-nan"),), "nan")
        self.assertEqual("%E" % (float("-nan"),), "NAN")
        self.assertEqual("%f" % (float("-nan"),), "nan")
        self.assertEqual("%F" % (float("-nan"),), "NAN")
        self.assertEqual("%g" % (float("-nan"),), "nan")
        self.assertEqual("%G" % (float("-nan"),), "NAN")

    def test_f_format_with_precision_returns_string(self):
        number = 1.23456789123456789
        self.assertEqual("%.0f" % number, "1")
        self.assertEqual("%.1f" % number, "1.2")
        self.assertEqual("%.2f" % number, "1.23")
        self.assertEqual("%.3f" % number, "1.235")
        self.assertEqual("%.4f" % number, "1.2346")
        self.assertEqual("%.5f" % number, "1.23457")
        self.assertEqual("%.6f" % number, "1.234568")
        self.assertEqual("%f" % number, "1.234568")

        self.assertEqual("%.17f" % number, "1.23456789123456789")
        self.assertEqual("%.25f" % number, "1.2345678912345678934769921")
        self.assertEqual(
            "%.60f" % number,
            "1.234567891234567893476992139767389744520187377929687500000000",
        )

    def test_eEfFgG_format_with_precision_returns_string(self):
        number = 1.23456789123456789
        self.assertEqual("%.0e" % number, "1e+00")
        self.assertEqual("%.0E" % number, "1E+00")
        self.assertEqual("%.0f" % number, "1")
        self.assertEqual("%.0F" % number, "1")
        self.assertEqual("%.0g" % number, "1")
        self.assertEqual("%.0G" % number, "1")
        self.assertEqual("%.4e" % number, "1.2346e+00")
        self.assertEqual("%.4E" % number, "1.2346E+00")
        self.assertEqual("%.4f" % number, "1.2346")
        self.assertEqual("%.4F" % number, "1.2346")
        self.assertEqual("%.4g" % number, "1.235")
        self.assertEqual("%.4G" % number, "1.235")
        self.assertEqual("%e" % number, "1.234568e+00")
        self.assertEqual("%E" % number, "1.234568E+00")
        self.assertEqual("%f" % number, "1.234568")
        self.assertEqual("%F" % number, "1.234568")
        self.assertEqual("%g" % number, "1.23457")
        self.assertEqual("%G" % number, "1.23457")

    def test_g_format_with_flags_and_width_returns_string(self):
        self.assertEqual("%5g" % 7.0, "    7")
        self.assertEqual("%5g" % 7.2, "  7.2")
        self.assertEqual("% 5g" % 7.2, "  7.2")
        self.assertEqual("%+5g" % 7.2, " +7.2")
        self.assertEqual("%5g" % -7.2, " -7.2")
        self.assertEqual("% 5g" % -7.2, " -7.2")
        self.assertEqual("%+5g" % -7.2, " -7.2")

        self.assertEqual("%-5g" % 7.0, "7    ")
        self.assertEqual("%-5g" % 7.2, "7.2  ")
        self.assertEqual("%- 5g" % 7.2, " 7.2 ")
        self.assertEqual("%-+5g" % 7.2, "+7.2 ")
        self.assertEqual("%-5g" % -7.2, "-7.2 ")
        self.assertEqual("%- 5g" % -7.2, "-7.2 ")
        self.assertEqual("%-+5g" % -7.2, "-7.2 ")

        self.assertEqual("%#g" % 7.0, "7.00000")

        self.assertEqual("%#- 7.2g" % float("-nan"), " nan   ")
        self.assertEqual("%#- 7.2g" % float("inf"), " inf   ")
        self.assertEqual("%#- 7.2g" % float("-inf"), "-inf   ")

    def test_eEfFgG_format_with_flags_and_width_returns_string(self):
        number = 1.23456789123456789
        self.assertEqual("% -#12.3e" % number, " 1.235e+00  ")
        self.assertEqual("% -#12.3E" % number, " 1.235E+00  ")
        self.assertEqual("% -#12.3f" % number, " 1.235      ")
        self.assertEqual("% -#12.3F" % number, " 1.235      ")
        self.assertEqual("% -#12.3g" % number, " 1.23       ")
        self.assertEqual("% -#12.3G" % number, " 1.23       ")

    def test_ef_format_with_non_float_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "%e" % (None,)
        self.assertEqual(str(context.exception), "must be real number, not NoneType")

        class C:
            def __float__(self):
                return "not a float"

        with self.assertRaises(TypeError) as context:
            "%f" % (C(),)
        self.assertEqual(
            str(context.exception), "C.__float__ returned non-float (type str)"
        )

    def test_g_format_propogates_errors(self):
        class C:
            def __float__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            "%g" % (C(),)

    def test_efg_format_with_non_float_returns_string(self):
        class A(float):
            pass

        self.assertEqual("%e" % (A(9.625),), "%e" % (9.625,))

        class C:
            def __float__(self):
                return 3.5

        self.assertEqual("%f" % (C(),), "%f" % (3.5,))

        class D:
            def __float__(self):
                return A(-12.75)

        warnings.filterwarnings(
            action="ignore",
            category=DeprecationWarning,
            message=".*__float__ returned non-float.*",
            module=__name__,
        )
        self.assertEqual("%g" % (D(),), "%g" % (-12.75,))

    def test_percent_format_returns_percent(self):
        self.assertEqual("%%" % (), "%")

    def test_percent_with_flags_percision_and_width_returns_percent(self):
        self.assertEqual("%0.0%" % (), "%")
        self.assertEqual("%*.%" % (42,), "%")
        self.assertEqual("%.*%" % (88,), "%")
        self.assertEqual("%0#*.42%" % (1234,), "%")

    def test_flags_get_accepted(self):
        self.assertEqual("%-s" % "", "")
        self.assertEqual("%+s" % "", "")
        self.assertEqual("% s" % "", "")
        self.assertEqual("%#s" % "", "")
        self.assertEqual("%0s" % "", "")
        self.assertEqual("%#-#0+ -s" % "", "")

    def test_string_format_with_width_returns_string(self):
        self.assertEqual("%5s" % "oh", "   oh")
        self.assertEqual("%-5s" % "ah", "ah   ")
        self.assertEqual("%05s" % "uh", "   uh")
        self.assertEqual("%-# 5s" % "eh", "eh   ")

        self.assertEqual("%0s" % "foo", "foo")
        self.assertEqual("%-0s" % "foo", "foo")
        self.assertEqual("%10s" % "hello world", "hello world")
        self.assertEqual("%-10s" % "hello world", "hello world")

    def test_string_format_with_width_star_returns_string(self):
        self.assertEqual("%*s" % (7, "foo"), "    foo")
        self.assertEqual("%*s" % (-7, "bar"), "bar    ")
        self.assertEqual("%-*s" % (7, "baz"), "baz    ")
        self.assertEqual("%-*s" % (-7, "bam"), "bam    ")

    def test_string_format_with_precision_returns_string(self):
        self.assertEqual("%.3s" % "python", "pyt")
        self.assertEqual("%.0s" % "python", "")
        self.assertEqual("%.10s" % "python", "python")

    def test_string_format_with_precision_star_returns_string(self):
        self.assertEqual("%.*s" % (3, "monty"), "mon")
        self.assertEqual("%.*s" % (0, "monty"), "")
        self.assertEqual("%.*s" % (-4, "monty"), "")

    def test_string_format_with_width_and_precision_returns_string(self):
        self.assertEqual("%8.3s" % ("foobar",), "     foo")
        self.assertEqual("%-8.3s" % ("foobar",), "foo     ")
        self.assertEqual("%*.3s" % (8, "foobar"), "     foo")
        self.assertEqual("%*.3s" % (-8, "foobar"), "foo     ")
        self.assertEqual("%8.*s" % (3, "foobar"), "     foo")
        self.assertEqual("%-8.*s" % (3, "foobar"), "foo     ")
        self.assertEqual("%*.*s" % (8, 3, "foobar"), "     foo")
        self.assertEqual("%-*.*s" % (8, 3, "foobar"), "foo     ")

    def test_s_r_a_c_formats_accept_flags_width_precision_return_strings(self):
        self.assertEqual("%-*.3s" % (8, "foobar"), "foo     ")
        self.assertEqual("%-*.3r" % (8, "foobar"), "'fo     ")
        self.assertEqual("%-*.3a" % (8, "foobar"), "'fo     ")
        self.assertEqual("%-*.3c" % (8, 94), "^       ")

    def test_number_format_with_sign_flag_returns_string(self):
        self.assertEqual("%+d" % (42,), "+42")
        self.assertEqual("%+d" % (-42,), "-42")
        self.assertEqual("% d" % (17,), " 17")
        self.assertEqual("% d" % (-17,), "-17")
        self.assertEqual("%+ d" % (42,), "+42")
        self.assertEqual("%+ d" % (-42,), "-42")
        self.assertEqual("% +d" % (17,), "+17")
        self.assertEqual("% +d" % (-17,), "-17")

    def test_number_format_alt_flag_returns_string(self):
        self.assertEqual("%#d" % (23,), "23")
        self.assertEqual("%#x" % (23,), "0x17")
        self.assertEqual("%#X" % (23,), "0X17")
        self.assertEqual("%#o" % (23,), "0o27")

    def test_number_format_with_width_returns_string(self):
        self.assertEqual("%5d" % (123,), "  123")
        self.assertEqual("%5d" % (-8,), "   -8")
        self.assertEqual("%-5d" % (123,), "123  ")
        self.assertEqual("%-5d" % (-8,), "-8   ")

        self.assertEqual("%05d" % (123,), "00123")
        self.assertEqual("%05d" % (-8,), "-0008")
        self.assertEqual("%-05d" % (123,), "123  ")
        self.assertEqual("%0-5d" % (-8,), "-8   ")

        self.assertEqual("%#7x" % (42,), "   0x2a")
        self.assertEqual("%#7x" % (-42,), "  -0x2a")

        self.assertEqual("%5d" % (123456,), "123456")
        self.assertEqual("%-5d" % (-123456,), "-123456")

    def test_number_format_with_precision_returns_string(self):
        self.assertEqual("%.5d" % (123,), "00123")
        self.assertEqual("%.5d" % (-123,), "-00123")
        self.assertEqual("%.5d" % (1234567,), "1234567")
        self.assertEqual("%#.5x" % (99,), "0x00063")

    def test_number_format_with_width_precision_flags_returns_string(self):
        self.assertEqual("%8.3d" % (12,), "     012")
        self.assertEqual("%8.3d" % (-7,), "    -007")
        self.assertEqual("%05.3d" % (12,), "00012")
        self.assertEqual("%+05.3d" % (12,), "+0012")
        self.assertEqual("% 05.3d" % (12,), " 0012")
        self.assertEqual("% 05.3x" % (19,), " 0013")

        self.assertEqual("%-8.3d" % (12,), "012     ")
        self.assertEqual("%-8.3d" % (-7,), "-007    ")
        self.assertEqual("%- 8.3d" % (66,), " 066    ")

    def test_width_and_precision_star_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "%*d" % (42,)
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            "%.*d" % (42,)
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            "%*.*d" % (42,)
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            "%*.*d" % (1, 2)
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )

    def test_negative_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            "%.-2s" % "foo"
        self.assertEqual(
            str(context.exception), "unsupported format character '-' (0x2d) at index 2"
        )

    def test_two_specifiers_returns_string(self):
        self.assertEqual("%s%s" % ("foo", "bar"), "foobar")
        self.assertEqual(",%s%s" % ("foo", "bar"), ",foobar")
        self.assertEqual("%s,%s" % ("foo", "bar"), "foo,bar")
        self.assertEqual("%s%s," % ("foo", "bar"), "foobar,")
        self.assertEqual(",%s..%s---" % ("foo", "bar"), ",foo..bar---")
        self.assertEqual(",%s...%s--" % ("foo", "bar"), ",foo...bar--")
        self.assertEqual(",,%s.%s---" % ("foo", "bar"), ",,foo.bar---")
        self.assertEqual(",,%s...%s-" % ("foo", "bar"), ",,foo...bar-")
        self.assertEqual(",,,%s..%s-" % ("foo", "bar"), ",,,foo..bar-")
        self.assertEqual(",,,%s.%s--" % ("foo", "bar"), ",,,foo.bar--")

    def test_mixed_specifiers_with_percents_returns_string(self):
        self.assertEqual("%%%s%%%s%%" % ("foo", "bar"), "%foo%bar%")

    def test_mixed_specifiers_returns_string(self):
        self.assertEqual("a %d %g %s" % (123, 3.14, "baz"), "a 123 3.14 baz")

    def test_specifier_missing_format_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            "%" % ()
        self.assertEqual(str(context.exception), "incomplete format")
        with self.assertRaises(ValueError) as context:
            "%(foo)" % {"foo": None}
        self.assertEqual(str(context.exception), "incomplete format")

    def test_unknown_specifier_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            "try %Y" % (42)
        self.assertEqual(
            str(context.exception), "unsupported format character 'Y' (0x59) at index 5"
        )

    def test_too_few_args_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "%s%s" % ("foo",)
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )

    def test_too_many_args_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "hello" % (42)
        self.assertEqual(
            str(context.exception),
            "not all arguments converted during string formatting",
        )
        with self.assertRaises(TypeError) as context:
            "%d%s" % (1, "foo", 3)
        self.assertEqual(
            str(context.exception),
            "not all arguments converted during string formatting",
        )


class SumTests(unittest.TestCase):
    def test_str_as_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            sum([], "")
        self.assertEqual(
            str(context.exception), "sum() can't sum strings [use ''.join(seq) instead]"
        )

    def test_bytes_as_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            sum([], b"")
        self.assertEqual(
            str(context.exception), "sum() can't sum bytes [use b''.join(seq) instead]"
        )

    def test_bytearray_as_start_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            sum([], bytearray())
        self.assertEqual(
            str(context.exception),
            "sum() can't sum bytearray [use b''.join(seq) instead]",
        )

    def test_non_iterable_raises_type_error(self):
        with self.assertRaises(TypeError):
            sum(None)
        # TODO(T43043290): Check for the exception message.

    def test_int_list_without_start_returns_sum(self):
        result = sum([1, 2, 3])
        self.assertEqual(result, 6)

    def test_int_list_with_start_returns_sum(self):
        result = sum([1, 2, 3], -6)
        self.assertEqual(result, 0)


class TupleTests(unittest.TestCase):
    def test_dunder_lt_with_non_tuple_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            tuple.__lt__(None, ())

    def test_dunder_lt_with_non_tuple_other_returns_not_implemented(self):
        self.assertIs(tuple.__lt__((), None), NotImplemented)

    def test_dunder_new_with_no_iterable_arg_returns_empty_tuple(self):
        result = tuple.__new__(tuple)
        self.assertIs(result, ())

    def test_dunder_new_with_tuple_returns_same_object(self):
        src = (1, 2, 3)
        result = tuple.__new__(tuple, src)
        self.assertIs(result, src)

    def test_dunder_new_with_list_returns_tuple(self):
        self.assertEqual(tuple([1, 2, 3]), (1, 2, 3))

    def test_dunder_new_with_tuple_subclass_and_tuple_returns_new_object(self):
        class C(tuple):
            pass

        src = (1, 2, 3)
        result = tuple.__new__(C, src)
        self.assertTrue(result is not src)

    def test_dunder_new_with_iterable_returns_tuple_with_elements(self):
        result = tuple.__new__(tuple, [1, 2, 3])
        self.assertEqual(result, (1, 2, 3))

    def test_dunder_new_with_tuple_subclass_calls_dunder_iter(self):
        class C(tuple):
            def __iter__(self):
                raise UserWarning("foo")

        c = C()
        self.assertRaises(UserWarning, tuple, c)

    def test_dunder_new_with_list_subclass_calls_dunder_iter(self):
        class C(list):
            def __iter__(self):
                raise UserWarning("foo")

        c = C()
        self.assertRaises(UserWarning, tuple, c)

    def test_dunder_new_with_raising_dunder_iter_descriptor_raises_type_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("foo")

        class C:
            __iter__ = Desc()

        with self.assertRaises(TypeError) as context:
            tuple(C())
        self.assertEqual(str(context.exception), "'C' object is not iterable")

    def test_dunder_new_with_raising_dunder_next_descriptor_propagates_exception(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("foo")

        class C:
            def __iter__(self):
                return self

            __next__ = Desc()

        with self.assertRaises(UserWarning):
            tuple(C())


class TypeTests(unittest.TestCase):
    def test_abstract_methods_get_with_builtin_type_raises_attribute_error(self):
        with self.assertRaises(AttributeError) as context:
            type.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_abstract_methods_get_with_type_subclass_raises_attribute_error(self):
        class Foo(type):
            pass

        with self.assertRaises(AttributeError) as context:
            Foo.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_abstract_methods_set_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            int.__abstractmethods__ = ["foo"]
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'int'",
        )

    def test_abstract_methods_get_with_type_subclass_sets_attribute(self):
        class Foo(type):
            pass

        Foo.__abstractmethods__ = 1
        self.assertEqual(Foo.__abstractmethods__, 1)

    def test_abstract_methods_del_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            del str.__abstractmethods__
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'str'",
        )

    def test_abstract_methods_del_unset_with_type_subclass_raises_attribute_error(self):
        class Foo(type):
            pass

        with self.assertRaises(AttributeError) as context:
            del Foo.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_dunder_bases_del_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            del object.__bases__
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'object'",
        )

    def test_dunder_bases_del_with_user_type_raises_type_error(self):
        class C:
            pass

        with self.assertRaises(TypeError) as context:
            del C.__bases__
        self.assertEqual(str(context.exception), "can't delete C.__bases__")

    def test_dunder_bases_get_with_builtin_type_returns_tuple(self):
        self.assertEqual(object.__bases__, ())
        self.assertEqual(type.__bases__, (object,))
        self.assertEqual(int.__bases__, (object,))
        self.assertEqual(bool.__bases__, (int,))

    def test_dunder_bases_get_with_user_type_returns_tuple(self):
        class A:
            pass

        class B:
            pass

        class C(A):
            pass

        class D(C, B):
            pass

        self.assertEqual(A.__bases__, (object,))
        self.assertEqual(B.__bases__, (object,))
        self.assertEqual(C.__bases__, (A,))
        self.assertEqual(D.__bases__, (C, B))

    def test_dunder_instancecheck_with_instance_returns_true(self):
        self.assertIs(int.__instancecheck__(3), True)
        self.assertIs(int.__instancecheck__(False), True)
        self.assertIs(object.__instancecheck__(type), True)
        self.assertIs(str.__instancecheck__("123"), True)
        self.assertIs(type.__instancecheck__(type, int), True)
        self.assertIs(type.__instancecheck__(type, object), True)

    def test_dunder_instancecheck_with_non_instance_returns_false(self):
        self.assertIs(bool.__instancecheck__(3), False)
        self.assertIs(int.__instancecheck__("123"), False)
        self.assertIs(str.__instancecheck__(b"123"), False)
        self.assertIs(type.__instancecheck__(type, 3), False)

    def test_dunder_subclasscheck_with_subclass_returns_true(self):
        self.assertIs(int.__subclasscheck__(int), True)
        self.assertIs(int.__subclasscheck__(bool), True)
        self.assertIs(object.__subclasscheck__(int), True)
        self.assertIs(object.__subclasscheck__(type), True)

    def test_dunder_subclasscheck_with_non_subclass_returns_false(self):
        self.assertIs(bool.__subclasscheck__(int), False)
        self.assertIs(int.__subclasscheck__(object), False)
        self.assertIs(str.__subclasscheck__(object), False)
        self.assertIs(type.__subclasscheck__(type, object), False)

    def test_dunder_subclasses_with_leaf_type_returns_empty_list(self):
        class C:
            pass

        self.assertEqual(C.__subclasses__(), [])

    def test_dunder_subclasses_with_supertype_returns_list(self):
        class C:
            pass

        class D(C):
            pass

        self.assertEqual(C.__subclasses__(), [D])

    def test_dunder_subclasses_returns_new_list(self):
        class C:
            pass

        self.assertIsNot(C.__subclasses__(), C.__subclasses__())

    def test_mro_with_custom_method_propagates_exception(self):
        class Meta(type):
            def mro(cls):
                raise KeyError

        with self.assertRaises(KeyError):

            class Foo(metaclass=Meta):
                pass

    def test_new_duplicates_dict(self):
        d = {"foo": 42, "bar": 17}
        T = type("T", (object,), d)
        d["foo"] = -7
        del d["bar"]
        self.assertEqual(T.foo, 42)
        self.assertEqual(T.bar, 17)
        T.foo = 20
        self.assertEqual(d["foo"], -7)
        self.assertFalse("bar" in d)

    def test_setattr_with_metaclass_does_not_abort(self):
        class Meta(type):
            pass

        class C(metaclass=Meta):
            __slots__ = "attr"

            def __init__(self, data):
                self.attr = data

        m = C("foo")
        self.assertEqual(m.attr, "foo")
        m.attr = "bar"
        self.assertEqual(m.attr, "bar")


class VarsTests(unittest.TestCase):
    def test_no_arg_delegates_to_locals(self):
        def foo():
            a = 4
            a = a  # noqa: F841
            b = 5
            b = b  # noqa: F841
            return vars()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        self.assertEqual(result["b"], 5)

    def test_arg_with_no_dunder_dict_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            vars(None)
        self.assertEqual(
            str(context.exception), "vars() argument must have __dict__ attribute"
        )

    def test_arg_with_dunder_dict_raising_exception_raises_type_error(self):
        class Desc:
            def __get__(self, obj, type):
                raise UserWarning("called descriptor")

        class C:
            __dict__ = Desc()

        c = C()
        with self.assertRaises(TypeError) as context:
            vars(c)
        self.assertEqual(
            str(context.exception), "vars() argument must have __dict__ attribute"
        )

    def test_arg_with_dunder_dict_returns_dunder_dict(self):
        class C:
            __dict__ = 4

        c = C()
        self.assertEqual(vars(c), 4)


if __name__ == "__main__":
    unittest.main()
