#!/usr/bin/env python3
import unittest

import _codecs


class CodecsTests(unittest.TestCase):
    def test_register_error_with_non_string_first_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.register_error([], [])

    def test_register_error_with_non_callable_second_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.register_error("", [])

    def test_lookup_error_with_non_string_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.lookup_error([])

    def test_lookup_error_with_unknown_name_raises_lookup_error(self):
        with self.assertRaises(LookupError):
            _codecs.lookup_error("not-an-error")

    def test_lookup_error_with_ignore_returns_ignore_function(self):
        func = _codecs.lookup_error("ignore")
        self.assertEqual(func.__name__, "ignore_errors")

    def test_lookup_error_with_strict_returns_strict_function(self):
        func = _codecs.lookup_error("strict")
        self.assertEqual(func.__name__, "strict_errors")

    def test_lookup_error_with_registered_error_returns_function(self):
        def test_func():
            pass

        _codecs.register_error("test", test_func)
        func = _codecs.lookup_error("test")
        self.assertEqual(func.__name__, "test_func")

    def test_register_with_non_callable_fails(self):
        with self.assertRaises(TypeError):
            _codecs.register("not-a-callable")

    def test_lookup_unknown_codec_raises_lookup_error(self):
        with self.assertRaises(LookupError):
            _codecs.lookup("not-the-name-of-a-codec")

    def test_lookup_that_doesnt_return_tuple_raises_type_error(self):
        def lookup_function(encoding):
            if encoding == "lookup_that_doesnt_return_tuple":
                return "not-a-tuple"

        _codecs.register(lookup_function)
        with self.assertRaises(TypeError):
            _codecs.lookup("lookup_that_doesnt_return_tuple")

    def test_lookup_that_doesnt_return_four_tuple_raises_type_error(self):
        def lookup_function(encoding):
            if encoding == "lookup_that_doesnt_return_four_tuple":
                return "one", "two", "three"

        _codecs.register(lookup_function)
        with self.assertRaises(TypeError):
            _codecs.lookup("lookup_that_doesnt_return_four_tuple")

    def test_lookup_returns_first_registered_codec(self):
        def first_lookup_function(encoding):
            if encoding == "lookup_return_of_registered_codec":
                return 1, 2, 3, 4

        def second_lookup_function(encoding):
            if encoding == "lookup_return_of_registered_codec":
                return 5, 6, 7, 8

        _codecs.register(first_lookup_function)
        _codecs.register(second_lookup_function)
        self.assertEqual(
            _codecs.lookup("lookup_return_of_registered_codec"), (1, 2, 3, 4)
        )

    def test_lookup_properly_caches_encodings(self):
        accumulator = []

        def incrementing_lookup_function(encoding):
            if (
                encoding == "incrementing_state_one"
                or encoding == "incrementing_state_two"
            ):
                accumulator.append(1)
                return len(accumulator), 0, 0, 0

        _codecs.register(incrementing_lookup_function)
        first_one = _codecs.lookup("incrementing_state_one")
        first_two = _codecs.lookup("incrementing_state_two")
        second_one = _codecs.lookup("incrementing_state_one")
        self.assertNotEqual(first_one, first_two)
        self.assertEqual(first_one, second_one)

    def test_lookup_properly_normalizes_encoding_string(self):
        def lookup_function(encoding):
            if encoding == "normalized-string":
                return 0, 0, 0, 0

        _codecs.register(lookup_function)
        self.assertEqual(_codecs.lookup("normalized string"), (0, 0, 0, 0))


class DecodeASCIITests(unittest.TestCase):
    def test_decode_ascii_with_non_bytes_first_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.ascii_decode([])

    def test_decode_ascii_with_non_string_second_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.ascii_decode(b"", [])

    def test_decode_ascii_with_zero_length_returns_empty_string(self):
        decoded, consumed = _codecs.ascii_decode(b"")
        self.assertEqual(decoded, "")
        self.assertEqual(consumed, 0)

    def test_decode_ascii_with_well_formed_ascii_returns_string(self):
        decoded, consumed = _codecs.ascii_decode(b"hello")
        self.assertEqual(decoded, "hello")
        self.assertEqual(consumed, 5)

    def test_decode_ascii_with_custom_error_handler_returns_string(self):
        _codecs.register_error("test", lambda x: ("-testing-", x.end))
        decoded, consumed = _codecs.ascii_decode(b"ab\x90c", "test")
        self.assertEqual(decoded, "ab-testing-c")
        self.assertEqual(consumed, 4)


class EncodeUTF8Tests(unittest.TestCase):
    def test_encode_utf_8_with_non_str_first_argument_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.utf_8_encode([])

    def test_encode_utf_8_with_non_str_second_argument_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.utf_8_encode("", [])

    def test_encode_utf_8_with_zero_length_returns_empty_bytes(self):
        encoded, consumed = _codecs.utf_8_encode("")
        self.assertEqual(encoded, b"")
        self.assertEqual(consumed, 0)

    def test_encode_utf_8_with_well_formed_ascii_returns_bytes(self):
        encoded, consumed = _codecs.utf_8_encode("hello")
        self.assertEqual(encoded, b"hello")
        self.assertEqual(consumed, 5)

    def test_encode_utf_8_with_custom_error_handler_mid_bytes_error_returns_bytes(self):
        _codecs.register_error("test", lambda x: (b"-testing-", x.end))
        encoded, consumed = _codecs.utf_8_encode("ab\udc80c", "test")
        self.assertEqual(encoded, b"ab-testing-c")
        self.assertEqual(consumed, 4)

    def test_encode_utf_8_with_custom_error_handler_end_bytes_error_returns_bytes(self):
        _codecs.register_error("test", lambda x: (b"-testing-", x.end))
        encoded, consumed = _codecs.utf_8_encode("ab\udc80", "test")
        self.assertEqual(encoded, b"ab-testing-")
        self.assertEqual(consumed, 3)

    def test_encode_utf_8_with_non_ascii_error_handler_raises_decode_error(self):
        _codecs.register_error("test", lambda x: ("\x80", x.end))
        with self.assertRaises(UnicodeEncodeError):
            _codecs.utf_8_encode("ab\udc80", "test")


class GeneralizedErrorHandlerTests(unittest.TestCase):
    def test_call_decode_error_with_strict_raises_unicode_decode_error(self):
        with self.assertRaises(UnicodeDecodeError):
            _codecs._call_decode_errorhandler(
                "strict", b"bad input", bytearray(), "reason", "encoding", 0, 0
            )

    def test_call_decode_error_with_ignore_returns_tuple(self):
        new_input, new_pos = _codecs._call_decode_errorhandler(
            "ignore", b"bad_input", bytearray(), "reason", "encoding", 1, 2
        )
        self.assertEqual(new_input, b"bad_input")
        self.assertEqual(new_pos, 2)

    def test_call_decode_error_with_non_tuple_return_raises_type_error(self):
        def error_function(exc):
            return "not-a-tuple"

        _codecs.register_error("not-a-tuple", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_decode_errorhandler(
                "not-a-tuple", b"bad_input", bytearray(), "reason", "encoding", 1, 2
            )

    def test_call_decode_error_with_small_tuple_return_raises_type_error(self):
        def error_function(exc):
            return ("one",)

        _codecs.register_error("small-tuple", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_decode_errorhandler(
                "small-tuple", b"bad_input", bytearray(), "reason", "encoding", 1, 2
            )

    def test_call_decode_error_with_int_first_tuple_return_raises_type_error(self):
        def error_function(exc):
            return 1, 1

        _codecs.register_error("int-first", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_decode_errorhandler(
                "int-first", b"bad_input", bytearray(), "reason", "encoding", 1, 2
            )

    def test_call_decode_error_with_string_second_tuple_return_raises_type_error(self):
        def error_function(exc):
            return "str_to_append", "new_pos"

        _codecs.register_error("str-second", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_decode_errorhandler(
                "str-second", b"bad_input", bytearray(), "reason", "encoding", 1, 2
            )

    def test_call_decode_error_with_non_bytes_changed_input_returns_error(self):
        def error_function(err):
            err.object = 1
            return "str_to_append", err.end

        _codecs.register_error("change-input-to-int", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_decode_errorhandler(
                "change-input-to-int",
                b"bad_input",
                bytearray(),
                "reason",
                "encoding",
                1,
                2,
            )

    def test_call_decode_error_with_improper_index_returns_error(self):
        def error_function(exc):
            return "str_to_append", 10

        _codecs.register_error("out-of-bounds-pos", error_function)
        with self.assertRaises(IndexError):
            _codecs._call_decode_errorhandler(
                "out-of-bounds-pos",
                b"bad_input",
                bytearray(),
                "reason",
                "encoding",
                1,
                2,
            )

    def test_call_decode_error_with_negative_index_return_returns_proper_index(self):
        def error_function(exc):
            return "str_to_append", -1

        _codecs.register_error("negative-pos", error_function)
        new_input, new_pos = _codecs._call_decode_errorhandler(
            "negative-pos", b"bad_input", bytearray(), "reason", "encoding", 1, 2
        )
        self.assertEqual(new_input, b"bad_input")
        self.assertEqual(new_pos, 8)

    def test_call_decode_appends_string_to_output(self):
        def error_function(exc):
            return "str_to_append", exc.end

        _codecs.register_error("well-behaved-test", error_function)
        result = bytearray()
        _codecs._call_decode_errorhandler(
            "well-behaved-test", b"bad_input", result, "reason", "encoding", 1, 2
        )
        self.assertEqual(bytes(result), b"str_to_append")

    def test_call_encode_error_with_strict_calls_function(self):
        with self.assertRaises(UnicodeEncodeError):
            _codecs._call_encode_errorhandler(
                "strict", "bad_input", "reason", "encoding", 0, 0
            )

    def test_call_encode_error_with_ignore_calls_function(self):
        result, new_pos = _codecs._call_encode_errorhandler(
            "ignore", "bad_input", "reason", "encoding", 1, 2
        )
        self.assertEqual(result, "")
        self.assertEqual(new_pos, 2)

    def test_call_encode_error_with_non_tuple_return_raises_type_error(self):
        def error_function(exc):
            return "not-a-tuple"

        _codecs.register_error("not-a-tuple", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_encode_errorhandler(
                "not-a-tuple", "bad_input", "reason", "encoding", 1, 2
            )

    def test_call_encode_error_with_small_tuple_return_raises_type_error(self):
        def error_function(exc):
            return ("one",)

        _codecs.register_error("small-tuple", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_encode_errorhandler(
                "small-tuple", "bad_input", "reason", "encoding", 1, 2
            )

    def test_call_encode_error_with_int_first_tuple_return_raises_type_error(self):
        def error_function(exc):
            return 1, 1

        _codecs.register_error("int-first", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_encode_errorhandler(
                "int-first", "bad_input", "reason", "encoding", 1, 2
            )

    def test_call_encode_error_with_str_second_tuple_return_raises_type_error(self):
        def error_function(exc):
            return "str_to_append", "new_pos"

        _codecs.register_error("str-second", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_encode_errorhandler(
                "str-second", "bad_input", "reason", "encoding", 1, 2
            )

    def test_call_encode_error_with_changed_input_ignores_change(self):
        def error_function(err):
            err.object = 1
            return "str_to_append", err.end

        _codecs.register_error("change-input-to-int", error_function)
        result, new_pos = _codecs._call_encode_errorhandler(
            "change-input-to-int", "bad_input", "reason", "encoding", 1, 2
        )
        self.assertEqual(result, "str_to_append")
        self.assertEqual(new_pos, 2)

    def test_call_encode_error_with_out_of_bounds_index_raises_index_error(self):
        def error_function(exc):
            return "str_to_append", 10

        _codecs.register_error("out-of-bounds-pos", error_function)
        with self.assertRaises(IndexError):
            _codecs._call_encode_errorhandler(
                "out-of-bounds-pos", "bad_input", "reason", "encoding", 1, 2
            )

    def test_call_encode_error_with_negative_index_returns_proper_index(self):
        def error_function(exc):
            return "str_to_append", -1

        _codecs.register_error("negative-pos", error_function)
        result, new_pos = _codecs._call_encode_errorhandler(
            "negative-pos", "bad_input", "reason", "encoding", 1, 2
        )
        self.assertEqual(result, "str_to_append")
        self.assertEqual(new_pos, 8)


if __name__ == "__main__":
    unittest.main()
