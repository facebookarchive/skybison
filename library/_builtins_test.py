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
    def test_getframe_function_with_int_subclass(self):
        class C(int):
            pass

        def f():
            return _builtins._getframe_function(C(0))

        self.assertEqual(f(), f)

    def test_getframe_function_returns_function(self):
        def f():
            return _builtins._getframe_function(0)

        self.assertEqual(f(), f)

    def test_getframe_function_returns_class_body(self):
        from types import FunctionType

        class C:
            result = _builtins._getframe_function(0)

        self.assertIsInstance(C.result, FunctionType)
        self.assertEqual(C.result.__code__.co_name, "C")

    def test_getframe_function_returns_parent_function(self):
        def f():
            return g()

        def g():
            return h()

        def h():
            return (
                _builtins._getframe_function(0),
                _builtins._getframe_function(1),
                _builtins._getframe_function(2),
            )

        self.assertEqual(f(), (h, g, f))

    def test_getframe_function_returns_none(self):
        with self.assertRaises(ValueError) as context:
            _builtins._getframe_function(9999)
        self.assertIn("call stack is not deep enough", str(context.exception))

    def test_getframe_lineno_returns_int(self):
        self.assertIsInstance(_builtins._getframe_lineno(0), int)

    def test_getframe_locals_returns_local_vars(self):
        def foo():
            a = 4
            a = a  # noqa: F841
            b = 5
            b = b  # noqa: F841
            return _builtins._getframe_locals(0)

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        self.assertEqual(result["b"], 5)

    def test_getframe_locals_returns_free_vars(self):
        def foo():
            a = 4

            def bar():
                nonlocal a
                a = 5
                return _builtins._getframe_locals(0)

            return bar()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 1)
        self.assertEqual(result["a"], 5)

    def test_getframe_locals_returns_cell_vars(self):
        def foo():
            a = 4

            def bar(b):
                return a + b

            return _builtins._getframe_locals(0)

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        from types import FunctionType

        self.assertIsInstance(result["bar"], FunctionType)

    def test_getframe_locals_in_class_body_returns_dict_with_class_variables(self):
        class C:
            foo = 4
            bar = 5
            baz = _builtins._getframe_locals(0)

        result = C.baz
        self.assertIsInstance(result, dict)
        self.assertEqual(result["foo"], 4)
        self.assertEqual(result["bar"], 5)

    def test_getframe_locals_in_exec_scope_returns_given_locals_instance(self):
        result_key = None
        result_value = None

        class C:
            def __getitem__(self, key):
                if key == "_getframe_locals":
                    return _builtins._getframe_locals
                raise Exception

            def __setitem__(self, key, value):
                nonlocal result_key
                nonlocal result_value
                result_key = key
                result_value = value

        # TODO(T54956257): Use regular dict for globals.
        from types import ModuleType

        module = ModuleType("test_module")
        c = C()
        exec("result = _getframe_locals(0)", module.__dict__, c)
        self.assertEqual(result_key, "result")
        self.assertIs(result_value, c)

    def test_getframe_locals_with_too_large_depth_raise_value_error(self):
        with self.assertRaises(ValueError) as context:
            _builtins._getframe_locals(100)
        self.assertIn("call stack is not deep enough", str(context.exception))

    def test_list_new_default_fill_returns_list(self):
        self.assertListEqual(_builtins._list_new(-1), [])
        self.assertListEqual(_builtins._list_new(0), [])
        self.assertListEqual(_builtins._list_new(3), [None, None, None])

    def test_list_new_default_fill_returns_list(self):
        self.assertListEqual(_builtins._list_new(0, 1), [])
        self.assertListEqual(_builtins._list_new(3, 1), [1, 1, 1])

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


if __name__ == "__main__":
    unittest.main()
