#!/usr/bin/env python3
import unittest


class BytesTest(unittest.TestCase):
    def test_dunder_new_with_str_without_encoding_raises_type_error(self):
        with self.assertRaises(TypeError):
            bytes("foo")

    def test_dunder_new_with_str_and_encoding_returns_bytes(self):
        self.assertEquals(bytes("foo", "ascii"), b"foo")

    def test_dunder_new_with_ignore_errors_returns_bytes(self):
        self.assertEquals(bytes("fo\x80o", "ascii", "ignore"), b"foo")


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


class GeneratorTests(unittest.TestCase):
    def test_managed_stop_iteration(self):
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


class IntTests(unittest.TestCase):
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
            self.assertEqual(
                str(context.exception), "issubclass() arg 1 must be a class"
            )

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


class ListTests(unittest.TestCase):
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


class RangeTests(unittest.TestCase):
    def test_range_attrs_are_set(self):
        obj = range(0, 1, 2)
        self.assertEqual(obj.start, 0)
        self.assertEqual(obj.stop, 1)
        self.assertEqual(obj.step, 2)

    def test_range_start_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).start = 2

    def test_range_step_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).step = 2

    def test_range_stop_is_readonly(self):
        with self.assertRaises(AttributeError):
            range(0, 1, 2).stop = 2


if __name__ == "__main__":
    unittest.main()
