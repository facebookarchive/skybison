import glob
import inspect
from compiler.pycodegen import make_compiler
from test.bytecode_helper import BytecodeTestCase
from types import CodeType
from os import path
from subprocess import run, PIPE
from io import StringIO

_UNSPECIFIED = object()

def get_repo_root():
    dirname = path.dirname(__file__)
    completed_process = run(
        ["git", "rev-parse", "--show-toplevel"], cwd=dirname, stdout=PIPE, stderr=PIPE
    )
    if completed_process.returncode:
        print("Error occurred", file=sys.stderr)
        sys.exit(1)
    return completed_process.stdout.strip().decode("utf8")


def glob_test(dir, pattern, adder):
    base_path = path.dirname(__file__)
    target_dir = path.join(base_path, dir)
    for fname in glob.glob(path.join(target_dir, pattern), recursive=True):
        modname = path.relpath(fname, base_path)
        adder(modname, fname)


class CompilerTest(BytecodeTestCase):
    def compile(self, code, generator=None):
        code = inspect.cleandoc("\n" + code)
        gen = make_compiler(code, "", "exec", generator=generator)
        return gen.getCode()

    def run_code(self, code, generator=None):
        compiled = self.compile(code, generator)
        d = {}
        exec(compiled, d)
        return d

    def find_code(self, code):
        consts = [const for const in code.co_consts if isinstance(const, CodeType)]
        if len(consts) == 0:
            self.assertFail("No const available")
        elif len(consts) != 1:
            self.assertFail("Too many consts")
        return consts[0]

    def to_graph(self, code, generator=None):
        code = inspect.cleandoc("\n" + code)
        gen = make_compiler(code, "", "exec", generator=generator)
        return gen.graph

    def dump_graph(self, graph):
        io = StringIO()
        graph.dump(io)
        return io.getvalue()

    def graph_to_instrs(self, graph):
        for block in graph.getBlocks():
            yield from block.getInstructions()

    def assertNotInGraph(self, graph, opname, argval=_UNSPECIFIED):
        for block in graph.getBlocks():
            for instr in block.getInstructions():
                if instr.opname == opname:
                    if argval == _UNSPECIFIED:
                        msg = '%s occurs in bytecode:\n%s' % (opname, self.dump_graph(graph))
                        self.fail(msg)
                    elif instr.oparg == argval:
                        msg = '(%s,%r) occurs in bytecode:\n%s'
                        msg = msg % (opname, argval, self.dump_graph(graph))
                        self.fail(msg)

    def assertInGraph(self, graph, opname, argval=_UNSPECIFIED):
        """Returns instr if op is found, otherwise throws AssertionError"""
        for block in graph.getBlocks():
            for instr in block.getInstructions():
                if instr.opname == opname:
                    if argval is _UNSPECIFIED or instr.oparg == argval:
                        return instr
        disassembly = self.dump_graph(graph)
        if argval is _UNSPECIFIED:
            msg = '%s not found in bytecode:\n%s' % (opname, disassembly)
        else:
            msg = '(%s,%r) not found in bytecode:\n%s'
            msg = msg % (opname, argval, disassembly)
        self.fail(msg)
