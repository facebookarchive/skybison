import ast
import dis
import re
import unittest
from .common import CompilerTest
from compiler import compile


class ApiTests(CompilerTest):
    def test_compile_single(self):
        code = compile('42', 'foo', 'single')
        self.assertInBytecode(code, 'LOAD_CONST', 42)
        self.assertInBytecode(code, 'PRINT_EXPR')

    def test_compile_eval(self):
        code = compile('42', 'foo', 'eval')
        self.assertInBytecode(code, 'LOAD_CONST', 42)
        self.assertInBytecode(code, 'RETURN_VALUE')

    def test_bad_mode(self):
        expected = re.escape("compile() 3rd arg must be 'exec' or 'eval' or 'single'")
        with self.assertRaisesRegex(ValueError, expected):
             compile('42', 'foo', 'foo')

if __name__ == "__main__":
    unittest.main()
