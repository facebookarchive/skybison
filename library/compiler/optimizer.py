import ast
import operator
from ast import Bytes, Constant, Ellipsis, NameConstant, Num, Str, cmpop, copy_location
from typing import Dict, Iterable, Optional, Type

from .consts import PyCF_REWRITE_PRINTF
from .peephole import safe_lshift, safe_mod, safe_multiply, safe_power
from .visitor import ASTRewriter


def is_const(node):
    return isinstance(node, (Constant, Num, Str, Bytes, Ellipsis, NameConstant))


def get_const_value(node):
    if isinstance(node, (Constant, NameConstant)):
        return node.value
    elif isinstance(node, Num):
        return node.n
    elif isinstance(node, (Str, Bytes)):
        return node.s
    elif isinstance(node, Ellipsis):
        return ...

    raise TypeError("Bad constant value")


class Py37Limits:
    MAX_INT_SIZE = 128
    MAX_COLLECTION_SIZE = 256
    MAX_STR_SIZE = 4096
    MAX_TOTAL_ITEMS = 1024


UNARY_OPS = {
    ast.Invert: operator.invert,
    ast.Not: operator.not_,
    ast.UAdd: operator.pos,
    ast.USub: operator.neg,
}
INVERSE_OPS: Dict[Type[cmpop], Type[cmpop]] = {
    ast.Is: ast.IsNot,
    ast.IsNot: ast.Is,
    ast.In: ast.NotIn,
    ast.NotIn: ast.In,
}

BIN_OPS = {
    ast.Add: operator.add,
    ast.Sub: operator.sub,
    ast.Mult: lambda l, r: safe_multiply(l, r, Py37Limits),
    ast.Div: operator.truediv,
    ast.FloorDiv: operator.floordiv,
    ast.Mod: lambda l, r: safe_mod(l, r, Py37Limits),
    ast.Pow: lambda l, r: safe_power(l, r, Py37Limits),
    ast.LShift: lambda l, r: safe_lshift(l, r, Py37Limits),
    ast.RShift: operator.rshift,
    ast.BitOr: operator.or_,
    ast.BitXor: operator.xor,
    ast.BitAnd: operator.and_,
}


class AstOptimizer(ASTRewriter):
    def __init__(self, optimize: bool = False, flags: int = 0):
        super().__init__()
        self.optimize = optimize
        self.flags = flags

    def visitUnaryOp(self, node: ast.UnaryOp) -> ast.expr:
        op = self.visit(node.operand)
        if is_const(op):
            conv = UNARY_OPS[type(node.op)]
            val = get_const_value(op)
            try:
                return copy_location(Constant(conv(val)), node)
            except Exception:
                pass
        elif (
            isinstance(node.op, ast.Not)
            and isinstance(op, ast.Compare)
            and len(op.ops) == 1
        ):
            cmp_op = op.ops[0]
            new_op = INVERSE_OPS.get(type(cmp_op))
            if new_op is not None:
                return self.update_node(op, ops=[new_op()])

        return self.update_node(node, operand=op)

    def should_rewrite_printf(self, node):
        return isinstance(node.left, ast.Str) and isinstance(node.op, ast.Mod)

    def create_conversion_call(self, name, value):
        method = ast.Attribute(ast.Str(""), name, ast.Load())
        return ast.Call(method, args=[value], keywords=[])

    def rewrite_str_mod(self, node):
        if isinstance(node.right, ast.Constant):
            return self.try_constant_fold_mod(node)
        format_string = node.left.s
        try:
            # Try and collapse the whole expression into a string
            const_tuple = self.makeConstTuple(node.right.elts)
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

        rhs = node.right
        if isinstance(node.right, ast.Tuple):
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
                converted = self.create_conversion_call("_mod_check_single_arg", rhs)
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
                converted = self.create_conversion_call("_mod_convert_number", value)
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
        left_node = self.visit(node.left)
        right_node = self.visit(node.right)

        if is_const(left_node) and is_const(right_node):
            handler = BIN_OPS.get(type(node.op))
            if handler is not None:
                lval = get_const_value(left_node)
                rval = get_const_value(right_node)
                try:
                    return copy_location(Constant(handler(lval, rval)), node)
                except Exception:
                    pass

        if self.flags & PyCF_REWRITE_PRINTF and self.should_rewrite_printf(node):
            result = self.rewrite_str_mod(node)
            if result:
                return self.visit(result)

        return self.update_node(node, left=left_node, right=right_node)

    def makeConstTuple(self, elts: Iterable[ast.expr]) -> Optional[Constant]:
        if all(is_const(elt) for elt in elts):
            return Constant(tuple(get_const_value(elt) for elt in elts))

        return None

    def visitTuple(self, node: ast.Tuple) -> ast.expr:
        elts = self.walk_list(node.elts)

        if isinstance(node.ctx, ast.Load):
            res = self.makeConstTuple(elts)
            if res is not None:
                return copy_location(res, node)

        return self.update_node(node, elts=elts)

    def visitSubscript(self, node: ast.Subscript) -> ast.expr:
        value = self.visit(node.value)
        slice = self.visit(node.slice)

        if (
            isinstance(node.ctx, ast.Load)
            and is_const(value)
            and isinstance(slice, ast.Index)
            # pyre-ignore[16]: `Index` has no attribute `value`.
            and is_const(slice.value)
        ):
            try:
                return copy_location(
                    Constant(get_const_value(value)[get_const_value(slice.value)]), node
                )
            except Exception:
                pass

        return self.update_node(node, value=value, slice=slice)

    def _visitIter(self, node: ast.expr) -> ast.expr:
        if isinstance(node, ast.List):
            elts = self.walk_list(node.elts)
            res = self.makeConstTuple(elts)
            if res is not None:
                return copy_location(res, node)

            return self.update_node(node, elts=elts)
        elif isinstance(node, ast.Set):
            elts = self.walk_list(node.elts)
            res = self.makeConstTuple(elts)
            if res is not None:
                return copy_location(Constant(frozenset(res.value)), node)

            return self.update_node(node, elts=elts)

        return self.generic_visit(node)

    def visitcomprehension(self, node: ast.comprehension) -> ast.comprehension:
        target = self.visit(node.target)
        iter = self.visit(node.iter)
        ifs = self.walk_list(node.ifs)
        iter = self._visitIter(iter)

        return self.update_node(node, target=target, iter=iter, ifs=ifs)

    def visitFor(self, node: ast.For) -> ast.For:
        target = self.visit(node.target)
        iter = self.visit(node.iter)
        body = self.walk_list(node.body)
        orelse = self.walk_list(node.orelse)

        iter = self._visitIter(iter)
        return self.update_node(
            node, target=target, iter=iter, body=body, orelse=orelse
        )

    def visitCompare(self, node: ast.Compare) -> ast.expr:
        left = self.visit(node.left)
        comparators = self.walk_list(node.comparators)

        if isinstance(node.ops[-1], (ast.In, ast.NotIn)):
            new_iter = self._visitIter(comparators[-1])
            if new_iter is not None and new_iter is not comparators[-1]:
                comparators = list(comparators)
                comparators[-1] = new_iter

        return self.update_node(node, left=left, comparators=comparators)

    def visitName(self, node: ast.Name):
        if node.id == "__debug__":
            return copy_location(Constant(not self.optimize), node)

        return self.generic_visit(node)

    def visitAssert(self, node: ast.Assert):
        if self.optimize:
            # Skip asserts if we're optimizing
            return None
        return self.generic_visit(node)
