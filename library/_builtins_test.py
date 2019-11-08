#!/usr/bin/env python3
import builtins
import unittest

from test_support import pyro_only


try:
    import _builtins
except ImportError:
    pass


@pyro_only
class UnderBuiltinsTests(unittest.TestCase):
    def test_getframe_code_with_int_subclass(self):
        class C(int):
            pass

        code = _builtins._getframe_code(C(0))
        self.assertEqual(code.co_name, "test_getframe_code_with_int_subclass")

    def test_getframe_code_returns_self(self):
        code = _builtins._getframe_code(0)
        self.assertEqual(code.co_name, "test_getframe_code_returns_self")

    def test_getframe_code_returns_class_run(self):
        code = _builtins._getframe_code(1)
        self.assertEqual(code.co_name, "run")

    def test_getframe_globals_with_int_subclass(self):
        class C(int):
            pass

        self.assertIsInstance(_builtins._getframe_globals(C(0)), builtins.module_proxy)

    def test_getframe_globals_returns_module_proxy(self):
        self.assertIsInstance(_builtins._getframe_globals(0), builtins.module_proxy)

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


if __name__ == "__main__":
    unittest.main()
