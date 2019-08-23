#!/usr/bin/env python3
import sys
import unittest

from test_support import pyro_only


class QuickStringIO:
    def __init__(self):
        self.val = ""

    def write(self, text):
        self.val += text

    def getvalue(self):
        return self.val


class DisplayhookTest(unittest.TestCase):
    def test_displayhook_with_none_does_not_set_underscore(self):
        import builtins

        if hasattr(builtins, "_"):
            del builtins._

        orig_out = sys.stdout
        # TODO(T46541598): Test output with real StringIO
        out = QuickStringIO()
        sys.stdout = out
        sys.displayhook(None)
        self.assertEqual(out.getvalue(), "")
        self.assertTrue(not hasattr(builtins, "_"))
        sys.stdout = orig_out

    def test_displayhook_with_int_sets_underscore(self):
        import builtins

        orig_out = sys.stdout
        # TODO(T46541598): Test output with real StringIO
        out = QuickStringIO()
        sys.stdout = out
        sys.displayhook(42)
        self.assertEqual(out.getvalue(), "42\n")
        self.assertEqual(builtins._, 42)
        sys.stdout = orig_out


class SysTests(unittest.TestCase):
    class Mgr:
        def __enter__(self):
            pass

        def __exit__(self, type, value, tb):
            return True

    def test_exc_info_with_context_manager(self):
        try:
            raise RuntimeError()
        except RuntimeError:
            info1 = sys.exc_info()
            with self.Mgr():
                raise ValueError()
            info2 = sys.exc_info()
            self.assertEqual(info1, info2)

    def test_getsizeof_without_dunder_sizeof_raises_type_error(self):
        class M(type):
            def mro(cls):
                return (cls,)

        class C(metaclass=M):
            __new__ = type.__new__
            __call__ = type.__call__

        with self.assertRaises(TypeError):
            sys.getsizeof(C())

    def test_getsizeof_with_non_int_without_default_raises_type_error(self):
        class C:
            def __sizeof__(self):
                return "not an integer"

        with self.assertRaises(TypeError):
            sys.getsizeof(C())

    def test_getsizeof_with_non_int_returns_default(self):
        class C:
            def __sizeof__(self):
                return "not an integer"

        self.assertEqual(sys.getsizeof(C(), 42), 42)

    def test_getsizeof_with_negative_raises_value_error(self):
        class C:
            def __sizeof__(self):
                return -1

        with self.assertRaises(ValueError):
            sys.getsizeof(C())

    @pyro_only
    def test_getsizeof_without_default_returns_size_int(self):
        class C:
            def __sizeof__(self):
                return 42

        self.assertEqual(sys.getsizeof(C()), 42)

    @pyro_only
    def test_getsizeof_with_default_returns_size_int(self):
        class C:
            def __sizeof__(self):
                return 42

        self.assertEqual(sys.getsizeof(C(), 3), 42)

    @pyro_only
    def test_getsizeof_with_int_subclass_returns_int(self):
        class N(int):
            pass

        class C:
            def __sizeof__(self):
                return N(42)

        result = sys.getsizeof(C())
        self.assertIs(type(result), int)
        self.assertEqual(result, 42)

    @pyro_only
    def test_getframe_code_with_int_subclass(self):
        class C(int):
            pass

        code = sys._getframe_code(C(0))
        self.assertEqual(code.co_name, "test_getframe_code_with_int_subclass")

    @pyro_only
    def test_getframe_code_returns_self(self):
        code = sys._getframe_code(0)
        self.assertEqual(code.co_name, "test_getframe_code_returns_self")

    @pyro_only
    def test_getframe_code_returns_class_run(self):
        code = sys._getframe_code(1)
        self.assertEqual(code.co_name, "run")

    @pyro_only
    def test_getframe_globals_with_int_subclass(self):
        class C(int):
            pass

        self.assertIsInstance(sys._getframe_globals(C(0)), dict)

    @pyro_only
    def test_getframe_globals_returns_dict(self):
        self.assertIsInstance(sys._getframe_globals(0), dict)

    @pyro_only
    def test_getframe_lineno_returns_int(self):
        self.assertIsInstance(sys._getframe_lineno(0), int)

    @pyro_only
    def test_getframe_locals_returns_local_vars(self):
        def foo():
            a = 4
            a = a  # noqa: F841
            b = 5
            b = b  # noqa: F841
            return sys._getframe_locals()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        self.assertEqual(result["b"], 5)

    @pyro_only
    def test_getframe_locals_returns_free_vars(self):
        def foo():
            a = 4

            def bar():
                nonlocal a
                a = 5
                return sys._getframe_locals()

            return bar()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 1)
        self.assertEqual(result["a"], 5)

    @pyro_only
    def test_getframe_locals_returns_cell_vars(self):
        def foo():
            a = 4

            def bar(b):
                return a + b

            return sys._getframe_locals()

        result = foo()
        self.assertIsInstance(result, dict)
        self.assertEqual(len(result), 2)
        self.assertEqual(result["a"], 4)
        from types import FunctionType

        self.assertIsInstance(result["bar"], FunctionType)

    @pyro_only
    def test_getframe_locals_with_too_large_depth_raise_value_error(self):
        with self.assertRaises(ValueError) as context:
            sys._getframe_locals(100)
        self.assertIn("call stack is not deep enough", str(context.exception))


if __name__ == "__main__":
    unittest.main()
