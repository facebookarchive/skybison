#!/usr/bin/env python3
import unittest

from _string import formatter_field_name_split, formatter_parser


class FormatterFieldNameSplitTests(unittest.TestCase):
    def test_multiple_subscripts_returns_multiple_subscripts(self):
        res, it = formatter_field_name_split("0[2][3]")
        self.assertEqual(res, 0)
        self.assertEqual(tuple(it), ((False, 2), (False, 3)))

    def test_non_dot_or_bracket_between_brackets_raises_value_error(self):
        res, it = formatter_field_name_split("0[4]a[20]")
        self.assertEqual(res, 0)
        self.assertEqual(it.__next__(), (False, 4))
        with self.assertRaises(ValueError) as context:
            it.__next__()
        self.assertEqual(
            str(context.exception),
            "Only '.' or '[' may follow ']' in format field specifier",
        )

    def test_string_as_subscript_uses_string_as_subscript(self):
        res, it = formatter_field_name_split("0[a]")
        self.assertEqual(res, 0)
        self.assertEqual(tuple(it), ((False, "a"),))

    def test_left_bracket_as_subscript_uses_bracket_as_subscript(self):
        res, it = formatter_field_name_split("0[[]")
        self.assertEqual(res, 0)
        self.assertEqual(tuple(it), ((False, "["),))

    def test_incomplete_subscript_raises_value_error(self):
        res, it = formatter_field_name_split("0[2")
        self.assertEqual(res, 0)
        with self.assertRaises(ValueError) as context:
            it.__next__()
        self.assertEqual(str(context.exception), "Missing ']' in format string")

        res, it = formatter_field_name_split("[2")
        # TODO(T64268423): Once CPython has a fix, test for 0 instead of "" or
        # 0.
        self.assertTrue(res == "" or res == 0)
        with self.assertRaises(ValueError):
            it.__next__()

        res, it = formatter_field_name_split("][")
        self.assertEqual(res, "]")
        with self.assertRaises(ValueError):
            it.__next__()

    def test_missing_left_bracket_includes_right_bracket_in_field_name(self):
        res, it = formatter_field_name_split("2]")
        self.assertEqual(res, "2]")
        self.assertEqual(tuple(it), ())

        res, it = formatter_field_name_split("]2")
        self.assertEqual(res, "]2")
        self.assertEqual(tuple(it), ())

    def test_missing_subscript_raises_value_error(self):
        res, it = formatter_field_name_split("[]")
        # TODO(T64268423): Once CPython has a fix, test for 0 instead of "" or
        # 0.
        self.assertTrue(res == "" or res == 0)
        with self.assertRaises(ValueError):
            it.__next__()

    def test_number_indexed_with_subscript(self):
        res, it = formatter_field_name_split("0[2]")
        self.assertEqual(res, 0)
        self.assertEqual(tuple(it), ((False, 2),))

    def test_name_indexed_with_subscript(self):
        res, it = formatter_field_name_split("hello[2]")
        self.assertEqual(res, "hello")
        self.assertEqual(tuple(it), ((False, 2),))

    def test_no_name_indexed_with_subscript(self):
        res, it = formatter_field_name_split("[2]")
        # TODO(T64268423): Once CPython has a fix, test for 0 instead of "" or
        # 0.
        self.assertTrue(res == "" or res == 0)
        self.assertEqual(tuple(it), ((False, 2),))

    def test_select_attr_from_name(self):
        res, it = formatter_field_name_split("hello.attr")
        self.assertEqual(res, "hello")
        self.assertEqual(tuple(it), ((True, "attr"),))

    def test_select_multiple_attr_from_name(self):
        res, it = formatter_field_name_split("hello.first.second.third")
        self.assertEqual(res, "hello")
        self.assertEqual(
            tuple(it), ((True, "first"), (True, "second"), (True, "third"))
        )

    def test_select_attr_from_number(self):
        res, it = formatter_field_name_split("0.attr")
        self.assertEqual(res, 0)
        self.assertEqual(tuple(it), ((True, "attr"),))

    def test_select_attr_from_name_and_index_with_subscript(self):
        res1, it1 = formatter_field_name_split("hello.attr[2]")
        res2, it2 = formatter_field_name_split("hello[2].attr")
        self.assertEqual(res1, res2, "hello")
        self.assertEqual(tuple(it1), ((True, "attr"), (False, 2)))
        self.assertEqual(tuple(it2), ((False, 2), (True, "attr")))

    def test_select_attr_from_number_and_index_with_subscript(self):
        res1, it1 = formatter_field_name_split("0.attr[2]")
        res2, it2 = formatter_field_name_split("0[2].attr")
        self.assertEqual(res1, res2, 0)
        self.assertEqual(tuple(it1), ((True, "attr"), (False, 2)))
        self.assertEqual(tuple(it2), ((False, 2), (True, "attr")))

    def test_missing_attr_raises_value_error(self):
        res, it = formatter_field_name_split("hello.")
        with self.assertRaises(ValueError) as context:
            it.__next__()
        self.assertEqual(str(context.exception), "Empty attribute in format string")

    def test_simple_string_returns_string_iter_tuple(self):
        res, it = formatter_field_name_split("foo")
        self.assertEqual(res, "foo")
        self.assertEqual(tuple(it), ())

    def test_empty_string_returns_string_iter_tuple(self):
        res, it = formatter_field_name_split("")
        self.assertEqual(res, "")
        self.assertEqual(tuple(it), ())

    def test_simple_string_returns_int_iter_tuple(self):
        res, it = formatter_field_name_split("42")
        self.assertEqual(res, 42)
        self.assertEqual(tuple(it), ())

    def test_non_string_raises_type_error(self):
        with self.assertRaises(TypeError):
            formatter_field_name_split(42)


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

    def test_format_with_square_in_spec_returns_tuple(self):
        self.assertEqual(
            tuple(formatter_parser("ab{0[0]}cd")),
            (("ab", "0[0]", "", None), ("cd", None, None, None)),
        )
        self.assertEqual(
            tuple(formatter_parser("ab{hello[0]}cd")),
            (("ab", "hello[0]", "", None), ("cd", None, None, None)),
        )
        self.assertEqual(
            tuple(formatter_parser("ab{[0]}cd")),
            (("ab", "[0]", "", None), ("cd", None, None, None)),
        )
        self.assertEqual(
            tuple(formatter_parser("ab{[0][1][2]}cd")),
            (("ab", "[0][1][2]", "", None), ("cd", None, None, None)),
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
