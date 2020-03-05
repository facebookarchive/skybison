import unittest
from .test_api import ApiTests
from .test_code_sbs import CodeTests
from .test_peephole import PeepHoleTests
from .test_flags import FlagTests
from .test_graph import GraphTests
from .test_py37 import Python37Tests
from .test_sbs_stdlib import SbsCompileTests
from .test_errors import ErrorTests, ErrorTestsBuiltin
from .test_symbols import SymbolVisitorTests
from .test_unparse import UnparseTests
from .test_visitor import VisitorTests

if __name__ == "__main__":
    unittest.main()
