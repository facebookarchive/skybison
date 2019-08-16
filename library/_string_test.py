#!/usr/bin/env python3
import unittest

from _string import formatter_parser


class FormatterParserTests(unittest.TestCase):
    def test_empty_string_returns_empty_tuple(self):
        self.assertEqual(tuple(formatter_parser("")), ())

    def test_simple_string_returns_tuple(self):
        self.assertEqual(tuple(formatter_parser(" ")), ((" ", None, None, None),))
        self.assertEqual(
            tuple(formatter_parser("hello")), (("hello", None, None, None),)
        )
        self.assertEqual(
            tuple(formatter_parser("~!@#$%^&*()<>.")),
            (("~!@#$%^&*()<>.", None, None, None),),
        )

    def test_double_curlies(self):
        self.assertEqual(tuple(formatter_parser("{{")), (("{", None, None, None),))
        self.assertEqual(tuple(formatter_parser("}}")), (("}", None, None, None),))
        self.assertEqual(
            tuple(formatter_parser("{{ab")),
            (("{", None, None, None), ("ab", None, None, None)),
        )
        self.assertEqual(
            tuple(formatter_parser("}}ab")),
            (("}", None, None, None), ("ab", None, None, None)),
        )
        self.assertEqual(tuple(formatter_parser("ab{{")), (("ab{", None, None, None),))
        self.assertEqual(tuple(formatter_parser("ab}}")), (("ab}", None, None, None),))
        self.assertEqual(
            tuple(formatter_parser("ab{{cd")),
            (("ab{", None, None, None), ("cd", None, None, None)),
        )
        self.assertEqual(
            tuple(formatter_parser("ab}}cd")),
            (("ab}", None, None, None), ("cd", None, None, None)),
        )
        self.assertEqual(
            tuple(formatter_parser("he{{}}llo")),
            (
                ("he{", None, None, None),
                ("}", None, None, None),
                ("llo", None, None, None),
            ),
        )

    def test_format_with_identifier_returns_tuple(self):
        self.assertEqual(tuple(formatter_parser("{}")), (("", "", "", None),))
        self.assertEqual(tuple(formatter_parser("{foo}")), (("", "foo", "", None),))
        self.assertEqual(
            tuple(formatter_parser("foo{bar}")), (("foo", "bar", "", None),)
        )

    def test_format_with_conversion_specifier_returns_tuple(self):
        self.assertEqual(tuple(formatter_parser("{!x}")), (("", "", "", "x"),))
        self.assertEqual(tuple(formatter_parser("{foo!x}")), (("", "foo", "", "x"),))
        self.assertEqual(tuple(formatter_parser("{!x}")), (("", "", "", "x"),))
        self.assertEqual(
            tuple(formatter_parser("foo{bar!b}")), (("foo", "bar", "", "b"),)
        )
        self.assertEqual(tuple(formatter_parser("{!}}")), (("", "", "", "}"),))
        self.assertEqual(tuple(formatter_parser("{!{}")), (("", "", "", "{"),))

    def test_format_with_spec_returns_tuple(self):
        self.assertEqual(tuple(formatter_parser("{:foo}")), (("", "", "foo", None),))
        self.assertEqual(
            tuple(formatter_parser("{:foo!x}")), (("", "", "foo!x", None),)
        )
        self.assertEqual(
            tuple(formatter_parser("{foo:bar}")), (("", "foo", "bar", None),)
        )
        self.assertEqual(
            tuple(formatter_parser("foo{bar:baz}")), (("foo", "bar", "baz", None),)
        )

    def test_format_with_curly_in_spec_returns_tuple(self):
        self.assertEqual(
            tuple(formatter_parser("{:{foo}}")), (("", "", "{foo}", None),)
        )
        self.assertEqual(tuple(formatter_parser("{:{}}")), (("", "", "{}", None),))
        self.assertEqual(
            tuple(formatter_parser("{:{foo{bar}baz}}")),
            (("", "", "{foo{bar}baz}", None),),
        )
        self.assertEqual(
            tuple(formatter_parser("{:foo{bar}baz}")), (("", "", "foo{bar}baz", None),)
        )

    def test_format_with_all_returns_tuple(self):
        self.assertEqual(
            tuple(formatter_parser("foo{bar!r:baz}bam {15:2} {xxx!a}")),
            (
                ("foo", "bar", "baz", "r"),
                ("bam ", "15", "2", None),
                (" ", "xxx", "", "a"),
            ),
        )

    def test_unbalanced_rcurly_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            tuple(formatter_parser("}"))
        self.assertEqual(
            str(context.exception), "Single '}' encountered in format string"
        )

        with self.assertRaises(ValueError) as context:
            tuple(formatter_parser("a} }"))
        self.assertEqual(
            str(context.exception), "Single '}' encountered in format string"
        )

    def test_unbalanced_lcurly_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            tuple(formatter_parser("{"))
        self.assertEqual(
            str(context.exception), "Single '{' encountered in format string"
        )

        with self.assertRaises(ValueError) as context:
            tuple(formatter_parser("a{bcd"))
        self.assertEqual(str(context.exception), "expected '}' before end of string")

        with self.assertRaises(ValueError) as context:
            tuple(formatter_parser("a{bcd!"))
        self.assertEqual(
            str(context.exception),
            "end of string while looking for conversion specifier",
        )

        with self.assertRaises(ValueError) as context:
            tuple(formatter_parser("a{bcd!x"))
        self.assertEqual(str(context.exception), "unmatched '{' in format spec")

        with self.assertRaises(ValueError) as context:
            tuple(formatter_parser("a{bcd:"))
        self.assertEqual(str(context.exception), "unmatched '{' in format spec")

    def test_missing_conversion_specifier_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            tuple(formatter_parser("a{bcd!}"))
        self.assertEqual(str(context.exception), "unmatched '{' in format spec")

    def test_chars_after_conversion_specifier_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            tuple(formatter_parser("a{bcd!xy}"))
        self.assertEqual(
            str(context.exception), "expected ':' after conversion specifier"
        )

    def test_lcurly_in_field_name_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            tuple(formatter_parser("{foo{bar}baz}"))
        self.assertEqual(str(context.exception), "unexpected '{' in field name")

    def test_non_string_raise_type_error(self):
        with self.assertRaises(TypeError):
            tuple(formatter_parser(42))


if __name__ == "__main__":
    unittest.main()
