import ast
from compiler import opcode37
from compiler.optimizer import AstOptimizer, BIN_OPS, is_const, get_const_value
from compiler.pyassem import PyFlowGraph37
from compiler.pycodegen import Python37CodeGenerator

import _compiler_opcode as opcodepyro


def should_rewrite_printf(node):
    return isinstance(node.left, ast.Str) and isinstance(node.op, ast.Mod)


def create_conversion_call(name, value):
    method = ast.Attribute(ast.Str(""), name, ast.Load())
    return ast.Call(method, args=[value], keywords=[])


def try_constant_fold_mod(format_string, right):
    r = get_const_value(right)
    return ast.Str(format_string.__mod__(r))


class AstOptimizerPyro(AstOptimizer):
    def rewrite_str_mod(self, left, right):  # noqa: C901
        format_string = left.s
        try:
            if is_const(right):
                return try_constant_fold_mod(format_string, right)
            # Try and collapse the whole expression into a string
            const_tuple = self.makeConstTuple(right.elts)
            if const_tuple:
                return ast.Str(format_string.__mod__(const_tuple.value))
        except Exception:
            pass
        n_specifiers = 0
        i = 0
        length = len(format_string)
        while i < length:
            i = format_string.find("%", i)
            if i == -1:
                break
            ch = format_string[i]
            i += 1

            if i >= length:
                # Invalid format string ending in a single percent
                return None
            ch = format_string[i]
            i += 1
            if ch == "%":
                # Break the string apart at '%'
                continue
            elif ch == "(":
                # We don't support dict lookups and may get confused from
                # inner '%' chars
                return None
            n_specifiers += 1

        rhs = right
        if isinstance(right, ast.Tuple):
            rhs_values = rhs.elts
            num_values = len(rhs_values)
        else:
            # If RHS is not a tuple constructor, then we only support the
            # situation with a single format specifier in the string, by
            # normalizing `rhs` to a one-element tuple:
            # `_mod_check_single_arg(rhs)[0]`
            rhs_values = None
            if n_specifiers != 1:
                return None
            num_values = 1
        i = 0
        value_idx = 0
        segment_begin = 0
        strings = []
        while i < length:
            i = format_string.find("%", i)
            if i == -1:
                break
            ch = format_string[i]
            i += 1

            segment_end = i - 1
            if segment_end - segment_begin > 0:
                substr = format_string[segment_begin:segment_end]
                strings.append(ast.Str(substr))

            if i >= length:
                return None
            ch = format_string[i]
            i += 1

            # Parse flags and width
            spec_begin = i - 1
            have_width = False
            while True:
                if ch == "0":
                    # TODO(matthiasb): Support ' ', '+', '#', etc
                    # They mostly have the same meaning. However they can
                    # appear in any order here but must follow stricter
                    # conventions in f-strings.
                    if i >= length:
                        return None
                    ch = format_string[i]
                    i += 1
                    continue
                break
            if "1" <= ch <= "9":
                have_width = True
                if i >= length:
                    return None
                ch = format_string[i]
                i += 1
                while "0" <= ch <= "9":
                    if i >= length:
                        return None
                    ch = format_string[i]
                    i += 1
            spec_str = None
            if i - 1 - spec_begin > 0:
                spec_str = format_string[spec_begin : i - 1]

            if ch == "%":
                # Handle '%%'
                segment_begin = i - 1
                continue

            # Handle remaining supported cases that use a value from RHS
            if rhs_values is not None:
                if value_idx >= num_values:
                    return None
                value = rhs_values[value_idx]
            else:
                # We have a situation like `"%s" % x` without tuple on RHS.
                # Transform to: f"{''._mod_check_single_arg(x)[0]}"
                converted = create_conversion_call("_mod_check_single_arg", rhs)
                value = ast.Subscript(converted, ast.Index(ast.Num(0)), ast.Load())
            value_idx += 1

            if ch in "sra":
                # Rewrite "%s" % (x,) to f"{x!s}"
                if have_width:
                    # Need to explicitly specify alignment because `%5s`
                    # aligns right, while `f"{x:5}"` aligns left.
                    spec_str = ">" + spec_str
                format_spec = ast.Str(spec_str) if spec_str is not None else None
                formatted = ast.FormattedValue(value, ord(ch), format_spec)
                strings.append(formatted)
            elif ch in "diu":
                # Rewrite "%d" % (x,) to f"{''._mod_convert_number(x)}".
                # Calling a method on the empty string is a hack to access a
                # well-known function regardless of the surrounding
                # environment.
                converted = create_conversion_call("_mod_convert_number", value)
                format_spec = ast.Str(spec_str) if spec_str is not None else None
                formatted = ast.FormattedValue(converted, -1, format_spec)
                strings.append(formatted)
            else:
                return None
            # Begin next segment after specifier
            segment_begin = i

        if value_idx != num_values:
            return None

        segment_end = length
        if segment_end - segment_begin > 0:
            substr = format_string[segment_begin:segment_end]
            strings.append(ast.Str(substr))

        return ast.JoinedStr(strings)

    def visitBinOp(self, node: ast.BinOp) -> ast.expr:
        left = self.visit(node.left)
        right = self.visit(node.right)

        if is_const(left) and is_const(right):
            handler = BIN_OPS.get(type(node.op))
            if handler is not None:
                lval = get_const_value(left)
                rval = get_const_value(right)
                try:
                    return ast.copy_location(ast.Constant(handler(lval, rval)), node)
                except Exception:
                    pass

        if should_rewrite_printf(node):
            result = self.rewrite_str_mod(left, right)
            if result:
                return self.visit(result)

        return self.update_node(node, left=left, right=right)


class PyroFlowGraph(PyFlowGraph37):
    opcode = opcodepyro.opcode


class PyroCodeGenerator(Python37CodeGenerator):
    flow_graph = PyroFlowGraph

    @classmethod
    def optimize_tree(cls, optimize: int, tree: ast.AST):
        return AstOptimizerPyro(optimize=optimize > 0).visit(tree)
