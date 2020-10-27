#!/usr/bin/env python3
try:
    import _parser
except ImportError:
    pass
import __future__

import ast
import unittest

from test_support import pyro_only


@pyro_only
class UnderParserTests(unittest.TestCase):
    def test_parse_with_bad_mode_raises_value_error(self):
        with self.assertRaises(ValueError):
            _parser.parse("1", "foo.py", "invalidmode")

    def test_parse_with_bad_source_raises_type_error(self):
        with self.assertRaises(TypeError):
            _parser.parse(1, "foo.py", "exec")

    def test_parse_with_source_with_nul_raises_value_error(self):
        with self.assertRaises(ValueError):
            _parser.parse(b"123\x00123", "foo.py", "exec")

    def test_parse_with_str_returns_ast(self):
        instance = _parser.parse("1", "foo.py", "exec")
        self.assertIsInstance(instance, ast.AST)

    def test_parse_without_barry_as_bdfl_flag_raises_syntax_error(self):
        with self.assertRaises(SyntaxError):
            _parser.parse("a <> b", "foo.py", "exec")

    def test_parse_with_barry_as_bdfl_flag_allows_ne_operator(self):
        instance = _parser.parse(
            "a <> b", "foo.py", "exec", __future__.CO_FUTURE_BARRY_AS_BDFL
        )
        self.assertIsInstance(instance, ast.AST)


if __name__ == "__main__":
    unittest.main()
