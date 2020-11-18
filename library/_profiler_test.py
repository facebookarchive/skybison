import os
import sys
import unittest
import warnings
from tempfile import TemporaryDirectory

from test_support import pyro_only

try:
    import _profiler
except ImportError:
    pass


@pyro_only
@unittest.skipIf(
    "PYRO_CPP_INTERPRETER" in os.environ,
    "opcode counting/profiling not support in C++ interpreter",
)
class ProfilerTest(unittest.TestCase):
    def test_dump_callgrind_writes_file(self):
        def bar():
            pass

        def foo():
            for _ in range(5):
                bar()

        with TemporaryDirectory() as temp_dir:
            warnings.filterwarnings(
                action="ignore",
                category=RuntimeWarning,
                message="Interpreter switching .*",
                module="_builtins",
            )
            _profiler.install()
            foo()
            _profiler.dump_callgrind(f"{temp_dir}/profile.cg")
            _profiler.uninstall()

            with open(f"{temp_dir}/profile.cg") as fp:
                contents = fp.read()

            this_module = sys.modules[__name__]

            expected = f""".*\
# callgrind format
version: 1
creator: _profiler
positions: line
events: Op

fl=\\(0\\) <empty>
fn=\\(0\\) builtins.range.__new__
0 1

fn=\\(1\\) _builtins._profiler_exclude
0 1

fl=\\(1\\) {this_module.__file__}
fn=\\(2\\) __main__.ProfilerTest.test_dump_callgrind_writes_file.<locals>.bar
[0-9]+ [0-9]+

fl=\\(2\\) {_profiler.__file__}
fn=\\(3\\) _profiler.dump_callgrind
[0-9]+ 0
cfi=\\(0\\)
cfn=\\(1\\)
calls=1 0
[0-9]+ 1

fl=\\(1\\)
fn=\\(4\\) __main__.ProfilerTest.test_dump_callgrind_writes_file.<locals>.foo
[0-9]+ [0-9]+
cfn=\\(0\\)
calls=1 0
[0-9]+ 1
cfi=\\(1\\)
cfn=\\(2\\)
calls=5 [0-9]+
[0-9]+ [0-9]+

fn=\\(5\\) test_dump_callgrind_writes_file
[0-9]+ 0
cfn=\\(4\\)
calls=1 [0-9]+
[0-9]+ [0-9]+
cfi=\\(2\\)
cfn=\\(3\\)
calls=1 [0-9]+
[0-9]+ 0.*"""
            self.assertRegex(contents, expected)


if __name__ == "__main__":
    unittest.main()
