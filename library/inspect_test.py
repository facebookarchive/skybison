#!/usr/bin/env python3
import inspect
import math
import unittest


class InspectModuleTest(unittest.TestCase):
    def test_signature_with_function_returns_signature(self):
        def foo(arg0, arg1):
            pass

        result = inspect.signature(foo)
        self.assertEqual(str(result), "(arg0, arg1)")

    def test_signature_with_classmethod_returns_signature(self):
        class C:
            @classmethod
            def foo(cls, arg0, arg1):
                pass

        result = inspect.signature(C.foo)
        self.assertEqual(str(result), "(arg0, arg1)")

    def test_signature_with_dunder_call_returns_signature(self):
        class C:
            def __call__(self, arg0, arg1):
                pass

        instance = C()
        result = inspect.signature(instance)
        self.assertEqual(str(result), "(arg0, arg1)")

    def test_signature_with_c_function_raises_value_error(self):
        with self.assertRaises(ValueError):
            inspect.signature(math.log)

    def test_getmodule_with_frame_returns_module(self):
        from types import ModuleType

        frame = inspect.currentframe()
        module = inspect.getmodule(frame)
        self.assertIsInstance(module, ModuleType)
        self.assertIs(module.__name__, __name__)


if __name__ == "__main__":
    unittest.main()
