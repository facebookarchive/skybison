#!/usr/bin/env python3
import sys
import unittest


if sys.implementation.name == "pyro":
    from _json import loads, JSONDecodeError
else:
    from json import loads, JSONDecodeError


class LoadsTests(unittest.TestCase):
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
