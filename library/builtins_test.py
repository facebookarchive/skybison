#!/usr/bin/env python3
import unittest


class DictTests(unittest.TestCase):
    def test_clear_with_non_dict_raises_type_error(self):
        with self.assertRaises(TypeError):
            dict.clear(None)

    def test_clear_removes_all_elements(self):
        d = {"a": 1}
        self.assertEqual(dict.clear(d), None)
        self.assertEqual(d.__len__(), 0)
        self.assertNotIn("a", d)

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


class IntTests(unittest.TestCase):
    def test_new_with_base_without_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            int(base=8)

    def test_new_with_bool_returns_int(self):
        self.assertIs(int(False), 0)
        self.assertIs(int(True), 1)

    def test_new_with_bytearray_returns_int(self):
        self.assertEqual(int(bytearray(b"23")), 23)
        self.assertEqual(int(bytearray(b"-23"), 8), -0o23)
        self.assertEqual(int(bytearray(b"abc"), 16), 0xABC)
        self.assertEqual(int(bytearray(b"0xabc"), 0), 0xABC)

    def test_new_with_bytes_returns_int(self):
        self.assertEqual(int(b"-23"), -23)
        self.assertEqual(int(b"23", 8), 0o23)
        self.assertEqual(int(b"abc", 16), 0xABC)
        self.assertEqual(int(b"0xabc", 0), 0xABC)

    def test_new_with_empty_bytearray_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(bytearray())

    def test_new_with_empty_bytes_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(b"")

    def test_new_with_empty_str_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("")

    def test_new_with_int_returns_int(self):
        self.assertEqual(int(23), 23)

    def test_new_with_int_and_base_raises_type_error(self):
        with self.assertRaises(TypeError):
            int(4, 5)

    def test_new_with_invalid_base_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("0", 1)

    def test_new_with_invalid_chars_raises_value_error(self):
        with self.assertRaises(ValueError):
            int("&2*")

    def test_new_with_invalid_digits_raises_value_error(self):
        with self.assertRaises(ValueError):
            int(b"789", 6)

    def test_new_with_str_returns_int(self):
        self.assertEqual(int("23"), 23)
        self.assertEqual(int("-23", 8), -0o23)
        self.assertEqual(int("-abc", 16), -0xABC)
        self.assertEqual(int("0xabc", 0), 0xABC)

    def test_new_with_zero_args_returns_zero(self):
        self.assertIs(int(), 0)


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

        with self.assertRaises(TypeError):
            reversed(C())

    def test_reversed_length_hint(self):
        it = reversed([1, 2, 3])
        self.assertEqual(it.__length_hint__(), 3)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 2)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 1)
        it.__next__()
        self.assertEqual(it.__length_hint__(), 0)


class SetTests(unittest.TestCase):
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


class StrTests(unittest.TestCase):
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


if __name__ == "__main__":
    unittest.main()
