#!/usr/bin/env python3
import sys
import unittest


if sys.implementation.name == "pyro":
    from _json import loads, JSONDecodeError
else:
    from json import loads, JSONDecodeError


class LoadsTests(unittest.TestCase):
    def test_number_returns_int(self):
        self.assertEqual(loads("0"), 0)
        self.assertEqual(loads("1"), 1)
        self.assertEqual(loads("2"), 2)
        self.assertEqual(loads("3"), 3)
        self.assertEqual(loads("4"), 4)
        self.assertEqual(loads("5"), 5)
        self.assertEqual(loads("6"), 6)
        self.assertEqual(loads("7"), 7)
        self.assertEqual(loads("8"), 8)
        self.assertEqual(loads("9"), 9)
        self.assertEqual(loads("10"), 10)
        self.assertEqual(loads("91284647"), 91284647)
        self.assertEqual(loads("-1"), -1)
        self.assertEqual(loads("-42"), -42)
        self.assertEqual(loads("-0"), 0)
        self.assertEqual(loads("1000000000000000000"), 1000000000000000000)
        self.assertEqual(loads("-1000000000000000000"), -1000000000000000000)

    def test_large_number_returns_int(self):
        self.assertEqual(loads("10000000000000000000"), 10000000000000000000)
        self.assertEqual(loads("-10000000000000000000"), -10000000000000000000)
        self.assertEqual(
            loads("123456789123456789123456789123456789123456789"),
            123456789123456789123456789123456789123456789,
        )
        self.assertEqual(
            loads("-666666666666666666666666666666"), -666666666666666666666666666666
        )

    def test_number_with_whitespace_returns_int(self):
        self.assertEqual(loads("0 "), 0)
        self.assertEqual(loads("\r0"), 0)
        self.assertEqual(loads("-1\t"), -1)
        self.assertEqual(loads("\n-1"), -1)
        self.assertEqual(loads("99 "), 99)
        self.assertEqual(loads("\t99"), 99)
        self.assertEqual(loads(" \r\n\t-5700 "), -5700)
        self.assertEqual(loads(" -5700\t\r \t\t \n"), -5700)

    def test_number_calls_parse_int(self):
        arg = None
        marker = object()

        def func(string):
            nonlocal arg
            arg = string
            return marker

        self.assertIs(loads("12", parse_int=func), marker)
        self.assertEqual(arg, "12")
        self.assertIs(loads(" 0\t", parse_int=func), marker)
        self.assertEqual(arg, "0")
        self.assertIs(loads("-712\r\n", parse_int=func), marker)
        self.assertEqual(arg, "-712")
        self.assertIs(
            loads("   1234567890123456789012345687898239746924673564", parse_int=func),
            marker,
        )
        self.assertEqual(arg, "1234567890123456789012345687898239746924673564")
        self.assertIs(
            loads("-12233344445555566666677777778888888999999999", parse_int=func),
            marker,
        )
        self.assertEqual(arg, "-12233344445555566666677777778888888999999999")

    def test_minus_without_digit_raises_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 1 column 1 \(char 0\)"
        ):
            loads("-")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 1 column 1 \(char 0\)"
        ):
            loads("-a")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 1 column 1 \(char 0\)"
        ):
            loads("- 5")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 1 column 1 \(char 0\)"
        ):
            loads("- Infinity")

    def test_leading_zeros_raises_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError, r"Extra data: line 1 column 2 \(char 1\)"
        ):
            loads("00")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Extra data: line 1 column 2 \(char 1\)"
        ):
            loads("04")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Extra data: line 1 column 2 \(char 1\)"
        ):
            loads("099")

    def test_string_returns_str(self):
        self.assertEqual(loads('""'), "")
        self.assertEqual(loads('" "'), " ")
        self.assertEqual(loads('"hello"'), "hello")
        self.assertEqual(loads('"hello y\'all"'), "hello y'all")

    def test_control_character_in_string_raises_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError, r"Invalid control character at: line 1 column 2 \(char 1\)"
        ):
            loads('"\x00"')
        with self.assertRaisesRegex(
            JSONDecodeError, r"Invalid control character at: line 1 column 7 \(char 6\)"
        ):
            loads('"hello\x01"')
        with self.assertRaisesRegex(
            JSONDecodeError, r"Invalid control character at: line 2 column 5 \(char 5\)"
        ):
            loads('\n"hel\x10lo"')
        with self.assertRaisesRegex(
            JSONDecodeError, r"Invalid control character at: line 1 column 4 \(char 3\)"
        ):
            loads('\t "\x1fhello"')

    def test_unterminated_string_raises_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError,
            r"Unterminated string starting at: line 1 column 1 \(char 0\)",
        ):
            loads('"')
        with self.assertRaisesRegex(
            JSONDecodeError,
            r"Unterminated string starting at: line 2 column 1 \(char 2\)",
        ):
            loads('\t\n"he\nlo', strict=False)

    def test_with_strict_false_returns_str(self):
        self.assertEqual(loads('"he\x00llo"', strict=False), "he\x00llo")
        control_chars = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1a\x1b\x1c\x1d\x1e\x1f"
        self.assertEqual(loads(f'"{control_chars}"', strict=False), control_chars)

    def test_string_with_escape_returns_str(self):
        self.assertEqual(loads('"\\""'), '"')
        self.assertEqual(loads('"\\\\"'), "\\")
        self.assertEqual(loads('"\\/"'), "/")
        self.assertEqual(loads('"\\b"'), "\b")
        self.assertEqual(loads('"\\f"'), "\f")
        self.assertEqual(loads('"\\n"'), "\n")
        self.assertEqual(loads('"\\r"'), "\r")
        self.assertEqual(loads('"\\t"'), "\t")

    def test_string_with_u_escape_returns_str(self):
        self.assertEqual(loads('"\\u0059"'), "Y")
        self.assertEqual(loads('"\\u1234"'), "\u1234")
        self.assertEqual(loads('"\\ucafe"'), "\ucafe")
        self.assertEqual(loads('"\\uCAFE"'), "\ucafe")
        self.assertEqual(loads('"\\uCa00"'), "\uca00")
        self.assertEqual(loads('"\\u99fE"'), "\u99fe")

    def test_string_with_u_escape_combines_surrogates(self):
        self.assertEqual(loads('"\\ud83e\\udd20"'), "\U0001f920")
        self.assertEqual(loads('"\\udbff\\udfff"'), "\U0010ffff")

    def test_string_with_lone_surrogates_returns_str(self):
        self.assertEqual(loads('"\\ud83e"'), "\ud83e")
        self.assertEqual(loads('"\\ud83e\\ud83e"'), "\ud83e\ud83e")
        self.assertEqual(loads('"\\udd20"'), "\udd20")
        self.assertEqual(loads('"\\udd20\\ud83e"'), "\udd20\ud83e")

    def test_string_with_whitespace_returns_str(self):
        self.assertEqual(loads(' ""'), "")
        self.assertEqual(loads('"" '), "")
        self.assertEqual(loads(' "" '), "")

    def test_chars_after_string_raises_json_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError, r"Extra data: line 1 column 3 \(char 2\)"
        ):
            loads('""a')
        with self.assertRaisesRegex(
            JSONDecodeError, r"Extra data: line 1 column 7 \(char 6\)"
        ):
            loads('""    ""')

    def test_string_with_unterminated_escape_raises_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError,
            r"Unterminated string starting at: line 1 column 1 \(char 0\)",
        ):
            loads('"\\')

    def test_string_with_invalid_escape_raises_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError, r"Invalid \\escape: line 1 column 2 \(char 1\)"
        ):
            loads('"\\x"')
        with self.assertRaisesRegex(
            JSONDecodeError, r"Invalid \\escape: line 1 column 2 \(char 1\)"
        ):
            loads('"\\\U0001f974"')

    def test_string_with_invalid_u_escape_raises_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError, r"Invalid \\uXXXX escape: line 1 column 3 \(char 2\)"
        ):
            loads('"\\u')
        with self.assertRaisesRegex(
            JSONDecodeError, r"Invalid \\uXXXX escape: line 1 column 3 \(char 2\)"
        ):
            loads('"\\u123')
        with self.assertRaisesRegex(
            JSONDecodeError, r"Invalid \\uXXXX escape: line 1 column 3 \(char 2\)"
        ):
            loads('"\\u000g')
        with self.assertRaisesRegex(
            JSONDecodeError, r"Invalid \\uXXXX escape: line 1 column 9 \(char 8\)"
        ):
            loads('"\\ud83e\\u5x"')

    def test_true_returns_bool(self):
        self.assertIs(loads("true"), True)

    def test_false_returns_bool(self):
        self.assertIs(loads("false"), False)

    def test_null_returns_none(self):
        self.assertIs(loads("null"), None)

    def test_infinity_returns_float(self):
        self.assertEqual(loads("Infinity"), float("inf"))

    def test_minus_infinity_returns_float(self):
        self.assertEqual(loads("-Infinity"), float("-inf"))

    def test_nan_returns_float(self):
        self.assertEqual(str(loads("NaN")), "nan")

    def test_whitespace_around_constant_is_ignored(self):
        self.assertIs(loads("true "), True)
        self.assertIs(loads("\tfalse"), False)
        self.assertIs(loads("\r null \n"), None)
        self.assertEqual(loads("  Infinity   "), float("inf"))
        self.assertEqual(loads("\n\r-Infinity\t"), float("-inf"))
        self.assertEqual(str(loads("\r\nNaN\t")), "nan")

    def test_calls_parse_constant(self):
        arg = None
        marker = object()

        def func(string):
            nonlocal arg
            arg = string
            return marker

        self.assertIs(loads("NaN", parse_constant=func), marker)
        self.assertEqual(arg, "NaN")
        self.assertIs(loads(" -Infinity\t\r", parse_constant=func), marker)
        self.assertEqual(arg, "-Infinity")
        self.assertIs(loads("  Infinity\n\r", parse_constant=func), marker)
        self.assertEqual(arg, "Infinity")

    def test_does_not_call_parse_constant(self):
        def func(string):
            raise Exception("should not be called")

        self.assertIs(loads("true", parse_constant=func), True)
        self.assertIs(loads("false", parse_constant=func), False)
        self.assertIs(loads("null", parse_constant=func), None)

    def test_parse_constant_propagates_exception(self):
        def func(string):
            raise UserWarning("test")

        with self.assertRaises(UserWarning):
            loads("NaN", parse_constant=func)

    def test_constant_with_extra_chars_raises_json_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError, r"Extra data: line 1 column 5 \(char 4\)"
        ):
            loads("truee")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Extra data: line 1 column 7 \(char 6\)"
        ):
            loads("false null")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Extra data: line 2 column 2 \(char 5\)"
        ):
            loads("NaN\n\ta")

    def test_whitespace_string_raises_json_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 1 column 2 \(char 1\)"
        ):
            loads(" ")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 2 column 1 \(char 4\)"
        ):
            loads("\t\r \n")

    def test_empty_string_raises_json_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 1 column 1 \(char 0\)"
        ):
            loads("")

    def test_unexpected_char_raises_json_decode_error(self):
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 1 column 1 \(char 0\)"
        ):
            loads("a")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 1 column 1 \(char 0\)"
        ):
            loads("$")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 1 column 1 \(char 0\)"
        ):
            loads("\x00")
        with self.assertRaisesRegex(
            JSONDecodeError, r"Expecting value: line 1 column 1 \(char 0\)"
        ):
            loads("\U0001f480")

    def test_json_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "the JSON object must be str, bytes or bytearray, not float"
        ):
            loads(42.42)


if __name__ == "__main__":
    unittest.main()
