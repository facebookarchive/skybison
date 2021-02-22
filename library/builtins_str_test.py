#!/usr/bin/env python3
import unittest
import warnings
from unittest.mock import Mock


class StrTests(unittest.TestCase):
    def test_binary_add_with_non_str_other_falls_back_on_other_dunder_radd(self):
        class C:
            def __radd__(self, other):
                return (self, other)

        instance = C()
        self.assertEqual("a" + instance, (instance, "a"))

    def test_binary_add_with_str_subclass_other_falls_back_on_other_dunder_radd(self):
        class C(str):
            def __radd__(self, other):
                return (self, other)

        instance = C("foo")
        self.assertEqual("a" + instance, (instance, "a"))

    def test_dunder_getitem_with_int_returns_code_point(self):
        s = "a\u05D0b\u05D1c\u05D2"
        self.assertEqual(s[0], "a")
        self.assertEqual(s[1], "\u05D0")
        self.assertEqual(s[2], "b")
        self.assertEqual(s[3], "\u05D1")
        self.assertEqual(s[4], "c")
        self.assertEqual(s[5], "\u05D2")

    def test_dunder_getitem_with_large_int_raises_index_error(self):
        s = "hello"
        with self.assertRaises(IndexError) as context:
            s[2 ** 63]
        self.assertEqual(
            str(context.exception), "cannot fit 'int' into an index-sized integer"
        )

    def test_dunder_getitem_with_negative_int_indexes_from_end(self):
        s = "a\u05D0b\u05D1c\u05D2"
        self.assertEqual(s[-6], "a")
        self.assertEqual(s[-5], "\u05D0")
        self.assertEqual(s[-4], "b")
        self.assertEqual(s[-3], "\u05D1")
        self.assertEqual(s[-2], "c")
        self.assertEqual(s[-1], "\u05D2")

    def test_dunder_getitem_negative_out_of_bounds_raises_index_error(self):
        s = "hello"
        with self.assertRaises(IndexError) as context:
            s[-6]
        self.assertEqual(str(context.exception), "string index out of range")

    def test_dunder_getitem_positive_out_of_bounds_raises_index_error(self):
        s = "hello"
        with self.assertRaises(IndexError) as context:
            s[5]
        self.assertEqual(str(context.exception), "string index out of range")

    def test_dunder_getitem_with_slice_returns_str(self):
        s = "hello world"
        self.assertEqual(s[6:], "world")
        self.assertEqual(s[:5], "hello")
        self.assertEqual(s[::2], "hlowrd")
        self.assertEqual(s[::-1], "dlrow olleh")
        self.assertEqual(s[1:8:2], "el o")
        self.assertEqual(s[-1:3:-3], "doo")

    def test_dunder_getitem_with_slice_returns_empty_str(self):
        s = "hello world"
        self.assertEqual(s[0:0], "")
        self.assertEqual(s[1:0], "")
        self.assertEqual(s[5:2], "")
        self.assertEqual(s[5:-6], "")
        self.assertEqual(s[-4:-7], "")

    def test_dunder_getitem_with_slice_indexes_by_code_point(self):
        s = "# \xc2\xa9 2018 Unicode\xc2\xae, Inc.\n"
        self.assertEqual(s[10:], "Unicode\xc2\xae, Inc.\n")
        self.assertEqual(s[:9], "# \xc2\xa9 2018")
        self.assertEqual(s[::2], "#\xc2 08Uioe\xae n.")
        self.assertEqual(s[1:8:2], " \xa921")
        self.assertEqual(s[-1:3:-3], "\nn,ecU1 ")

    def test_dunder_getitem_with_slice_uses_adjusted_bounds(self):
        s = "hello world"
        self.assertEqual(s[-20:5], "hello")
        self.assertEqual(s[6:42], "world")

    def test_expandtabs_with_non_int_raises_type_error(self):
        with self.assertRaises(TypeError):
            "expand\tme".expandtabs("not-int")

    def test_expandtabs_with_no_args_expands_to_eight_spaces(self):
        s = "expand\tme"
        self.assertEqual(s.expandtabs(), "expand  me")

    def test_expandtabs_with_zero_deletes_tabs(self):
        s = "expand\tme"
        self.assertEqual(s.expandtabs(0), "expandme")

    def test_expandtabs_expands_column_to_given_number_of_spaces(self):
        s = "expand\tme"
        self.assertEqual(s.expandtabs(3), "expand   me")

    def test_expandtabs_with_newlines_resets_col_position(self):
        s = "123\t12345\t\n1234\t1\r12\t1234\t123\t1"
        self.assertEqual(s.expandtabs(6), "123   12345 \n1234  1\r12    1234  123   1")

    def test_title_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.title(123)

    def test_title_with_empty_string_returns_itself(self):
        a = ""
        self.assertIs(a.title(), "")

    def test_title_with_non_empty_string_returns_new_string(self):
        a = "Hello World"
        result = a.title()
        self.assertEqual(a, result)
        self.assertIsNot(a, result)

    def test_title_with_single_lowercase_word_returns_titlecased(self):
        self.assertEqual("abc".title(), "Abc")

    def test_title_with_single_uppercase_word_returns_titlecased(self):
        self.assertEqual("ABC".title(), "Abc")

    def test_title_with_multiple_words_returns_titlecased(self):
        self.assertEqual("foo bar baz".title(), "Foo Bar Baz")

    def test_title_with_words_with_special_chars_eturns_titlecased(self):
        self.assertEqual(
            "they're bill's friends from the UK".title(),
            "They'Re Bill'S Friends From The Uk",
        )

    def test_translate_without_dict(self):
        with self.assertRaises(TypeError):
            "abc".translate(123)

    def test_translate_without_non_valid_dict_value(self):
        with self.assertRaises(TypeError):
            "abc".translate({97: 123.456}, "int.*, None or str")

    def test_translate_without_characters_in_dict_returns_same_str(self):
        original = "abc"
        expected = "abc"
        table = {120: 130, 121: 131, 122: 132}
        self.assertEqual(original.translate(table), expected)

    def test_translate_with_none_values_returns_substr(self):
        original = "abc"
        expected = "bc"
        table = {97: None}
        self.assertEqual(original.translate(table), expected)

    def test_translate_returns_new_str(self):
        original = "abc"
        expected = "dbc"
        table = {97: 100}
        self.assertEqual(original.translate(table), expected)

    def test_translate_returns_longer_str(self):
        original = "abc"
        expected = "defbc"
        table = {97: "def"}
        self.assertEqual(original.translate(table), expected)

    def test_translate_with_short_str_returns_same_str(self):
        original = "foobar"
        expected = "foobar"
        table = "baz"
        self.assertEqual(original.translate(table), expected)

    def test_translate_with_lowercase_ascii_can_lowercase(self):
        table = "".join(
            [chr(i).lower() if chr(i).isupper() else chr(i) for i in range(128)]
        )
        original = "FoOBaR"
        expected = "foobar"
        self.assertEqual(original.translate(table), expected)

    def test_translate_with_getitem_calls_it(self):
        class Sequence:
            def __getitem__(self, index):
                if index == ord("a"):
                    return "z"
                if index == ord("b"):
                    return None
                if index == ord("c"):
                    return 0xC6
                raise LookupError

        original = "abcd"
        expected = "z\u00c6d"
        self.assertEqual(original.translate(Sequence()), expected)

    def test_translate_with_getitem_calls_type_slot(self):
        class Sequence:
            def __getitem__(self, index):
                if index == ord("a"):
                    return "z"

        s = Sequence()
        s.__getitem__ = None
        self.assertEqual("a".translate(s), "z")

    def test_translate_with_invalid_getitem_raises_type_error(self):
        class Sequence:
            def __getitem__(self, index):
                if index == ord("a"):
                    return b"a"

        with self.assertRaises(TypeError):
            "invalid".translate(Sequence())

    def test_maketrans_dict_with_non_str_or_int_keys_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, "translate table must be"):
            str.maketrans({123.456: 2})

    def test_maketrans_dict_with_int_key_returns_translation_table(self):
        result = str.maketrans({97: 2})
        expected = {97: 2}
        self.assertEqual(result, expected)

    def test_maketrans_dict_with_str_subtype_key_returns_translation_table(self):
        class substr(str):
            pass

        result = str.maketrans({substr("a"): 2})
        expected = {97: 2}
        self.assertEqual(result, expected)

    def test_maketrans_with_dict_returns_translation_table(self):
        result = str.maketrans({"a": 2})
        expected = {97: 2}
        self.assertEqual(result, expected)

    def test_maketrans_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.maketrans(1, "b")
        with self.assertRaises(TypeError):
            str.maketrans("a", 98)

    def test_maketrans_with_non_equal_len_str_raises_value_error(self):
        with self.assertRaisesRegex(ValueError, "equal len"):
            str.maketrans("a", "ab")

    def test_maketrans_with_str_returns_translation_table(self):
        result = str.maketrans("abc", "def")
        expected = {97: 100, 98: 101, 99: 102}
        self.assertEqual(result, expected)

    def test_maketrans_with_third_arg_non_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.maketrans("abc", "def", 123)

    def test_maketrans_with_third_arg_returns_translation_table_with_none(self):
        result = str.maketrans("abc", "def", "cd")
        expected = {97: 100, 98: 101, 99: None, 100: None}
        self.assertEqual(result, expected)

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

    def test_casefold_with_non_str_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.casefold(1)
        self.assertIn(
            "'casefold' requires a 'str' object but received a 'int'",
            str(context.exception),
        )

    def test_casefold_with_empty_string_returns_itself(self):
        self.assertEqual("".casefold(), "")

    def test_casefold_with_ascii_returns_folded_str(self):
        self.assertEqual("abc".casefold(), "abc")
        self.assertEqual("ABC".casefold(), "abc")
        self.assertEqual("aBcDe".casefold(), "abcde")
        self.assertEqual("aBc123".casefold(), "abc123")

    def test_casefold_with_nonascii_returns_folded_str(self):
        self.assertEqual("\u00c5\u00c6\u00c7".casefold(), "\u00e5\u00e6\u00e7")
        self.assertEqual("\u00c5A\u00c6B\u00c7".casefold(), "\u00e5a\u00e6b\u00e7")
        self.assertEqual("A\u00c5\u00c6\u00c7C".casefold(), "a\u00e5\u00e6\u00e7c")

    def test_casefold_with_lower_extended_case_returns_folded_str(self):
        self.assertEqual("der Flu\u00DF".casefold(), "der fluss")
        self.assertEqual("\u00DF".casefold(), "ss")

    def test_casefold_with_str_subclass_returns_folded_str(self):
        class C(str):
            pass

        s = C("der Flu\u00DF").casefold()
        self.assertIs(type(s), str)
        self.assertEqual(s, "der fluss")

    def test_center_with_non_int_index_width_calls_dunder_index(self):
        class W:
            def __index__(self):
                return 5

        self.assertEqual(str.center("abc", W()), " abc ")

    def test_center_returns_justified_string(self):
        self.assertEqual(str.center("abc", -1), "abc")
        self.assertEqual(str.center("abc", 0), "abc")
        self.assertEqual(str.center("abc", 1), "abc")
        self.assertEqual(str.center("abc", 2), "abc")
        self.assertEqual(str.center("abc", 3), "abc")
        self.assertEqual(str.center("abc", 4), "abc ")
        self.assertEqual(str.center("abc", 5), " abc ")
        self.assertEqual(str.center("abc", 6), " abc  ")
        self.assertEqual(str.center("abc", 7), "  abc  ")
        self.assertEqual(str.center("abc", 8), "  abc   ")
        self.assertEqual(str.center("", 4), "    ")
        self.assertEqual(str.center("\t \n", 6), " \t \n  ")

    def test_center_with_custom_fillchar_returns_str(self):
        self.assertEqual(str.center("ba", 7, "@"), "@@@ba@@")
        self.assertEqual(
            str.center("x\U0001f43bx", 6, "\U0001f40d"),
            "\U0001f40dx\U0001f43bx\U0001f40d\U0001f40d",
        )

    def test_center_returns_identity(self):
        s = "foo bar baz bam!"
        self.assertIs(str.center(s, -1), s)
        self.assertIs(str.center(s, 5), s)
        self.assertIs(str.center(s, len(s)), s)

    def test_center_with_fillchar_not_char_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "The fill character must be exactly one character long"
        ):
            str.center("", 2, "")
        with self.assertRaisesRegex(
            TypeError, "The fill character must be exactly one character long"
        ):
            str.center("", 2, ",,")

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

    def test_encode_idna_returns_ascii_encoded_str(self):
        self.assertEqual("foo".encode("idna"), b"foo")

    def test_endswith_with_non_str_or_tuple_prefix_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "".endswith([])
        self.assertEqual(
            str(context.exception),
            "endswith first arg must be str or a tuple of str, not list",
        )

    def test_endswith_with_non_str_or_tuple_prefix_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "hello".endswith(("e", b"", "h"))
        self.assertEqual(
            str(context.exception),
            "tuple for endswith must only contain str, not bytes",
        )

    def test_endswith_with_empty_string_returns_true_for_valid_bounds(self):
        self.assertTrue("".endswith(""))
        self.assertTrue("".endswith("", -1))
        self.assertTrue("".endswith("", 0, 10))
        self.assertTrue("hello".endswith(""))
        self.assertTrue("hello".endswith("", 5))
        self.assertTrue("hello".endswith("", 2, -1))

    def test_endswith_with_empty_string_returns_false_for_invalid_bounds(self):
        self.assertFalse("".endswith("", 1))
        self.assertFalse("hello".endswith("", 6))
        self.assertFalse("hello".endswith("", -1, 1))

    def test_endswith_with_suffix_returns_true(self):
        self.assertTrue("hello".endswith("o"))
        self.assertTrue("hello".endswith("lo"))
        self.assertTrue("hello".endswith("llo"))
        self.assertTrue("hello".endswith("ello"))
        self.assertTrue("hello".endswith("hello"))

    def test_endswith_with_nonsuffix_returns_false(self):
        self.assertFalse("hello".endswith("l"))
        self.assertFalse("hello".endswith("allo"))
        self.assertFalse("hello".endswith("he"))
        self.assertFalse("hello".endswith("elo"))
        self.assertFalse("hello".endswith("foo"))

    def test_endswith_with_substr_at_end_returns_true(self):
        self.assertTrue("hello".endswith("e", 0, 2))
        self.assertTrue("hello".endswith("ll", 1, 4))
        self.assertTrue("hello".endswith("lo", 2, 7))
        self.assertTrue("hello".endswith("o", 0, 100))

    def test_endswith_outside_bounds_returns_false(self):
        self.assertFalse("hello".endswith("hello", 0, 4))
        self.assertFalse("hello".endswith("llo", 1, 4))
        self.assertFalse("hello".endswith("hell", 0, 2))
        self.assertFalse("hello".endswith("o", 2, 4))

    def test_endswith_with_negative_bounds_relative_to_end(self):
        self.assertTrue("hello".endswith("ll", 1, -1))
        self.assertTrue("hello".endswith("e", 0, -3))
        self.assertFalse("hello".endswith("ello", -10, -1))

    def test_endswith_with_tuple(self):
        self.assertTrue("hello".endswith(("l", "o", "l")))
        self.assertFalse("hello".endswith(("foo", "bar")))

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
        self.assertIn("index out of range", str(context.exception))

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
        self.assertIn("cannot switch", str(context.exception))
        self.assertIn("automatic field numbering", str(context.exception))
        self.assertIn("manual field specification", str(context.exception))

    def test_format_auto_index_field_with_keyword_returns_formatted_str(self):
        result = str.format("a{}b{keyword}c{}d", 0, 1, keyword=888)
        self.assertEqual(result, "a0b888c1d")

    def test_format_explicit_index_fields_returns_formatted_str(self):
        result = str.format("a{2}b{1}c{0}d{1}e", 0, 1, 2)
        self.assertEqual(result, "a2b1c0d1e")

    def test_format_keyword_fields_returns_formatted_str(self):
        result = str.format("1{a}2{b}3{c}4{b}5", a="a", b="b", c="c")
        self.assertEqual(result, "1a2b3c4b5")

    def test_format_with_unnamed_subscript(self):
        result = str.format("{[1]}", (1, 2, 3))
        self.assertEqual(result, "2")

    def test_format_with_numbered_subscript(self):
        result = str.format("{1[1]}", (1, 2, 3), (4, 5, 6))
        self.assertEqual(result, "5")

    def test_format_with_named_subscript(self):
        result = str.format("{b[1]}", a=(1, 2, 3), b=(4, 5, 6))
        self.assertEqual(result, "5")

    def test_format_with_multiple_subscripts(self):
        result = str.format("{b[1][2]}", a=(1, 2, 3), b=(4, (5, 6, 7), 6))
        self.assertEqual(result, "7")

    def test_format_with_self_keyword(self):
        result = "be {self}".format(self="yourself")
        self.assertEqual(result, "be yourself")

    def test_format_with_no_positional_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.format(self="not_self")

    def test_format_with_mistyped_self_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.format(1, self="not_self")
        # Ensure that the error message mentions the int being formatted.
        self.assertTrue("int" in str(context.exception))

    def test_isalnum_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isalnum())) for x in range(128)),
            "0000000000000000000000000000000000000000000000001111111111000000"
            "0111111111111111111111111110000001111111111111111111111111100000",
        )

    def test_isalnum_with_empty_string_returns_false(self):
        self.assertFalse("".isalnum())

    def test_isalnum_with_multichar_string_returns_bool(self):
        self.assertTrue("1a5b".isalnum())
        self.assertFalse("1b)".isalnum())

    def test_isalnum_with_non_ascii_returns_bool(self):
        self.assertTrue("\u0e50ab2\u00e9".isalnum())
        self.assertFalse("\u0e50 \u00e9".isalnum())

    def test_isalnum_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isalnum(None)
        self.assertIn("'isalnum' requires a 'str' object", str(context.exception))

    def test_isalpha_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isalpha())) for x in range(128)),
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0111111111111111111111111110000001111111111111111111111111100000",
        )

    def test_isalpha_with_empty_string_returns_false(self):
        self.assertFalse("".isalpha())

    def test_isalpha_with_multichar_string_returns_bool(self):
        self.assertTrue("hElLo".isalpha())
        self.assertFalse("x8".isalpha())

    def test_isalpha_with_non_ascii_returns_bool(self):
        self.assertTrue("resum\u00e9".isalpha())
        self.assertTrue("\u00c9tude".isalpha())
        self.assertTrue("\u01c8udevit".isalpha())

        self.assertFalse("23\u00e9".isalpha())
        self.assertFalse("\u01c8 ".isalpha())

    def test_isalpha_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isalpha(None)
        self.assertIn("'isalpha' requires a 'str' object", str(context.exception))

    @unittest.skipUnless(hasattr(str, "isascii"), "Added in 3.7")
    def test_isascii_with_empty_string_returns_true(self):
        self.assertTrue("".isascii())

    @unittest.skipUnless(hasattr(str, "isascii"), "Added in 3.7")
    def test_isascii_with_ascii_values_returns_true(self):
        self.assertTrue("howdy".isascii())
        self.assertTrue("\x00".isascii())
        self.assertTrue("\x7f".isascii())
        self.assertTrue("\x00\x7f".isascii())

    @unittest.skipUnless(hasattr(str, "isascii"), "Added in 3.7")
    def test_isascii_with_nonascii_values_return_false(self):
        self.assertFalse("\x80".isascii())
        self.assertFalse("\xe9".isascii())

    def test_isdecimal_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isdecimal())) for x in range(128)),
            "0000000000000000000000000000000000000000000000001111111111000000"
            "0000000000000000000000000000000000000000000000000000000000000000",
        )

    def test_isdecimal_with_empty_string_returns_false(self):
        self.assertFalse("".isdecimal())

    def test_isdecimal_with_multichar_string_returns_bool(self):
        self.assertTrue("8725".isdecimal())
        self.assertFalse("8-4".isdecimal())

    def test_isdecimal_with_non_ascii_returns_bool(self):
        self.assertTrue("\u0e50".isdecimal())  # THAI DIGIT ZERO
        self.assertFalse("\u00b2".isdecimal())  # SUPERSCRIPT TWO
        self.assertFalse("\u00bd".isdecimal())  # VULGAR FRACTION ONE HALF

    def test_isdecimal_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isdecimal(None)
        self.assertIn("'isdecimal' requires a 'str' object", str(context.exception))

    def test_isdigit_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isdigit())) for x in range(128)),
            "0000000000000000000000000000000000000000000000001111111111000000"
            "0000000000000000000000000000000000000000000000000000000000000000",
        )

    def test_isdigit_with_empty_string_returns_false(self):
        self.assertFalse("".isdigit())

    def test_isdigit_with_multichar_string_returns_bool(self):
        self.assertTrue("8725".isdigit())
        self.assertFalse("8-4".isdigit())

    def test_isdigit_with_non_ascii_returns_bool(self):
        self.assertTrue("\u0e50".isdigit())  # THAI DIGIT ZERO
        self.assertTrue("\u00b2".isdigit())  # SUPERSCRIPT TWO
        self.assertFalse("\u00bd".isdigit())  # VULGAR FRACTION ONE HALF

    def test_isdigit_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isdigit(None)
        self.assertIn("'isdigit' requires a 'str' object", str(context.exception))

    def test_isidentifier_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isidentifier())) for x in range(128)),
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0111111111111111111111111110000101111111111111111111111111100000",
        )
        self.assertEqual(
            "".join(str(int(("_" + chr(x)).isidentifier())) for x in range(128)),
            "0000000000000000000000000000000000000000000000001111111111000000"
            "0111111111111111111111111110000101111111111111111111111111100000",
        )

    def test_isidentifier_with_empty_string_returns_false(self):
        self.assertFalse("".isidentifier())

    def test_isidentifier_with_multichar_string_returns_bool(self):
        self.assertTrue("foo_8".isidentifier())
        self.assertTrue("Foo_8".isidentifier())
        self.assertTrue("_FOO_8".isidentifier())
        self.assertFalse("foo bar".isidentifier())

    def test_isidentifier_with_non_ascii_string_returns_bool(self):
        self.assertTrue("resum\u00e9".isidentifier())
        self.assertTrue("\u00e9tude".isidentifier())
        self.assertFalse("\U0001F643".isidentifier())

    def test_isidentifier_with_invalid_start_returns_false(self):
        self.assertFalse("8foo".isidentifier())
        self.assertFalse(".foo".isidentifier())

    def test_isidentifier_with_symbol_returns_false(self):
        self.assertFalse("foo!".isidentifier())
        self.assertFalse("foo.bar".isidentifier())

    def test_isidentifier_allows_underscore_start(self):
        self.assertTrue("_hello".isidentifier())
        self.assertFalse("_world!".isidentifier())

    def test_isidentifier_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isidentifier(None)
        self.assertIn("'isidentifier' requires a 'str' object", str(context.exception))

    def test_islower_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).islower())) for x in range(128)),
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0000000000000000000000000000000001111111111111111111111111100000",
        )

    def test_islower_with_empty_string_returns_false(self):
        self.assertFalse("".islower())

    def test_islower_with_multichar_string_returns_bool(self):
        self.assertTrue("hello".islower())
        self.assertTrue("...a...".islower())
        self.assertFalse("hEllo".islower())
        self.assertFalse("...A...".islower())
        self.assertFalse("......".islower())

    def test_islower_with_non_ascii_returns_bool(self):
        self.assertTrue("resum\u00e9".islower())
        self.assertFalse("\u00c9tude".islower())  # uppercase
        self.assertFalse("\u01c8udevit".islower())  # titlecase

    def test_islower_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.islower(None)
        self.assertIn("'islower' requires a 'str' object", str(context.exception))

    def test_isnumeric_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isnumeric())) for x in range(128)),
            "0000000000000000000000000000000000000000000000001111111111000000"
            "0000000000000000000000000000000000000000000000000000000000000000",
        )

    def test_isnumeric_with_empty_string_returns_false(self):
        self.assertFalse("".isnumeric())

    def test_isnumeric_with_multichar_string_returns_bool(self):
        self.assertTrue("28741".isnumeric())
        self.assertFalse("5e4".isnumeric())

    def test_isnumeric_with_non_ascii_returns_bool(self):
        self.assertTrue("\u0e50".isnumeric())  # THAI DIGIT ZERO
        self.assertTrue("\u00b2".isnumeric())  # SUPERSCRIPT TWO
        self.assertTrue("\u00bd".isnumeric())  # VULGAR FRACTION ONE HALF

    def test_isnumeric_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isnumeric(None)
        self.assertIn("'isnumeric' requires a 'str' object", str(context.exception))

    def test_isprintable_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isprintable())) for x in range(128)),
            "0000000000000000000000000000000011111111111111111111111111111111"
            "1111111111111111111111111111111111111111111111111111111111111110",
        )

    def test_isprintable_with_empty_string_returns_true(self):
        self.assertTrue("".isprintable())

    def test_isprintable_with_multichar_string_returns_bool(self):
        self.assertTrue("Hello World!".isprintable())
        self.assertFalse("Hello\tWorld!".isprintable())

    def test_isprintable_with_non_ascii_returns_bool(self):
        self.assertTrue("\u0e50 foo \u00e3!".isprintable())
        self.assertFalse("\u0e50 \t \u00e3!".isprintable())

    def test_isprintable_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isprintable(None)
        self.assertIn("'isprintable' requires a 'str' object", str(context.exception))

    def test_isspace_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isspace())) for x in range(128)),
            "0000000001111100000000000000111110000000000000000000000000000000"
            "0000000000000000000000000000000000000000000000000000000000000000",
        )

    def test_isspace_with_empty_string_returns_false(self):
        self.assertFalse("".isspace())

    def test_isspace_with_multichar_string_returns_bool(self):
        self.assertTrue(" \t\r\n".isspace())
        self.assertFalse(" _".isspace())

    def test_isspace_with_multichar_unicode_spaces_returns_true(self):
        self.assertTrue(" \t\u3000\n\u202f".isspace())

    def test_isspace_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isspace(None)
        self.assertIn("'isspace' requires a 'str' object", str(context.exception))

    def test_istitle_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).istitle())) for x in range(128)),
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0111111111111111111111111110000000000000000000000000000000000000",
        )
        self.assertEqual(
            "".join(str(int(("A" + chr(x)).istitle())) for x in range(128)),
            "1111111111111111111111111111111111111111111111111111111111111111"
            "1000000000000000000000000001111111111111111111111111111111111111",
        )

    def test_istitle_with_empty_string_returns_false(self):
        self.assertFalse("".istitle())

    def test_istitle_with_multichar_string_returns_bool(self):
        self.assertTrue("Hello\t!".istitle())
        self.assertTrue("...A...".istitle())
        self.assertTrue("Aa Bbb Cccc".istitle())
        self.assertFalse("HeLlo".istitle())
        self.assertFalse("...a...".istitle())
        self.assertFalse("...".istitle())

    def test_istitle_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.istitle(None)
        self.assertIn("'istitle' requires a 'str' object", str(context.exception))

    def test_istitle_with_non_ascii_returns_bool(self):
        self.assertTrue("Resum\u00e9".istitle())
        self.assertTrue("\u00c9tude".istitle())
        self.assertTrue("\u01c8udevit".istitle())

        self.assertFalse("resum\u00e9".istitle())
        self.assertFalse("\u00c9tudE".istitle())
        self.assertFalse("L\u01c8udevit".istitle())

    def test_isupper_with_ascii_char_returns_bool(self):
        self.assertEqual(
            "".join(str(int(chr(x).isupper())) for x in range(128)),
            "0000000000000000000000000000000000000000000000000000000000000000"
            "0111111111111111111111111110000000000000000000000000000000000000",
        )

    def test_isupper_with_empty_string_returns_false(self):
        self.assertFalse("".isupper())

    def test_isupper_with_multichar_string_returns_bool(self):
        self.assertTrue("HELLO".isupper())
        self.assertTrue("...A...".isupper())
        self.assertFalse("HElLLO".isupper())
        self.assertFalse("...a...".isupper())
        self.assertFalse("......".isupper())
        self.assertFalse("12345".isupper())
        self.assertTrue("ABC12345".isupper())
        self.assertFalse("ABC12345abc".isupper())

    def test_isupper_with_nonascii_string_returns_bool(self):
        self.assertFalse("RESUM\u00e9".isupper())  # lowercase
        self.assertTrue("\u00c9TUDE".isupper())  # uppercase
        self.assertFalse("\u01c8UDEVIT".isupper())  # titlecase

    def test_isupper_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.isupper(None)
        self.assertIn("'isupper' requires a 'str' object", str(context.exception))

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

    def test_ljust_returns_justified_string(self):
        self.assertEqual(str.ljust("abc", -1), "abc")
        self.assertEqual(str.ljust("abc", 0), "abc")
        self.assertEqual(str.ljust("abc", 1), "abc")
        self.assertEqual(str.ljust("abc", 2), "abc")
        self.assertEqual(str.ljust("abc", 3), "abc")
        self.assertEqual(str.ljust("abc", 4), "abc ")
        self.assertEqual(str.ljust("abc", 5), "abc  ")
        self.assertEqual(str.ljust("abc", 6), "abc   ")
        self.assertEqual(str.ljust("", 4), "    ")
        self.assertEqual(str.ljust("\t \n", 5), "\t \n  ")

    def test_ljust_with_custom_fillchar_returns_str(self):
        self.assertEqual(str.ljust("ba", 8, "@"), "ba@@@@@@")
        self.assertEqual(
            str.ljust("x\U0001f43bx", 5, "\U0001f40d"),
            "x\U0001f43bx\U0001f40d\U0001f40d",
        )

    def test_ljust_returns_identity(self):
        s = "foo bar baz bam!"
        self.assertIs(str.ljust(s, -1), s)
        self.assertIs(str.ljust(s, 5), s)
        self.assertIs(str.ljust(s, len(s)), s)

    def test_ljust_with_non_int_index_width_calls_dunder_index(self):
        class W:
            def __index__(self):
                return 5

        self.assertEqual(str.ljust("abc", W()), "abc  ")

    def test_ljust_with_fillchar_not_char_raises_type_error(self):
        self.assertRaises(TypeError, str.ljust, "", 2, "")
        self.assertRaises(TypeError, str.ljust, "", 2, ",,")
        self.assertRaises(TypeError, str.ljust, "", 2, 3)

    def test_lower_returns_lowercased_string(self):
        self.assertEqual(str.lower("HELLO"), "hello")
        self.assertEqual(str.lower("HeLLo_WoRlD"), "hello_world")
        self.assertEqual(str.lower("hellO World!"), "hello world!")
        self.assertEqual(str.lower(""), "")
        self.assertEqual(str.lower("1234"), "1234")
        self.assertEqual(str.lower("$%^*("), "$%^*(")

    def test_lower_with_non_ascii_returns_lowercased_string(self):
        self.assertEqual(str.lower("foo\u01BCBAR"), "foo\u01BDbar")
        # uppercase without lowercase
        self.assertEqual(str.lower("FOO\U0001D581bar"), "foo\U0001D581bar")
        # one to many lowercasing
        self.assertEqual(str.lower("foo\u0130bar"), "foo\x69\u0307bar")

    def test_lower_with_str_subclass_returns_str(self):
        class C(str):
            pass

        self.assertEqual(str.lower(C("HeLLo")), "hello")
        self.assertIs(type(str.lower(C("HeLLo"))), str)
        self.assertIs(type(str.lower(C(""))), str)
        self.assertIs(type(str.lower(C("lower"))), str)

    def test_replace(self):
        test = "mississippi"
        self.assertEqual(test.replace("i", "a"), "massassappa")
        self.assertEqual(test.replace("i", "vv"), "mvvssvvssvvppvv")
        self.assertEqual(test.replace("ss", "x"), "mixixippi")

    def test_replace_with_count(self):
        test = "mississippi"
        self.assertEqual(test.replace("i", "a", 0), "mississippi")
        self.assertEqual(test.replace("i", "a", 2), "massassippi")

    def test_replace_int_error(self):
        test = "a b"
        self.assertRaises(TypeError, test.replace, 32, "")

    def test_rindex_with_bytes_self_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.rindex(b"", "")

    def test_rindex_with_subsequence_returns_last_in_range(self):
        haystack = "-a-a--a--"
        needle = "a"
        self.assertEqual(haystack.rindex(needle, 1, 6), 3)

    def test_rindex_with_missing_raises_value_error(self):
        haystack = "abc"
        needle = "d"
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle)
        self.assertEqual(str(context.exception), "substring not found")

    def test_rindex_outside_of_bounds_raises_value_error(self):
        haystack = "abc"
        needle = "c"
        with self.assertRaises(ValueError) as context:
            haystack.rindex(needle, 0, 2)
        self.assertEqual(str(context.exception), "substring not found")

    def test_rjust_returns_justified_string(self):
        self.assertEqual(str.rjust("abc", -1), "abc")
        self.assertEqual(str.rjust("abc", 0), "abc")
        self.assertEqual(str.rjust("abc", 1), "abc")
        self.assertEqual(str.rjust("abc", 2), "abc")
        self.assertEqual(str.rjust("abc", 3), "abc")
        self.assertEqual(str.rjust("abc", 4), " abc")
        self.assertEqual(str.rjust("abc", 5), "  abc")
        self.assertEqual(str.rjust("abc", 6), "   abc")
        self.assertEqual(str.rjust("", 4), "    ")
        self.assertEqual(str.rjust("\t \n", 5), "  \t \n")

    def test_rjust_with_custom_fillchar_returns_str(self):
        self.assertEqual(str.rjust("ba", 8, "@"), "@@@@@@ba")
        self.assertEqual(
            str.rjust("x\U0001f43bx", 5, "\U0001f40d"),
            "\U0001f40d\U0001f40dx\U0001f43bx",
        )

    def test_rjust_returns_identity(self):
        s = "foo bar baz bam!"
        self.assertIs(str.rjust(s, -1), s)
        self.assertIs(str.rjust(s, 5), s)
        self.assertIs(str.rjust(s, len(s)), s)

    def test_rjust_with_non_int_index_width_calls_dunder_index(self):
        class W:
            def __index__(self):
                return 5

        self.assertEqual(str.rjust("abc", W()), "  abc")

    def test_rjust_with_fillchar_not_char_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "The fill character must be exactly one character long"
        ):
            str.rjust("", 2, "")
        with self.assertRaisesRegex(
            TypeError, "The fill character must be exactly one character long"
        ):
            str.rjust("", 2, ",,")

    def test_rpartition_with_non_str_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError, "requires a 'str'", str.rpartition, None, "hello"
        )

    def test_rpartition_with_non_str_sep_raises_type_error(self):
        self.assertRaises(TypeError, str.rpartition, "hello", None)

    def test_rpartition_partitions_str(self):
        self.assertEqual("hello".rpartition("l"), ("hel", "l", "o"))

    def test_split_with_empty_separator_list_raises_type_error(self):
        self.assertRaisesRegex(TypeError, "must be str or None", str.split, "abc", [])

    def test_split_with_empty_separator_list_and_bad_maxsplit_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "cannot be interpreted as an integer",
            str.split,
            "abc",
            [],
            maxsplit="bad",
        )

    def test_split_with_empty_separator_raises_value_error(self):
        self.assertRaisesRegex(ValueError, "empty separator", str.split, "abc", "")

    def test_split_with_repeated_sep_consolidates_spaces(self):
        self.assertEqual("hello".split("l"), ["he", "", "o"])

    def test_split_with_none_separator_splits_on_whitespace(self):
        self.assertEqual("a\t\n b c".split(), ["a", "b", "c"])

    def test_split_with_adjacent_separators_coalesces_spaces(self):
        self.assertEqual("hello".split("l"), ["he", "", "o"])

    def test_split_with_negative_maxsplit_splits_all(self):
        self.assertEqual("a b c".split(" ", maxsplit=-10), ["a", "b", "c"])

    def test_split_with_non_int_maxsplit_calls_dunder_index(self):
        class C:
            __index__ = Mock(name="__index__", return_value=1)

        result = "a b c".split(" ", maxsplit=C())
        self.assertEqual(result, ["a", "b c"])
        C.__index__.assert_called_once()

    def test_split_with_int_subclass_maxsplit_does_not_call_dunder_index(self):
        class C(int):
            __index__ = Mock(name="__index__", return_value=1)

        result = "a b c".split(" ", maxsplit=C(5))
        self.assertEqual(result, ["a", "b", "c"])
        C.__index__.assert_not_called()

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

    def test_startswith_with_non_str_or_tuple_prefix_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "".startswith([])
        self.assertEqual(
            str(context.exception),
            "startswith first arg must be str or a tuple of str, not list",
        )

    def test_startswith_with_non_str_or_tuple_prefix_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "hello".startswith(("e", b"", "h"))
        self.assertEqual(
            str(context.exception),
            "tuple for startswith must only contain str, not bytes",
        )

    def test_startswith_with_empty_string_returns_true_for_valid_start(self):
        self.assertTrue("".startswith(""))
        self.assertTrue("".startswith("", -1))
        self.assertTrue("hello".startswith("", 5))

    def test_startswith_with_empty_string_returns_false_for_invalid_start(self):
        self.assertFalse("".startswith("", 1))
        self.assertFalse("hello".startswith("", 6))

    def test_startswith_with_prefix_returns_true(self):
        self.assertTrue("hello".startswith("h"))
        self.assertTrue("hello".startswith("he"))
        self.assertTrue("hello".startswith("hel"))
        self.assertTrue("hello".startswith("hell"))
        self.assertTrue("hello".startswith("hello"))

    def test_startswith_with_nonprefix_returns_false(self):
        self.assertFalse("hello".startswith("e"))
        self.assertFalse("hello".startswith("l"))
        self.assertFalse("hello".startswith("el"))
        self.assertFalse("hello".startswith("llo"))
        self.assertFalse("hello".startswith("hi"))
        self.assertFalse("hello".startswith("hello there"))

    def test_startswith_with_substr_at_start_returns_true(self):
        self.assertTrue("hello".startswith("e", 1))
        self.assertTrue("hello".startswith("ll", 2))
        self.assertTrue("hello".startswith("lo", 3))
        self.assertTrue("hello".startswith("o", 4))

    def test_startswith_outside_bounds_returns_false(self):
        self.assertFalse("hello".startswith("hello", 0, 4))
        self.assertFalse("hello".startswith("llo", 2, 3))
        self.assertFalse("hello".startswith("lo", 3, 4))
        self.assertFalse("hello".startswith("o", 4, 4))

    def test_startswith_with_negative_bounds_relative_to_end(self):
        self.assertTrue("hello".startswith("he", 0, -1))
        self.assertTrue("hello".startswith("llo", -3))
        self.assertFalse("hello".startswith("ello", -4, -2))

    def test_startswith_with_tuple(self):
        self.assertTrue("hello".startswith(("r", "h", "s")))
        self.assertFalse("hello".startswith(("foo", "bar")))

    def test_str_new_with_bytes_and_no_encoding_returns_str(self):
        decoded = str.__new__(str, b"abc")
        self.assertEqual(decoded, "b'abc'")

    def test_str_new_with_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.__new__(str, "", encoding="utf_8")

    def test_str_new_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            str.__new__(str, 1, encoding="utf_8")

    def test_str_new_with_bytes_and_encoding_returns_decoded_str(self):
        decoded = str.__new__(str, b"abc", encoding="ascii")
        self.assertEqual(decoded, "abc")

    def test_str_new_with_no_object_and_encoding_returns_empty_string(self):
        self.assertEqual(str.__new__(str, encoding="ascii"), "")

    def test_str_new_with_class_without_dunder_str_returns_str(self):
        class A:
            def __repr__(self):
                return "test"

        self.assertEqual(str.__new__(str, A()), "test")

    def test_str_new_with_class_with_faulty_dunder_str_raises_type_error(self):
        with self.assertRaises(TypeError):

            class A:
                def __str__(self):
                    return 1

            str.__new__(str, A())

    def test_str_new_with_class_with_proper_duner_str_returns_str(self):
        class A:
            def __str__(self):
                return "test"

        self.assertEqual(str.__new__(str, A()), "test")

    def test_swapcase_with_int_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.swapcase(4)
        self.assertIn(
            "'swapcase' requires a 'str' object but received a 'int'",
            str(context.exception),
        )

    def test_swapcase_returns_inverted_string(self):
        self.assertEqual("hEllO CoMPuTErS".swapcase(), "HeLLo cOmpUteRs")
        self.assertEqual("TTT".swapcase(), "ttt")
        self.assertEqual("ttt".swapcase(), "TTT")
        self.assertEqual("T".swapcase(), "t")
        self.assertEqual(" ".swapcase(), " ")
        self.assertEqual("".swapcase(), "")

    def test_swapcase_with_unicode_returns_inverted_string(self):
        self.assertEqual("\U0001044F".swapcase(), "\U00010427")
        self.assertEqual("\U00010427".swapcase(), "\U0001044F")
        self.assertEqual("\U0001044F\U0001044F".swapcase(), "\U00010427\U00010427")
        self.assertEqual("\U00010427\U0001044F".swapcase(), "\U0001044F\U00010427")
        self.assertEqual("\U0001044F\U00010427".swapcase(), "\U00010427\U0001044F")
        self.assertEqual("X\U00010427x\U0001044F".swapcase(), "x\U0001044FX\U00010427")
        self.assertEqual("ﬁ".swapcase(), "FI")
        self.assertEqual("\u0130".swapcase(), "\u0069\u0307")
        self.assertEqual("\u00e9TuDe".swapcase(), "\u00c9tUdE")

    def test_swapcase_with_capital_sigma_character(self):
        """
        Test cases that match Final Capital Sigma context
        ("\u03A3" should be lower-cased to "\u03C2")
        (Note: {# C} = # cased characters, {# CI} = # case-ignorable characters)
        """
        # {1 C} {0 CI} Sigma {0 CI} {0 C}
        self.assertEqual("A\u03A3".swapcase(), "a\u03C2")
        # {1 C} {1 CI} Sigma {0 CI} {0 C}
        self.assertEqual("a.\u03A3".swapcase(), "A.\u03C2")
        # {1 C} {0 CI} Sigma {1 CI} {0 C}
        self.assertEqual("A\u03A3.".swapcase(), "a\u03C2.")
        # {1 C} {1 CI} Sigma {1 CI} {0 C}
        self.assertEqual("a.\u03A3.".swapcase(), "A.\u03C2.")
        """
        Test cases that do NOT match Final Capital Sigma context
        ("\u03A3" should be lower-cased to "\u03C3")
        """
        # {0 C} {0 CI} Sigma {0 CI} {0 C}
        self.assertEqual("\u03A3".swapcase(), "\u03C3")
        # {0 C} {1 CI} Sigma {0 CI} {0 C}
        self.assertEqual(".\u03A3".swapcase(), ".\u03C3")
        # {0 C} {0 CI} Sigma {1 CI} {0 C}
        self.assertEqual("\u03A3.".swapcase(), "\u03C3.")
        # {0 C} {1 CI} Sigma {1 CI} {0 C}
        self.assertEqual(".\u03A3.".swapcase(), ".\u03C3.")
        # {0 C} {0 CI} Sigma {1 CI} {1 C}
        self.assertEqual("\u03A3.a".swapcase(), "\u03C3.A")
        # {0 C} {1 CI} Sigma {1 CI} {1 C}
        self.assertEqual(".\u03A3.A".swapcase(), ".\u03C3.a")
        # {1 C} {0 CI} Sigma {1 CI} {1 C}
        self.assertEqual("A\u03A3.a".swapcase(), "a\u03C3.A")
        # {1 C} {1 CI} Sigma {1 CI} {1 C}
        self.assertEqual("a.\u03A3.A".swapcase(), "A.\u03C3.a")
        # {0 C} {0 CI} Sigma {0 CI} {1 C}
        self.assertEqual("\u03A3a".swapcase(), "\u03C3A")
        # {0 C} {1 CI} Sigma {0 CI} {1 C}
        self.assertEqual(".\u03A3A".swapcase(), ".\u03C3a")
        # {1 C} {0 CI} Sigma {0 CI} {1 C}
        self.assertEqual("A\u03A3a".swapcase(), "a\u03C3A")
        # {1 C} {1 CI} Sigma {0 CI} {1 C}
        self.assertEqual("a.\u03A3A".swapcase(), "A.\u03C3a")

    def test_title_with_returns_titlecased_string(self):
        self.assertEqual("".title(), "")
        self.assertEqual("1234!@#$".title(), "1234!@#$")
        self.assertEqual("hello world".title(), "Hello World")
        self.assertEqual("HELLO_WORLD".title(), "Hello_World")

        self.assertEqual("resum\u00e9".title(), "Resum\u00e9")
        self.assertEqual("\u00e9TuDe".title(), "\u00c9tude")
        self.assertEqual("\u01c7uDeViT".title(), "\u01c8udevit")

    def test_upper_returns_uppercased_string(self):
        self.assertEqual(str.upper("hello"), "HELLO")
        self.assertEqual(str.upper("HeLLo_WoRlD"), "HELLO_WORLD")
        self.assertEqual(str.upper("hellO World!"), "HELLO WORLD!")
        self.assertEqual(str.upper(""), "")
        self.assertEqual(str.upper("1234"), "1234")
        self.assertEqual(str.upper("$%^*("), "$%^*(")

    def test_upper_with_str_subclass_returns_str(self):
        class C(str):
            pass

        self.assertEqual(str.upper(C("HeLLo")), "HELLO")
        self.assertIs(type(str.upper(C("HeLLo"))), str)
        self.assertIs(type(str.upper(C(""))), str)
        self.assertIs(type(str.upper(C("upper"))), str)

    def test_upper_with_non_ascii_returns_uppercased_string(self):
        self.assertEqual(str.upper("foo\u01BDBAR"), "FOO\u01BCBAR")
        # lowercase without uppercase
        self.assertEqual(str.upper("FOO\u0237bar"), "FOO\u0237BAR")
        # one to many uppercasing
        self.assertEqual(str.upper("foo\xDFbar"), "FOO\x53\x53BAR")


class StrDunderFormatTests(unittest.TestCase):
    def test_empty_format_returns_string(self):
        self.assertEqual(str.__format__("hello", ""), "hello")
        self.assertEqual(str.__format__("", ""), "")
        self.assertEqual(str.__format__("\U0001f40d", ""), "\U0001f40d")

    def test_format_with_width_returns_string(self):
        self.assertEqual(str.__format__("hello", "1"), "hello")
        self.assertEqual(str.__format__("hello", "5"), "hello")
        self.assertEqual(str.__format__("hello", "8"), "hello   ")

        self.assertEqual(str.__format__("hello", "<"), "hello")
        self.assertEqual(str.__format__("hello", "<0"), "hello")
        self.assertEqual(str.__format__("hello", "<5"), "hello")
        self.assertEqual(str.__format__("hello", "<8"), "hello   ")

        self.assertEqual(str.__format__("hello", ">"), "hello")
        self.assertEqual(str.__format__("hello", ">0"), "hello")
        self.assertEqual(str.__format__("hello", ">5"), "hello")
        self.assertEqual(str.__format__("hello", ">8"), "   hello")

        self.assertEqual(str.__format__("hello", "^"), "hello")
        self.assertEqual(str.__format__("hello", "^0"), "hello")
        self.assertEqual(str.__format__("hello", "^5"), "hello")
        self.assertEqual(str.__format__("hello", "^8"), " hello  ")
        self.assertEqual(str.__format__("hello", "^9"), "  hello  ")

    def test_format_with_fill_char_returns_string(self):
        self.assertEqual(str.__format__("hello", ".^9"), "..hello..")
        self.assertEqual(str.__format__("hello", "*>9"), "****hello")
        self.assertEqual(str.__format__("hello", "^<9"), "hello^^^^")
        self.assertEqual(
            str.__format__("hello", "\U0001f40d>7"), "\U0001f40d\U0001f40dhello"
        )

    def test_format_with_precision_returns_string(self):
        self.assertEqual(str.__format__("hello", ".0"), "")
        self.assertEqual(str.__format__("hello", ".2"), "he")
        self.assertEqual(str.__format__("hello", ".5"), "hello")
        self.assertEqual(str.__format__("hello", ".7"), "hello")

    def test_format_with_precision_and_width_returns_string(self):
        self.assertEqual(str.__format__("hello", "10.8"), "hello     ")
        self.assertEqual(str.__format__("hello", "^5.2"), " he  ")

    def test_works_with_str_subclass(self):
        class C(str):
            pass

        self.assertEqual(str.__format__("hello", C("7")), "hello  ")
        self.assertEqual(str.__format__(C("hello"), "7"), "hello  ")
        self.assertIs(type(str.__format__(C("hello"), "7")), str)
        self.assertEqual(str.__format__(C("hello"), ""), "hello")
        self.assertIs(type(str.__format__(C("hello"), "")), str)

    def test_format_zero_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "0")
        self.assertEqual(
            str(context.exception),
            "'=' alignment not allowed in string format specifier",
        )

    def test_format_with_equal_alignment_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "=")
        self.assertEqual(
            str(context.exception),
            "'=' alignment not allowed in string format specifier",
        )

    def test_non_s_format_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "d")
        self.assertEqual(
            str(context.exception), "Unknown format code 'd' for object of type 'str'"
        )

        class C(str):
            pass

        with self.assertRaises(ValueError) as context:
            str.__format__(C(""), "\U0001f40d")
        self.assertEqual(
            str(context.exception),
            "Unknown format code '\\x1f40d' for object of type 'C'",
        )

    def test_big_width_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "999999999999999999999")
        self.assertEqual(
            str(context.exception), "Too many decimal digits in format string"
        )

    def test_missing_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", ".")
        self.assertEqual(str(context.exception), "Format specifier missing precision")

    def test_big_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", ".999999999999999999999")
        self.assertEqual(
            str(context.exception), "Too many decimal digits in format string"
        )

    def test_extra_chars_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "sX")
        self.assertEqual(str(context.exception), "Invalid format specifier")

    def test_comma_underscore_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", ",_")
        self.assertEqual(str(context.exception), "Cannot specify both ',' and '_'.")

    def test_comma_comma_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", ",,")
        self.assertEqual(str(context.exception), "Cannot specify both ',' and '_'.")

    def test_underscore_underscore_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "__")
        self.assertEqual(str(context.exception), "Cannot specify '_' with '_'.")

    def test_unexpected_thousands_separator_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__format__("", "_")
        self.assertEqual(str(context.exception), "Cannot specify '_' with 's'.")

    def test_zfill_with_empty_string_input_returns_padding_only(self):
        self.assertEqual("000", "".zfill(3))

    def test_zfill_with_positive_prefix_input_returns_prepended_padding(self):
        self.assertEqual("+0123", "+123".zfill(5))
        self.assertEqual("+00", "+".zfill(3))

    def test_zfill_with_negative_prefix_input_returns_prepended_padding(self):
        self.assertEqual("-0123", "-123".zfill(5))
        self.assertEqual("-00", "-".zfill(3))

    def test_zfill_without_sign_prefix_input_returns_prepended_padding(self):
        self.assertEqual("-0123", "-123".zfill(5))
        self.assertEqual("0123", "123".zfill(4))
        self.assertEqual("0034", "34".zfill(4))

    def test_zfill_without_sign_prefix_input_returns_self_when_width_lte_len(self):
        test_str = "123"
        for width in range(len(test_str) + 1):
            result = test_str.zfill(width)
            self.assertIs(test_str, result)

    def test_zfill_with_positive_prefix_input_returns_self_when_width_lte_len(self):
        test_str = "+123"
        for width in range(len(test_str) + 1):
            result = test_str.zfill(width)
            self.assertIs(test_str, result)

    def test_zfill_with_negative_prefix_input_returns_self_when_width_lte_len(self):
        test_str = "-123"
        for width in range(len(test_str) + 1):
            result = test_str.zfill(width)
            self.assertIs(test_str, result)

    def test_zfill_with_float_width_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            "123".zfill(3.2)
        self.assertEqual(str(context.exception), "integer argument expected, got float")


class StrModTests(unittest.TestCase):
    def test_empty_format_returns_empty_string(self):
        self.assertEqual(str.__mod__("", ()), "")

    def test_simple_string_returns_string(self):
        self.assertEqual(str.__mod__("foo bar (}", ()), "foo bar (}")

    def test_with_non_tuple_args_returns_string(self):
        self.assertEqual(str.__mod__("%s", "foo"), "foo")
        self.assertEqual(str.__mod__("%d", 42), "42")
        self.assertEqual(str.__mod__("%s", {"foo": "bar"}), "{'foo': 'bar'}")

    def test_with_named_args_returns_string(self):
        self.assertEqual(
            str.__mod__("%(foo)s %(bar)d", {"foo": "ho", "bar": 42}), "ho 42"
        )
        self.assertEqual(str.__mod__("%()x", {"": 123}), "7b")
        self.assertEqual(str.__mod__(")%(((()) ()))s(", {"((()) ())": 99}), ")99(")
        self.assertEqual(str.__mod__("%(%s)s", {"%s": -5}), "-5")

    def test_with_custom_mapping_returns_string(self):
        class C:
            def __getitem__(self, key):
                return "getitem called with " + key

        self.assertEqual(str.__mod__("%(foo)s", C()), "getitem called with foo")

    def test_with_custom_mapping_propagates_errors(self):
        with self.assertRaises(KeyError) as context:
            str.__mod__("%(foo)s", {})
        self.assertEqual(str(context.exception), "'foo'")

        class C:
            def __getitem__(self, key):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%(foo)s", C())

    def test_without_mapping_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%(foo)s", None)
        self.assertEqual(str(context.exception), "format requires a mapping")
        with self.assertRaises(TypeError) as context:
            str.__mod__("%(foo)s", "foobar")
        self.assertEqual(str(context.exception), "format requires a mapping")
        with self.assertRaises(TypeError) as context:
            str.__mod__("%(foo)s", ("foobar",))
        self.assertEqual(str(context.exception), "format requires a mapping")

    def test_with_mapping_does_not_raise_type_error(self):
        # The following must not raise
        # "not all arguments converted during string formatting".
        self.assertEqual(str.__mod__("foo", {"bar": 42}), "foo")

    def test_positional_after_named_arg_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%(foo)s %s", {"foo": "bar"})
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )

    def test_mix_named_and_tuple_args_returns_string(self):
        self.assertEqual(str.__mod__("%s %(a)s", {"a": 77}), "{'a': 77} 77")

    def test_mapping_in_tuple_returns_string(self):
        self.assertEqual(str.__mod__("%s", ({"foo": "bar"},)), "{'foo': 'bar'}")

    def test_c_format_returns_string(self):
        self.assertEqual(str.__mod__("%c", ("x",)), "x")
        self.assertEqual(str.__mod__("%c", ("\U0001f44d",)), "\U0001f44d")
        self.assertEqual(str.__mod__("%c", (76,)), "L")
        self.assertEqual(str.__mod__("%c", (0x1F40D,)), "\U0001f40d")

    def test_c_format_with_non_int_returns_string(self):
        class C:
            def __index__(self):
                return 42

        self.assertEqual(str.__mod__("%c", (C(),)), "*")

    def test_c_format_raises_overflow_error(self):
        import sys

        maxunicode_range = str.__mod__("range(0x%x)", (sys.maxunicode + 1))
        with self.assertRaises(OverflowError) as context:
            str.__mod__("%c", (sys.maxunicode + 1,))
        self.assertEqual(str(context.exception), f"%c arg not in {maxunicode_range}")
        with self.assertRaises(OverflowError) as context:
            str.__mod__("%c", (-1,))
        self.assertEqual(str(context.exception), f"%c arg not in {maxunicode_range}")

    def test_c_format_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%c", (None,))
        self.assertEqual(str(context.exception), "%c requires int or char")
        with self.assertRaises(TypeError) as context:
            str.__mod__("%c", ("ab",))
        self.assertEqual(str(context.exception), "%c requires int or char")
        with self.assertRaises(TypeError) as context:
            str.__mod__("%c", (123456789012345678901234567890,))
        self.assertEqual(str(context.exception), "%c requires int or char")

        class C:
            def __index__(self):
                raise UserWarning()

        with self.assertRaises(TypeError) as context:
            str.__mod__("%c", (C(),))
        self.assertEqual(str(context.exception), "%c requires int or char")

    def test_s_format_returns_string(self):
        self.assertEqual(str.__mod__("%s", ("foo",)), "foo")

        class C:
            __repr__ = None

            def __str__(self):
                return "str called"

        self.assertEqual(str.__mod__("%s", (C(),)), "str called")

    def test_s_format_propagates_errors(self):
        class C:
            def __str__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%s", (C(),))

    def test_r_format_returns_string(self):
        self.assertEqual(str.__mod__("%r", (42,)), "42")
        self.assertEqual(str.__mod__("%r", ("foo",)), "'foo'")
        self.assertEqual(
            str.__mod__("%r", ({"foo": "\U0001d4eb\U0001d4ea\U0001d4fb"},)),
            "{'foo': '\U0001d4eb\U0001d4ea\U0001d4fb'}",
        )

        class C:
            def __repr__(self):
                return "repr called"

            __str__ = None

        self.assertEqual(str.__mod__("%r", (C(),)), "repr called")

    def test_r_format_propagates_errors(self):
        class C:
            def __repr__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%r", (C(),))

    def test_a_format_returns_string(self):
        self.assertEqual(str.__mod__("%a", (42,)), "42")
        self.assertEqual(str.__mod__("%a", ("foo",)), "'foo'")

        class C:
            def __repr__(self):
                return "repr called"

            __str__ = None

        self.assertEqual(str.__mod__("%a", (C(),)), "repr called")
        # TODO(T39861344, T38702699): We should have a test with some non-ascii
        # characters here proving that they are escaped. Unfortunately
        # builtins.ascii() does not work in that case yet.

    def test_a_format_propagates_errors(self):
        class C:
            def __repr__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%a", (C(),))

    def test_diu_format_returns_string(self):
        self.assertEqual(str.__mod__("%d", (0,)), "0")
        self.assertEqual(str.__mod__("%d", (-1,)), "-1")
        self.assertEqual(str.__mod__("%d", (42,)), "42")
        self.assertEqual(str.__mod__("%i", (0,)), "0")
        self.assertEqual(str.__mod__("%i", (-1,)), "-1")
        self.assertEqual(str.__mod__("%i", (42,)), "42")
        self.assertEqual(str.__mod__("%u", (0,)), "0")
        self.assertEqual(str.__mod__("%u", (-1,)), "-1")
        self.assertEqual(str.__mod__("%u", (42,)), "42")

    def test_diu_format_with_largeint_returns_string(self):
        self.assertEqual(
            str.__mod__("%d", (-123456789012345678901234567890,)),
            "-123456789012345678901234567890",
        )
        self.assertEqual(
            str.__mod__("%i", (-123456789012345678901234567890,)),
            "-123456789012345678901234567890",
        )
        self.assertEqual(
            str.__mod__("%u", (-123456789012345678901234567890,)),
            "-123456789012345678901234567890",
        )

    def test_diu_format_with_non_int_returns_string(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise UserWarning()

        self.assertEqual(str.__mod__("%d", (C(),)), "42")
        self.assertEqual(str.__mod__("%i", (C(),)), "42")
        self.assertEqual(str.__mod__("%u", (C(),)), "42")

    def test_diu_format_raises_typeerrors(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%d", (None,))
        self.assertEqual(
            str(context.exception), "%d format: a number is required, not NoneType"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%i", (None,))
        self.assertEqual(
            str(context.exception), "%i format: a number is required, not NoneType"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%u", (None,))
        self.assertEqual(
            str(context.exception), "%u format: a number is required, not NoneType"
        )

    def test_diu_format_propagates_errors(self):
        class C:
            def __int__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%d", (C(),))
        with self.assertRaises(UserWarning):
            str.__mod__("%i", (C(),))
        with self.assertRaises(UserWarning):
            str.__mod__("%u", (C(),))

    def test_diu_format_reraises_typerrors(self):
        class C:
            def __int__(self):
                raise TypeError("foobar")

        with self.assertRaises(TypeError) as context:
            str.__mod__("%d", (C(),))
        self.assertEqual(
            str(context.exception), "%d format: a number is required, not C"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%i", (C(),))
        self.assertEqual(
            str(context.exception), "%i format: a number is required, not C"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%u", (C(),))
        self.assertEqual(
            str(context.exception), "%u format: a number is required, not C"
        )

    def test_xX_format_returns_string(self):
        self.assertEqual(str.__mod__("%x", (0,)), "0")
        self.assertEqual(str.__mod__("%x", (-123,)), "-7b")
        self.assertEqual(str.__mod__("%x", (42,)), "2a")
        self.assertEqual(str.__mod__("%X", (0,)), "0")
        self.assertEqual(str.__mod__("%X", (-123,)), "-7B")
        self.assertEqual(str.__mod__("%X", (42,)), "2A")

    def test_xX_format_with_largeint_returns_string(self):
        self.assertEqual(
            str.__mod__("%x", (-123456789012345678901234567890,)),
            "-18ee90ff6c373e0ee4e3f0ad2",
        )
        self.assertEqual(
            str.__mod__("%X", (-123456789012345678901234567890,)),
            "-18EE90FF6C373E0EE4E3F0AD2",
        )

    def test_xX_format_with_non_int_returns_string(self):
        class C:
            def __float__(self):
                return 3.3

            def __index__(self):
                return 77

        self.assertEqual(str.__mod__("%x", (C(),)), "4d")
        self.assertEqual(str.__mod__("%X", (C(),)), "4D")

    def test_xX_format_raises_typeerrors(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%x", (None,))
        self.assertEqual(
            str(context.exception), "%x format: an integer is required, not NoneType"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%X", (None,))
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
            str.__mod__("%x", (C(),))
        with self.assertRaises(UserWarning):
            str.__mod__("%X", (C(),))

    def test_xX_format_reraises_typerrors(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise TypeError("foobar")

        with self.assertRaises(TypeError) as context:
            str.__mod__("%x", (C(),))
        self.assertEqual(
            str(context.exception), "%x format: an integer is required, not C"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%X", (C(),))
        self.assertEqual(
            str(context.exception), "%X format: an integer is required, not C"
        )

    def test_o_format_returns_string(self):
        self.assertEqual(str.__mod__("%o", (0,)), "0")
        self.assertEqual(str.__mod__("%o", (-123,)), "-173")
        self.assertEqual(str.__mod__("%o", (42,)), "52")

    def test_o_format_with_largeint_returns_string(self):
        self.assertEqual(
            str.__mod__("%o", (-123456789012345678901234567890)),
            "-143564417755415637016711617605322",
        )

    def test_o_format_with_non_int_returns_string(self):
        class C:
            def __float__(self):
                return 3.3

            def __index__(self):
                return 77

        self.assertEqual(str.__mod__("%o", (C(),)), "115")

    def test_o_format_raises_typeerrors(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%o", (None,))
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
            str.__mod__("%o", (C(),))

    def test_o_format_reraises_typerrors(self):
        class C:
            def __int__(self):
                return 42

            def __index__(self):
                raise TypeError("foobar")

        with self.assertRaises(TypeError) as context:
            str.__mod__("%o", (C(),))
        self.assertEqual(
            str(context.exception), "%o format: an integer is required, not C"
        )

    def test_f_format_returns_string(self):
        self.assertEqual(str.__mod__("%f", (0.0,)), "0.000000")
        self.assertEqual(str.__mod__("%f", (-0.0,)), "-0.000000")
        self.assertEqual(str.__mod__("%f", (1.0,)), "1.000000")
        self.assertEqual(str.__mod__("%f", (-1.0,)), "-1.000000")
        self.assertEqual(str.__mod__("%f", (42.125,)), "42.125000")

        self.assertEqual(str.__mod__("%f", (1e3,)), "1000.000000")
        self.assertEqual(str.__mod__("%f", (1e6,)), "1000000.000000")
        self.assertEqual(
            str.__mod__("%f", (1e40,)),
            "10000000000000000303786028427003666890752.000000",
        )

    def test_F_format_returns_string(self):
        self.assertEqual(str.__mod__("%F", (42.125,)), "42.125000")

    def test_e_format_returns_string(self):
        self.assertEqual(str.__mod__("%e", (0.0,)), "0.000000e+00")
        self.assertEqual(str.__mod__("%e", (-0.0,)), "-0.000000e+00")
        self.assertEqual(str.__mod__("%e", (1.0,)), "1.000000e+00")
        self.assertEqual(str.__mod__("%e", (-1.0,)), "-1.000000e+00")
        self.assertEqual(str.__mod__("%e", (42.125,)), "4.212500e+01")

        self.assertEqual(str.__mod__("%e", (1e3,)), "1.000000e+03")
        self.assertEqual(str.__mod__("%e", (1e6,)), "1.000000e+06")
        self.assertEqual(str.__mod__("%e", (1e40,)), "1.000000e+40")

    def test_E_format_returns_string(self):
        self.assertEqual(str.__mod__("%E", (1.0,)), "1.000000E+00")

    def test_g_format_returns_string(self):
        self.assertEqual(str.__mod__("%g", (0.0,)), "0")
        self.assertEqual(str.__mod__("%g", (-1.0,)), "-1")
        self.assertEqual(str.__mod__("%g", (0.125,)), "0.125")
        self.assertEqual(str.__mod__("%g", (3.5,)), "3.5")

    def test_eEfFgG_format_with_inf_returns_string(self):
        self.assertEqual(str.__mod__("%e", (float("inf"),)), "inf")
        self.assertEqual(str.__mod__("%E", (float("inf"),)), "INF")
        self.assertEqual(str.__mod__("%f", (float("inf"),)), "inf")
        self.assertEqual(str.__mod__("%F", (float("inf"),)), "INF")
        self.assertEqual(str.__mod__("%g", (float("inf"),)), "inf")
        self.assertEqual(str.__mod__("%G", (float("inf"),)), "INF")

        self.assertEqual(str.__mod__("%e", (-float("inf"),)), "-inf")
        self.assertEqual(str.__mod__("%E", (-float("inf"),)), "-INF")
        self.assertEqual(str.__mod__("%f", (-float("inf"),)), "-inf")
        self.assertEqual(str.__mod__("%F", (-float("inf"),)), "-INF")
        self.assertEqual(str.__mod__("%g", (-float("inf"),)), "-inf")
        self.assertEqual(str.__mod__("%G", (-float("inf"),)), "-INF")

    def test_eEfFgG_format_with_nan_returns_string(self):
        self.assertEqual(str.__mod__("%e", (float("nan"),)), "nan")
        self.assertEqual(str.__mod__("%E", (float("nan"),)), "NAN")
        self.assertEqual(str.__mod__("%f", (float("nan"),)), "nan")
        self.assertEqual(str.__mod__("%F", (float("nan"),)), "NAN")
        self.assertEqual(str.__mod__("%g", (float("nan"),)), "nan")
        self.assertEqual(str.__mod__("%G", (float("nan"),)), "NAN")

        self.assertEqual(str.__mod__("%e", (float("-nan"),)), "nan")
        self.assertEqual(str.__mod__("%E", (float("-nan"),)), "NAN")
        self.assertEqual(str.__mod__("%f", (float("-nan"),)), "nan")
        self.assertEqual(str.__mod__("%F", (float("-nan"),)), "NAN")
        self.assertEqual(str.__mod__("%g", (float("-nan"),)), "nan")
        self.assertEqual(str.__mod__("%G", (float("-nan"),)), "NAN")

    def test_f_format_with_precision_returns_string(self):
        number = 1.23456789123456789
        self.assertEqual(str.__mod__("%.0f", number), "1")
        self.assertEqual(str.__mod__("%.1f", number), "1.2")
        self.assertEqual(str.__mod__("%.2f", number), "1.23")
        self.assertEqual(str.__mod__("%.3f", number), "1.235")
        self.assertEqual(str.__mod__("%.4f", number), "1.2346")
        self.assertEqual(str.__mod__("%.5f", number), "1.23457")
        self.assertEqual(str.__mod__("%.6f", number), "1.234568")
        self.assertEqual(str.__mod__("%f", number), "1.234568")

        self.assertEqual(str.__mod__("%.17f", number), "1.23456789123456789")
        self.assertEqual(str.__mod__("%.25f", number), "1.2345678912345678934769921")
        self.assertEqual(
            str.__mod__("%.60f", number),
            "1.234567891234567893476992139767389744520187377929687500000000",
        )

    def test_eEfFgG_format_with_precision_returns_string(self):
        number = 1.23456789123456789
        self.assertEqual(str.__mod__("%.0e", number), "1e+00")
        self.assertEqual(str.__mod__("%.0E", number), "1E+00")
        self.assertEqual(str.__mod__("%.0f", number), "1")
        self.assertEqual(str.__mod__("%.0F", number), "1")
        self.assertEqual(str.__mod__("%.0g", number), "1")
        self.assertEqual(str.__mod__("%.0G", number), "1")
        self.assertEqual(str.__mod__("%.4e", number), "1.2346e+00")
        self.assertEqual(str.__mod__("%.4E", number), "1.2346E+00")
        self.assertEqual(str.__mod__("%.4f", number), "1.2346")
        self.assertEqual(str.__mod__("%.4F", number), "1.2346")
        self.assertEqual(str.__mod__("%.4g", number), "1.235")
        self.assertEqual(str.__mod__("%.4G", number), "1.235")
        self.assertEqual(str.__mod__("%e", number), "1.234568e+00")
        self.assertEqual(str.__mod__("%E", number), "1.234568E+00")
        self.assertEqual(str.__mod__("%f", number), "1.234568")
        self.assertEqual(str.__mod__("%F", number), "1.234568")
        self.assertEqual(str.__mod__("%g", number), "1.23457")
        self.assertEqual(str.__mod__("%G", number), "1.23457")

    def test_g_format_with_flags_and_width_returns_string(self):
        self.assertEqual(str.__mod__("%5g", 7.0), "    7")
        self.assertEqual(str.__mod__("%5g", 7.2), "  7.2")
        self.assertEqual(str.__mod__("% 5g", 7.2), "  7.2")
        self.assertEqual(str.__mod__("%+5g", 7.2), " +7.2")
        self.assertEqual(str.__mod__("%5g", -7.2), " -7.2")
        self.assertEqual(str.__mod__("% 5g", -7.2), " -7.2")
        self.assertEqual(str.__mod__("%+5g", -7.2), " -7.2")

        self.assertEqual(str.__mod__("%-5g", 7.0), "7    ")
        self.assertEqual(str.__mod__("%-5g", 7.2), "7.2  ")
        self.assertEqual(str.__mod__("%- 5g", 7.2), " 7.2 ")
        self.assertEqual(str.__mod__("%-+5g", 7.2), "+7.2 ")
        self.assertEqual(str.__mod__("%-5g", -7.2), "-7.2 ")
        self.assertEqual(str.__mod__("%- 5g", -7.2), "-7.2 ")
        self.assertEqual(str.__mod__("%-+5g", -7.2), "-7.2 ")

        self.assertEqual(str.__mod__("%#g", 7.0), "7.00000")

        self.assertEqual(str.__mod__("%#- 7.2g", float("-nan")), " nan   ")
        self.assertEqual(str.__mod__("%#- 7.2g", float("inf")), " inf   ")
        self.assertEqual(str.__mod__("%#- 7.2g", float("-inf")), "-inf   ")

    def test_eEfFgG_format_with_flags_and_width_returns_string(self):
        number = 1.23456789123456789
        self.assertEqual(str.__mod__("% -#12.3e", number), " 1.235e+00  ")
        self.assertEqual(str.__mod__("% -#12.3E", number), " 1.235E+00  ")
        self.assertEqual(str.__mod__("% -#12.3f", number), " 1.235      ")
        self.assertEqual(str.__mod__("% -#12.3F", number), " 1.235      ")
        self.assertEqual(str.__mod__("% -#12.3g", number), " 1.23       ")
        self.assertEqual(str.__mod__("% -#12.3G", number), " 1.23       ")

    def test_ef_format_with_non_float_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%e", (None,))
        self.assertEqual(str(context.exception), "must be real number, not NoneType")

        class C:
            def __float__(self):
                return "not a float"

        with self.assertRaises(TypeError) as context:
            str.__mod__("%f", (C(),))
        self.assertEqual(
            str(context.exception), "C.__float__ returned non-float (type str)"
        )

    def test_g_format_propogates_errors(self):
        class C:
            def __float__(self):
                raise UserWarning()

        with self.assertRaises(UserWarning):
            str.__mod__("%g", (C(),))

    def test_efg_format_with_non_float_returns_string(self):
        class A(float):
            pass

        self.assertEqual(str.__mod__("%e", (A(9.625),)), str.__mod__("%e", (9.625,)))

        class C:
            def __float__(self):
                return 3.5

        self.assertEqual(str.__mod__("%f", (C(),)), str.__mod__("%f", (3.5,)))

        class D:
            def __float__(self):
                return A(-12.75)

        warnings.filterwarnings(
            action="ignore",
            category=DeprecationWarning,
            message=".*__float__ returned non-float.*",
            module=__name__,
        )
        self.assertEqual(str.__mod__("%g", (D(),)), str.__mod__("%g", (-12.75,)))

    def test_percent_format_returns_percent(self):
        self.assertEqual(str.__mod__("%%", ()), "%")

    def test_escaped_percent_with_characters_between_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%0.0%", ())
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%*.%", (42,))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%d %*.%", (42,))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%.*%", (88,))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%0#*.42%", (1234,))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )

    def test_escaped_percent_with_characters_between_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__mod__("%*.%", (42, 1))
        self.assertEqual(
            str(context.exception), "unsupported format character '%' (0x25) at index 3"
        )

    def test_flags_get_accepted(self):
        self.assertEqual(str.__mod__("%-s", ""), "")
        self.assertEqual(str.__mod__("%+s", ""), "")
        self.assertEqual(str.__mod__("% s", ""), "")
        self.assertEqual(str.__mod__("%#s", ""), "")
        self.assertEqual(str.__mod__("%0s", ""), "")
        self.assertEqual(str.__mod__("%#-#0+ -s", ""), "")

    def test_string_format_with_width_returns_string(self):
        self.assertEqual(str.__mod__("%5s", "oh"), "   oh")
        self.assertEqual(str.__mod__("%-5s", "ah"), "ah   ")
        self.assertEqual(str.__mod__("%05s", "uh"), "   uh")
        self.assertEqual(str.__mod__("%-# 5s", "eh"), "eh   ")

        self.assertEqual(str.__mod__("%0s", "foo"), "foo")
        self.assertEqual(str.__mod__("%-0s", "foo"), "foo")
        self.assertEqual(str.__mod__("%10s", "hello world"), "hello world")
        self.assertEqual(str.__mod__("%-10s", "hello world"), "hello world")

    def test_string_format_with_width_star_returns_string(self):
        self.assertEqual(str.__mod__("%*s", (7, "foo")), "    foo")
        self.assertEqual(str.__mod__("%*s", (-7, "bar")), "bar    ")
        self.assertEqual(str.__mod__("%-*s", (7, "baz")), "baz    ")
        self.assertEqual(str.__mod__("%-*s", (-7, "bam")), "bam    ")

    def test_string_format_with_precision_returns_string(self):
        self.assertEqual(str.__mod__("%.3s", "python"), "pyt")
        self.assertEqual(str.__mod__("%.0s", "python"), "")
        self.assertEqual(str.__mod__("%.10s", "python"), "python")

    def test_string_format_with_precision_star_returns_string(self):
        self.assertEqual(str.__mod__("%.*s", (3, "monty")), "mon")
        self.assertEqual(str.__mod__("%.*s", (0, "monty")), "")
        self.assertEqual(str.__mod__("%.*s", (-4, "monty")), "")

    def test_string_format_with_width_and_precision_returns_string(self):
        self.assertEqual(str.__mod__("%8.3s", ("foobar",)), "     foo")
        self.assertEqual(str.__mod__("%-8.3s", ("foobar",)), "foo     ")
        self.assertEqual(str.__mod__("%*.3s", (8, "foobar")), "     foo")
        self.assertEqual(str.__mod__("%*.3s", (-8, "foobar")), "foo     ")
        self.assertEqual(str.__mod__("%8.*s", (3, "foobar")), "     foo")
        self.assertEqual(str.__mod__("%-8.*s", (3, "foobar")), "foo     ")
        self.assertEqual(str.__mod__("%*.*s", (8, 3, "foobar")), "     foo")
        self.assertEqual(str.__mod__("%-*.*s", (8, 3, "foobar")), "foo     ")

    def test_s_r_a_c_formats_accept_flags_width_precision_return_strings(self):
        self.assertEqual(str.__mod__("%-*.3s", (8, "foobar")), "foo     ")
        self.assertEqual(str.__mod__("%-*.3r", (8, "foobar")), "'fo     ")
        self.assertEqual(str.__mod__("%-*.3a", (8, "foobar")), "'fo     ")
        self.assertEqual(str.__mod__("%-*.3c", (8, 94)), "^       ")

    def test_number_format_with_sign_flag_returns_string(self):
        self.assertEqual(str.__mod__("%+d", (42,)), "+42")
        self.assertEqual(str.__mod__("%+d", (-42,)), "-42")
        self.assertEqual(str.__mod__("% d", (17,)), " 17")
        self.assertEqual(str.__mod__("% d", (-17,)), "-17")
        self.assertEqual(str.__mod__("%+ d", (42,)), "+42")
        self.assertEqual(str.__mod__("%+ d", (-42,)), "-42")
        self.assertEqual(str.__mod__("% +d", (17,)), "+17")
        self.assertEqual(str.__mod__("% +d", (-17,)), "-17")

    def test_number_format_alt_flag_returns_string(self):
        self.assertEqual(str.__mod__("%#d", (23,)), "23")
        self.assertEqual(str.__mod__("%#x", (23,)), "0x17")
        self.assertEqual(str.__mod__("%#X", (23,)), "0X17")
        self.assertEqual(str.__mod__("%#o", (23,)), "0o27")

    def test_number_format_with_width_returns_string(self):
        self.assertEqual(str.__mod__("%5d", (123,)), "  123")
        self.assertEqual(str.__mod__("%5d", (-8,)), "   -8")
        self.assertEqual(str.__mod__("%-5d", (123,)), "123  ")
        self.assertEqual(str.__mod__("%-5d", (-8,)), "-8   ")

        self.assertEqual(str.__mod__("%05d", (123,)), "00123")
        self.assertEqual(str.__mod__("%05d", (-8,)), "-0008")
        self.assertEqual(str.__mod__("%-05d", (123,)), "123  ")
        self.assertEqual(str.__mod__("%0-5d", (-8,)), "-8   ")

        self.assertEqual(str.__mod__("%#7x", (42,)), "   0x2a")
        self.assertEqual(str.__mod__("%#7x", (-42,)), "  -0x2a")

        self.assertEqual(str.__mod__("%5d", (123456,)), "123456")
        self.assertEqual(str.__mod__("%-5d", (-123456,)), "-123456")

    def test_number_format_with_precision_returns_string(self):
        self.assertEqual(str.__mod__("%.5d", (123,)), "00123")
        self.assertEqual(str.__mod__("%.5d", (-123,)), "-00123")
        self.assertEqual(str.__mod__("%.5d", (1234567,)), "1234567")
        self.assertEqual(str.__mod__("%#.5x", (99,)), "0x00063")

    def test_number_format_with_width_precision_flags_returns_string(self):
        self.assertEqual(str.__mod__("%8.3d", (12,)), "     012")
        self.assertEqual(str.__mod__("%8.3d", (-7,)), "    -007")
        self.assertEqual(str.__mod__("%05.3d", (12,)), "00012")
        self.assertEqual(str.__mod__("%+05.3d", (12,)), "+0012")
        self.assertEqual(str.__mod__("% 05.3d", (12,)), " 0012")
        self.assertEqual(str.__mod__("% 05.3x", (19,)), " 0013")

        self.assertEqual(str.__mod__("%-8.3d", (12,)), "012     ")
        self.assertEqual(str.__mod__("%-8.3d", (-7,)), "-007    ")
        self.assertEqual(str.__mod__("%- 8.3d", (66,)), " 066    ")

    def test_width_and_precision_star_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%*d", (42,))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%.*d", (42,))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%*.*d", (42,))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%*.*d", (1, 2))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )

    def test_negative_precision_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__mod__("%.-2s", "foo")
        self.assertEqual(
            str(context.exception), "unsupported format character '-' (0x2d) at index 2"
        )

    def test_two_specifiers_returns_string(self):
        self.assertEqual(str.__mod__("%s%s", ("foo", "bar")), "foobar")
        self.assertEqual(str.__mod__(",%s%s", ("foo", "bar")), ",foobar")
        self.assertEqual(str.__mod__("%s,%s", ("foo", "bar")), "foo,bar")
        self.assertEqual(str.__mod__("%s%s,", ("foo", "bar")), "foobar,")
        self.assertEqual(str.__mod__(",%s..%s---", ("foo", "bar")), ",foo..bar---")
        self.assertEqual(str.__mod__(",%s...%s--", ("foo", "bar")), ",foo...bar--")
        self.assertEqual(str.__mod__(",,%s.%s---", ("foo", "bar")), ",,foo.bar---")
        self.assertEqual(str.__mod__(",,%s...%s-", ("foo", "bar")), ",,foo...bar-")
        self.assertEqual(str.__mod__(",,,%s..%s-", ("foo", "bar")), ",,,foo..bar-")
        self.assertEqual(str.__mod__(",,,%s.%s--", ("foo", "bar")), ",,,foo.bar--")

    def test_mixed_specifiers_with_percents_returns_string(self):
        self.assertEqual(str.__mod__("%%%s%%%s%%", ("foo", "bar")), "%foo%bar%")

    def test_mixed_specifiers_returns_string(self):
        self.assertEqual(
            str.__mod__("a %d %g %s", (123, 3.14, "baz")), "a 123 3.14 baz"
        )

    def test_specifier_missing_format_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__mod__("%", ())
        self.assertEqual(str(context.exception), "incomplete format")
        with self.assertRaises(ValueError) as context:
            str.__mod__("%(foo)", {"foo": None})
        self.assertEqual(str(context.exception), "incomplete format")

    def test_unknown_specifier_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            str.__mod__("try %Y", (42,))
        self.assertEqual(
            str(context.exception), "unsupported format character 'Y' (0x59) at index 5"
        )

    def test_too_few_args_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("%s%s", ("foo",))
        self.assertEqual(
            str(context.exception), "not enough arguments for format string"
        )

    def test_too_many_args_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            str.__mod__("hello", 42)
        self.assertEqual(
            str(context.exception),
            "not all arguments converted during string formatting",
        )
        with self.assertRaises(TypeError) as context:
            str.__mod__("%d%s", (1, "foo", 3))
        self.assertEqual(
            str(context.exception),
            "not all arguments converted during string formatting",
        )


if __name__ == "__main__":
    unittest.main()
