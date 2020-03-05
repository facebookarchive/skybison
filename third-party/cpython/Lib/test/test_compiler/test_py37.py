import ast
import dis
import math
from .common import CompilerTest
from compiler.pyassem import PyFlowGraph
from compiler.pycodegen import CodeGenerator, Python37CodeGenerator
from compiler.optimizer import AstOptimizer
from compiler.unparse import to_expr
from compiler.consts import (
    CO_OPTIMIZED,
    CO_NOFREE,
    CO_NEWLOCALS,
    CO_FUTURE_GENERATOR_STOP,
    CO_FUTURE_ANNOTATIONS,
)


LOAD_METHOD = "LOAD_METHOD"
if LOAD_METHOD not in dis.opmap:
    LOAD_METHOD = "<160>"

CALL_METHOD = "CALL_METHOD"
if CALL_METHOD not in dis.opmap:
    CALL_METHOD = "<161>"
STORE_ANNOTATION = "STORE_ANNOTATION"
if STORE_ANNOTATION not in dis.opmap:
    STORE_ANNOTATION = "<0>"

class Python37Tests(CompilerTest):
    def test_compile_method(self):
        code = self.compile('x.f()', Python37CodeGenerator)
        self.assertInBytecode(code, LOAD_METHOD)
        self.assertInBytecode(code, CALL_METHOD, 0)

        code = self.compile('x.f(42)', Python37CodeGenerator)
        self.assertInBytecode(code, LOAD_METHOD)
        self.assertInBytecode(code, CALL_METHOD, 1)

    def test_compile_method_varargs(self):
        code = self.compile('x.f(*foo)', Python37CodeGenerator)
        self.assertNotInBytecode(code, LOAD_METHOD)

    def test_compile_method_kwarg(self):
        code = self.compile('x.f(kwarg=1)', Python37CodeGenerator)
        self.assertNotInBytecode(code, LOAD_METHOD)

    def test_compile_method_normal(self):
        code = self.compile('f()', Python37CodeGenerator)
        self.assertNotInBytecode(code, LOAD_METHOD)

    def test_future_gen_stop(self):
        code = self.compile("from __future__ import generator_stop", Python37CodeGenerator)
        self.assertEqual(
            code.co_flags,
            CO_NOFREE,
        )

    def test_future_annotations_flag(self):
        code = self.compile("from __future__ import annotations", Python37CodeGenerator)
        self.assertEqual(
            code.co_flags,
            CO_NOFREE |  CO_FUTURE_ANNOTATIONS,
        )

    def test_async_aiter(self):
        # Make sure GET_AITER isn't followed by LOAD_CONST None as Python 3.7 doesn't support async __aiter__
        for generator in (Python37CodeGenerator, CodeGenerator):
            outer_graph = self.to_graph("""
                async def f():
                    async for x in y:
                        pass
            """, generator)
            for outer_instr in self.graph_to_instrs(outer_graph):
                if outer_instr.opname == "LOAD_CONST" and isinstance(outer_instr.oparg, CodeGenerator):
                    saw_aiter = False
                    for instr in self.graph_to_instrs(outer_instr.oparg.graph):
                        if saw_aiter:
                            self.assertEqual(instr.opname == "LOAD_CONST", generator is CodeGenerator)
                            if generator is CodeGenerator:
                                self.assertIsNone(instr.oparg)
                            break

                        if instr.opname == "GET_AITER":
                            saw_aiter = True
                    break

    def test_try_except_pop_except(self):
        """POP_EXCEPT moved after END_FINALLY in Python 3.7"""
        for generator in (Python37CodeGenerator, CodeGenerator):
            graph = self.to_graph("""
                try:
                    pass
                except Exception as e:
                    pass
            """, generator)
            prev_instr = None
            for instr in self.graph_to_instrs(graph):
                if instr.opname == "POP_EXCEPT":
                    self.assertEqual(prev_instr.opname == "END_FINALLY", generator is Python37CodeGenerator, prev_instr.opname)
                prev_instr = instr

    def test_func_doc_str(self):
        """POP_EXCEPT moved after END_FINALLY in Python 3.7"""
        test_code = """
            def f():

                '''hello there

                '''
            """

        py37_code = self.find_code(self.compile(test_code, Python37CodeGenerator))
        self.assertEqual(py37_code.co_lnotab, b"\x00\x04")

        py36_code = self.find_code(self.compile(test_code, CodeGenerator))
        self.assertEqual(py36_code.co_lnotab, b"")

    def test_future_annotations(self):
        annotations = ["42"]
        for annotation in annotations:
            code = self.compile(f"from __future__ import annotations\ndef f() -> {annotation}:\n    pass", Python37CodeGenerator)
            self.assertInBytecode(code, 'LOAD_CONST', annotation)
        self.assertEqual(
            code.co_flags,
            CO_NOFREE |  CO_FUTURE_ANNOTATIONS,
        )

    def test_circular_import_as(self):
        """verifies that we emit an IMPORT_FROM to enable circular imports
        when compiling an absolute import to verify that they can support
        circular imports"""
        code = self.compile(f"import x.y as b", Python37CodeGenerator)
        self.assertInBytecode(code, 'IMPORT_FROM')
        self.assertNotInBytecode(code, 'LOAD_ATTR')

        code = self.compile(f"import x.y as b", CodeGenerator)
        self.assertNotInBytecode(code, 'IMPORT_FROM')
        self.assertInBytecode(code, 'LOAD_ATTR')

    def test_store_annotation_removed(self):
        code = self.compile(f"class C:\n    x: int = 42", Python37CodeGenerator)
        class_code = self.find_code(code)
        self.assertNotInBytecode(class_code, 'STORE_ANNOTATION')

        code = self.compile(f"class C:\n    x: int = 42", CodeGenerator)
        class_code = self.find_code(code)
        self.assertInBytecode(class_code, 'STORE_ANNOTATION')

    def test_compile_opt_unary_jump(self):
        graph = self.to_graph('if not abc: foo', Python37CodeGenerator)
        self.assertNotInGraph(graph, 'POP_JUMP_IF_FALSE')

        graph = self.to_graph('if not abc: foo', CodeGenerator)
        self.assertInGraph(graph, 'POP_JUMP_IF_FALSE')

    def test_compile_opt_bool_or_jump(self):
        graph = self.to_graph('if abc or bar: foo', Python37CodeGenerator)
        self.assertNotInGraph(graph, 'JUMP_IF_TRUE_OR_POP')

        graph = self.to_graph('if abc or bar: foo', CodeGenerator)
        self.assertInGraph(graph, 'JUMP_IF_TRUE_OR_POP')

    def test_compile_opt_bool_and_jump(self):
        graph = self.to_graph('if abc and bar: foo', Python37CodeGenerator)
        self.assertNotInGraph(graph, 'JUMP_IF_FALSE_OR_POP')

        graph = self.to_graph('if abc and bar: foo', CodeGenerator)
        self.assertInGraph(graph, 'JUMP_IF_FALSE_OR_POP')

    def test_compile_opt_assert_or_bool(self):
        graph = self.to_graph('assert abc or bar', Python37CodeGenerator)
        self.assertNotInGraph(graph, 'JUMP_IF_TRUE_OR_POP')

        graph = self.to_graph('assert abc or bar', CodeGenerator)
        self.assertInGraph(graph, 'JUMP_IF_TRUE_OR_POP')

    def test_compile_opt_assert_and_bool(self):
        graph = self.to_graph('assert abc and bar', Python37CodeGenerator)
        self.assertNotInGraph(graph, 'JUMP_IF_FALSE_OR_POP')

        graph = self.to_graph('assert abc and bar', CodeGenerator)
        self.assertInGraph(graph, 'JUMP_IF_FALSE_OR_POP')

    def test_compile_opt_if_exp(self):
        graph = self.to_graph('assert not a if c else b', Python37CodeGenerator)
        self.assertNotInGraph(graph, 'UNARY_NOT')

        graph = self.to_graph('assert not a if c else b', CodeGenerator)
        self.assertInGraph(graph, 'UNARY_NOT')

    def test_compile_opt_cmp_op(self):
        graph = self.to_graph('assert not a > b', Python37CodeGenerator)
        self.assertNotInGraph(graph, 'UNARY_NOT')

        graph = self.to_graph('assert not a > b', CodeGenerator)
        self.assertInGraph(graph, 'UNARY_NOT')

    def test_compile_opt_chained_cmp_op(self):
        graph = self.to_graph('assert not a > b > c', Python37CodeGenerator)
        self.assertNotInGraph(graph, 'UNARY_NOT')

        graph = self.to_graph('assert not a > b > c', CodeGenerator)
        self.assertInGraph(graph, 'UNARY_NOT')

    def test_compile_opt_enabled(self):
        graph = self.to_graph('x = -1', Python37CodeGenerator)
        self.assertNotInGraph(graph, 'UNARY_NEGATIVE')

        graph = self.to_graph('x = -1', CodeGenerator)
        self.assertInGraph(graph, 'UNARY_NEGATIVE')

    def test_opt_debug(self):
        graph = self.to_graph('if not __debug__:\n    x = 42', Python37CodeGenerator)
        self.assertNotInGraph(graph, 'STORE_NAME')

        graph = self.to_graph('if not __debug__:\n    x = 42', CodeGenerator)
        self.assertInGraph(graph, 'STORE_NAME')

    def test_opt_debug_del(self):
        code = 'def f(): del __debug__'
        outer_graph = self.to_graph(code, Python37CodeGenerator)
        for outer_instr in self.graph_to_instrs(outer_graph):
            if outer_instr.opname == "LOAD_CONST" and isinstance(outer_instr.oparg, CodeGenerator):
                graph = outer_instr.oparg.graph
                self.assertInGraph(graph, 'LOAD_CONST', True)
                self.assertNotInGraph(graph, 'DELETE_FAST', '__debug__')

        outer_graph = self.to_graph(code, CodeGenerator)
        for outer_instr in self.graph_to_instrs(outer_graph):
            if outer_instr.opname == "LOAD_CONST" and isinstance(outer_instr.oparg, CodeGenerator):
                graph = outer_instr.oparg.graph
                self.assertNotInGraph(graph, 'LOAD_CONST', True)
                self.assertInGraph(graph, 'DELETE_FAST', '__debug__')

    def test_const_fold(self):
        code = self.compile('x = 0.0\ny=-0.0', Python37CodeGenerator)
        self.assertEqual(code.co_consts, (0.0, -0.0, None))
        self.assertEqual(math.copysign(1, code.co_consts[0]), 1)
        self.assertEqual(math.copysign(1, code.co_consts[1]), -1)

    def test_const_fold_tuple(self):
        code = self.compile('x = (0.0, )\ny=(-0.0, )', Python37CodeGenerator)
        self.assertEqual(code.co_consts, ((0.0, ), (-0.0, ), None))
        self.assertEqual(math.copysign(1, code.co_consts[0][0]), 1)
        self.assertEqual(math.copysign(1, code.co_consts[1][0]), -1)

    def test_const_fold(self):
        code = '{' + '**{}, '*256 + '}'
        graph = self.to_graph(code, Python37CodeGenerator)
        self.assertInGraph(graph, 'BUILD_MAP_UNPACK', 256)

        code = '{' + '**{}, '*256 + '}'
        graph = self.to_graph(code, CodeGenerator)
        self.assertInGraph(graph, 'BUILD_MAP_UNPACK', 255)
        self.assertInGraph(graph, 'BUILD_MAP_UNPACK', 2)

    def test_ast_optimizer(self):
        cases = [
            ("+1", "1"),
            ("--1", "1"),
            ("~1", "-2"),
            ("not 1", "False"),
            ("not x is y", "x is not y"),
            ("not x is not y", "x is y"),
            ("not x in y", "x not in y"),
            ("~1.1", "~1.1"),
            ("+'str'", "+'str'"),
            ("1 + 2", "3"),
            ("1 + 3", "4"),
            ("'abc' + 'def'", "'abcdef'"),
            ("b'abc' + b'def'", "b'abcdef'"),
            ("b'abc' + 'def'", "b'abc' + 'def'"),
            ("b'abc' + --2", "b'abc' + 2"),
            ("--2 + 'abc'", "2 + 'abc'"),
            ("5 - 3", "2"),
            ("6 - 3", "3"),
            ("2 * 2", "4"),
            ("2 * 3", "6"),
            ("'abc' * 2", "'abcabc'"),
            ("b'abc' * 2", "b'abcabc'"),
            ("1 / 2", "0.5"),
            ("6 / 2", "3.0"),
            ("6 // 2", "3"),
            ("5 // 2", "2"),
            ("2 >> 1", "1"),
            ("6 >> 1", "3"),
            ("1 | 2", "3"),
            ("1 | 1", "1"),
            ("1 ^ 3", "2"),
            ("1 ^ 1", "0"),
            ("1 & 2", "0"),
            ("1 & 3", "1"),
            ("'abc' + 1", "'abc' + 1"),
            ("1 / 0", "1 / 0"),
            ("1 + None", "1 + None"),
            ("True + None", "True + None"),
            ("True + 1", "2"),
            ("(1, 2)", "(1, 2)"),
            ("(1, 2) * 2", "(1, 2, 1, 2)"),
            ("(1, --2, abc)", "(1, 2, abc)"),
            ("(1, 2)[0]", "1"),
            ("1[0]", "1[0]"),
            ("x[+1]", "x[1]"),
            ("(+1)[x]", "1[x]"),
            ("[x for x in [1,2,3]]", "[x for x in (1, 2, 3)]"),
            ("(x for x in [1,2,3])", "(x for x in (1, 2, 3))"),
            ("{x for x in [1,2,3]}", "{x for x in (1, 2, 3)}"),
            ("{x for x in [--1,2,3]}", "{x for x in (1, 2, 3)}"),
            ("{--1 for x in [1,2,3]}", "{1 for x in (1, 2, 3)}"),
            ("x in [1,2,3]", "x in (1, 2, 3)"),
            ("x in x in [1,2,3]", "x in x in (1, 2, 3)"),
            ("x in [1,2,3] in x", "x in [1, 2, 3] in x"),
        ]
        for inp, expected in cases:
            optimizer = AstOptimizer()
            tree = ast.parse(inp)
            optimized = to_expr(optimizer.visit(tree).body[0].value)
            self.assertEqual(expected, optimized, "Input was: " + inp)

    def test_ast_optimizer_for(self):
            optimizer = AstOptimizer()
            tree = ast.parse('for x in [1,2,3]: pass')
            optimized = optimizer.visit(tree).body[0]
            self.assertEqual(to_expr(optimized.iter), "(1, 2, 3)")
