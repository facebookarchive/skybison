#!/usr/bin/env python3
import builtins
import unittest

from test_support import pyro_only


try:
    import _builtins
    from _builtins import _Unbound
except ImportError:
    pass


@pyro_only
class UnderBuiltinsTests(unittest.TestCase):
    def test_dict_get_raises_exception_from_dunder_eq(self):
        class ExceptionRaiser:
            def __eq__(self, other):
                raise UserWarning("ExceptionRaiser.__eq__")

            def __hash__(self):
                return 400

        class C:
            def __hash__(self):
                return 400

        exception_raiser = ExceptionRaiser()
        c = C()
        d = {exception_raiser: 500}
        with self.assertRaises(UserWarning) as context:
            d[c]
        self.assertEqual(str(context.exception), "ExceptionRaiser.__eq__")

    def test_list_new_default_fill_returns_list(self):
        self.assertListEqual(_builtins._list_new(-1), [])
        self.assertListEqual(_builtins._list_new(0), [])
        self.assertListEqual(_builtins._list_new(3), [None, None, None])

    def test_list_new_default_fill_returns_list(self):
        self.assertListEqual(_builtins._list_new(0, 1), [])
        self.assertListEqual(_builtins._list_new(3, 1), [1, 1, 1])

    def test_set_function_flag_iterable_coroutine_makes_generator_usable_with_await(
        self,
    ):
        def generator():
            yield

        async def await_on_f(f):
            await f()

        with self.assertRaises(TypeError):
            await_on_f(generator).send(None)

        _builtins._set_function_flag_iterable_coroutine(generator)

        await_on_f(generator).send(None)

    def test_set_function_flag_iterable_coroutine_sets_flag_in_dunder_code_dunder(self):
        def generator():
            yield

        _builtins._set_function_flag_iterable_coroutine(generator)

        # 0x100 = CO_ITERABLE_COROUTINE flag
        self.assertTrue(generator.__code__.co_flags & 0x100)

    def test_set_function_flag_iterable_coroutine_only_works_with_functions(self):
        with self.assertRaises(TypeError):
            _builtins._set_function_flag_iterable_coroutine("str")

    def test_str_ctor_with_bytes_and_no_encoding_returns_str(self):
        decoded = _builtins._str_ctor(str, b"abc")
        self.assertEqual(decoded, "b'abc'")

    def test_str_ctor_with_str_raises_type_error(self):
        with self.assertRaises(TypeError):
            _builtins._str_ctor(str, "", encoding="utf_8")

    def test_str_ctor_with_non_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            _builtins._str_ctor(str, 1, encoding="utf_8")

    def test_str_ctor_with_bytes_and_encoding_returns_decoded_str(self):
        decoded = _builtins._str_ctor(str, b"abc", encoding="ascii")
        self.assertEqual(decoded, "abc")

    def test_str_ctor_with_no_object_and_encoding_returns_empty_string(self):
        self.assertEqual(_builtins._str_ctor(str, encoding="ascii"), "")

    def test_str_ctor_with_class_without_dunder_str_returns_str(self):
        class A:
            def __repr__(self):
                return "test"

        self.assertEqual(_builtins._str_ctor(str, A()), "test")

    def test_str_ctor_with_class_with_faulty_dunder_str_raises_type_error(self):
        with self.assertRaises(TypeError):

            class A:
                def __str__(self):
                    return 1

            _builtins._str_ctor(str, A())

    def test_str_ctor_with_class_with_proper_duner_str_returns_str(self):
        class A:
            def __str__(self):
                return "test"

        self.assertEqual(_builtins._str_ctor(str, A()), "test")

    def test_str_mod_fast_path_returns_string(self):
        self.assertEqual(_builtins._str_mod_fast_path("", ()), "")
        self.assertEqual(_builtins._str_mod_fast_path("hello", ()), "hello")
        self.assertEqual(_builtins._str_mod_fast_path("%s", ("foo",)), "foo")
        self.assertEqual(_builtins._str_mod_fast_path("%s%s", ("foo", "bar")), "foobar")
        self.assertEqual(
            _builtins._str_mod_fast_path("%s.%s", ("foo", "bar")), "foo.bar"
        )
        self.assertEqual(
            _builtins._str_mod_fast_path("%s..%s", ("foo", "bar")), "foo..bar"
        )
        self.assertEqual(
            _builtins._str_mod_fast_path("0%s12%s345", ("foo", "bar")), "0foo12bar345"
        )
        self.assertEqual(
            _builtins._str_mod_fast_path("012%s34%s5", ("foo", "bar")), "012foo34bar5"
        )
        self.assertEqual(
            _builtins._str_mod_fast_path(
                "/-%sH%s%s'\u26f7\ufe0f^", ("a", "foob", "\U0001f40d")
            ),
            "/-aHfoob\U0001f40d'\u26f7\ufe0f^",
        )

    def test_str_mod_fast_path_with_subclasses(self):
        class C(str):
            def __str__(self):
                return "huh"

        class D(tuple):
            pass

        self.assertIs(
            _builtins._str_mod_fast_path(C("%s;%s"), D(("foo", "bar"))), "foo;bar"
        )

    def test_str_mod_fast_path_with_non_tuple_args_returns_unbound(self):
        self.assertIs(_builtins._str_mod_fast_path("%s", "foo"), _Unbound)
        self.assertIs(_builtins._str_mod_fast_path("%(foo)", {"foo": 1}), _Unbound)

    def test_str_mod_fast_path_with_unsupported_modifier_returns_unbound(self):
        self.assertIs(_builtins._str_mod_fast_path("%%", ()), _Unbound)
        self.assertIs(_builtins._str_mod_fast_path("%d", (4,)), _Unbound)
        self.assertIs(_builtins._str_mod_fast_path("%s%%", ("a",)), _Unbound)
        self.assertIs(_builtins._str_mod_fast_path("    %x", ("a",)), _Unbound)
        self.assertIs(_builtins._str_mod_fast_path("%", ()), _Unbound)

    def test_str_mod_fast_path_with_invalid_arguments_returns_unbound(self):
        self.assertIs(_builtins._str_mod_fast_path("", ("",)), _Unbound)
        self.assertIs(_builtins._str_mod_fast_path("%s", (1,)), _Unbound)
        self.assertIs(_builtins._str_mod_fast_path("%s", ("a", "b")), _Unbound)
        self.assertIs(_builtins._str_mod_fast_path(" %s;%s", ("a",)), _Unbound)

    def test_str_mod_fast_path_with_str_subclass_returns_unbound(self):
        class C(str):
            def __str__(self):
                return "foo"

        self.assertIs(_builtins._str_mod_fast_path("%s", (C("bar"),)), _Unbound)

    def test_str_mod_fast_path_with_too_many_modifiers_returns_unbound(self):
        self.assertIs(
            _builtins._str_mod_fast_path(
                "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s",
                tuple("abcdefghijklmnopqrst"),
            ),
            _Unbound,
        )
        self.assertIs(
            _builtins._str_mod_fast_path(
                "%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s", ()
            ),
            _Unbound,
        )

    def test_traceback_next_set_with_none_modifies_traceback(self):
        def foo():
            raise RuntimeError()

        try:
            foo()
        except Exception as e:
            tb = e.__traceback__
            self.assertIsNotNone(tb.tb_next)
            tb.tb_next = None
            self.assertIsNone(tb.tb_next)


if __name__ == "__main__":
    unittest.main()
