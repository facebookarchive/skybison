import unittest

from .test_api import ApiTests  # noqa: F401
from .test_code_sbs import CodeTests  # noqa: F401
from .test_errors import ErrorTests, ErrorTestsBuiltin  # noqa: F401
from .test_flags import FlagTests  # noqa: F401
from .test_graph import GraphTests  # noqa: F401
from .test_peephole import PeepHoleTests  # noqa: F401
from .test_py37 import Python37Tests  # noqa: F401
from .test_sbs_stdlib import SbsCompileTests  # noqa: F401

# TODO(T78727149): Enable static python in compiler/ in pyro
# from .test_static import StaticCompilationTests, StaticRuntimeTests  # noqa: F401
from .test_symbols import SymbolVisitorTests  # noqa: F401
from .test_unparse import UnparseTests  # noqa: F401
from .test_visitor import VisitorTests  # noqa: F401


if __name__ == "__main__":
    unittest.main()
