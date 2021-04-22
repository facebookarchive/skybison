#!/usr/bin/env python3
import _codecs
import unittest

from test_support import pyro_only


try:
    from builtins import _str_array
except ImportError:
    pass


class CharmapTests(unittest.TestCase):
    def test_charmap_decode_with_empty_bytes_input_returns_tuple(self):
        self.assertEqual(_codecs.charmap_decode(b"", "strict", "abc"), ("", 0))
        self.assertEqual(
            _codecs.charmap_decode(b"", "strict", {0: "a", 1: "b", 2: "c"}), ("", 0)
        )

    def test_charmap_decode_with_string_map_raises_unicode_decode_error(self):
        self.assertRaises(
            UnicodeDecodeError, _codecs.charmap_decode, b"\x00\x01\x02", "strict", "ab"
        )

    def test_charmap_decode_with_string_map_with_bom_raises_unicode_decode_error(self):
        self.assertRaises(
            UnicodeDecodeError,
            _codecs.charmap_decode,
            b"\x00\x01\x02",
            "strict",
            "ab\ufffe",
        )

    def test_charmap_decode_with_string_map_returns_tuple(self):
        self.assertEqual(
            _codecs.charmap_decode(b"\x00\x01\x02", "strict", "abc"), ("abc", 3)
        )

    def test_charmap_decode_with_int2str_map_returns_tuple(self):
        self.assertEqual(
            _codecs.charmap_decode(b"\x00\x01\x02", "strict", {0: "a", 1: "b", 2: "c"}),
            ("abc", 3),
        )
        self.assertEqual(
            _codecs.charmap_decode(
                b"\x00\x01\x02", "strict", {0: "Aa", 1: "Bb", 2: "Cc"}
            ),
            ("AaBbCc", 3),
        )
        self.assertEqual(
            _codecs.charmap_decode(
                b"\x00\x01\x02", "strict", {0: "\U0010FFFF", 1: "b", 2: "c"}
            ),
            ("\U0010FFFFbc", 3),
        )
        self.assertEqual(
            _codecs.charmap_decode(b"\x00\x01\x02", "strict", {0: "a", 1: "b", 2: ""}),
            ("ab", 3),
        )
        self.assertEqual(
            _codecs.charmap_decode(b"\x00\x01\x02", "strict", {0: "A", 1: "Bb", 2: 37}),
            ("ABb%", 3),
        )

    def test_charmap_decode_with_int2str_map_raises_unicode_decode_error(self):
        self.assertRaises(
            UnicodeDecodeError,
            _codecs.charmap_decode,
            b"\x00\x01\x02",
            "strict",
            {0: "a", 1: "b"},
        )

    def test_charmap_decode_with_int2str_map_with_none_raises_unicode_decode_error(
        self,
    ):
        self.assertRaises(
            UnicodeDecodeError,
            _codecs.charmap_decode,
            b"\x00\x01\x02",
            "strict",
            {0: "a", 1: "b", 2: None},
        )

    def test_charmap_decode_with_int2str_map_with_bom_raises_unicode_decode_error(self):
        self.assertRaises(
            UnicodeDecodeError,
            _codecs.charmap_decode,
            b"\x00\x01\x02",
            "strict",
            {0: "a", 1: "b", 2: "\ufffe"},
        )

    def test_charmap_decode_with_mapped_value_tuple_raises_type_error(self):
        self.assertRaises(
            TypeError,
            _codecs.charmap_decode,
            b"\x00\x01\x02",
            "strict",
            {0: "a", 1: "b", 2: ("c",)},
        )

    def test_charmap_decode_with_mapped_value_list_raises_type_error(self):
        self.assertRaises(
            TypeError,
            _codecs.charmap_decode,
            b"\x00\x01\x02",
            "strict",
            {0: "a", 1: "b", 2: ["c"]},
        )

    def test_charmap_decode_with_mapped_value_out_of_range_min_raises_type_error(self):
        self.assertRaises(
            TypeError,
            _codecs.charmap_decode,
            b"\x00\x01\x02",
            "strict",
            {0: "a", 1: "b", 2: -2},
        )

    def test_charmap_decode_with_mapped_value_out_of_range_max_raises_type_error(self):
        self.assertRaises(
            TypeError,
            _codecs.charmap_decode,
            b"\x00\x01\x02",
            "strict",
            {0: "a", 1: "b", 2: 9999999},
        )


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

    def test_decode_with_unknown_codec_raises_lookup_error(self):
        with self.assertRaises(LookupError) as context:
            _codecs.decode(b"bytes", "not-a-codec")
        self.assertEqual(str(context.exception), "unknown encoding: not-a-codec")

    def test_decode_with_function_with_int_codec_raises_type_error(self):
        def lookup_function(encoding):
            if encoding == "decode-with-function-with-int-codec":
                return 0, 0, 0, 0

        _codecs.register(lookup_function)
        with self.assertRaises(TypeError) as context:
            _codecs.decode(b"bytes", "decode-with-function-with-int-codec")
        self.assertIn("'int' object is not callable", str(context.exception))

    def test_decode_with_function_with_non_tuple_return_raises_type_error(self):
        def lookup_function(encoding):
            if encoding == "decode-with-function-with-faulty-codec":
                return 0, (lambda uni: 0), 0, 0

        _codecs.register(lookup_function)
        with self.assertRaises(TypeError) as context:
            _codecs.decode(b"bytes", "decode-with-function-with-faulty-codec")
        self.assertEqual(
            str(context.exception), "decoder must return a tuple (object,integer)"
        )

    def test_decode_with_function_with_tuple_return_returns_first_element(self):
        def decoder(s):
            return ("one", "two")

        def lookup_function(encoding):
            if encoding == "decode-with-function-with-two-tuple-codec":
                return 0, decoder, 0, 0

        _codecs.register(lookup_function)
        self.assertEqual(
            _codecs.decode(b"bytes", "decode-with-function-with-two-tuple-codec"), "one"
        )

    def test_decode_with_errors_passes_multiple_arguments(self):
        def decoder(s, err):
            return (s, err)

        def lookup_function(encoding):
            if encoding == "decode-with-function-with-two-arguments":
                return 0, decoder, 0, 0

        _codecs.register(lookup_function)
        self.assertEqual(
            _codecs.decode(
                b"bytes", "decode-with-function-with-two-arguments", "error"
            ),
            b"bytes",
        )

    def test_encode_with_unknown_codec_raises_lookup_error(self):
        with self.assertRaises(LookupError) as context:
            _codecs.encode("str", "not-a-codec")
        self.assertEqual(str(context.exception), "unknown encoding: not-a-codec")

    def test_encode_with_function_with_int_codec_raises_type_error(self):
        def lookup_function(encoding):
            if encoding == "encode-with-function-with-int-codec":
                return 0, 0, 0, 0

        _codecs.register(lookup_function)
        with self.assertRaises(TypeError) as context:
            _codecs.encode("str", "encode-with-function-with-int-codec")
        self.assertIn("'int' object is not callable", str(context.exception))

    def test_encode_with_function_with_non_tuple_return_raises_type_error(self):
        def lookup_function(encoding):
            if encoding == "encode-with-function-with-faulty-codec":
                return (lambda uni: 0), 0, 0, 0

        _codecs.register(lookup_function)
        with self.assertRaises(TypeError) as context:
            _codecs.encode("str", "encode-with-function-with-faulty-codec")
        self.assertEqual(
            str(context.exception), "encoder must return a tuple (object, integer)"
        )

    def test_encode_with_function_with_tuple_return_returns_first_element(self):
        def encoder(s):
            return ("one", "two")

        def lookup_function(encoding):
            if encoding == "encode-with-function-with-two-tuple-codec":
                return encoder, 0, 0, 0

        _codecs.register(lookup_function)
        self.assertEqual(
            _codecs.encode("str", "encode-with-function-with-two-tuple-codec"), "one"
        )

    def test_encode_with_errors_passes_multiple_arguments(self):
        def encoder(s, err):
            return (s, err)

        def lookup_function(encoding):
            if encoding == "encode-with-function-with-two-arguments":
                return encoder, 0, 0, 0

        _codecs.register(lookup_function)
        self.assertEqual(
            _codecs.encode("str", "encode-with-function-with-two-arguments", "error"),
            "str",
        )

    @pyro_only
    def test_getincrementaldecoder_with_utf_8_returns_utf_8_incremental_decoder(self):
        inc_dec = _codecs.getincrementaldecoder("utf-8")
        self.assertIs(inc_dec, _codecs.UTF8IncrementalDecoder)

    @pyro_only
    def test_getincrementalencoder_with_utf_8_returns_utf_8_incremental_encoder(self):
        inc_dec = _codecs.getincrementalencoder("utf-8")
        self.assertIs(inc_dec, _codecs.UTF8IncrementalEncoder)


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

    def test_decode_ascii_with_well_formed_ascii_bytearray_returns_string(self):
        decoded, consumed = _codecs.ascii_decode(bytearray(b"hello"))
        self.assertEqual(decoded, "hello")
        self.assertEqual(consumed, 5)

    def test_decode_ascii_with_well_formed_ascii_bytearray_subclass_returns_string(
        self,
    ):
        class B(bytearray):
            pass

        decoded, consumed = _codecs.ascii_decode(B(b"hello"))
        self.assertEqual(decoded, "hello")
        self.assertEqual(consumed, 5)

    def test_decode_ascii_with_custom_error_handler_returns_string(self):
        _codecs.register_error("test", lambda x: ("-testing-", x.end))
        decoded, consumed = _codecs.ascii_decode(b"ab\x90c", "test")
        self.assertEqual(decoded, "ab-testing-c")
        self.assertEqual(consumed, 4)


class DecodeEscapeTests(unittest.TestCase):
    def test_decode_escape_with_non_bytes_first_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.escape_decode([])

    def test_decode_escape_with_non_string_second_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.escape_decode(b"", [])

    def test_decode_escape_with_zero_length_returns_empty_string(self):
        decoded, consumed = _codecs.escape_decode(b"")
        self.assertEqual(decoded, b"")
        self.assertEqual(consumed, 0)

    # TODO(atalaba): This should not need @pyro_only
    @pyro_only
    def test_decode_escape_with_well_formed_latin_1_returns_string(self):
        decoded, consumed = _codecs.escape_decode(b"hello\x95")
        self.assertEqual(decoded, b"hello\xC2\x95")
        self.assertEqual(consumed, 6)

    def test_decode_escape_with_end_of_string_slash_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            _codecs.escape_decode(b"ab\\")
        self.assertEqual(str(context.exception), "Trailing \\ in string")

    def test_decode_escape_with_truncated_hex_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            _codecs.escape_decode(b"ab\\x1h")
        self.assertEqual(str(context.exception), "invalid \\x escape at position 2")

    def test_decode_escape_with_truncated_hex_unknown_error_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            _codecs.escape_decode(b"ab\\x1h", "unknown")
        self.assertEqual(
            str(context.exception),
            "decoding error; unknown error handling code: unknown",
        )

    @pyro_only
    def test_decode_escape_stateful_returns_first_invalid_escape(self):
        decoded, consumed, first_invalid = _codecs._escape_decode_stateful(b"ab\\yc")
        self.assertEqual(decoded, b"ab\\yc")
        self.assertEqual(consumed, 5)
        self.assertEqual(first_invalid, 3)


class DecodeLatin1Tests(unittest.TestCase):
    def test_decode_latin_1_with_non_bytes_first_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.latin_1_decode([])

    def test_decode_latin_1_with_non_string_second_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.latin_1_decode(b"", [])

    def test_decode_latin_1_with_zero_length_returns_empty_string(self):
        decoded, consumed = _codecs.latin_1_decode(b"")
        self.assertEqual(decoded, "")
        self.assertEqual(consumed, 0)

    def test_decode_latin_1_with_ascii_returns_string(self):
        decoded, consumed = _codecs.latin_1_decode(b"hello")
        self.assertEqual(decoded, "hello")
        self.assertEqual(consumed, 5)

    def test_decode_latin_1_with_ascii_bytearray_returns_string(self):
        decoded, consumed = _codecs.latin_1_decode(bytearray(b"hello"))
        self.assertEqual(decoded, "hello")
        self.assertEqual(consumed, 5)

    def test_decode_latin_1_with_ascii_bytearray_subclass_returns_string(self):
        class B(bytearray):
            pass

        decoded, consumed = _codecs.latin_1_decode(B(b"hello"))
        self.assertEqual(decoded, "hello")
        self.assertEqual(consumed, 5)

    def test_decode_latin_1_with_latin_1_returns_string(self):
        decoded, consumed = _codecs.latin_1_decode(b"\x7D\x7E\x7F\x80\x81\x82")
        self.assertEqual(decoded, "\x7D\x7E\x7F\x80\x81\x82")
        self.assertEqual(consumed, 6)


class DecodeUnicodeEscapeTests(unittest.TestCase):
    def test_decode_unicode_escape_with_non_bytes_first_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.unicode_escape_decode([])

    def test_decode_unicode_escape_with_non_string_second_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.unicode_escape_decode(b"", [])

    def test_decode_unicode_escape_with_zero_length_returns_empty_string(self):
        decoded, consumed = _codecs.unicode_escape_decode(b"")
        self.assertEqual(decoded, "")
        self.assertEqual(consumed, 0)

    def test_decode_unicode_escape_with_well_formed_latin_1_returns_string(self):
        decoded, consumed = _codecs.unicode_escape_decode(b"hello\x95")
        self.assertEqual(decoded, "hello\x95")
        self.assertEqual(consumed, 6)

    def test_decode_unicode_escape_with_well_formed_latin_1_bytearray_returns_string(
        self,
    ):
        decoded, consumed = _codecs.unicode_escape_decode(bytearray(b"hello\x95"))
        self.assertEqual(decoded, "hello\x95")
        self.assertEqual(consumed, 6)

    def test_decode_unicode_escape_with_latin_1_bytearray_subclass_returns_string(self):
        class B(bytearray):
            pass

        decoded, consumed = _codecs.unicode_escape_decode(B(b"hello\x95"))
        self.assertEqual(decoded, "hello\x95")
        self.assertEqual(consumed, 6)

    def test_decode_unicode_escape_with_escaped_back_slash_returns_string(self):
        decoded, consumed = _codecs.unicode_escape_decode(b"hello\\x95")
        self.assertEqual(decoded, "hello\x95")
        self.assertEqual(consumed, 9)

    def test_decode_unicode_escape_with_valid_hangul_returns_string(self):
        decoded, consumed = _codecs.unicode_escape_decode(
            b"\\N{HANGUL SYLLABLE BBYAENG}"
        )
        self.assertEqual(decoded, "\uBEC9")
        self.assertEqual(consumed, 27)

    def test_decode_unicode_escape_with_lowercase_hangul_raises_exception(self):
        with self.assertRaises(UnicodeDecodeError):
            _codecs.unicode_escape_decode(b"\\N{HANGUL SYLLABLE ddalg}")

    def test_decode_unicode_escape_with_invalid_leading_raises_exception(self):
        with self.assertRaises(UnicodeDecodeError):
            _codecs.unicode_escape_decode(b"\\N{HANGUL SYLLABLE BLANJ}")

    def test_decode_unicode_escape_with_invalid_vowel_raises_exception(self):
        with self.assertRaises(UnicodeDecodeError):
            _codecs.unicode_escape_decode(b"\\N{HANGUL SYLLABLE CAOGS}")

    def test_decode_unicode_escape_with_invalid_trailing_raises_exception(self):
        with self.assertRaises(UnicodeDecodeError):
            _codecs.unicode_escape_decode(b"\\N{HANGUL SYLLABLE PYOLL}")

    def test_decode_unicode_escape_with_valid_cjk_ideograph_returns_string(self):
        decoded, consumed = _codecs.unicode_escape_decode(
            b"\\N{CJK UNIFIED IDEOGRAPH-4DB0}"
        )
        self.assertEqual(decoded, "\u4DB0")
        self.assertEqual(consumed, 30)

        decoded, consumed = _codecs.unicode_escape_decode(
            b"\\N{CJK UNIFIED IDEOGRAPH-2B75A}"
        )
        self.assertEqual(decoded, "\U0002B75A")
        self.assertEqual(consumed, 31)

    def test_decode_unicode_escape_with_lowercase_cjk_ideograph_returns_string(self):
        with self.assertRaises(UnicodeDecodeError):
            _codecs.unicode_escape_decode(b"\\N{CJK UNIFIED IDEOGRAPH-4db0}")

    def test_decode_unicode_escape_with_invalid_cjk_ideograph_returns_string(self):
        with self.assertRaises(UnicodeDecodeError):
            _codecs.unicode_escape_decode(b"\\N{CJK UNIFIED IDEOGRAPH-4DB6}")

    def test_decode_unicode_escape_with_valid_name_escape_returns_string(self):
        decoded, consumed = _codecs.unicode_escape_decode(
            b"\\N{LATIN SMALL LETTER A WITH MACRON}"
        )
        self.assertEqual(decoded, "\u0101")
        self.assertEqual(consumed, 36)

    def test_decode_unicode_escape_with_invalid_word_raises_unicode_decode_error(self):
        with self.assertRaises(UnicodeDecodeError):
            _codecs.unicode_escape_decode(b"\\N{INVALID}")

    def test_decode_unicode_escape_with_invalid_name_raises_unicode_decode_error(self):
        with self.assertRaises(UnicodeDecodeError):
            _codecs.unicode_escape_decode(b"\\N{LATIN S LETTER CAPITAL}")

    def test_decode_unicode_escape_with_custom_error_handler_returns_string(self):
        _codecs.register_error("test", lambda x: ("-testing-", x.end))
        decoded, consumed = _codecs.unicode_escape_decode(b"ab\\U90gc", "test")
        self.assertEqual(decoded, "ab-testing-gc")
        self.assertEqual(consumed, 8)

    @pyro_only
    def test_decode_unicode_escape_stateful_returns_first_invalid_escape(self):
        decoded, consumed, first_invalid = _codecs._unicode_escape_decode_stateful(
            b"ab\\yc"
        )
        self.assertEqual(decoded, "ab\\yc")
        self.assertEqual(consumed, 5)
        self.assertEqual(first_invalid, 3)


class DecodeRawUnicodeEscapeTests(unittest.TestCase):
    def test_decode_raw_unicode_escape_with_non_bytes_first_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.raw_unicode_escape_decode([])

    def test_decode_raw_unicode_escape_with_non_string_second_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.raw_unicode_escape_decode(b"", [])

    def test_decode_raw_unicode_escape_with_zero_length_returns_empty_string(self):
        decoded, consumed = _codecs.raw_unicode_escape_decode(b"")
        self.assertEqual(decoded, "")
        self.assertEqual(consumed, 0)

    def test_decode_raw_unicode_escape_with_well_formed_latin_1_returns_string(self):
        decoded, consumed = _codecs.raw_unicode_escape_decode(b"hello\x95")
        self.assertEqual(decoded, "hello\x95")
        self.assertEqual(consumed, 6)

    def test_decode_raw_unicode_escape_with_escaped_back_slash_returns_string(self):
        decoded, consumed = _codecs.raw_unicode_escape_decode(b"hello\\x95")
        self.assertEqual(decoded, "hello\\x95")
        self.assertEqual(consumed, 9)

    def test_decode_raw_unicode_escape_with_well_formed_latin_1_bytearray_returns_string(
        self,
    ):
        decoded, consumed = _codecs.raw_unicode_escape_decode(bytearray(b"hello\x95"))
        self.assertEqual(decoded, "hello\x95")
        self.assertEqual(consumed, 6)

    def test_decode_raw_unicode_escape_with_latin_1_bytearray_subclass_returns_string(
        self,
    ):
        class B(bytearray):
            pass

        decoded, consumed = _codecs.raw_unicode_escape_decode(B(b"hello\x95"))
        self.assertEqual(decoded, "hello\x95")
        self.assertEqual(consumed, 6)

    def test_decode_raw_unicode_escape_with_out_of_range_32_bit_unicode_raises_error(
        self,
    ):
        with self.assertRaises(UnicodeDecodeError) as context:
            _codecs.raw_unicode_escape_decode(b"\\U00FFFFFF")
        exc = context.exception
        self.assertEqual(exc.encoding, "rawunicodeescape")
        self.assertEqual(exc.reason, "\\Uxxxxxxxx out of range")
        self.assertEqual(exc.object, b"\\U00FFFFFF")
        self.assertEqual(exc.start, 0)
        self.assertEqual(exc.end, 10)

    def test_decode_raw_unicode_escape_with_truncated_16_bit_unicode_raises_error(
        self,
    ):
        with self.assertRaises(UnicodeDecodeError) as context:
            _codecs.raw_unicode_escape_decode(b"\\u123")
        exc = context.exception
        self.assertEqual(exc.encoding, "rawunicodeescape")
        self.assertEqual(exc.reason, "truncated \\uXXXX escape")
        self.assertEqual(exc.object, b"\\u123")
        self.assertEqual(exc.start, 0)
        self.assertEqual(exc.end, 5)

    def test_decode_raw_unicode_escape_with_truncated_32_bit_unicode_raises_error(
        self,
    ):
        with self.assertRaises(UnicodeDecodeError) as context:
            _codecs.raw_unicode_escape_decode(b"\\U001234")
        exc = context.exception
        self.assertEqual(exc.encoding, "rawunicodeescape")
        self.assertEqual(exc.reason, "truncated \\UXXXXXXXX escape")
        self.assertEqual(exc.object, b"\\U001234")
        self.assertEqual(exc.start, 0)
        self.assertEqual(exc.end, 8)

    def test_decode_raw_unicode_escape_with_valid_unicode_returns_string(self):
        decoded, consumed = _codecs.raw_unicode_escape_decode(b"\u26f7")
        self.assertEqual(decoded, "\u26F7")
        self.assertEqual(consumed, 6)

    def test_decode_raw_unicode_escape_with_valid_cjk_ideograph_returns_string(self):
        decoded, consumed = _codecs.unicode_escape_decode(b"\\u4DB0")
        self.assertEqual(decoded, "\u4DB0")
        self.assertEqual(consumed, 6)

        decoded, consumed = _codecs.raw_unicode_escape_decode(b"\\U0002B75A")
        self.assertEqual(decoded, "\U0002B75A")
        self.assertEqual(consumed, 10)

    def test_decode_raw_unicode_escape_with_valid_name_escape_returns_string(self):
        decoded, consumed = _codecs.raw_unicode_escape_decode(
            b"\\N{HANGUL SYLLABLE BBYAENG}"
        )
        self.assertEqual(decoded, "\\N{HANGUL SYLLABLE BBYAENG}")
        self.assertEqual(consumed, 27)

    def test_decode_raw_unicode_escape_with_invalid_word_returns_string(self):
        decoded, consumed = _codecs.raw_unicode_escape_decode(b"\\N{INVALID}")
        self.assertEqual(decoded, "\\N{INVALID}")
        self.assertEqual(consumed, 11)

    def test_decode_raw_unicode_escape_with_ignore_error_handler_returns_string(self):
        decoded, consumed = _codecs.raw_unicode_escape_decode(b"ab\\U90gc", "ignore")
        self.assertEqual(decoded, "abgc")
        self.assertEqual(consumed, 8)

    def test_decode_raw_unicode_escape_with_custom_error_handler_returns_string(self):
        _codecs.register_error("test", lambda x: ("-testing-", x.end))
        decoded, consumed = _codecs.raw_unicode_escape_decode(b"ab\\U90gc", "test")
        self.assertEqual(decoded, "ab-testing-gc")
        self.assertEqual(consumed, 8)

    def test_decode_raw_unicode_escape_with_replace_error_handler_returns_string(self):
        decoded, consumed = _codecs.raw_unicode_escape_decode(b"ab\\U90gc", "replace")
        self.assertEqual(decoded, "ab\uFFFDgc")
        self.assertEqual(consumed, 8)

    def test_decode_raw_unicode_escape_with_trailing_back_slash_returns_string(
        self,
    ):
        decoded, consumed = _codecs.raw_unicode_escape_decode(b"porcupine\\")
        self.assertEqual(decoded, "porcupine\\")
        self.assertEqual(consumed, 10)


class DecodeUTF8Tests(unittest.TestCase):
    def test_decode_utf_8_with_non_bytes_first_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.utf_8_decode([])

    def test_decode_utf_8_with_non_string_second_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.utf_8_decode(b"", [])

    def test_decode_utf_8_with_zero_length_returns_empty_string(self):
        decoded, consumed = _codecs.utf_8_decode(b"")
        self.assertEqual(decoded, "")
        self.assertEqual(consumed, 0)

    def test_decode_utf_8_with_well_formed_utf_8_returns_string(self):
        decoded, consumed = _codecs.utf_8_decode(
            b"\xf0\x9f\x86\x92h\xc3\xa4l\xe2\xb3\x80"
        )
        self.assertEqual(decoded, "\U0001f192h\xe4l\u2cc0")
        self.assertEqual(consumed, 11)

    def test_decode_utf_8_with_well_formed_utf8_bytearray_returns_string(self):
        decoded, consumed = _codecs.utf_8_decode(
            bytearray(b"\xf0\x9f\x86\x92h\xc3\xa4l\xe2\xb3\x80")
        )
        self.assertEqual(decoded, "\U0001f192h\xe4l\u2cc0")
        self.assertEqual(consumed, 11)

    def test_decode_utf_8_with_well_formed_utf8_bytearray_subclass_returns_string(self):
        class B(bytearray):
            pass

        decoded, consumed = _codecs.utf_8_decode(
            B(b"\xf0\x9f\x86\x92h\xc3\xa4l\xe2\xb3\x80")
        )
        self.assertEqual(decoded, "\U0001f192h\xe4l\u2cc0")
        self.assertEqual(consumed, 11)

    def test_decode_utf_8_with_custom_error_handler_returns_string(self):
        _codecs.register_error("test", lambda x: ("-testing-", x.end))
        decoded, consumed = _codecs.utf_8_decode(b"ab\x90c", "test")
        self.assertEqual(decoded, "ab-testing-c")
        self.assertEqual(consumed, 4)

    def test_decode_utf_8_with_invalid_start_byte_raises_decode_error(self):
        with self.assertRaises(UnicodeDecodeError) as context:
            _codecs.utf_8_decode(b"ab\x90c")
        self.assertEqual(str(context.exception.reason), "invalid start byte")


class EncodeASCIITests(unittest.TestCase):
    def test_encode_ascii_with_non_str_first_argument_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.ascii_encode([])

    def test_encode_ascii_with_non_str_second_argument_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.ascii_encode("", [])

    def test_encode_ascii_with_zero_length_returns_empty_bytes(self):
        encoded, consumed = _codecs.ascii_encode("")
        self.assertEqual(encoded, b"")
        self.assertEqual(consumed, 0)

    def test_encode_ascii_with_well_formed_ascii_returns_bytes(self):
        encoded, consumed = _codecs.ascii_encode("hello")
        self.assertEqual(encoded, b"hello")
        self.assertEqual(consumed, 5)

    def test_encode_ascii_with_well_formed_latin_1_raises_encode_error(self):
        with self.assertRaises(UnicodeEncodeError):
            _codecs.ascii_encode("hell\xe5")

    def test_encode_ascii_with_custom_error_handler_mid_bytes_error_returns_bytes(self):
        _codecs.register_error("test", lambda x: (b"-testing-", x.end))
        encoded, consumed = _codecs.ascii_encode("ab\udc80c", "test")
        self.assertEqual(encoded, b"ab-testing-c")
        self.assertEqual(consumed, 4)

    def test_encode_ascii_with_custom_error_handler_end_bytes_error_returns_bytes(self):
        _codecs.register_error("test", lambda x: (b"-testing-", x.end))
        encoded, consumed = _codecs.ascii_encode("ab\x80", "test")
        self.assertEqual(encoded, b"ab-testing-")
        self.assertEqual(consumed, 3)

    def test_encode_ascii_with_non_ascii_error_handler_raises_encode_error(self):
        _codecs.register_error("test", lambda x: ("\x80", x.end))
        with self.assertRaises(UnicodeEncodeError) as context:
            _codecs.ascii_encode("ab\x80", "test")
        exc = context.exception
        self.assertEqual(exc.encoding, "ascii")
        self.assertEqual(exc.reason, "ordinal not in range(128)")
        self.assertEqual(exc.object, "ab\x80")
        self.assertEqual(exc.start, 2)
        self.assertEqual(exc.end, 3)


class EncodeLatin1Tests(unittest.TestCase):
    def test_encode_latin_1_with_non_str_first_argument_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.latin_1_encode([])

    def test_encode_latin_1_with_non_str_second_argument_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.latin_1_encode("", [])

    def test_encode_latin_1_with_zero_length_returns_empty_bytes(self):
        encoded, consumed = _codecs.latin_1_encode("")
        self.assertEqual(encoded, b"")
        self.assertEqual(consumed, 0)

    def test_encode_latin_1_with_well_formed_latin_1_returns_bytes(self):
        encoded, consumed = _codecs.latin_1_encode("hell\xe5")
        self.assertEqual(encoded, b"hell\xe5")
        self.assertEqual(consumed, 5)

    def test_encode_ascii_with_well_formed_non_latin_1_raises_encode_error(self):
        with self.assertRaises(UnicodeEncodeError):
            _codecs.ascii_encode("hell\u01ff")

    def test_encode_latin_1_with_custom_error_handler_mid_bytes_error_returns_bytes(
        self,
    ):
        _codecs.register_error("test", lambda x: (b"-testing-", x.end))
        encoded, consumed = _codecs.latin_1_encode("ab\udc80c", "test")
        self.assertEqual(encoded, b"ab-testing-c")
        self.assertEqual(consumed, 4)

    def test_encode_latin_1_with_custom_error_handler_end_bytes_error_returns_bytes(
        self,
    ):
        _codecs.register_error("test", lambda x: (b"-testing-", x.end))
        encoded, consumed = _codecs.latin_1_encode("ab\u0180", "test")
        self.assertEqual(encoded, b"ab-testing-")
        self.assertEqual(consumed, 3)

    def test_encode_latin_1_with_non_ascii_error_handler_returns_bytes(self):
        _codecs.register_error("test", lambda x: ("\x80", x.end))
        encoded, consumed = _codecs.latin_1_encode("ab\u0180", "test")
        self.assertEqual(encoded, b"ab\x80")
        self.assertEqual(consumed, 3)

    def test_encode_latin_1_with_non_latin_1_error_handler_raises_encode_error(self):
        _codecs.register_error("test", lambda x: ("\u0180", x.end))
        with self.assertRaises(UnicodeEncodeError) as context:
            _codecs.latin_1_encode("ab\u0f80", "test")
        exc = context.exception
        self.assertEqual(exc.encoding, "latin-1")
        self.assertEqual(exc.reason, "ordinal not in range(256)")
        self.assertEqual(exc.object, "ab\u0f80")
        self.assertEqual(exc.start, 2)
        self.assertEqual(exc.end, 3)


class EncodeUTF16Tests(unittest.TestCase):
    def test_encode_utf_16_with_non_str_first_argument_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.utf_16_encode([])

    def test_encode_utf_16_with_non_str_second_argument_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.utf_16_encode("", [])

    def test_encode_utf_16_with_zero_length_returns_bom(self):
        encoded, consumed = _codecs.utf_16_encode("")
        self.assertEqual(encoded, b"\xff\xfe")
        self.assertEqual(consumed, 0)

    def test_encode_utf_16_with_ascii_returns_bytes(self):
        encoded, consumed = _codecs.utf_16_encode("hi")
        self.assertEqual(encoded, b"\xff\xfeh\x00i\x00")
        self.assertEqual(consumed, 2)

    def test_encode_utf_16_with_latin_1_returns_bytes(self):
        encoded, consumed = _codecs.utf_16_encode("h\xe5")
        self.assertEqual(encoded, b"\xff\xfeh\x00\xe5\x00")
        self.assertEqual(consumed, 2)

    def test_encode_utf_16_with_bmp_returns_bytes(self):
        encoded, consumed = _codecs.utf_16_encode("h\u1005")
        self.assertEqual(encoded, b"\xff\xfeh\x00\x05\x10")
        self.assertEqual(consumed, 2)

    def test_encode_utf_16_with_supplementary_plane_returns_bytes(self):
        encoded, consumed = _codecs.utf_16_encode("h\U0001d1f0i")
        self.assertEqual(encoded, b"\xff\xfeh\x004\xd8\xf0\xddi\x00")
        self.assertEqual(consumed, 3)

    def test_encode_utf_16_le_with_supplementary_plane_returns_bytes(self):
        encoded, consumed = _codecs.utf_16_le_encode("h\U0001d1f0i")
        self.assertEqual(encoded, b"h\x004\xd8\xf0\xddi\x00")
        self.assertEqual(consumed, 3)

    def test_encode_utf_16_be_with_supplementary_plane_returns_bytes(self):
        encoded, consumed = _codecs.utf_16_be_encode("h\U0001d1f0i")
        self.assertEqual(encoded, b"\x00h\xd84\xdd\xf0\x00i")
        self.assertEqual(consumed, 3)

    def test_encode_utf_16_with_custom_error_handler_mid_bytes_error_returns_bytes(
        self,
    ):
        _codecs.register_error("test", lambda x: (b"--", x.end))
        encoded, consumed = _codecs.utf_16_encode("ab\udc80c", "test")
        self.assertEqual(encoded, b"\xff\xfea\x00b\x00--c\x00")
        self.assertEqual(consumed, 4)

    def test_encode_utf_16_with_custom_error_handler_end_bytes_error_returns_bytes(
        self,
    ):
        _codecs.register_error("test", lambda x: (b"--", x.end))
        encoded, consumed = _codecs.utf_16_encode("ab\udc80", "test")
        self.assertEqual(encoded, b"\xff\xfea\x00b\x00--")
        self.assertEqual(consumed, 3)

    def test_encode_utf_16_with_string_returning_error_handler_returns_bytes(self):
        _codecs.register_error("test", lambda x: ("h", x.end))
        encoded, consumed = _codecs.utf_16_encode("ab\udc80", "test")
        self.assertEqual(encoded, b"\xff\xfea\x00b\x00h\x00")
        self.assertEqual(consumed, 3)

    def test_encode_utf_16_with_non_ascii_error_handler_raises_encode_error(self):
        _codecs.register_error("test", lambda x: ("\x80", x.end))
        with self.assertRaises(UnicodeEncodeError) as context:
            _codecs.utf_16_encode("ab\udc80", "test")
        exc = context.exception
        self.assertEqual(exc.encoding, "utf-16")
        self.assertEqual(exc.reason, "surrogates not allowed")
        self.assertEqual(exc.object, "ab\udc80")
        self.assertEqual(exc.start, 2)
        self.assertEqual(exc.end, 3)


class EncodeUTF32Tests(unittest.TestCase):
    def test_encode_utf_32_with_non_str_first_argument_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.utf_32_encode([])

    def test_encode_utf_32_with_non_str_second_argument_raises_type_error(self):
        with self.assertRaises(TypeError):
            _codecs.utf_32_encode("", [])

    def test_encode_utf_32_with_zero_length_returns_bom(self):
        encoded, consumed = _codecs.utf_32_encode("")
        self.assertEqual(encoded, b"\xff\xfe\x00\x00")
        self.assertEqual(consumed, 0)

    def test_encode_utf_32_with_ascii_returns_bytes(self):
        encoded, consumed = _codecs.utf_32_encode("hi")
        self.assertEqual(encoded, b"\xff\xfe\x00\x00h\x00\x00\x00i\x00\x00\x00")
        self.assertEqual(consumed, 2)

    def test_encode_utf_32_with_latin_1_returns_bytes(self):
        encoded, consumed = _codecs.utf_32_encode("h\xe5")
        self.assertEqual(encoded, b"\xff\xfe\x00\x00h\x00\x00\x00\xe5\x00\x00\x00")
        self.assertEqual(consumed, 2)

    def test_encode_utf_32_with_bmp_returns_bytes(self):
        encoded, consumed = _codecs.utf_32_encode("h\u1005")
        self.assertEqual(encoded, b"\xff\xfe\x00\x00h\x00\x00\x00\x05\x10\x00\x00")
        self.assertEqual(consumed, 2)

    def test_encode_utf_32_with_supplementary_plane_returns_bytes(self):
        encoded, consumed = _codecs.utf_32_encode("h\U0001d1f0i")
        self.assertEqual(
            encoded, b"\xff\xfe\x00\x00h\x00\x00\x00\xf0\xd1\x01\x00i\x00\x00\x00"
        )
        self.assertEqual(consumed, 3)

    def test_encode_utf_32_le_with_supplementary_plane_returns_bytes(self):
        encoded, consumed = _codecs.utf_32_le_encode("h\U0001d1f0i")
        self.assertEqual(encoded, b"h\x00\x00\x00\xf0\xd1\x01\x00i\x00\x00\x00")
        self.assertEqual(consumed, 3)

    def test_encode_utf_32_be_with_supplementary_plane_returns_bytes(self):
        encoded, consumed = _codecs.utf_32_be_encode("h\U0001d1f0i")
        self.assertEqual(encoded, b"\x00\x00\x00h\x00\x01\xd1\xf0\x00\x00\x00i")
        self.assertEqual(consumed, 3)

    def test_encode_utf_32_with_custom_error_handler_mid_bytes_error_returns_bytes(
        self,
    ):
        _codecs.register_error("test", lambda x: (b"----", x.end))
        encoded, consumed = _codecs.utf_32_encode("ab\udc80c", "test")
        self.assertEqual(
            encoded, b"\xff\xfe\x00\x00a\x00\x00\x00b\x00\x00\x00----c\x00\x00\x00"
        )
        self.assertEqual(consumed, 4)

    def test_encode_utf_32_with_custom_error_handler_end_bytes_error_returns_bytes(
        self,
    ):
        _codecs.register_error("test", lambda x: (b"----", x.end))
        encoded, consumed = _codecs.utf_32_encode("ab\udc80", "test")
        self.assertEqual(encoded, b"\xff\xfe\x00\x00a\x00\x00\x00b\x00\x00\x00----")
        self.assertEqual(consumed, 3)

    def test_encode_utf_32_with_string_returning_error_handler_returns_bytes(self):
        _codecs.register_error("test", lambda x: ("h", x.end))
        encoded, consumed = _codecs.utf_32_encode("ab\udc80", "test")
        self.assertEqual(
            encoded, b"\xff\xfe\x00\x00a\x00\x00\x00b\x00\x00\x00h\x00\x00\x00"
        )
        self.assertEqual(consumed, 3)

    def test_encode_utf_32_with_non_ascii_error_handler_raises_encode_error(self):
        _codecs.register_error("test", lambda x: ("\x80", x.end))
        with self.assertRaises(UnicodeEncodeError) as context:
            _codecs.utf_32_encode("ab\udc80", "test")
        exc = context.exception
        self.assertEqual(exc.encoding, "utf-32")
        self.assertEqual(exc.reason, "surrogates not allowed")
        self.assertEqual(exc.object, "ab\udc80")
        self.assertEqual(exc.start, 2)
        self.assertEqual(exc.end, 3)


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

    def test_encode_utf_8_with_surrogatepass_passes_surrogate(self):
        # high surrogate
        encoded, consumed = _codecs.utf_8_encode("ab\udc80c", "surrogatepass")
        self.assertEqual(encoded, b"ab\xed\xb2\x80c")
        self.assertEqual(consumed, 4)

        # low surrogate
        encoded, consumed = _codecs.utf_8_encode("ab\ud9a0c", "surrogatepass")
        self.assertEqual(encoded, b"ab\xed\xa6\xa0c")
        self.assertEqual(consumed, 4)

    def test_encode_utf_8_without_surrogatepass_raises_on_surrogate(self):
        with self.assertRaises(UnicodeEncodeError):
            _codecs.utf_8_encode("ab\udc80c")

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

    def test_encode_utf_8_with_non_ascii_error_handler_raises_encode_error(self):
        _codecs.register_error("test", lambda x: ("\x80", x.end))
        with self.assertRaises(UnicodeEncodeError) as context:
            _codecs.utf_8_encode("ab\udc80", "test")
        exc = context.exception
        self.assertEqual(exc.encoding, "utf-8")
        self.assertEqual(exc.reason, "surrogates not allowed")
        self.assertEqual(exc.object, "ab\udc80")
        self.assertEqual(exc.start, 2)
        self.assertEqual(exc.end, 3)


class EncodeRawUnicodeEscapeTests(unittest.TestCase):
    def test_encode_raw_unicode_escape_with_non_str_first_argument_raises_type_error(
        self,
    ):
        with self.assertRaises(TypeError):
            _codecs.raw_unicode_escape_encode([])

    def test_encode_raw_unicode_escape_with_non_str_second_argument_raises_type_error(
        self,
    ):
        with self.assertRaises(TypeError):
            _codecs.raw_unicode_escape_encode("", [])

    def test_encode_raw_unicode_escape_with_zero_length_returns_bytes(self):
        encoded, consumed = _codecs.raw_unicode_escape_encode("")
        self.assertEqual(encoded, b"")
        self.assertEqual(consumed, 0)

    def test_encode_raw_unicode_escape_with_ascii_returns_bytes(self):
        encoded, consumed = _codecs.raw_unicode_escape_encode("hi")
        self.assertEqual(encoded, b"hi")
        self.assertEqual(consumed, 2)

    def test_encode_raw_unicode_escape_with_latin_1_returns_bytes(self):
        encoded, consumed = _codecs.raw_unicode_escape_encode("h\xe5")
        self.assertEqual(encoded, b"h\xe5")
        self.assertEqual(consumed, 2)

    def test_encode_raw_unicode_escape_with_bmp_returns_bytes(self):
        encoded, consumed = _codecs.raw_unicode_escape_encode("h\u1005")
        self.assertEqual(encoded, b"h\\u1005")
        self.assertEqual(consumed, 2)

    def test_encode_raw_unicode_escape_with_supplementary_plane_returns_bytes(self):
        encoded, consumed = _codecs.raw_unicode_escape_encode("h\U0001d1f0i")
        self.assertEqual(encoded, b"h\\U0001d1f0i")
        self.assertEqual(consumed, 3)


@pyro_only
class GeneralizedErrorHandlerTests(unittest.TestCase):
    def test_call_decode_error_with_strict_raises_unicode_decode_error(self):
        with self.assertRaises(UnicodeDecodeError):
            _codecs._call_decode_errorhandler(
                "strict", b"bad input", _str_array(), "reason", "encoding", 0, 0
            )

    def test_call_decode_error_with_ignore_returns_tuple(self):
        new_input, new_pos = _codecs._call_decode_errorhandler(
            "ignore", b"bad_input", _str_array(), "reason", "encoding", 1, 2
        )
        self.assertEqual(new_input, b"bad_input")
        self.assertEqual(new_pos, 2)

    def test_call_decode_error_with_non_tuple_return_raises_type_error(self):
        def error_function(exc):
            return "not-a-tuple"

        _codecs.register_error("not-a-tuple", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_decode_errorhandler(
                "not-a-tuple", b"bad_input", _str_array(), "reason", "encoding", 1, 2
            )

    def test_call_decode_error_with_small_tuple_return_raises_type_error(self):
        def error_function(exc):
            return ("one",)

        _codecs.register_error("small-tuple", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_decode_errorhandler(
                "small-tuple", b"bad_input", _str_array(), "reason", "encoding", 1, 2
            )

    def test_call_decode_error_with_int_first_tuple_return_raises_type_error(self):
        def error_function(exc):
            return 1, 1

        _codecs.register_error("int-first", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_decode_errorhandler(
                "int-first", b"bad_input", _str_array(), "reason", "encoding", 1, 2
            )

    def test_call_decode_error_with_string_second_tuple_return_raises_type_error(self):
        def error_function(exc):
            return "str_to_append", "new_pos"

        _codecs.register_error("str-second", error_function)
        with self.assertRaises(TypeError):
            _codecs._call_decode_errorhandler(
                "str-second", b"bad_input", _str_array(), "reason", "encoding", 1, 2
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
                _str_array(),
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
                _str_array(),
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
            "negative-pos", b"bad_input", _str_array(), "reason", "encoding", 1, 2
        )
        self.assertEqual(new_input, b"bad_input")
        self.assertEqual(new_pos, 8)

    def test_call_decode_appends_string_to_output(self):
        def error_function(exc):
            return "str_to_append", exc.end

        _codecs.register_error("well-behaved-test", error_function)
        result = _str_array()
        _codecs._call_decode_errorhandler(
            "well-behaved-test", b"bad_input", result, "reason", "encoding", 1, 2
        )
        self.assertEqual(str(result), "str_to_append")

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
