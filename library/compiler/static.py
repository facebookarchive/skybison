from __future__ import annotations

import ast
import linecache
import sys
from ast import (
    AST,
    And,
    AnnAssign,
    Assign,
    AsyncFor,
    AsyncFunctionDef,
    AsyncWith,
    Attribute,
    AugAssign,
    Await,
    BinOp,
    BoolOp,
    Bytes,
    Call,
    ClassDef,
    Compare,
    Constant,
    DictComp,
    Ellipsis,
    For,
    FormattedValue,
    FunctionDef,
    GeneratorExp,
    If,
    IfExp,
    Import,
    ImportFrom,
    Index,
    Is,
    IsNot,
    JoinedStr,
    Lambda,
    ListComp,
    Module,
    Name,
    NameConstant,
    Num,
    Return,
    SetComp,
    Slice,
    Starred,
    Str,
    Subscript,
    Try,
    UnaryOp,
    While,
    With,
    Yield,
    YieldFrom,
    cmpop,
    expr,
)
from contextlib import contextmanager
from types import CodeType, MethodDescriptorType
from typing import (
    Callable as typingCallable,
    Collection,
    Dict,
    Generator,
    Generic,
    Iterable,
    List,
    Mapping,
    Optional,
    Sequence,
    Set,
    Tuple,
    Type,
    TypeVar,
    Union,
    cast,
)

from __static__ import chkdict  # pyre-ignore[21]: unknown module

from . import symbols, opcode38static
from .consts import SC_LOCAL
from .opcodebase import Opcode
from .optimizer import AstOptimizer
from .pyassem import Block, PyFlowGraph, PyFlowGraph38
from .pycodegen import (
    AugAttribute,
    AugName,
    AugSubscript,
    CodeGenerator,
    Python38CodeGenerator,
    compile,
    wrap_aug,
    FOR_LOOP,
)
from .symbols import Scope, SymbolVisitor
from .visitor import ASTVisitor


try:
    from xxclassloader import spamobj  # pyre-ignore[21]: unknown module
except ImportError:
    spamobj = None


def exec_static(
    source: str,
    locals: Dict[str, object],
    globals: Dict[str, object],
    modname: str = "<module>",
) -> None:
    code = compile(
        source, "<module>", "exec", compiler=StaticCodeGenerator, modname=modname
    )
    exec(code, locals, globals)  # noqa: P204


CBOOL_TYPE: CIntType
INT8_TYPE: CIntType
INT16_TYPE: CIntType
INT32_TYPE: CIntType
INT64_TYPE: CIntType
INT64_VALUE: CIntInstance
SIGNED_CINT_TYPES: Sequence[CIntType]
INT_TYPE: NumClass
INT_EXACT_TYPE: NumClass
FLOAT_TYPE: NumClass
COMPLEX_TYPE: NumClass
BOOL_TYPE: Class
ARRAY_TYPE: Class
DICT_TYPE: Class
LIST_TYPE: Class
TUPLE_TYPE: Class
SET_TYPE: Class

OBJECT_TYPE: Class
OBJECT: Value

DYNAMIC_TYPE: DynamicClass
DYNAMIC: DynamicInstance
FUNCTION_TYPE: Class
METHOD_TYPE: Class
MEMBER_TYPE: Class
NONE_TYPE: Class
TYPE_TYPE: Class
ARG_TYPE: Class
SLICE_TYPE: Class

CHAR_TYPE: CIntType
DOUBLE_TYPE: DoubleType

RESOLVED_OBJECT_TYPE: ResolvedTypeRef

# Prefix for temporary var names. It's illegal in normal
# Python, so there's no chance it will ever clash with a
# user defined name.
_TMP_VAR_PREFIX = "_pystatic_.0._tmp__"


class TypeRef:
    """Stores unresolved typed references, capturing the referring module
    as well as the annotation"""

    def __init__(self, module: ModuleTable, ref: ast.expr) -> None:
        self.module = module
        self.ref = ref

    @property
    def resolved(self) -> Class:
        res = self.module.resolve_annotation(self.ref)
        if res is None:
            return DYNAMIC_TYPE
        return res

    def __repr__(self) -> str:
        return f"TypeRef({self.module.name}, {ast.dump(self.ref)})"


class ResolvedTypeRef(TypeRef):
    def __init__(self, type: Class) -> None:
        self._resolved = type

    @property
    def resolved(self) -> Class:
        return self._resolved

    def __repr__(self) -> str:
        return f"ResolvedTypeRef({self.resolved})"


class GenericTypeParamRef(TypeRef):
    def __init__(self, index: int) -> None:
        self.index = index

    @property
    def resolved(self) -> GenericParameter:
        return GenericParameter("T" + str(self.index), self.index)

    def __repr__(self) -> str:
        return f"GenericTypeParamRef({self.index})"


# Pyre doesn't support recursive generics, so we can't represent the recursively
# nested tuples that make up a type_descr. Fortunately we don't need to, since
# we don't parse them in Python, we just generate them and emit them as
# constants. So just call them `Tuple[object, ...]`
TypeDescr = Tuple[object, ...]


class TypeName:
    def __init__(self, module: str, name: str) -> None:
        self.module = module
        self.name = name

    @property
    def type_descr(self) -> TypeDescr:
        """The metadata emitted into the const pool to describe a type.

        For normal types this is just the fully qualified type name as a tuple
        ('mypackage', 'mymod', 'C'). For optional types we have an extra '?'
        element appended. For generic types we append a tuple of the generic
        args' type_descrs.
        """
        return (self.module, self.name)

    @property
    def friendly_name(self) -> str:
        if self.module:
            return f"{self.module}.{self.name}"
        return self.name


class GenericTypeName(TypeName):
    def __init__(self, module: str, name: str, args: Tuple[Class, ...]) -> None:
        super().__init__(module, name)
        self.args = args

    @property
    def type_descr(self) -> TypeDescr:
        gen_args: List[TypeDescr] = []
        for arg in self.args:
            gen_args.append(arg.type_descr)
        return (self.module, self.name, tuple(gen_args))

    @property
    def friendly_name(self) -> str:
        args = ", ".join(arg.type_name.friendly_name for arg in self.args)
        return f"{self.module}.{self.name}[{args}]"


GenericTypeIndex = Tuple["Class", ...]
GenericTypesDict = Dict["Class", Dict[GenericTypeIndex, "Class"]]


class SymbolTable:
    def __init__(self) -> None:
        self.modules: Dict[str, ModuleTable] = {}
        builtins_children = {
            "object": OBJECT_TYPE,
            "type": TYPE_TYPE,
            "None": NONE_TYPE.instance,
            "int": INT_EXACT_TYPE,
            "str": STR_TYPE,
            "bytes": BYTES_TYPE,
            "bool": BOOL_TYPE,
            "float": FLOAT_TYPE,
            "len": LenFunction(FUNCTION_TYPE),
            "min": ExtremumFunction(FUNCTION_TYPE, is_min=True),
            "max": ExtremumFunction(FUNCTION_TYPE, is_min=False),
            "list": LIST_EXACT_TYPE,
            "tuple": TUPLE_EXACT_TYPE,
            "set": SET_EXACT_TYPE,
            "Exception": EXCEPTION_TYPE,
            "BaseException": BASE_EXCEPTION_TYPE,
            "isinstance": IsInstanceFunction(),
            "issubclass": IsSubclassFunction(),
            "staticmethod": STATIC_METHOD_TYPE,
        }
        strict_builtins = StrictBuiltins(builtins_children)
        typing_children = {
            # TODO: Need typed members for dict
            "Dict": CHECKED_DICT_TYPE,
            "List": LIST_TYPE,
            # TODO we should have a CHECKED_MAPPING_TYPE that disallows mutation
            "Mapping": CHECKED_DICT_TYPE,
            "NamedTuple": NAMED_TUPLE_TYPE,
            "Optional": OPTIONAL_TYPE,
            "Union": UNION_TYPE,
            "TYPE_CHECKING": BOOL_TYPE.instance,
        }

        builtins_children["<builtins>"] = strict_builtins
        builtins_children["<fixed-modules>"] = StrictBuiltins(
            {"typing": StrictBuiltins(typing_children)}
        )

        self.builtins = self.modules["builtins"] = ModuleTable(
            "builtins",
            self,
            builtins_children,
        )
        self.typing = self.modules["typing"] = ModuleTable(
            "typing", self, typing_children
        )
        self.statics = self.modules["__static__"] = ModuleTable(
            "__static__",
            self,
            {
                "Array": ARRAY_EXACT_TYPE,
                "box": BoxFunction(FUNCTION_TYPE),
                "cast": CastFunction(FUNCTION_TYPE),
                "size_t": INT64_TYPE,
                "cbool": CBOOL_TYPE,
                "int8": INT8_TYPE,
                "int16": INT16_TYPE,
                "int32": INT32_TYPE,
                "int64": INT64_TYPE,
                "uint8": UINT8_TYPE,
                "uint16": UINT16_TYPE,
                "uint32": UINT32_TYPE,
                "uint64": UINT64_TYPE,
                "char": CHAR_TYPE,
                "double": DOUBLE_TYPE,
                "unbox": UnboxFunction(FUNCTION_TYPE),
                "nonchecked_dicts": BOOL_TYPE.instance,
                "pydict": DICT_TYPE,
                "PyDict": DICT_TYPE,
                # should have a MAPPING_TYPE that disallows mutation
                "PyMapping": DICT_TYPE,
            },
        )

        if SPAM_OBJ is not None:
            self.modules["xxclassloader"] = ModuleTable(
                "xxclassloader",
                self,
                {"spamobj": SPAM_OBJ},
            )

        # We need to clone the dictionaries for each type so that as we populate
        # generic instantations that we don't store them in the global dict for
        # built-in types
        self.generic_types: GenericTypesDict = {
            k: dict(v) for k, v in BUILTIN_GENERICS.items()
        }

    def __getitem__(self, name: str) -> ModuleTable:
        return self.modules[name]

    def __setitem__(self, name: str, value: ModuleTable) -> None:
        self.modules[name] = value

    def add_module(self, name: str, filename: str, tree: AST) -> None:
        decl_visit = DeclarationVisitor(name, filename, self)
        decl_visit.visit(tree)

    def compile(
        self, name: str, filename: str, tree: AST, optimize: int = 0
    ) -> CodeType:
        self[name].finish_bind()

        tree = AstOptimizer(optimize=optimize > 0).visit(tree)

        # Analyze variable scopes
        s = SymbolVisitor()
        s.visit(tree)

        # Analyze the types of objects within local scopes
        type_binder = TypeBinder(s, filename, self, name)
        type_binder.visit(tree)

        # Compile the code w/ the static compiler
        graph = StaticCodeGenerator.flow_graph(
            name, filename, s.scopes[tree], peephole_enabled=True
        )
        graph.setFlag(StaticCodeGenerator.consts.CO_STATICALLY_COMPILED)

        code_gen = StaticCodeGenerator(None, tree, s, graph, self, name, optimize)
        code_gen.visit(tree)

        return code_gen.getCode()


class ModuleTable:
    def __init__(
        self,
        name: str,
        symtable: SymbolTable,
        members: Optional[Dict[str, Value]] = None,
    ) -> None:
        self.name = name
        self.children: Dict[str, Value] = members or {}
        self.symtable = symtable
        self.types: Dict[Union[AST, AugName], Value] = {}
        self.nonchecked_dicts = False
        self.tinyframe = False
        self.noframe = False
        self.decls: List[Tuple[AST, Optional[Value]]] = []

    def finish_bind(self) -> None:
        for node, value in self.decls:
            if value is not None:
                value.finish_bind()
            if isinstance(node, ast.Import):
                pass
            elif isinstance(node, ast.ImportFrom):
                mod_name = node.module
                if node.level or not mod_name:
                    raise NotImplementedError("relative imports aren't supported")

                mod = self.symtable.modules.get(mod_name)
                if mod is not None:
                    for name in node.names:
                        val = mod.children.get(name.name)
                        if val is not None:
                            self.children[name.asname or name.name] = val

        # We don't need these anymore...
        self.decls.clear()

    def resolve_annotation(
        self, node: ast.AST, type_ctx: Optional[Class] = None
    ) -> Optional[Class]:
        klass = self._resolve_annotation(node, type_ctx)
        # Even if we know that e.g. `builtins.str` is the exact `str` type and
        # not a subclass, and it's useful to track that knowledge, when we
        # annotate `x: str` that annotation should not exclude subclasses.
        return inexact(klass) if klass else None

    def _resolve_annotation(
        self, node: ast.AST, type_ctx: Optional[Class] = None
    ) -> Optional[Class]:
        if isinstance(node, ast.Name):
            res = self.resolve_name(node.id)
            if isinstance(res, Class):
                return res
        elif isinstance(node, Subscript):
            slice = node.slice
            if isinstance(slice, Index):
                val = self.resolve_annotation(node.value)
                if val is not None:
                    # pyre-fixme[16]: `Index` has no attribute `value`.
                    value = slice.value
                    if isinstance(value, ast.Tuple):
                        anns = []
                        for elt in value.elts:
                            ann = self.resolve_annotation(elt)
                            if ann is None:
                                return None
                            anns.append(ann)
                        values = tuple(anns)
                        return val.make_generic_type(
                            values, self.symtable.generic_types
                        )
                    else:
                        index = self.resolve_annotation(value)
                        if index is not None:
                            return val.make_generic_type(
                                (index,), self.symtable.generic_types
                            )
        elif isinstance(node, ast.Str):
            # pyre-fixme[16]: `AST` has no attribute `body`.
            return self.resolve_annotation(ast.parse(node.s, "", "eval").body)
        elif isinstance(node, ast.Constant):
            sval = node.value
            if sval is None:
                return NONE_TYPE
            elif isinstance(sval, str):
                return self.resolve_annotation(ast.parse(node.value, "", "eval").body)
        elif isinstance(node, NameConstant):
            if node.value in (True, False):
                return BOOL_TYPE
            elif node.value is None:
                return NONE_TYPE
            # should never happen
            raise TypedSyntaxError("invalid annotation")
        elif isinstance(node, ast.BinOp) and isinstance(node.op, ast.BitOr):
            ltype = self.resolve_annotation(node.left)
            rtype = self.resolve_annotation(node.right)
            if ltype is None or rtype is None:
                return None
            return UNION_TYPE.make_generic_type(
                (ltype, rtype), self.symtable.generic_types
            )

    def resolve_name(self, name: str) -> Optional[Value]:
        return self.children.get(name) or self.symtable.builtins.children.get(name)


TClass = TypeVar("TClass", bound="Class", covariant=True)
TClassInv = TypeVar("TClassInv", bound="Class")


class Value:
    """base class for all values tracked at compile time."""

    def __init__(self, klass: Class) -> None:
        """name: the name of the value, for instances this is used solely for
        debug/reporting purposes.  In Class subclasses this will be the
        qualified name (e.g. module.Foo).
        klass: the Class of this object"""
        self.klass = klass

    @property
    def name(self) -> str:
        return type(self).__name__

    def finish_bind(self) -> None:
        pass

    def make_generic_type(
        self, index: GenericTypeIndex, generic_types: GenericTypesDict
    ) -> Optional[Class]:
        pass

    def get_iter_type(self, node: ast.expr, visitor: TypeBinder) -> Value:
        """returns the type that is produced when iterating over this value"""
        raise visitor.syntax_error(f"cannot iterate over {self.klass.name}", node)

    def as_oparg(self) -> int:
        raise TypeError(f"{self.klass.name} not valid here")

    def bind_attr(
        self, node: ast.Attribute, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        visitor.visit(node.value)
        raise visitor.syntax_error(
            f"cannot load attribute from {self.klass.name}", node
        )

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        raise visitor.syntax_error(f"cannot call {self.klass.name}", node)

    def check_args_for_primitives(self, node: ast.Call, visitor: TypeBinder) -> None:
        for arg in node.args:
            if isinstance(visitor.get_type(arg), CInstance):
                raise visitor.syntax_error("Call argument cannot be a primitive", arg)
        for arg in node.keywords:
            if isinstance(visitor.get_type(arg.value), CInstance):
                raise visitor.syntax_error(
                    "Call argument cannot be a primitive", arg.value
                )

    def bind_descr_get(
        self,
        node: ast.Attribute,
        inst: Optional[Object[TClassInv]],
        ctx: TClassInv,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> None:
        raise visitor.syntax_error(f"cannot get descriptor {self.klass.name}", node)

    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        raise visitor.syntax_error(f"cannot index {self.klass.name}", node)

    def emit_subscr(
        self, node: ast.Subscript, aug_flag: bool, code_gen: Static38CodeGenerator
    ) -> None:
        code_gen.defaultVisit(node, aug_flag)

    def emit_store_subscr(
        self, node: ast.Subscript, code_gen: Static38CodeGenerator
    ) -> None:
        code_gen.emit("ROT_THREE")
        code_gen.emit("STORE_SUBSCR")

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        code_gen.defaultVisit(node)

    def emit_attr(
        self, node: Union[ast.Attribute, AugAttribute], code_gen: Static38CodeGenerator
    ) -> None:
        if isinstance(node.ctx, ast.Store):
            code_gen.emit("STORE_ATTR", code_gen.mangle(node.attr))
        elif isinstance(node.ctx, ast.Del):
            code_gen.emit("DELETE_ATTR", code_gen.mangle(node.attr))
        else:
            code_gen.emit("LOAD_ATTR", code_gen.mangle(node.attr))

    def bind_compare(
        self,
        node: ast.Compare,
        left: expr,
        op: cmpop,
        right: expr,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> bool:
        raise visitor.syntax_error(f"cannot compare with {self.klass.name}", node)

    def bind_reverse_compare(
        self,
        node: ast.Compare,
        left: expr,
        op: cmpop,
        right: expr,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> bool:
        raise visitor.syntax_error(f"cannot reverse  with {self.klass.name}", node)

    def emit_compare(self, op: cmpop, code_gen: Static38CodeGenerator) -> None:
        code_gen.defaultEmitCompare(op)

    def bind_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        raise visitor.syntax_error(f"cannot bin op with {self.klass.name}", node)

    def bind_reverse_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        raise visitor.syntax_error(
            f"cannot reverse bin op with {self.klass.name}", node
        )

    def bind_unaryop(
        self, node: ast.UnaryOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        raise visitor.syntax_error(
            f"cannot reverse unary op with {self.klass.name}", node
        )

    def emit_binop(self, node: ast.BinOp, code_gen: Static38CodeGenerator) -> None:
        code_gen.defaultVisit(node)

    def emit_forloop(self, node: ast.For, code_gen: Static38CodeGenerator) -> None:
        start = code_gen.newBlock("default_forloop_start")
        anchor = code_gen.newBlock("default_forloop_anchor")
        after = code_gen.newBlock("default_forloop_after")

        code_gen.set_lineno(node)
        code_gen.push_loop(FOR_LOOP, start, after)
        code_gen.visit(node.iter)
        code_gen.emit("GET_ITER")

        code_gen.nextBlock(start)
        code_gen.emit("FOR_ITER", anchor)
        code_gen.visit(node.target)
        code_gen.visit(node.body)
        code_gen.emit("JUMP_ABSOLUTE", start)
        code_gen.nextBlock(anchor)
        code_gen.pop_loop()

        if node.orelse:
            code_gen.visit(node.orelse)
        code_gen.nextBlock(after)

    def emit_unaryop(self, node: ast.UnaryOp, code_gen: Static38CodeGenerator) -> None:
        code_gen.defaultVisit(node)

    def emit_augassign(
        self, node: ast.AugAssign, code_gen: Static38CodeGenerator
    ) -> None:
        code_gen.defaultVisit(node)

    def emit_augname(
        self, node: AugName, code_gen: Static38CodeGenerator, mode: str
    ) -> None:
        code_gen.defaultVisit(node, mode)

    def bind_constant(self, node: ast.Constant, visitor: TypeBinder) -> None:
        raise visitor.syntax_error(f"cannot constant with {self.klass.name}", node)

    def emit_constant(
        self, node: ast.Constant, code_gen: Static38CodeGenerator
    ) -> None:
        return code_gen.defaultVisit(node)

    def bind_num(self, node: ast.Num, visitor: TypeBinder) -> None:
        raise visitor.syntax_error(f"cannot num with {self.klass.name}", node)

    def emit_num(self, node: ast.Num, code_gen: Static38CodeGenerator) -> None:
        return code_gen.defaultVisit(node)

    def emit_name(self, node: ast.Name, code_gen: Static38CodeGenerator) -> None:
        return code_gen.defaultVisit(node)

    def emit_jumpif(
        self, test: AST, next: Block, is_if_true: bool, code_gen: Static38CodeGenerator
    ) -> None:
        Python38CodeGenerator.compileJumpIf(code_gen, test, next, is_if_true)

    def emit_jumpif_pop(
        self, test: AST, next: Block, is_if_true: bool, code_gen: Static38CodeGenerator
    ) -> None:
        Python38CodeGenerator.compileJumpIfPop(code_gen, test, next, is_if_true)

    def emit_box(self, node: expr, code_gen: Static38CodeGenerator) -> None:
        raise RuntimeError(f"Unsupported box type: {code_gen.get_type(node)}")

    def emit_unbox(self, node: expr, code_gen: Static38CodeGenerator) -> None:
        raise RuntimeError("Unsupported unbox type")

    def emit_len(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        return self.emit_call(node, code_gen)

    def make_generic(
        self, name: GenericTypeName, generic_types: GenericTypesDict
    ) -> Value:
        return self


class Object(Value, Generic[TClass]):
    """Represents an instance of a type at compile time"""

    klass: TClass

    @property
    def name(self) -> str:
        return self.klass.name + " instance"

    def as_oparg(self) -> int:
        return TYPED_OBJECT

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        visitor.set_type(node, DYNAMIC)
        for arg in node.args:
            visitor.visit(arg)

        for arg in node.keywords:
            visitor.visit(arg.value)
        self.check_args_for_primitives(node, visitor)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        code_gen.defaultVisit(node)

    def bind_attr(
        self, node: ast.Attribute, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        for base in self.klass.mro:
            member = base.members.get(node.attr)
            if member is not None:
                member.bind_descr_get(node, self, self.klass, visitor, type_ctx)
                return

        visitor.visit(node.value)
        if node.attr == "__class__":
            visitor.set_type(node, self.klass)
        else:
            visitor.set_type(node, DYNAMIC)

    def emit_attr(
        self, node: Union[ast.Attribute, AugAttribute], code_gen: Static38CodeGenerator
    ) -> None:
        for base in self.klass.mro:
            member = base.members.get(node.attr)
            if member is not None and isinstance(member, Slot):
                type_descr = member.decl_type.type_descr
                type_descr += (member.slot_name,)
                if isinstance(node.ctx, ast.Store):
                    code_gen.emit("STORE_FIELD", type_descr)
                elif isinstance(node.ctx, ast.Del):
                    code_gen.emit("DELETE_ATTR", node.attr)
                else:
                    code_gen.emit("LOAD_FIELD", type_descr)
                return

        super().emit_attr(node, code_gen)

    def bind_descr_get(
        self,
        node: ast.Attribute,
        inst: Optional[Object[TClass]],
        ctx: Class,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> None:
        visitor.set_type(node, DYNAMIC)

    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        visitor.check_can_assign_from(DYNAMIC_TYPE, type.klass, node)
        visitor.set_type(node, DYNAMIC)

    def bind_compare(
        self,
        node: ast.Compare,
        left: expr,
        op: cmpop,
        right: expr,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> bool:
        visitor.set_type(op, DYNAMIC)
        visitor.set_type(node, DYNAMIC)
        return False

    def bind_reverse_compare(
        self,
        node: ast.Compare,
        left: expr,
        op: cmpop,
        right: expr,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> bool:
        visitor.set_type(op, DYNAMIC)
        visitor.set_type(node, DYNAMIC)
        return False

    def bind_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        return False

    def bind_reverse_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        # we'll set the type in case we're the only one called
        visitor.set_type(node, DYNAMIC)
        return False

    def bind_unaryop(
        self, node: ast.UnaryOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        if isinstance(node.op, ast.Not):
            visitor.set_type(node, BOOL_TYPE.instance)
        else:
            visitor.set_type(node, DYNAMIC)

    def bind_constant(self, node: ast.Constant, visitor: TypeBinder) -> None:
        node_type = CONSTANT_TYPES[type(node.value)]
        visitor.set_type(node, node_type)
        visitor.check_can_assign_from(self.klass, node_type.klass, node)

    def bind_num(self, node: ast.Num, visitor: TypeBinder) -> None:
        node_type = CONSTANT_TYPES[type(node.n)]
        visitor.set_type(node, node_type)
        visitor.check_can_assign_from(self.klass, node_type.klass, node)

    def get_iter_type(self, node: ast.expr, visitor: TypeBinder) -> Value:
        """returns the type that is produced when iterating over this value"""
        return DYNAMIC

    def get_fast_len_type(self) -> int:
        raise NotImplementedError()

    def __repr__(self) -> str:
        return f"<{self.klass.name} instance>"


class Class(Object["Class"]):
    """Represents a type object at compile time"""

    def __init__(
        self,
        type_name: TypeName,
        bases: Optional[List[TypeRef]] = None,
        instance: Optional[Value] = None,
        klass: Optional[Class] = None,
        members: Optional[Dict[str, Value]] = None,
        is_exact: bool = False,
        pytype: Optional[Type[object]] = None,
    ) -> None:
        super().__init__(klass or TYPE_TYPE)
        assert isinstance(bases, (type(None), list))
        self.type_name = type_name
        self.instance: Value = instance or Object(self)
        self._bases: List[TypeRef] = bases or []
        self._mro: Optional[List[Class]] = None
        self._mro_type_descrs: Optional[Set[TypeDescr]] = None
        self.members: Dict[str, Value] = members or {}
        self.is_exact = is_exact
        if pytype:
            self.members.update(make_type_dict(self, pytype))
        # store attempted slot redefinitions during type declaration, for resolution in finish_bind
        self._slot_redefs: Dict[str, List[TypeRef]] = {}

    @property
    def name(self) -> str:
        name = self.type_name.friendly_name
        if self.is_exact:
            name = f"Exact[{name}]"
        return name

    @property
    def is_generic_parameter(self) -> bool:
        """Returns True if this Class represents a generic parameter"""
        return False

    @property
    def contains_generic_parameters(self) -> bool:
        """Returns True if this class contains any generic parameters"""
        return False

    @property
    def is_generic_type(self) -> bool:
        """Returns True if this class is a generic type"""
        return False

    @property
    def is_generic_type_definition(self) -> bool:
        """Returns True if this class is a generic type definition.
        It'll be a generic type which still has unbound generic type
        parameters"""
        return False

    @property
    def generic_type_def(self) -> Optional[Class]:
        """Gets the generic type definition that defined this class"""
        return None

    def make_generic_type(
        self,
        index: Tuple[Class, ...],
        generic_types: GenericTypesDict,
    ) -> Optional[Class]:
        """Binds the generic type parameters to a generic type definition"""
        return None

    def bind_attr(
        self, node: ast.Attribute, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        for base in self.mro:
            member = base.members.get(node.attr)
            if member is not None:
                member.bind_descr_get(node, None, self, visitor, type_ctx)
                return

        super().bind_attr(node, visitor, type_ctx)

    def bind_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        if isinstance(node.op, ast.BitOr):
            rtype = visitor.get_type(node.right)
            if rtype is NONE_TYPE.instance:
                rtype = NONE_TYPE
            if rtype is DYNAMIC:
                rtype = DYNAMIC_TYPE
            if not isinstance(rtype, Class):
                raise visitor.syntax_error(
                    f"unsupported operand type(s) for |: {self.klass.name} and {rtype.klass.name}",
                    node,
                )
            union = UNION_TYPE.make_generic_type(
                (self, rtype), visitor.symtable.generic_types
            )
            visitor.set_type(node, union)
            return True

        return super().bind_binop(node, visitor, type_ctx)

    @property
    def can_be_narrowed(self) -> bool:
        return True

    @property
    def type_descr(self) -> TypeDescr:
        return self.type_name.type_descr

    @property
    def bases(self) -> Sequence[Class]:
        return [base.resolved for base in self._bases]

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        visitor.set_type(node, self.instance)
        for arg in node.args:
            visitor.visit(arg)
        for arg in node.keywords:
            visitor.visit(arg.value)
        self.check_args_for_primitives(node, visitor)
        return NO_EFFECT

    def can_assign_from(self, src: Class) -> bool:
        """checks to see if the src value can be assigned to this value.  Currently
        you can assign a derived type to a base type.  You cannot assign a primitive
        type to an object type.

        At some point we may also support some form of interfaces via protocols if we
        implement a more efficient form of interface dispatch than doing the dictionary
        lookup for the member."""
        return src is self or (
            not self.is_exact and not isinstance(src, CType) and self.issubclass(src)
        )

    def __repr__(self) -> str:
        return f"<{self.name} class>"

    def isinstance(self, src: Value) -> bool:
        return self.issubclass(src.klass)

    def issubclass(self, src: Class) -> bool:
        return self.type_descr in src.mro_type_descrs

    def finish_bind(self) -> None:
        for name, new_type_refs in self._slot_redefs.items():
            cur_slot = self.members[name]
            assert isinstance(cur_slot, Slot)
            cur_type = cur_slot.type_ref.resolved
            if any(tr.resolved != cur_type for tr in new_type_refs):
                raise TypedSyntaxError(
                    f"conflicting type definitions for slot {name} in class {self.name}"
                )
        self._slot_redefs = {}

        for base_ in self._bases:
            base = base_.resolved

            if base is NAMED_TUPLE_TYPE:
                # This is a named tuple, which means the fields are actually elements
                # of the tuple, so we can't do any advanced binding against it.
                self.members.clear()
                return

        inherited = set()
        for name, my_value in self.members.items():
            for base in self.mro[1:]:
                value = base.members.get(name)
                if value is not None and type(my_value) != type(value):
                    # TODO: There's more checking we should be doing to ensure
                    # this is a compatible override
                    raise TypedSyntaxError(
                        f"class cannot hide inherited member: {value!r}"
                    )
                elif isinstance(value, Slot):
                    inherited.add(name)

        for name in inherited:
            assert type(self.members[name]) is Slot
            del self.members[name]

    def define_slot(self, name: str, type_ref: Optional[TypeRef] = None) -> None:
        existing = self.members.get(name)
        if existing is None:
            self.members[name] = Slot(
                type_ref or ResolvedTypeRef(DYNAMIC_TYPE), name, self
            )
        elif isinstance(existing, Slot):
            if type_ref is not None:
                self._slot_redefs.setdefault(name, []).append(type_ref)
        else:
            raise TypedSyntaxError(
                f"slot conflicts with other member {name} in class {self.name}"
            )

    def define_function(
        self,
        name: str,
        func: Union[FunctionDef, AsyncFunctionDef],
        visitor: DeclarationVisitor,
    ) -> None:
        if name in self.members:
            raise TypedSyntaxError(
                f"function conflicts with other member {name} in class {self.name}"
            )

        function = Function(
            name,
            self.type_name,
            func,
            visitor.type_ref(func.returns),
            self.type_name.type_descr + (name,),
        )
        process_function_args(function, func.args, visitor, self_type=self)

        if not func.decorator_list:
            self.members[name] = function
        elif len(func.decorator_list) == 1:
            dec = visitor.module.resolve_annotation(func.decorator_list[0])
            if dec is STATIC_METHOD_TYPE:
                method = StaticMethod(self.type_name, func, function)
                self.members[name] = method

    @property
    def mro(self) -> Sequence[Class]:
        mro = self._mro
        if mro is None:
            if not all(self.bases):
                # TODO: We can't compile w/ unknown bases
                mro = []
            else:
                mro = _mro(self)
            self._mro = mro

        return mro

    @property
    def mro_type_descrs(self) -> Collection[TypeDescr]:
        cached = self._mro_type_descrs
        if cached is None:
            self._mro_type_descrs = cached = {b.type_descr for b in self.mro}
        return cached

    def bind_generics(
        self,
        name: GenericTypeName,
        generic_types: Dict[Class, Dict[Tuple[Class, ...], Class]],
    ) -> Class:
        return self


class GenericClass(Class):
    type_name: GenericTypeName
    is_variadic = False

    def __init__(
        self,
        name: GenericTypeName,
        bases: Optional[List[TypeRef]] = None,
        instance: Optional[Object[Class]] = None,
        klass: Optional[Class] = None,
        members: Optional[Dict[str, Value]] = None,
        type_def: Optional[GenericClass] = None,
        is_exact: bool = False,
        pytype: Optional[Type[object]] = None,
    ) -> None:
        super().__init__(name, bases, instance, klass, members, is_exact, pytype)
        self.gen_name = name
        self.type_def = type_def

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        if self.contains_generic_parameters:
            raise visitor.syntax_error(
                f"cannot create instances of a generic type {self.name}", node
            )
        return super().bind_call(node, visitor, type_ctx)

    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        slice = node.slice
        if not isinstance(slice, ast.Index):
            raise visitor.syntax_error("can't slice generic types", node)

        visitor.visit(node.slice)
        # pyre-fixme[16]: `Index` has no attribute `value`.
        val = slice.value

        if isinstance(val, ast.Tuple):
            multiple: List[Class] = []
            for elt in val.elts:
                klass = visitor.cur_mod.resolve_annotation(elt)
                if klass is None:
                    visitor.set_type(node, DYNAMIC)
                    return
                multiple.append(klass)

            index = tuple(multiple)
            if (not self.is_variadic) and len(val.elts) != len(self.gen_name.args):
                raise visitor.syntax_error(
                    "incorrect number of generic arguments", node
                )
        else:
            if (not self.is_variadic) and len(self.gen_name.args) != 1:
                raise visitor.syntax_error(
                    "incorrect number of generic arguments", node
                )

            single = visitor.cur_mod.resolve_annotation(val)
            if single is None:
                visitor.set_type(node, DYNAMIC)
                return

            index = (single,)

        klass = self.make_generic_type(index, visitor.symtable.generic_types)
        visitor.set_type(node, klass)

    @property
    def type_args(self) -> Sequence[Class]:
        return self.type_name.args

    @property
    def contains_generic_parameters(self) -> bool:
        for arg in self.gen_name.args:
            if arg.is_generic_parameter:
                return True
        return False

    @property
    def is_generic_type(self) -> bool:
        return True

    @property
    def is_generic_type_definition(self) -> bool:
        return self.type_def is None

    @property
    def generic_type_def(self) -> Optional[Class]:
        """Gets the generic type definition that defined this class"""
        return self.type_def

    def make_generic_type(
        self,
        index: Tuple[Class, ...],
        generic_types: GenericTypesDict,
    ) -> Class:
        instantiations = generic_types.get(self)
        if instantiations is not None:
            instance = instantiations.get(index)
            if instance is not None:
                return instance
        else:
            generic_types[self] = instantiations = {}

        type_args = index
        type_name = GenericTypeName(
            self.type_name.module, self.type_name.name, type_args
        )
        generic_bases: List[Optional[Class]] = [
            (
                base.make_generic_type(index, generic_types)
                if base.contains_generic_parameters
                else base
            )
            for base in self.bases
        ]
        bases: List[TypeRef] = [
            ResolvedTypeRef(base) for base in generic_bases if base is not None
        ]
        InstanceType = type(self.instance)
        instance: Object[Class] = InstanceType.__new__(InstanceType)
        instance.__dict__.update(self.instance.__dict__)
        concrete = type(self)(
            type_name,
            bases,
            instance,
            self.klass,
            {},
            is_exact=self.is_exact,
            type_def=self,
        )

        instance.klass = concrete

        instantiations[index] = concrete
        concrete.members.update(
            {
                k: v.make_generic(type_name, generic_types)
                for k, v in self.members.items()
            }
        )
        return concrete


class GenericParameter(Class):
    def __init__(self, name: str, index: int) -> None:
        super().__init__(TypeName("", name), [], None, None, {})
        self.index = index

    @property
    def name(self) -> str:
        return self.type_name.name

    @property
    def is_generic_parameter(self) -> bool:
        return True

    def bind_generics(
        self,
        name: GenericTypeName,
        generic_types: Dict[Class, Dict[Tuple[Class, ...], Class]],
    ) -> Class:
        return name.args[self.index]


class CType(Class):
    """base class for primitives that aren't heap allocated"""

    def __init__(
        self,
        type_name: TypeName,
        bases: Optional[List[TypeRef]] = None,
        instance: Optional[CInstance[Class]] = None,
        klass: Optional[Class] = None,
        members: Optional[Dict[str, Value]] = None,
        is_exact: bool = True,
        pytype: Optional[Type[object]] = None,
    ) -> None:
        super().__init__(type_name, bases, instance, klass, members, is_exact, pytype)

    @property
    def name(self) -> str:
        return self.type_name.friendly_name

    @property
    def can_be_narrowed(self) -> bool:
        return False

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        """
        Almost the same as the base class method, but this allows args to be primitives
        so we can write something like (explicit conversions):
        x = int32(int8(5))
        """
        visitor.set_type(node, self.instance)
        for arg in node.args:
            visitor.visit(arg)
        return NO_EFFECT


class DynamicClass(Class):
    instance: DynamicInstance

    def __init__(self) -> None:
        super().__init__(
            TypeName("builtins", "object"),
            bases=[ResolvedTypeRef(OBJECT_TYPE)],
            instance=DynamicInstance(self),
        )

    @property
    def name(self) -> str:
        return "dynamic"

    @property
    def type_descr(self) -> TypeDescr:
        # Any references to dynamic at runtime are object
        return OBJECT_TYPE.type_descr

    def can_assign_from(self, src: Class) -> bool:
        # No automatic boxing to the dynamic type
        return not isinstance(src, CType)


class DynamicInstance(Object[DynamicClass]):
    def __init__(self, klass: DynamicClass) -> None:
        super().__init__(klass)

    @property
    def name(self) -> str:
        return "dynamic"

    def bind_constant(self, node: ast.Constant, visitor: TypeBinder) -> None:
        n = node.value
        if isinstance(n, bool):
            visitor.set_type(node, BOOL_TYPE.instance)
        elif isinstance(n, int):
            visitor.set_type(node, INT_EXACT_TYPE.instance)
        elif isinstance(n, float):
            visitor.set_type(node, FLOAT_TYPE.instance)
        elif isinstance(n, complex):
            visitor.set_type(node, COMPLEX_TYPE.instance)
        elif isinstance(n, str):
            visitor.set_type(node, STR_TYPE.instance)
        elif isinstance(n, bytes):
            visitor.set_type(node, BYTES_TYPE.instance)
        elif n is None:
            visitor.set_type(node, NONE_TYPE.instance)
        else:
            # could be a tuple
            visitor.set_type(node, DYNAMIC)

    def bind_num(self, node: ast.Num, visitor: TypeBinder) -> None:
        n = node.n
        if isinstance(n, int):
            visitor.set_type(node, INT_EXACT_TYPE.instance)
        elif isinstance(n, float):
            visitor.set_type(node, FLOAT_TYPE.instance)
        elif isinstance(n, complex):
            visitor.set_type(node, COMPLEX_TYPE.instance)


class NoneType(Class):
    def __init__(self) -> None:
        super().__init__(
            TypeName("builtins", "None"),
            [RESOLVED_OBJECT_TYPE],
            NoneInstance(self),
            is_exact=True,
        )

    @property
    def name(self) -> str:
        return "builtins.None"


UNARY_SYMBOLS: Mapping[Type[ast.unaryop], str] = {
    ast.UAdd: "+",
    ast.USub: "-",
    ast.Invert: "~",
}


class NoneInstance(Object[NoneType]):
    def bind_attr(
        self, node: ast.Attribute, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        raise visitor.syntax_error(
            f"'NoneType' object has no attribute '{node.attr}'", node
        )

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        raise visitor.syntax_error("'NoneType' object is not callable", node)

    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        raise visitor.syntax_error("'NoneType' object is not subscriptable", node)

    def bind_unaryop(
        self, node: ast.UnaryOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        raise visitor.syntax_error(
            f"bad operand type for unary {UNARY_SYMBOLS[type(node.op)]}: 'NoneType'",
            node,
        )

    def bind_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        # support `None | int` as a union type; None is special in that it is
        # not a type but can be used synonymously with NoneType for typing.
        if isinstance(node.op, ast.BitOr):
            return self.klass.bind_binop(node, visitor, type_ctx)
        else:
            return super().bind_binop(node, visitor, type_ctx)


# https://www.python.org/download/releases/2.3/mro/
def _merge(seqs: Iterable[List[Class]]) -> List[Class]:
    res = []
    i = 0
    while True:
        nonemptyseqs = [seq for seq in seqs if seq]
        if not nonemptyseqs:
            return res
        i += 1
        cand = None
        for seq in nonemptyseqs:  # find merge candidates among seq heads
            cand = seq[0]
            nothead = [s for s in nonemptyseqs if cand in s[1:]]
            if nothead:
                cand = None  # reject candidate
            else:
                break
        if not cand:
            types = {seq[0]: None for seq in nonemptyseqs}
            raise SyntaxError(
                "Cannot create a consistent method resolution order (MRO) for bases: "
                + ", ".join(t.name for t in types)
            )
        res.append(cand)
        for seq in nonemptyseqs:  # remove cand
            if seq[0] == cand:
                del seq[0]


def _mro(C: Class) -> List[Class]:
    "Compute the class precedence list (mro) according to C3"
    return _merge([[C]] + list(map(_mro, C.bases)) + [list(C.bases)])


class Parameter:
    def __init__(
        self,
        name: str,
        idx: int,
        type_ref: TypeRef,
        has_default: bool,
        default_val: object,
        is_kwonly: bool,
    ) -> None:
        self.name = name
        self.type_ref = type_ref
        self.index = idx
        self.has_default = has_default
        self.default_val = default_val
        self.is_kwonly = is_kwonly

    def __repr__(self) -> str:
        return (
            f"<Parameter name={self.name}, ref={self.type_ref}, "
            f"index={self.index}, has_default={self.has_default}>"
        )

    def bind_generics(
        self,
        name: GenericTypeName,
        generic_types: Dict[Class, Dict[Tuple[Class, ...], Class]],
    ) -> Parameter:
        klass = self.type_ref.resolved
        if klass.is_generic_parameter:
            assert isinstance(klass, GenericParameter)
            return Parameter(
                self.name,
                self.index,
                ResolvedTypeRef(name.args[klass.index]),
                self.has_default,
                self.default_val,
                self.is_kwonly,
            )
        elif klass.contains_generic_parameters:
            assert isinstance(klass, GenericClass)
            type_args = [
                arg for arg in klass.type_name.args if isinstance(arg, GenericParameter)
            ]
            assert len(type_args) == len(klass.type_name.args)
            # map the generic type parameters for the type to the parameters provided
            bind_args = tuple(name.args[arg.index] for arg in type_args)
            # We don't yet support generic methods, so all of the generic parameters are coming from the
            # type definition.
            return Parameter(
                self.name,
                self.index,
                ResolvedTypeRef(klass.make_generic_type(bind_args, generic_types)),
                self.has_default,
                self.default_val,
                self.is_kwonly,
            )

        return self


def is_subsequence(a: Iterable[object], b: Iterable[object]) -> bool:
    # for loops go brrrr :)
    # https://ericlippert.com/2020/03/27/new-grad-vs-senior-dev/
    itr = iter(a)
    for each in b:
        if each not in itr:
            return False
    return True


class Callable(Object[TClass]):
    def __init__(
        self,
        klass: Class,
        func_name: str,
        args: List[Parameter],
        args_by_name: Dict[str, Parameter],
        num_required_args: int,
        vararg: Optional[Parameter],
        kwarg: Optional[Parameter],
        return_type: TypeRef,
        type_descr: TypeDescr,
    ) -> None:
        super().__init__(klass)
        self.func_name = func_name
        self.args = args
        self.args_by_name = args_by_name
        self.num_required_args = num_required_args
        self.vararg = vararg
        self.kwarg = kwarg
        assert return_type is not None
        self.return_type = return_type
        self.type_descr: TypeDescr = type_descr

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        if self.vararg or self.kwarg:
            return super().bind_call(node, visitor, type_ctx)

        result = self.bind_call_self(node, visitor, type_ctx)
        self.check_args_for_primitives(node, visitor)
        return result

    def bind_call_self(
        self,
        node: ast.Call,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
        self_type: Optional[Value] = None,
    ) -> NarrowingEffect:
        visitor.set_type(node, self.return_type.resolved.instance)
        if type_ctx is not None:
            visitor.check_can_assign_from(
                type_ctx.klass,
                self.return_type.resolved,
                node,
                "is an invalid return type, expected",
            )

        start = 0
        if self_type:
            # Typecheck the "self" arg (if provided)
            resolved_type = self.args[0].type_ref.resolved
            called_type = self_type
            visitor.check_can_assign_from(
                resolved_type, called_type.klass, node, "mismatched type of self arg"
            )
            start = 1  # skip the first arg, we already checked it

        # TODO: handle duplicate args and other weird stuff a-la
        # https://fburl.com/diffusion/q6tpinw8
        for idx, (defined_arg, call_arg) in enumerate(
            zip(self.args[start:], node.args)
        ):
            if defined_arg.is_kwonly:
                raise visitor.syntax_error(
                    f"{self.func_name} takes {idx + start} positional args but {len(node.args) + start} {'was' if len(node.args) + start == 1 else 'were'} given",
                    node,
                )

            resolved_type = defined_arg.type_ref.resolved
            exc = None
            try:
                visitor.visit(
                    call_arg, resolved_type.instance if resolved_type else None
                )
            except TypedSyntaxError as e:
                # We may report a better error message below...
                exc = e

            if isinstance(call_arg, Starred):
                # Skip type verification here, f(a, b, *something)
                # TODO: add support for this by implementing type constrained tuples
                if exc is not None:
                    raise exc
                continue

            visitor.check_can_assign_from(
                resolved_type,
                visitor.get_type(call_arg).klass,
                node,
                "positional argument type mismatch",
            )
            if exc is not None:
                raise exc

        for kw_arg in node.keywords:
            argname = kw_arg.arg
            call_arg = kw_arg.value
            if argname is None:
                # Gotta skip this, we cannot handle verification of types for `f(**something)`.
                # TODO: add support for this by implementing type constrained dicts
                visitor.visit(call_arg)
                continue

            if argname not in self.args_by_name:
                raise visitor.syntax_error(
                    f"Given argument {kw_arg.arg} "
                    f"does not exist in the definition of {self.func_name}",
                    node,
                )

            defined_arg = self.args_by_name[argname]
            resolved_type = defined_arg.type_ref.resolved
            exc = None
            try:
                visitor.visit(call_arg, resolved_type.instance)
            except TypedSyntaxError as e:
                # We may report a better error message below
                exc = e

            visitor.check_can_assign_from(
                resolved_type,
                visitor.get_type(call_arg).klass,
                node,
                "keyword argument type mismatch",
            )
            if exc is not None:
                raise exc

        return NO_EFFECT

    def _emit_kwarg_temps(
        self, keywords: List[ast.keyword], code_gen: Static38CodeGenerator
    ) -> Dict[str, str]:
        temporaries = {}
        for each in keywords:
            name = each.arg
            if name is not None:
                code_gen.visit(each.value)
                temp_var_name = f"{_TMP_VAR_PREFIX}{name}"
                code_gen.emit("STORE_FAST", temp_var_name)
                temporaries[name] = temp_var_name
        return temporaries

    def _find_provided_kwargs(
        self, node: ast.Call
    ) -> Tuple[Dict[int, int], Optional[int]]:
        # This is a mapping of indices from index in the function definition --> node.keywords
        provided_kwargs: Dict[int, int] = {}
        # Index of `**something` in the call
        variadic_idx: Optional[int] = None
        for idx, argument in enumerate(node.keywords):
            name = argument.arg
            if name is not None:
                provided_kwargs[self.args_by_name[name].index] = idx
            else:
                # Because of the constraints above, we will only ever reach here once
                variadic_idx = idx
        return provided_kwargs, variadic_idx

    def can_call_self(self, node: ast.Call, has_self: bool) -> bool:
        if self.vararg or self.kwarg:
            return False

        has_default_args = self.num_required_args < len(self.args)
        num_star_args = [isinstance(a, ast.Starred) for a in node.args].count(True)
        num_dstar_args = [(a.arg is None) for a in node.keywords].count(True)
        num_kwonly = len([arg for arg in self.args if arg.is_kwonly])

        start = 1 if has_self else 0
        for arg in self.args[start + len(node.args) :]:
            if arg.has_default and isinstance(arg.default_val, ast.expr):
                for kw_arg in node.keywords:
                    if kw_arg.arg == arg.name:
                        break
                else:
                    return False
        if (
            # We don't support f(*a, *b)
            num_star_args > 1
            # We don't support f(**a, **b)
            or num_dstar_args > 1
            # We don't support f(1, 2, *a) if f has any default arg values
            or (has_default_args and num_star_args > 0)
            or num_kwonly
        ):
            return False

        return True

    def emit_call_self(
        self,
        node: ast.Call,
        code_gen: Static38CodeGenerator,
        self_type: Optional[TypeName] = None,
    ) -> None:
        assert self.can_call_self(node, self_type is not None)

        temporaries: Dict[str, str] = {}
        # We need to use temporaries if the args in the Call are not in the same order as
        # the args in the function definition. That way, the functions x, y, and z in a
        # call like "my_func(a=x(), b=y(), c=z())" are called in the expected order.
        should_use_temporaries = not is_subsequence(
            [i.name for i in self.args], [a.arg for a in node.keywords]
        )
        nvariadic = 0
        nseen = 0

        if self_type:
            nseen += 1

        # - Handle the positional args.. they're uncomplicated
        for idx, (defined_arg, argument) in enumerate(
            zip(self.args[nseen:], node.args)
        ):
            if isinstance(argument, ast.Starred):
                code_gen.visit(argument.value)
                remaining_args = self.num_required_args - idx
                for arg_idx in range(remaining_args):
                    code_gen.emit("LOAD_ITERABLE_ARG", arg_idx)
                    cur_arg = self.args[nseen]
                    if (
                        cur_arg.type_ref.resolved is not None
                        and cur_arg.type_ref.resolved is not DYNAMIC
                    ):
                        code_gen.emit("ROT_TWO")
                        code_gen.emit("CAST", cur_arg.type_ref.resolved.type_descr)
                        code_gen.emit("ROT_TWO")
                    nseen += 1
                # Remove the tuple from TOS
                code_gen.emit("POP_TOP")
                nvariadic += 1
            else:
                arg_type = code_gen.get_type(argument)
                code_gen.visit(argument)
                code_gen.emit_type_check(
                    (defined_arg.type_ref.resolved or DYNAMIC_TYPE), arg_type.klass
                )
                nseen += 1

        # - Handle keyword args.. these are annoying
        provided_kwargs, variadic_idx = self._find_provided_kwargs(node)
        if variadic_idx is not None:
            nvariadic += 1

        if should_use_temporaries:
            # The called args are not in the same order as defined, we'll throw them in temps
            # and then do the call
            temporaries = self._emit_kwarg_temps(node.keywords, code_gen)

        # For all args that are not passed positionally, push the default value
        for idx in range(nseen, len(self.args)):
            if idx in provided_kwargs:
                kwarg = node.keywords[provided_kwargs[idx]]
                kwarg_name = kwarg.arg
                assert kwarg_name is not None
                if should_use_temporaries and kwarg_name:
                    code_gen.emit("LOAD_FAST", temporaries[kwarg_name])
                else:
                    code_gen.visit(kwarg.value)
                code_gen.emit_type_check(
                    self.args_by_name[kwarg_name].type_ref.resolved or DYNAMIC_TYPE,
                    code_gen.get_type(kwarg.value).klass,
                )
            elif variadic_idx is not None:
                # We have a f(**something), if the arg is unavailable, we
                # load it from the mapping
                matching_arg = self.args[idx]
                if matching_arg.has_default:
                    code_gen.emit("LOAD_CONST", matching_arg.default_val)
                code_gen.visit(node.keywords[variadic_idx].value)
                code_gen.emit("LOAD_CONST", self.args[idx].name)
                if matching_arg.has_default:
                    code_gen.emit("LOAD_MAPPING_ARG", 3)
                else:
                    code_gen.emit("LOAD_MAPPING_ARG", 2)
                code_gen.emit_type_check(
                    matching_arg.type_ref.resolved or DYNAMIC_TYPE, DYNAMIC_TYPE
                )
            else:
                matching_arg = self.args[idx]
                if not matching_arg.has_default:
                    # It's an error if this arg did not have a default value in the definition
                    raise code_gen.syntax_error(
                        f"Function {self.func_name} expects a value for "
                        f"argument {matching_arg.name}",
                        node,
                    )
                # In this case, the opcode will just push a NULL on the stack
                # and move on
                code_gen.emit("LOAD_CONST", matching_arg.default_val)
            nseen += 1

        # nseen must equal number of defined args if no variadic args are used
        if nvariadic == 0 and (nseen != len(self.args)):
            raise code_gen.syntax_error(
                f"Mismatched number of args for {self.name}. "
                f"Expected {len(self.args)}, got {nseen}",
                node,
            )

        if self_type is not None:
            code_gen.emit_invoke_method(self.type_descr, nseen - 1)
        else:
            code_gen.emit("EXTENDED_ARG", 0)
            code_gen.emit("INVOKE_FUNCTION", (self.type_descr, nseen))


class Function(Callable[Class]):
    def __init__(
        self,
        name: str,
        decl_type: Optional[TypeName],
        node: Union[AsyncFunctionDef, FunctionDef],
        ret_type: TypeRef,
        type_descr: TypeDescr,
    ) -> None:
        super().__init__(
            FUNCTION_TYPE,
            name,
            [],
            {},
            0,
            None,
            None,
            ret_type,
            type_descr,
        )
        self.decl_type = decl_type
        self.node = node

    @property
    def name(self) -> str:
        return f"function {self.func_name}"

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        if not self.can_call_self(node, False):
            return super().emit_call(node, code_gen)

        return self.emit_call_self(node, code_gen, None)

    def bind_descr_get(
        self,
        node: ast.Attribute,
        inst: Optional[Object[TClassInv]],
        ctx: TClassInv,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> None:
        decl_type = self.decl_type
        if decl_type is None:
            raise visitor.syntax_error(
                "internal compiler error: function bound to class", node
            )
        elif inst is None:
            visitor.set_type(node, self)
        else:
            visitor.set_type(node, MethodType(decl_type, self.node, node, self))

    def register_arg(
        self,
        name: str,
        idx: int,
        ref: TypeRef,
        has_default: bool,
        default_val: object,
        is_kwonly: bool,
    ) -> None:
        parameter = Parameter(name, idx, ref, has_default, default_val, is_kwonly)
        self.args.append(parameter)
        self.args_by_name[name] = parameter
        if not has_default:
            self.num_required_args += 1

    def __repr__(self) -> str:
        return f"<{self.klass.name} '{self.name}' instance, args={self.args}>"


class MethodType(Object[Class]):
    def __init__(
        self,
        decl_type: TypeName,
        node: Union[AsyncFunctionDef, FunctionDef],
        target: ast.Attribute,
        function: Function,
    ) -> None:
        super().__init__(METHOD_TYPE)
        self.decl_type = decl_type
        self.node = node
        self.target = target
        self.function = function

    @property
    def name(self) -> str:
        return "method " + self.function.func_name

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        result = self.function.bind_call_self(
            node, visitor, type_ctx, visitor.get_type(self.target.value)
        )
        self.check_args_for_primitives(node, visitor)
        return result

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        if not self.function.can_call_self(node, True):
            return super().emit_call(node, code_gen)

        code_gen.update_lineno(node)
        code_gen.visit(self.target.value)

        self.function.emit_call_self(node, code_gen, self.decl_type)


class StaticMethod(Object[Class]):
    def __init__(
        self,
        decl_type: TypeName,
        node: Union[AsyncFunctionDef, FunctionDef],
        function: Function,
    ) -> None:

        super().__init__(STATIC_METHOD_TYPE)
        self.decl_type = decl_type
        self.node = node
        self.function = function

    @property
    def name(self) -> str:
        return "staticmethod " + self.function.func_name

    def bind_descr_get(
        self,
        node: ast.Attribute,
        inst: Optional[Object[TClassInv]],
        ctx: TClassInv,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> None:
        visitor.set_type(node, self.function)


class BuiltinMethodDescriptor(Callable[Class]):
    def __init__(
        self,
        decl_type: TypeName,
        func_name: str,
        args: Optional[Tuple[Parameter, ...]] = None,
        return_type: Optional[TypeRef] = None,
    ) -> None:
        assert isinstance(return_type, (TypeRef, type(None)))
        super().__init__(
            BUILTIN_METHOD_DESC_TYPE,
            func_name,
            args,
            {},
            0,
            None,
            None,
            return_type or ResolvedTypeRef(DYNAMIC_TYPE),
            decl_type.type_descr + (func_name,),
        )
        self.decl_type = decl_type
        self.func_name = func_name

    def bind_call_self(
        self,
        node: ast.Call,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
        self_type: Optional[Value] = None,
    ) -> NarrowingEffect:
        if self.args is not None:
            return super().bind_call_self(node, visitor, type_ctx, self_type)
        elif node.keywords:
            return super().bind_call(node, visitor, type_ctx)

        visitor.set_type(node, DYNAMIC)
        for arg in node.args:
            visitor.visit(arg)

        return NO_EFFECT

    def bind_descr_get(
        self,
        node: ast.Attribute,
        inst: Optional[Object[TClassInv]],
        ctx: TClassInv,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> None:
        if inst is None:
            visitor.set_type(node, self)
        else:
            visitor.set_type(node, BuiltinMethod(self, node))

    def make_generic(
        self, name: GenericTypeName, generic_types: GenericTypesDict
    ) -> Value:
        cur_args = self.args
        cur_ret_type = self.return_type
        if cur_args is not None and cur_ret_type is not None:
            new_args = tuple(arg.bind_generics(name, generic_types) for arg in cur_args)
            new_ret_type = cur_ret_type.resolved.bind_generics(name, generic_types)
            return BuiltinMethodDescriptor(
                name, self.func_name, new_args, ResolvedTypeRef(new_ret_type)
            )
        else:
            return BuiltinMethodDescriptor(name, self.func_name)


class BuiltinMethod(Callable[Class]):
    def __init__(self, desc: BuiltinMethodDescriptor, target: ast.Attribute) -> None:
        super().__init__(
            BUILTIN_METHOD_TYPE,
            desc.func_name,
            desc.args,
            None,
            0,
            None,
            None,
            desc.return_type,
            desc.decl_type.type_descr + (desc.func_name,),
        )
        self.desc = desc
        self.target = target

    @property
    def name(self) -> str:
        return self.func_name

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        if self.args:
            return super().bind_call_self(
                node, visitor, type_ctx, visitor.get_type(self.target.value)
            )
        if node.keywords:
            return Object.bind_call(self, node, visitor, type_ctx)

        visitor.set_type(node, self.return_type.resolved.instance)
        visitor.visit(self.target.value)
        for arg in node.args:
            visitor.visit(arg)
        self.check_args_for_primitives(node, visitor)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        if node.keywords or (
            self.args is not None and not self.desc.can_call_self(node, True)
        ):
            return super().emit_call(node, code_gen)

        code_gen.update_lineno(node)
        code_gen.visit(self.target.value)

        code_gen.update_lineno(node)

        if self.args is not None:
            self.desc.emit_call_self(node, code_gen, self.desc.decl_type)
        else:
            # Untyped method, we can still do an INVOKE_METHOD

            for arg in node.args:
                code_gen.visit(arg)
            # Emit a zero EXTENDED_ARG before so that we can optimize and insert the
            # arg count
            descr = self.desc.decl_type.type_descr
            descr += (self.func_name,)
            code_gen.emit_invoke_method(descr, len(node.args))


class StrictBuiltins(Object[Class]):
    def __init__(self, builtins: Dict[str, Value]) -> None:
        super().__init__(DICT_TYPE)
        self.builtins = builtins

    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        slice = node.slice
        type = DYNAMIC
        if isinstance(slice, ast.Index):
            # pyre-ignore[16]: `Index` has no attribute `value`.
            val = slice.value
            if isinstance(val, ast.Str):
                builtin = self.builtins.get(val.s)
                if builtin is not None:
                    type = builtin
            elif isinstance(val, ast.Constant):
                svalue = val.value
                if isinstance(svalue, str):
                    builtin = self.builtins.get(svalue)
                    if builtin is not None:
                        type = builtin

        visitor.set_type(node, type)


def get_default_value(default: expr) -> object:
    if not isinstance(default, (Constant, Str, Num, Bytes, NameConstant, ast.Ellipsis)):

        default = AstOptimizer().visit(default)

    if isinstance(default, Str):
        return default.s
    elif isinstance(default, Num):
        return default.n
    elif isinstance(default, Bytes):
        return default.s
    elif isinstance(default, ast.Ellipsis):
        return ...
    elif isinstance(default, (ast.Constant, ast.NameConstant)):
        return default.value
    else:
        return default


def process_function_args(
    func: Function,
    arguments: ast.arguments,
    visitor: DeclarationVisitor,
    self_type: Optional[Class] = None,
) -> None:
    """
    Register type-refs for each function argument, assume DYNAMIC if annotation is missing.
    """
    nrequired = len(arguments.args) - len(arguments.defaults)
    no_defaults = cast(List[Optional[ast.expr]], [None] * nrequired)
    defaults = no_defaults + cast(List[Optional[ast.expr]], arguments.defaults)
    idx = 0
    for idx, (argument, default) in enumerate(zip(arguments.args, defaults)):
        annotation = argument.annotation
        default_val = None
        has_default = False
        if default is not None:
            has_default = True
            default_val = get_default_value(default)

        if annotation:
            visitor.visit(annotation)
            ref = TypeRef(visitor.module, annotation)
        elif argument.arg == "self" and idx == 0 and self_type is not None:
            ref = ResolvedTypeRef(self_type)
        else:
            ref = ResolvedTypeRef(DYNAMIC_TYPE)
        func.register_arg(argument.arg, idx, ref, has_default, default_val, False)

    base_idx = idx

    vararg = arguments.vararg
    if vararg:
        base_idx += 1
        func.vararg = Parameter(
            vararg.arg, base_idx, TUPLE_EXACT_TYPE, False, None, False
        )

    for argument, default in zip(arguments.kwonlyargs, arguments.kw_defaults):
        annotation = argument.annotation
        default_val = None
        has_default = default is not None
        if default is not None:
            default_val = get_default_value(default)
        if annotation:
            visitor.visit(annotation)
            ref = TypeRef(visitor.module, annotation)
        else:
            ref = ResolvedTypeRef(DYNAMIC_TYPE)
        base_idx += 1
        func.register_arg(argument.arg, base_idx, ref, has_default, default_val, True)

    kwarg = arguments.kwarg
    if kwarg:
        func.kwarg = Parameter(
            kwarg.arg, base_idx + 1, DICT_EXACT_TYPE, False, None, False
        )


# Bringing up the type system is a little special as we have dependencies
# amongst type and object
TYPE_TYPE = Class.__new__(Class)
TYPE_TYPE.type_name = TypeName("builtins", "type")
TYPE_TYPE.klass = TYPE_TYPE
TYPE_TYPE.instance = TYPE_TYPE
TYPE_TYPE.members = {}
TYPE_TYPE.is_exact = False
TYPE_TYPE._mro = None
TYPE_TYPE._mro_type_descrs = None


class Slot(Object[TClassInv]):
    def __init__(self, type_ref: TypeRef, name: str, decl_type: Class) -> None:
        super().__init__(MEMBER_TYPE)
        self.decl_type = decl_type
        self.slot_name = name
        self.type_ref = type_ref

    def bind_descr_get(
        self,
        node: ast.Attribute,
        inst: Optional[Object[TClassInv]],
        ctx: TClassInv,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> None:
        if inst is None:
            visitor.set_type(node, self)
            return

        visitor.set_type(node, self.type_ref.resolved.instance)

    @property
    def type_descr(self) -> TypeDescr:
        return self.type_ref.resolved.type_descr


# TODO (aniketpanse): move these to a better place
OBJECT_TYPE = Class(TypeName("builtins", "object"))
OBJECT = OBJECT_TYPE.instance

DYNAMIC_TYPE = DynamicClass()
DYNAMIC = DYNAMIC_TYPE.instance


class BoxFunction(Object[Class]):
    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        if len(node.args) != 1:
            raise visitor.syntax_error("box only accepts a single argument", node)

        for arg in node.args:
            visitor.visit(arg)
            if not isinstance(visitor.get_type(arg), CInstance):
                raise visitor.syntax_error(
                    f"can't box non-primitive: {visitor.get_type(arg).name}", node
                )
        visitor.set_type(node, DYNAMIC)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        code_gen.get_type(node.args[0]).emit_box(node.args[0], code_gen)


class UnboxFunction(Object[Class]):
    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        if len(node.args) != 1:
            raise visitor.syntax_error("unbox only accepts a single argument", node)

        for arg in node.args:
            visitor.visit(arg, DYNAMIC)
        self.check_args_for_primitives(node, visitor)
        visitor.set_type(node, INT64_VALUE)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        code_gen.get_type(node).emit_unbox(node.args[0], code_gen)


class LenFunction(Object[Class]):
    @property
    def name(self) -> str:
        return "len function"

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        if len(node.args) != 1:
            visitor.syntax_error(
                f"len() does not accept more than one arguments ({len(node.args)} given)",
                node,
            )
        visitor.visit(node.args[0])
        self.check_args_for_primitives(node, visitor)
        visitor.set_type(node, INT_EXACT_TYPE.instance)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        code_gen.get_type(node.args[0]).emit_len(node, code_gen)


class ExtremumFunction(Object[Class]):
    def __init__(self, klass: Class, is_min: bool) -> None:
        super().__init__(klass)
        self.is_min = is_min

    @property
    def _extremum(self) -> str:
        return "min" if self.is_min else "max"

    @property
    def name(self) -> str:
        return f"{self._extremum} function"

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        if (
            # We only specialize for two args
            len(node.args) != 2
            # We don't support specialization if any kwargs are present
            or len(node.keywords) > 0
            # If we have any *args, we skip specialization
            or any(isinstance(a, ast.Starred) for a in node.args)
        ):
            return super().emit_call(node, code_gen)

        # Compile `min(a, b)` to a ternary expression, `a if a <= b else b`.
        # Similar for `max(a, b).
        endblock = code_gen.newBlock(f"{self._extremum}_end")
        elseblock = code_gen.newBlock(f"{self._extremum}_else")

        for a in node.args:
            code_gen.visit(a)

        if self.is_min:
            op = "<="
        else:
            op = ">="

        code_gen.emit("DUP_TOP_TWO")
        code_gen.emit("COMPARE_OP", op)
        code_gen.emit("POP_JUMP_IF_FALSE", elseblock)
        # Remove `b` from stack, `a` was the minimum
        code_gen.emit("POP_TOP")
        code_gen.emit("JUMP_FORWARD", endblock)
        code_gen.nextBlock(elseblock)
        # Remove `a` from the stack, `b` was the minimum
        code_gen.emit("ROT_TWO")
        code_gen.emit("POP_TOP")
        code_gen.nextBlock(endblock)


class IsInstanceFunction(Object[Class]):
    def __init__(self) -> None:
        super().__init__(FUNCTION_TYPE)

    @property
    def name(self) -> str:
        return "isinstance function"

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        if node.keywords:
            visitor.syntax_error("isinstance() does not accept keyword arguments", node)
        for arg in node.args:
            visitor.visit(arg)
        self.check_args_for_primitives(node, visitor)
        visitor.set_type(node, BOOL_TYPE.instance)
        if len(node.args) == 2:
            arg0 = node.args[0]
            if not isinstance(arg0, ast.Name):
                return NO_EFFECT

            arg1 = node.args[1]
            klass_type = None
            if isinstance(arg1, ast.Tuple):
                types = tuple(visitor.get_type(el) for el in arg1.elts)
                if all(isinstance(t, Class) for t in types):
                    klass_type = UNION_TYPE.make_generic_type(
                        types, visitor.symtable.generic_types
                    )
            else:
                arg1_type = visitor.get_type(node.args[1])
                if isinstance(arg1_type, Class):
                    klass_type = arg1_type

            if klass_type is not None:
                return IsInstanceEffect(
                    arg0.id, visitor.get_type(arg0), klass_type.instance, visitor
                )

        return NO_EFFECT


class IsSubclassFunction(Object[Class]):
    def __init__(self) -> None:
        super().__init__(FUNCTION_TYPE)

    @property
    def name(self) -> str:
        return "issubclass function"

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        if node.keywords:
            visitor.syntax_error("issubclass() does not accept keyword arguments", node)
        for arg in node.args:
            visitor.visit(arg)
        visitor.set_type(node, BOOL_TYPE.instance)
        self.check_args_for_primitives(node, visitor)
        return NO_EFFECT


class NumClass(Class):
    def __init__(
        self,
        name: TypeName,
        pytype: Optional[Type[object]] = None,
        is_exact: bool = False,
    ) -> None:
        super().__init__(
            name,
            [RESOLVED_OBJECT_TYPE],
            NumInstance(self),
            pytype=pytype,
            is_exact=is_exact,
        )


class NumInstance(Object[NumClass]):
    def bind_unaryop(
        self, node: ast.UnaryOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        if isinstance(node.op, (ast.USub, ast.Invert, ast.UAdd)):
            visitor.set_type(node, self)
        else:
            assert isinstance(node.op, ast.Not)
            visitor.set_type(node, BOOL_TYPE.instance)

    def bind_num(self, node: ast.Num, visitor: TypeBinder) -> None:
        self._bind_constant(node.n, node, visitor)

    def bind_constant(self, node: ast.Constant, visitor: TypeBinder) -> None:
        self._bind_constant(node.value, node, visitor)

    def _bind_constant(
        self, value: object, node: ast.expr, visitor: TypeBinder
    ) -> None:
        value_inst = CONSTANT_TYPES.get(type(value), self)
        visitor.set_type(node, value_inst)
        visitor.check_can_assign_from(self.klass, value_inst.klass, node)

    def bind_reverse_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        visitor.set_type(node, self)
        return False


def parse_type(info: Dict[str, object]) -> Class:
    optional = info.get("optional", False)
    type = info.get("type")
    if type:
        if type == "object":
            klass = OBJECT_TYPE
        elif type == "str":
            klass = STR_TYPE
        elif type == "__static__.int64":
            klass = INT64_TYPE
        elif type == "__static__.int32":
            klass = INT32_TYPE
        elif type == "NoneType":
            klass = NONE_TYPE
        else:
            raise NotImplementedError("unsupported type: " + str(type))
    else:
        type_param = info.get("type_param")
        assert isinstance(type_param, int)
        klass = GenericParameter("T" + str(type_param), type_param)

    if optional:
        return OPTIONAL_TYPE.make_generic_type((klass,), BUILTIN_GENERICS)

    return klass


def parse_param(info: Dict[str, object], idx: int) -> Parameter:
    name = info.get("name", "")
    assert isinstance(name, str)

    return Parameter(
        name,
        idx,
        ResolvedTypeRef(parse_type(info)),
        "default" in info,
        info.get("default"),
        False,
    )


def make_type_dict(klass: Class, t: Type[object]) -> Dict[str, Value]:
    ret: Dict[str, Value] = {}
    for k in t.__dict__.keys():
        obj = getattr(t, k)
        if isinstance(obj, MethodDescriptorType):
            sig = getattr(obj, "__typed_signature__", None)
            if sig is not None:
                args = sig["args"]

                signature = [
                    Parameter("self", 0, ResolvedTypeRef(klass), False, None, False)
                ]
                for idx, arg in enumerate(args):
                    signature.append(parse_param(arg, idx + 1))
                return_type = parse_type(sig["return"])
                method = BuiltinMethodDescriptor(
                    klass.type_name, k, tuple(signature), ResolvedTypeRef(return_type)
                )
            else:
                method = BuiltinMethodDescriptor(klass.type_name, k)
            ret[k] = method

    return ret


def common_sequence_emit_len(
    node: ast.Call, code_gen: Static38CodeGenerator, oparg: int
) -> None:
    if len(node.args) != 1:
        raise code_gen.syntax_error(
            f"Can only pass a single argument when checking sequence length", node
        )
    code_gen.visit(node.args[0])
    code_gen.emit("FAST_LEN", oparg)
    signed = False
    code_gen.emit("INT_BOX", int(signed))


def common_sequence_emit_jumpif(
    test: AST,
    next: Block,
    is_if_true: bool,
    code_gen: Static38CodeGenerator,
    oparg: int,
) -> None:
    code_gen.visit(test)
    code_gen.emit("FAST_LEN", oparg)
    code_gen.emit("POP_JUMP_IF_NONZERO" if is_if_true else "POP_JUMP_IF_ZERO", next)


def common_sequence_emit_forloop(
    node: ast.For, code_gen: Static38CodeGenerator, oparg: int
) -> None:
    start = code_gen.newBlock(f"seq_forloop_start")
    anchor = code_gen.newBlock(f"seq_forloop_anchor")
    after = code_gen.newBlock(f"seq_forloop_after")
    with code_gen.new_loopidx() as loop_idx:
        code_gen.set_lineno(node)
        code_gen.push_loop(FOR_LOOP, start, after)
        code_gen.visit(node.iter)

        code_gen.emit("INT_LOAD_CONST", 0)
        code_gen.emit("STORE_LOCAL", (loop_idx, ("__static__", "int64")))
        code_gen.nextBlock(start)
        code_gen.emit("DUP_TOP")  # used for SEQUENCE_GET
        code_gen.emit("DUP_TOP")  # used for FAST_LEN
        code_gen.emit("FAST_LEN", oparg)
        code_gen.emit("LOAD_LOCAL", (loop_idx, ("__static__", "int64")))
        code_gen.emit("INT_COMPARE_OP", PRIM_OP_GT)
        code_gen.emit("POP_JUMP_IF_ZERO", anchor)
        code_gen.emit("LOAD_LOCAL", (loop_idx, ("__static__", "int64")))
        if oparg == FAST_LEN_LIST:
            code_gen.emit("SEQUENCE_GET", SEQ_LIST)
        else:
            # todo - we need to implement TUPLE_GET which supports primitive index
            code_gen.emit("INT_BOX", 1)  # 1 is for signed
            code_gen.emit("BINARY_SUBSCR", 2)
        code_gen.emit("LOAD_LOCAL", (loop_idx, ("__static__", "int64")))
        code_gen.emit("INT_LOAD_CONST", 1)
        code_gen.emit("INT_BINARY_OP", PRIM_OP_ADD)
        code_gen.emit("STORE_LOCAL", (loop_idx, ("__static__", "int64")))
        code_gen.visit(node.target)
        code_gen.visit(node.body)
        code_gen.emit("JUMP_ABSOLUTE", start)
        code_gen.nextBlock(anchor)
        code_gen.emit("POP_TOP")  # Pop loop index
        code_gen.emit("POP_TOP")  # Pop list
        code_gen.pop_loop()

        if node.orelse:
            code_gen.visit(node.orelse)
        code_gen.nextBlock(after)


class TupleClass(Class):
    def __init__(self, is_exact: bool = False) -> None:
        instance = TupleExactInstance(self) if is_exact else TupleInstance(self)
        super().__init__(
            type_name=TypeName("builtins", "tuple"),
            bases=[RESOLVED_OBJECT_TYPE],
            instance=instance,
            is_exact=is_exact,
            pytype=tuple,
        )


class TupleInstance(Object[TupleClass]):
    def get_fast_len_type(self) -> int:
        return FAST_LEN_TUPLE | ((not self.klass.is_exact) << 4)

    def emit_len(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        return common_sequence_emit_len(node, code_gen, self.get_fast_len_type())

    def emit_jumpif(
        self, test: AST, next: Block, is_if_true: bool, code_gen: Static38CodeGenerator
    ) -> None:
        return common_sequence_emit_jumpif(
            test, next, is_if_true, code_gen, self.get_fast_len_type()
        )


class TupleExactInstance(TupleInstance):
    def emit_forloop(self, node: ast.For, code_gen: Static38CodeGenerator) -> None:
        if not isinstance(node.target, ast.Name):
            # We don't yet support `for a, b in my_tuple: ...`
            return super().emit_forloop(node, code_gen)

        return common_sequence_emit_forloop(node, code_gen, FAST_LEN_TUPLE)


class SetClass(Class):
    def __init__(self, is_exact: bool = False) -> None:
        super().__init__(
            type_name=TypeName("builtins", "set"),
            bases=[RESOLVED_OBJECT_TYPE],
            instance=SetInstance(self),
            is_exact=is_exact,
            pytype=tuple,
        )


class SetInstance(Object[SetClass]):
    def get_fast_len_type(self) -> int:
        return FAST_LEN_SET | ((not self.klass.is_exact) << 4)

    def emit_len(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        if len(node.args) != 1:
            raise code_gen.syntax_error(
                "Can only pass a single argument when checking set length", node
            )
        code_gen.visit(node.args[0])
        code_gen.emit("FAST_LEN", self.get_fast_len_type())
        signed = False
        code_gen.emit("INT_BOX", int(signed))

    def emit_jumpif(
        self, test: AST, next: Block, is_if_true: bool, code_gen: Static38CodeGenerator
    ) -> None:
        code_gen.visit(test)
        code_gen.emit("FAST_LEN", self.get_fast_len_type())
        code_gen.emit("POP_JUMP_IF_NONZERO" if is_if_true else "POP_JUMP_IF_ZERO", next)


class ListClass(Class):
    def __init__(self, is_exact: bool = False) -> None:
        instance = ListExactInstance(self) if is_exact else ListInstance(self)
        super().__init__(
            type_name=TypeName("builtins", "list"),
            bases=[RESOLVED_OBJECT_TYPE],
            instance=instance,
            is_exact=is_exact,
            pytype=list,
        )


class ListInstance(Object[ListClass]):
    def get_fast_len_type(self) -> int:
        return FAST_LEN_LIST | ((not self.klass.is_exact) << 4)

    def emit_len(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        return common_sequence_emit_len(node, code_gen, self.get_fast_len_type())

    def emit_jumpif(
        self, test: AST, next: Block, is_if_true: bool, code_gen: Static38CodeGenerator
    ) -> None:
        return common_sequence_emit_jumpif(
            test, next, is_if_true, code_gen, self.get_fast_len_type()
        )


class ListExactInstance(ListInstance):
    def bind_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        if isinstance(node.op, ast.Mult) and INT_TYPE.can_assign_from(
            visitor.get_type(node.right).klass
        ):
            visitor.set_type(node, LIST_EXACT_TYPE.instance)
            return True
        return super().bind_binop(node, visitor, type_ctx)

    def bind_reverse_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        if isinstance(node.op, ast.Mult) and INT_TYPE.can_assign_from(
            visitor.get_type(node.left).klass
        ):
            visitor.set_type(node, LIST_EXACT_TYPE.instance)
            return True
        return super().bind_reverse_binop(node, visitor, type_ctx)

    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        if type.klass not in SIGNED_CINT_TYPES:
            super().bind_subscr(node, type, visitor)
        visitor.set_type(node, DYNAMIC)

    def emit_subscr(
        self, node: ast.Subscript, aug_flag: bool, code_gen: Static38CodeGenerator
    ) -> None:
        index_type = code_gen.get_type(node.slice)
        if index_type.klass not in SIGNED_CINT_TYPES:
            return super().emit_subscr(node, aug_flag, code_gen)

        code_gen.update_lineno(node)
        code_gen.visit(node.value)
        code_gen.visit(node.slice)
        if isinstance(node.ctx, ast.Load):
            code_gen.emit("SEQUENCE_GET", SEQ_LIST)
        elif isinstance(node.ctx, ast.Store):
            code_gen.emit("SEQUENCE_SET", SEQ_LIST)
        elif isinstance(node.ctx, ast.Del):
            code_gen.emit("LIST_DEL")

    def emit_forloop(self, node: ast.For, code_gen: Static38CodeGenerator) -> None:
        if not isinstance(node.target, ast.Name):
            # We don't yet support `for a, b in my_list: ...`
            return super().emit_forloop(node, code_gen)

        return common_sequence_emit_forloop(node, code_gen, FAST_LEN_LIST)


class DictClass(Class):
    def __init__(self, is_exact: bool = False) -> None:
        super().__init__(
            type_name=TypeName("builtins", "dict"),
            bases=[RESOLVED_OBJECT_TYPE],
            instance=DictInstance(self),
            is_exact=is_exact,
            pytype=dict,
        )


class DictInstance(Object[DictClass]):
    def get_fast_len_type(self) -> int:
        return FAST_LEN_DICT | ((not self.klass.is_exact) << 4)

    def emit_len(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        if len(node.args) != 1:
            raise code_gen.syntax_error(
                "Can only pass a single argument when checking dict length", node
            )
        code_gen.visit(node.args[0])
        code_gen.emit("FAST_LEN", self.get_fast_len_type())
        signed = False
        code_gen.emit("INT_BOX", int(signed))

    def emit_jumpif(
        self, test: AST, next: Block, is_if_true: bool, code_gen: Static38CodeGenerator
    ) -> None:
        code_gen.visit(test)
        code_gen.emit("FAST_LEN", self.get_fast_len_type())
        code_gen.emit("POP_JUMP_IF_NONZERO" if is_if_true else "POP_JUMP_IF_ZERO", next)


FUNCTION_TYPE = Class(TypeName("types", "FunctionType"))
METHOD_TYPE = Class(TypeName("types", "MethodType"))
MEMBER_TYPE = Class(TypeName("types", "MemberDescriptorType"))
BUILTIN_METHOD_DESC_TYPE = Class(TypeName("types", "MethodDescriptorType"))
BUILTIN_METHOD_TYPE = Class(TypeName("types", "BuiltinMethodType"))
ARG_TYPE = Class(TypeName("builtins", "arg"))
SLICE_TYPE = Class(TypeName("builtins", "slice"))

RESOLVED_OBJECT_TYPE = ResolvedTypeRef(OBJECT_TYPE)

# builtin types
NONE_TYPE = NoneType()
STR_TYPE = Class(TypeName("builtins", "str"), [RESOLVED_OBJECT_TYPE], pytype=str)
INT_TYPE = NumClass(TypeName("builtins", "int"), pytype=int)
INT_EXACT_TYPE = NumClass(TypeName("builtins", "int"), pytype=int, is_exact=True)
FLOAT_TYPE = NumClass(TypeName("builtins", "float"), pytype=float)
COMPLEX_TYPE = NumClass(TypeName("builtins", "complex"))
BYTES_TYPE = Class(TypeName("builtins", "bytes"), [RESOLVED_OBJECT_TYPE], pytype=bytes)
BOOL_TYPE = Class(TypeName("builtins", "bool"), [RESOLVED_OBJECT_TYPE], pytype=bool)
ELLIPSIS_TYPE = Class(
    TypeName("builtins", "ellipsis"), [RESOLVED_OBJECT_TYPE], pytype=type(...)
)
DICT_TYPE = DictClass(is_exact=False)
DICT_EXACT_TYPE = DictClass(is_exact=True)
TUPLE_TYPE = TupleClass()
TUPLE_EXACT_TYPE = TupleClass(is_exact=True)
SET_TYPE = SetClass()
SET_EXACT_TYPE = SetClass(is_exact=True)
LIST_TYPE = ListClass()
LIST_EXACT_TYPE = ListClass(is_exact=True)

BASE_EXCEPTION_TYPE = Class(TypeName("builtins", "BaseException"), pytype=BaseException)
EXCEPTION_TYPE = Class(
    TypeName("builtins", "Exception"),
    bases=[ResolvedTypeRef(BASE_EXCEPTION_TYPE)],
    pytype=Exception,
)
STATIC_METHOD_TYPE = Class(
    TypeName("builtins", "staticmethod"),
    bases=[ResolvedTypeRef(OBJECT_TYPE)],
    pytype=staticmethod,
)

RESOLVED_INT_TYPE = ResolvedTypeRef(INT_TYPE)
RESOLVED_STR_TYPE = ResolvedTypeRef(STR_TYPE)
RESOLVED_NONE_TYPE = ResolvedTypeRef(NONE_TYPE)

TYPE_TYPE._bases = [RESOLVED_OBJECT_TYPE]

CONSTANT_TYPES: Mapping[Type[object], Value] = {
    str: STR_TYPE.instance,
    int: INT_EXACT_TYPE.instance,
    float: FLOAT_TYPE.instance,
    complex: COMPLEX_TYPE.instance,
    bytes: BYTES_TYPE.instance,
    bool: BOOL_TYPE.instance,
    type(None): NONE_TYPE.instance,
    tuple: TUPLE_TYPE.instance,
    type(...): ELLIPSIS_TYPE.instance,
}

NAMED_TUPLE_TYPE = Class(TypeName("typing", "NamedTuple"))


class UnionTypeName(GenericTypeName):
    @property
    def opt_type(self) -> Optional[Class]:
        """If we're an Optional (i.e. Union[T, None]), return T, otherwise None."""
        # Assumes well-formed union (no duplicate elements, >1 element)
        opt_type = None
        if len(self.args) == 2:
            if self.args[0] is NONE_TYPE:
                opt_type = self.args[1]
            elif self.args[1] is NONE_TYPE:
                opt_type = self.args[0]
        return opt_type

    @property
    def type_descr(self) -> TypeDescr:
        opt_type = self.opt_type
        if opt_type is not None:
            return opt_type.type_descr + ("?",)
        # the runtime does not support unions beyond optional, so just fall back
        # to dynamic for runtime purposes
        return DYNAMIC_TYPE.type_descr

    @property
    def friendly_name(self) -> str:
        opt_type = self.opt_type
        if opt_type is not None:
            return f"typing.Optional[{opt_type.name}]"
        return super().friendly_name


class UnionType(GenericClass):
    type_name: UnionTypeName
    # Union is a variadic generic, so we don't give the unbound Union any
    # GenericParameters, and we allow it to accept any number of type args.
    is_variadic = True

    def __init__(
        self,
        type_name: Optional[UnionTypeName] = None,
        type_def: Optional[GenericClass] = None,
        instance_type: Optional[Type[Object[Class]]] = None,
    ) -> None:
        instance_type = instance_type or UnionInstance
        super().__init__(
            type_name or UnionTypeName("typing", "Union", ()),
            bases=[],
            instance=instance_type(self),
            type_def=type_def,
        )

    @property
    def opt_type(self) -> Optional[Class]:
        return self.type_name.opt_type

    def issubclass(self, src: Class) -> bool:
        if isinstance(src, UnionType):
            return all(self.issubclass(t) for t in src.type_args)
        return any(t.issubclass(src) for t in self.type_args)

    def make_generic_type(
        self,
        index: Tuple[Class, ...],
        generic_types: GenericTypesDict,
    ) -> Class:
        instantiations = generic_types.get(self)
        if instantiations is not None:
            instance = instantiations.get(index)
            if instance is not None:
                return instance
        else:
            generic_types[self] = instantiations = {}

        type_args = self._simplify_args(index)
        if len(type_args) == 1 and not type_args[0].is_generic_parameter:
            return type_args[0]
        type_name = UnionTypeName(self.type_name.module, self.type_name.name, type_args)
        ThisUnionType = type(self)
        if type_name.opt_type is not None:
            ThisUnionType = OptionalType
        instantiations[index] = concrete = ThisUnionType(
            type_name,
            type_def=self,
        )
        return concrete

    def _simplify_args(self, args: Sequence[Class]) -> Tuple[Class, ...]:
        args = self._flatten_args(args)
        remove = set()
        for i, arg1 in enumerate(args):
            if i in remove:
                continue
            for j, arg2 in enumerate(args):
                # TODO this should be is_subtype_of once we split that from can_assign_from
                if i != j and arg1.can_assign_from(arg2):
                    remove.add(j)
        return tuple(arg for i, arg in enumerate(args) if i not in remove)

    def _flatten_args(self, args: Sequence[Class]) -> Sequence[Class]:
        new_args = []
        for arg in args:
            if isinstance(arg, UnionType):
                new_args.extend(self._flatten_args(arg.type_args))
            else:
                new_args.append(arg)
        return new_args


class UnionInstance(Object[UnionType]):
    def _generic_bind(
        self,
        node: ast.AST,
        callback: typingCallable[[Class], object],
        description: str,
        visitor: TypeBinder,
    ) -> None:
        if self.klass.is_generic_type_definition:
            raise visitor.syntax_error(f"cannot {description} unbound Union", node)
        result_types: List[Class] = []
        try:
            for el in self.klass.type_args:
                callback(el)
                result_types.append(visitor.get_type(node).klass)
        except TypedSyntaxError as e:
            raise visitor.syntax_error(f"'{self.name}': {e.msg}", node)

        union = UNION_TYPE.make_generic_type(
            tuple(result_types), visitor.symtable.generic_types
        )
        visitor.set_type(node, union.instance)

    def bind_attr(
        self, node: ast.Attribute, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        def cb(el: Class) -> None:
            return el.instance.bind_attr(node, visitor, type_ctx)
        self._generic_bind(
            node,
            cb,
            "access attribute from",
            visitor,
        )

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        def cb(el: Class) -> NarrowingEffect:
            return el.instance.bind_call(node, visitor, type_ctx)
        self._generic_bind(node, cb, "call", visitor)
        return NO_EFFECT

    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        def cb(el: Class) -> None:
            return el.instance.bind_subscr(node, type, visitor)
        self._generic_bind(node, cb, "subscript", visitor)

    def bind_unaryop(
        self, node: ast.UnaryOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        def cb(el: Class) -> None:
            return el.instance.bind_unaryop(node, visitor, type_ctx)
        self._generic_bind(
            node,
            cb,
            "unary op",
            visitor,
        )


class OptionalType(UnionType):
    """UnionType for instantiations with [T, None], and to support Optional[T] special form."""

    is_variadic = False

    def __init__(
        self,
        type_name: Optional[UnionTypeName] = None,
        type_def: Optional[GenericClass] = None,
    ) -> None:
        super().__init__(
            type_name
            or UnionTypeName("typing", "Optional", (GenericParameter("T", 0),)),
            type_def=type_def,
            instance_type=OptionalInstance,
        )

    @property
    def opt_type(self) -> Class:
        opt_type = self.type_name.opt_type
        if opt_type is None:
            params = ", ".join(t.name for t in self.type_args)
            raise TypeError(f"OptionalType has invalid type parameters {params}")
        return opt_type

    def make_generic_type(
        self, index: Tuple[Class, ...], generic_types: GenericTypesDict
    ) -> Class:
        assert len(index) == 1
        if not index[0].is_generic_parameter:
            # Optional[T] is syntactic sugar for Union[T, None]
            index = index + (NONE_TYPE,)
        return super().make_generic_type(index, generic_types)


class OptionalInstance(UnionInstance):
    """Only exists for typing purposes (so we know .klass is OptionalType)."""

    klass: OptionalType


class ArrayInstance(Object["ArrayClass"]):
    def _seq_type(self) -> int:
        idx = self.klass.index
        if not isinstance(idx, CIntType):
            # should never happen
            raise SyntaxError(f"Invalid Array type: {idx}")
        size = idx.size
        if size == 0:
            return SEQ_ARRAY_INT8 if idx.signed else SEQ_ARRAY_UINT8
        elif size == 1:
            return SEQ_ARRAY_INT16 if idx.signed else SEQ_ARRAY_UINT16
        elif size == 2:
            return SEQ_ARRAY_INT32 if idx.signed else SEQ_ARRAY_UINT32
        elif size == 3:
            return SEQ_ARRAY_INT64 if idx.signed else SEQ_ARRAY_UINT64
        else:
            raise SyntaxError(f"Invalid Array size: {size}")

    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        if type == SLICE_TYPE.instance:
            # Slicing preserves type
            return visitor.set_type(node, self)

        visitor.set_type(node, self.klass.index.instance)

    def emit_subscr(
        self, node: ast.Subscript, aug_flag: bool, code_gen: Static38CodeGenerator
    ) -> None:
        index_type = code_gen.get_type(node.slice)
        is_del = isinstance(node.ctx, ast.Del)
        index_is_python_int = INT_TYPE.can_assign_from(index_type.klass)
        index_is_primitive_int = isinstance(index_type.klass, CIntType)

        # ARRAY_{GET,SET} support only integer indices and don't support del;
        # otherwise defer to the usual bytecode
        if is_del or not (index_is_python_int or index_is_primitive_int):
            return super().emit_subscr(node, aug_flag, code_gen)

        code_gen.update_lineno(node)
        code_gen.visit(node.value)
        code_gen.visit(node.slice)

        if index_is_python_int:
            # If the index is not a primitive, unbox its value to an int64, our implementation of
            # SEQUENCE_{GET/SET} expects the index to be a primitive int.
            code_gen.emit("INT_UNBOX", INT64_TYPE.instance.as_oparg())

        if isinstance(node.ctx, ast.Store) and not aug_flag:
            code_gen.emit("SEQUENCE_SET", self._seq_type())
        elif isinstance(node.ctx, ast.Load) or aug_flag:
            if aug_flag:
                code_gen.emit("DUP_TOP_TWO")
            code_gen.emit("SEQUENCE_GET", self._seq_type())

    def emit_store_subscr(
        self, node: ast.Subscript, code_gen: Static38CodeGenerator
    ) -> None:
        code_gen.emit("ROT_THREE")
        code_gen.emit("SEQUENCE_SET", self._seq_type())

    def __repr__(self) -> str:
        return f"Array[{self.klass.index.name!r}]"

    def get_fast_len_type(self) -> int:
        return FAST_LEN_ARRAY | ((not self.klass.is_exact) << 4)

    def emit_len(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        if len(node.args) != 1:
            raise code_gen.syntax_error(
                "Can only pass a single argument when checking array length", node
            )
        code_gen.visit(node.args[0])
        code_gen.emit("FAST_LEN", self.get_fast_len_type())
        signed = False
        code_gen.emit("INT_BOX", int(signed))


class ArrayClass(GenericClass):
    def __init__(
        self,
        name: GenericTypeName,
        bases: Optional[List[TypeRef]] = None,
        instance: Optional[Object[Class]] = None,
        klass: Optional[Class] = None,
        members: Optional[Dict[str, Value]] = None,
        type_def: Optional[GenericClass] = None,
        is_exact: bool = False,
        pytype: Optional[Type[object]] = None,
    ) -> None:
        default_bases: List[TypeRef] = [RESOLVED_OBJECT_TYPE]
        default_instance: Object[Class] = ArrayInstance(self)
        super().__init__(
            name,
            bases or default_bases,
            instance or default_instance,
            klass,
            members,
            type_def,
            is_exact,
            pytype,
        )

    @property
    def index(self) -> Class:
        return self.type_args[0]

    def make_generic_type(
        self, index: Tuple[Class, ...], generic_types: GenericTypesDict
    ) -> Class:
        for tp in index:
            if tp not in ALLOWED_ARRAY_TYPES:
                raise TypedSyntaxError(f"Invalid array element type: {tp.name}")
        return super().make_generic_type(index, generic_types)


BUILTIN_GENERICS: Dict[Class, Dict[GenericTypeIndex, Class]] = {}
UNION_TYPE = UnionType()
OPTIONAL_TYPE = OptionalType()
CHECKED_DICT_TYPE_NAME = GenericTypeName(
    "__static__", "chkdict", (GenericParameter("K", 0), GenericParameter("V", 1))
)


class CheckedDict(GenericClass):
    def __init__(
        self,
        name: GenericTypeName,
        bases: Optional[List[TypeRef]] = None,
        instance: Optional[Object[Class]] = None,
        klass: Optional[Class] = None,
        members: Optional[Dict[str, Value]] = None,
        type_def: Optional[GenericClass] = None,
        is_exact: bool = False,
        pytype: Optional[Type[object]] = None,
    ) -> None:
        if instance is None:
            instance = CheckedDictInstance(self)
        super().__init__(
            name,
            bases,
            instance,
            klass,
            members,
            type_def,
            is_exact,
            pytype,
        )


class CheckedDictInstance(Object[CheckedDict]):
    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        visitor.visit(node.slice, self.klass.gen_name.args[0].instance)
        visitor.set_type(node, self.klass.gen_name.args[1].instance)

    def emit_subscr(
        self, node: ast.Subscript, aug_flag: bool, code_gen: Static38CodeGenerator
    ) -> None:
        if isinstance(node.ctx, ast.Load):
            code_gen.visit(node.value)
            code_gen.visit(node.slice)
            dict_descr = self.klass.type_descr
            update_descr = dict_descr + ("__getitem__",)
            code_gen.emit_invoke_method(update_descr, 1)
        elif isinstance(node.ctx, ast.Store):
            code_gen.visit(node.value)
            code_gen.emit("ROT_TWO")
            code_gen.visit(node.slice)
            code_gen.emit("ROT_TWO")
            dict_descr = self.klass.type_descr
            setitem_descr = dict_descr + ("__setitem__",)
            code_gen.emit_invoke_method(setitem_descr, 2)
            code_gen.emit("POP_TOP")
        else:
            code_gen.defaultVisit(node, aug_flag)

    def get_fast_len_type(self) -> int:
        return FAST_LEN_DICT | ((not self.klass.is_exact) << 4)

    def emit_len(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        if len(node.args) != 1:
            raise code_gen.syntax_error(
                "Can only pass a single argument when checking dict length", node
            )
        code_gen.visit(node.args[0])
        code_gen.emit("FAST_LEN", self.get_fast_len_type())
        signed = False
        code_gen.emit("INT_BOX", int(signed))

    def emit_jumpif(
        self, test: AST, next: Block, is_if_true: bool, code_gen: Static38CodeGenerator
    ) -> None:
        code_gen.visit(test)
        code_gen.emit("FAST_LEN", self.get_fast_len_type())
        code_gen.emit("POP_JUMP_IF_NONZERO" if is_if_true else "POP_JUMP_IF_ZERO", next)


CHECKED_DICT_TYPE = CheckedDict(
    CHECKED_DICT_TYPE_NAME, [RESOLVED_OBJECT_TYPE], pytype=chkdict
)
CHECKED_DICT_EXACT_TYPE = CheckedDict(
    CHECKED_DICT_TYPE_NAME, [RESOLVED_OBJECT_TYPE], pytype=chkdict, is_exact=True
)


class CastFunction(Object[Class]):
    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        if len(node.args) != 2:
            raise visitor.syntax_error(
                "cast requires two parameters: type and value", node
            )

        for arg in node.args:
            visitor.visit(arg)
        self.check_args_for_primitives(node, visitor)

        cast_type = visitor.cur_mod.resolve_annotation(node.args[0])
        if cast_type is None:
            raise visitor.syntax_error("cast to unknown type", node)

        visitor.set_type(node, cast_type.instance)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        code_gen.visit(node.args[1])
        code_gen.emit("CAST", code_gen.get_type(node).klass.type_descr)


# These need to be kept in sync with the version in classloader.h:
TYPED_INT_8BIT = 0
TYPED_INT_16BIT = 1
TYPED_INT_32BIT = 2
TYPED_INT_64BIT = 3
TYPED_OBJECT = 0xFF
TYPED_ARRAY = 0x80

TYPED_INT_UNSIGNED = 0
TYPED_INT_SIGNED = 1

TYPED_INT8: int = (TYPED_INT_8BIT << 1) | TYPED_INT_SIGNED
TYPED_INT16: int = (TYPED_INT_16BIT << 1) | TYPED_INT_SIGNED
TYPED_INT32: int = (TYPED_INT_32BIT << 1) | TYPED_INT_SIGNED
TYPED_INT64: int = (TYPED_INT_64BIT << 1) | TYPED_INT_SIGNED

TYPED_UINT8: int = (TYPED_INT_8BIT << 1) | TYPED_INT_UNSIGNED
TYPED_UINT16: int = (TYPED_INT_16BIT << 1) | TYPED_INT_UNSIGNED
TYPED_UINT32: int = (TYPED_INT_32BIT << 1) | TYPED_INT_UNSIGNED
TYPED_UINT64: int = (TYPED_INT_64BIT << 1) | TYPED_INT_UNSIGNED

SEQ_LIST: int = 0
SEQ_TUPLE: int = 1
SEQ_ARRAY_INT8: int = (TYPED_INT8 << 4) | TYPED_ARRAY
SEQ_ARRAY_INT16: int = (TYPED_INT16 << 4) | TYPED_ARRAY
SEQ_ARRAY_INT32: int = (TYPED_INT32 << 4) | TYPED_ARRAY
SEQ_ARRAY_INT64: int = (TYPED_INT64 << 4) | TYPED_ARRAY
SEQ_ARRAY_UINT8: int = (TYPED_UINT8 << 4) | TYPED_ARRAY
SEQ_ARRAY_UINT16: int = (TYPED_UINT16 << 4) | TYPED_ARRAY
SEQ_ARRAY_UINT32: int = (TYPED_UINT32 << 4) | TYPED_ARRAY
SEQ_ARRAY_UINT64: int = (TYPED_UINT64 << 4) | TYPED_ARRAY

PRIM_OP_EQ = 0
PRIM_OP_NE = 1
PRIM_OP_LT = 2
PRIM_OP_LE = 3
PRIM_OP_GT = 4
PRIM_OP_GE = 5
PRIM_OP_LT_UN = 6
PRIM_OP_LE_UN = 7
PRIM_OP_GT_UN = 8
PRIM_OP_GE_UN = 9

PRIM_OP_ADD = 0
PRIM_OP_SUB = 1
PRIM_OP_MUL = 2
PRIM_OP_DIV = 3
PRIM_OP_DIV_UN = 4
PRIM_OP_MOD = 5
PRIM_OP_MOD_UN = 6
PRIM_OP_POW = 7
PRIM_OP_LSHIFT = 8
PRIM_OP_RSHIFT = 9
PRIM_OP_RSHIFT_UN = 10
PRIM_OP_XOR = 11
PRIM_OP_OR = 12
PRIM_OP_AND = 13

PRIM_OP_NEG = 0
PRIM_OP_INV = 1

FAST_LEN_INEXACT: int = 1 << 4
FAST_LEN_LIST = 0
FAST_LEN_DICT = 1
FAST_LEN_SET = 2
FAST_LEN_TUPLE = 3
FAST_LEN_ARRAY = 4


class CInstance(Value, Generic[TClass]):
    pass


class CIntInstance(CInstance["CIntType"]):
    def __init__(self, klass: CIntType, size: int, signed: bool) -> None:
        super().__init__(klass)
        self.size = size
        self.signed = signed

    def as_oparg(self) -> int:
        return (self.size << 1) | int(self.signed)

    @property
    def name(self) -> str:
        return self.klass.name

    _int_binary_opcode_signed: Mapping[Type[ast.AST], int] = {
        ast.Lt: PRIM_OP_LT,
        ast.Gt: PRIM_OP_GT,
        ast.Eq: PRIM_OP_EQ,
        ast.NotEq: PRIM_OP_NE,
        ast.LtE: PRIM_OP_LE,
        ast.GtE: PRIM_OP_GE,
        ast.Add: PRIM_OP_ADD,
        ast.Sub: PRIM_OP_SUB,
        ast.Mult: PRIM_OP_MUL,
        ast.FloorDiv: PRIM_OP_DIV,
        ast.Div: PRIM_OP_DIV,
        ast.Mod: PRIM_OP_MOD,
        ast.LShift: PRIM_OP_LSHIFT,
        ast.RShift: PRIM_OP_RSHIFT,
        ast.BitOr: PRIM_OP_OR,
        ast.BitXor: PRIM_OP_XOR,
        ast.BitAnd: PRIM_OP_AND,
    }

    _int_binary_opcode_unsigned: Mapping[Type[ast.AST], int] = {
        ast.Lt: PRIM_OP_LT_UN,
        ast.Gt: PRIM_OP_GT_UN,
        ast.Eq: PRIM_OP_EQ,
        ast.NotEq: PRIM_OP_NE,
        ast.LtE: PRIM_OP_LE_UN,
        ast.GtE: PRIM_OP_GE_UN,
        ast.Add: PRIM_OP_ADD,
        ast.Sub: PRIM_OP_SUB,
        ast.Mult: PRIM_OP_MUL,
        ast.FloorDiv: PRIM_OP_DIV_UN,
        ast.Div: PRIM_OP_DIV_UN,
        ast.Mod: PRIM_OP_MOD_UN,
        ast.LShift: PRIM_OP_LSHIFT,
        ast.RShift: PRIM_OP_RSHIFT,
        ast.RShift: PRIM_OP_RSHIFT_UN,
        ast.BitOr: PRIM_OP_OR,
        ast.BitXor: PRIM_OP_XOR,
        ast.BitAnd: PRIM_OP_AND,
    }

    _op_name: Dict[Type[ast.operator], str] = {
        ast.Add: "add",
        ast.Sub: "subtract",
        ast.Mult: "multiply",
        ast.FloorDiv: "divide",
        ast.Div: "divide",
        ast.Mod: "modulus",
        ast.LShift: "left shift",
        ast.RShift: "right shift",
        ast.BitOr: "bitwise or",
        ast.BitXor: "xor",
        ast.BitAnd: "bitwise and",
    }

    def get_op_id(self, op: AST) -> int:
        return (
            self._int_binary_opcode_signed[type(op)]
            if self.signed
            else (self._int_binary_opcode_unsigned[type(op)])
        )

    def validate_mixed_math(self, other: Value) -> Optional[Value]:
        if other is self:
            return self
        elif isinstance(other, CIntInstance):
            if self.signed == other.signed:
                # signs match, we can just treat this as a comparison of the larger type
                if self.size > other.size:
                    return self
                else:
                    return other
            else:
                new_size = max(
                    self.size if self.signed else self.size + 1,
                    other.size if other.signed else other.size + 1,
                )

                if new_size <= TYPED_INT_64BIT:
                    # signs don't match, but we can promote to the next highest data type
                    return SIGNED_CINT_TYPES[new_size].instance

        return None

    def bind_compare(
        self,
        node: ast.Compare,
        left: expr,
        op: cmpop,
        right: expr,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> bool:
        if visitor.get_type(right) != self:
            try:
                visitor.visit(right, type_ctx or INT64_VALUE)
            except TypedSyntaxError:
                # Report a better error message than the generic can't be used
                raise visitor.syntax_error(
                    f"can't compare {self.name} to {visitor.get_type(right).name}",
                    node,
                )

        compare_type = self.validate_mixed_math(visitor.get_type(right))
        if compare_type is None:
            raise visitor.syntax_error(
                f"can't compare {self.name} to {visitor.get_type(right).name}", node
            )

        visitor.set_type(op, compare_type)
        visitor.set_type(node, INT64_TYPE.instance)
        return True

    def bind_reverse_compare(
        self,
        node: ast.Compare,
        left: expr,
        op: cmpop,
        right: expr,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
    ) -> bool:
        if not isinstance(visitor.get_type(left), CIntInstance):
            try:
                visitor.visit(left, type_ctx or INT64_VALUE)
            except TypedSyntaxError:
                # Report a better error message than the generic can't be used
                raise visitor.syntax_error(
                    f"can't compare {self.name} to {visitor.get_type(right).name}", node
                )

            compare_type = self.validate_mixed_math(visitor.get_type(left))
            if compare_type is None:
                raise visitor.syntax_error(
                    f"can't compare {visitor.get_type(left).name} to {self.name}", node
                )

            visitor.set_type(op, compare_type)
            visitor.set_type(node, INT64_TYPE.instance)
            return True

        return False

    def emit_compare(self, op: cmpop, code_gen: Static38CodeGenerator) -> None:
        code_gen.emit("INT_COMPARE_OP", self.get_op_id(op))

    def emit_binop(self, node: ast.BinOp, code_gen: Static38CodeGenerator) -> None:
        code_gen.update_lineno(node)
        code_gen.visit(node.left)
        code_gen.visit(node.right)
        code_gen.emit("INT_BINARY_OP", self.get_op_id(node.op))

    def emit_augassign(
        self, node: ast.AugAssign, code_gen: Static38CodeGenerator
    ) -> None:
        code_gen.set_lineno(node)
        aug_node = wrap_aug(node.target)
        code_gen.visit(aug_node, "load")
        code_gen.visit(node.value)
        code_gen.emit("INT_BINARY_OP", self.get_op_id(node.op))
        code_gen.visit(aug_node, "store")

    def emit_augname(
        self, node: AugName, code_gen: Static38CodeGenerator, mode: str
    ) -> None:
        if mode == "load":
            code_gen.emit("LOAD_LOCAL", (node.id, ("__static__", self.klass.name)))
        elif mode == "store":
            code_gen.emit("STORE_LOCAL", (node.id, ("__static__", self.klass.name)))

    def validate_int(self, val: object, node: ast.AST, visitor: TypeBinder) -> None:
        if not isinstance(val, int):
            raise visitor.syntax_error(
                f"{type(val).__name__} cannot be used in a context where an int is expected",
                node,
            )

        bits = 8 << self.size
        if self.signed:
            low = -(1 << (bits - 1))
            high = (1 << (bits - 1)) - 1
        else:
            low = 0
            high = 1 << bits

        if not low <= val <= high:
            raise visitor.syntax_error(
                f"constant {val} is outside of the range {low} to {high} for {self.name}",
                node,
            )

    def bind_constant(self, node: ast.Constant, visitor: TypeBinder) -> None:
        self.validate_int(node.value, node, visitor)
        visitor.set_type(node, self)

    def emit_constant(
        self, node: ast.Constant, code_gen: Static38CodeGenerator
    ) -> None:
        if node.value < 0:
            code_gen.emit("INT_LOAD_CONST", -node.value)
            code_gen.emit("INT_UNARY_OP", PRIM_OP_NEG)
        elif node.value > 0x7FFFFFFF:
            code_gen.emit("LOAD_CONST", node.value)
            code_gen.emit("INT_UNBOX", self.as_oparg())
        else:
            code_gen.emit("INT_LOAD_CONST", node.value)

    def bind_num(self, node: ast.Num, visitor: TypeBinder) -> None:
        self.validate_int(node.n, node, visitor)
        visitor.set_type(node, self)

    def emit_num(self, node: ast.Num, code_gen: Static38CodeGenerator) -> None:
        # pyre-fixme[6]: `>` is not supported for operand types `complex` and `int`.
        if node.n > 0x7FFFFFFF:
            code_gen.emit("LOAD_CONST", node.n)
            code_gen.emit("INT_UNBOX", self.as_oparg())
        else:
            code_gen.emit("INT_LOAD_CONST", node.n)

    def emit_name(self, node: ast.Name, code_gen: Static38CodeGenerator) -> None:
        if isinstance(node.ctx, ast.Load):
            code_gen.emit("LOAD_LOCAL", (node.id, ("__static__", self.klass.name)))
        elif isinstance(node.ctx, ast.Store):
            code_gen.emit("STORE_LOCAL", (node.id, ("__static__", self.klass.name)))
        else:
            raise TypedSyntaxError("unsupported op")

    def emit_jumpif(
        self, test: AST, next: Block, is_if_true: bool, code_gen: Static38CodeGenerator
    ) -> None:
        code_gen.visit(test)
        code_gen.emit("POP_JUMP_IF_NONZERO" if is_if_true else "POP_JUMP_IF_ZERO", next)

    def emit_jumpif_pop(
        self, test: AST, next: Block, is_if_true: bool, code_gen: Static38CodeGenerator
    ) -> None:
        code_gen.visit(test)
        code_gen.emit(
            "JUMP_IF_NONZERO_OR_POP" if is_if_true else "JUMP_IF_ZERO_OR_POP", next
        )

    def bind_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        if visitor.get_type(node.right) != self:
            try:
                visitor.visit(node.right, type_ctx or INT64_VALUE)
            except TypedSyntaxError:
                # Report a better error message than the generic can't be used
                raise visitor.syntax_error(
                    self.binop_error(self, visitor.get_type(node.right), node.op),
                    node,
                )

        bin_type = self.validate_mixed_math(visitor.get_type(node.right))
        if bin_type is None:
            raise visitor.syntax_error(
                self.binop_error(self, visitor.get_type(node.right), node.op),
                node,
            )

        visitor.set_type(node, bin_type)
        return True

    def binop_error(self, left: Value, right: Value, op: ast.operator) -> str:
        return f"cannot {self._op_name[type(op)]} {left.name} and {right.name}"

    def bind_reverse_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> bool:
        try:
            visitor.visit(node.left, self)
        except TypedSyntaxError:
            raise visitor.syntax_error(
                self.binop_error(visitor.get_type(node.left), self, node.op), node
            )
        visitor.set_type(node, self)
        return True

    def emit_box(self, node: expr, code_gen: Static38CodeGenerator) -> None:
        code_gen.visit(node)
        type = code_gen.get_type(node)
        if isinstance(type, CIntInstance):
            code_gen.emit("INT_BOX", int(type.signed))
        else:
            raise RuntimeError("unsupported box type: " + type.name)

    def emit_unbox(self, node: expr, code_gen: Static38CodeGenerator) -> None:
        code_gen.visit(node)
        code_gen.emit("INT_UNBOX", self.as_oparg())

    def bind_unaryop(
        self, node: ast.UnaryOp, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> None:
        if isinstance(node.op, (ast.USub, ast.Invert, ast.UAdd)):
            visitor.set_type(node, self)
        else:
            assert isinstance(node.op, ast.Not)
            visitor.set_type(node, BOOL_TYPE.instance)

    def emit_unaryop(self, node: ast.UnaryOp, code_gen: Static38CodeGenerator) -> None:
        code_gen.update_lineno(node)
        if isinstance(node.op, ast.USub):
            code_gen.visit(node.operand)
            code_gen.emit("INT_UNARY_OP", PRIM_OP_NEG)
        elif isinstance(node.op, ast.Invert):
            code_gen.visit(node.operand)
            code_gen.emit("INT_UNARY_OP", PRIM_OP_INV)
        elif isinstance(node.op, ast.UAdd):
            code_gen.visit(node.operand)
        elif isinstance(node.op, ast.Not):
            raise NotImplementedError()


class CIntType(CType):
    instance: CIntInstance

    def __init__(
        self, size: int, signed: bool, name_override: Optional[str] = None
    ) -> None:
        if name_override is None:
            name = ("" if signed else "u") + "int" + str(8 << size)
        else:
            name = name_override
        self.size = size
        self.signed = signed
        super().__init__(
            TypeName("__static__", name),
            [],
            CIntInstance(self, size, signed),
        )

    @property
    def name(self) -> str:
        return self.type_name.name

    def can_assign_from(self, src: Class) -> bool:
        if isinstance(src, CIntType):
            if src.size <= self.size and src.signed == self.signed:
                # assignment to same or larger size, with same sign
                # is allowed
                return True
            if src.size < self.size and self.signed:
                # assignment to larger signed size from unsigned is
                # allowed
                return True

        return super().can_assign_from(src)

    @property
    def type_descr(self) -> TypeDescr:
        return ("__static__", self.name)

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        if len(node.args) != 1:
            raise visitor.syntax_error(
                f"{self.name} requires a single argument ({len(node.args)} given)", node
            )

        return super().bind_call(node, visitor, type_ctx)

    def emit_call(self, node: ast.Call, code_gen: Static38CodeGenerator) -> None:
        if len(node.args) != 1:
            raise code_gen.syntax_error(
                f"{self.name} requires a single argument ({len(node.args)} given)", node
            )

        arg = node.args[0]
        arg_type = code_gen.get_type(arg)
        if isinstance(arg, Constant):
            return self.instance.emit_constant(arg, code_gen)
        elif isinstance(arg, Num):
            return self.instance.emit_num(arg, code_gen)
        elif isinstance(arg_type, CIntInstance):
            code_gen.visit(arg)
            if self.size < arg_type.size:
                if self.signed and arg_type.signed:
                    extend_sign = 1
                else:
                    extend_sign = 0
                # The lower nibble of oparg indicates whether sign extension is needed. The higher
                # nibble indicates the size to be truncated to.
                code_gen.emit("CONVERT_PRIMITIVE", (self.size << 4) | extend_sign)
        else:
            return super().emit_call(node, code_gen)


class DoubleInstance(CInstance["DoubleType"]):
    pass


class DoubleType(CType):
    def __init__(self) -> None:
        super().__init__(
            TypeName("__static__", "double"),
            [RESOLVED_OBJECT_TYPE],
            DoubleInstance(self),
        )


CBOOL_TYPE = CIntType(TYPED_INT_8BIT, True, name_override="cbool")

INT8_TYPE = CIntType(TYPED_INT_8BIT, True)
INT16_TYPE = CIntType(TYPED_INT_16BIT, True)
INT32_TYPE = CIntType(TYPED_INT_32BIT, True)
INT64_TYPE = CIntType(TYPED_INT_64BIT, True)

UINT8_TYPE = CIntType(TYPED_INT_8BIT, False)
UINT16_TYPE = CIntType(TYPED_INT_16BIT, False)
UINT32_TYPE = CIntType(TYPED_INT_32BIT, False)
UINT64_TYPE = CIntType(TYPED_INT_64BIT, False)

INT64_VALUE = INT64_TYPE.instance

CHAR_TYPE = CIntType(TYPED_INT_8BIT, True, name_override="char")
DOUBLE_TYPE = DoubleType()
ARRAY_TYPE = ArrayClass(
    GenericTypeName("__static__", "Array", (GenericParameter("T", 0),))
)
ARRAY_EXACT_TYPE = ArrayClass(
    GenericTypeName("__static__", "Array", (GenericParameter("T", 0),)), is_exact=True
)

ALLOWED_ARRAY_TYPES: List[Class] = [
    INT8_TYPE,
    INT16_TYPE,
    INT32_TYPE,
    INT64_TYPE,
    UINT8_TYPE,
    UINT16_TYPE,
    UINT32_TYPE,
    UINT64_TYPE,
    CHAR_TYPE,
    DOUBLE_TYPE,
    FLOAT_TYPE,
]

SIGNED_CINT_TYPES = [INT8_TYPE, INT16_TYPE, INT32_TYPE, INT64_TYPE]
UNSIGNED_CINT_TYPES: List[CIntType] = [
    UINT8_TYPE,
    UINT16_TYPE,
    UINT32_TYPE,
    UINT64_TYPE,
]
ALL_CINT_TYPES: Sequence[CIntType] = SIGNED_CINT_TYPES + UNSIGNED_CINT_TYPES

EXACT_TYPES: Mapping[Class, Class] = {
    ARRAY_TYPE: ARRAY_EXACT_TYPE,
    LIST_TYPE: LIST_EXACT_TYPE,
    TUPLE_TYPE: TUPLE_EXACT_TYPE,
    INT_TYPE: INT_EXACT_TYPE,
    DICT_TYPE: DICT_EXACT_TYPE,
    CHECKED_DICT_TYPE: CHECKED_DICT_EXACT_TYPE,
    SET_TYPE: SET_EXACT_TYPE,
}

INEXACT_TYPES: Mapping[Class, Class] = {v: k for k, v in EXACT_TYPES.items()}


def exact(maybe_inexact: Class) -> Class:
    exact = EXACT_TYPES.get(maybe_inexact)
    return exact or maybe_inexact


def inexact(maybe_exact: Class) -> Class:
    inexact = INEXACT_TYPES.get(maybe_exact)
    return inexact or maybe_exact


if spamobj is not None:
    SPAM_OBJ = GenericClass(
        GenericTypeName("xxclassloader", "spamobj", (GenericParameter("T", 0),)),
        pytype=spamobj,
    )
else:
    SPAM_OBJ: Optional[GenericClass] = None


class GenericVisitor(ASTVisitor):
    def __init__(self, module_name: str, filename: str) -> None:
        super().__init__()
        self.module_name = module_name
        self.filename = filename

    def syntax_error(self, msg: str, node: AST) -> TypedSyntaxError:
        source_line = linecache.getline(self.filename, node.lineno)
        return TypedSyntaxError(
            msg, (self.filename, node.lineno, node.col_offset, source_line or None)
        )


class DeclarationVisitor(GenericVisitor):
    def __init__(self, mod_name: str, filename: str, symbols: SymbolTable) -> None:
        super().__init__(mod_name, filename)
        self.module = symbols[mod_name] = ModuleTable(mod_name, symbols)

    def visitClassDef(self, node: ClassDef) -> None:
        bases = [TypeRef(self.module, base) for base in node.bases]
        if not bases:
            bases.append(RESOLVED_OBJECT_TYPE)
        klass = self.module.children[node.name] = Class(
            TypeName(self.module_name, node.name), bases
        )
        self.module.decls.append((node, klass))
        for item in node.body:
            if isinstance(item, (AsyncFunctionDef, FunctionDef)):
                klass.define_function(item.name, item, self)
                if item.name != "__init__" or not item.args.args:
                    continue

                for func_item in item.body:

                    if isinstance(func_item, AnnAssign):
                        target = func_item.target
                        if isinstance(target, Attribute):
                            value = target.value
                            if (
                                isinstance(value, ast.Name)
                                and value.id == item.args.args[0].arg
                            ):
                                attr = target.attr
                                klass.define_slot(
                                    attr, TypeRef(self.module, func_item.annotation)
                                )
                    elif isinstance(func_item, Assign):
                        for target in func_item.targets:
                            if not isinstance(target, Attribute):
                                continue
                            value = target.value
                            if (
                                isinstance(value, ast.Name)
                                and value.id == item.args.args[0].arg
                            ):
                                attr = target.attr
                                klass.define_slot(attr)

            elif isinstance(item, AnnAssign):
                # class C:
                #    x: foo
                target = item.target
                if isinstance(target, ast.Name):
                    klass.define_slot(target.id, TypeRef(self.module, item.annotation))

    def _visitFunc(self, node: Union[FunctionDef, AsyncFunctionDef]) -> None:
        # Ignore functions with decorators, we don't know what they return yet
        if not node.decorator_list:
            function = self.module.children[node.name] = Function(
                f"{self.module.name}.{node.name}",
                None,
                node,
                self.type_ref(node.returns),
                (self.module.name, node.name),
            )
            self.module.decls.append((node, function))
            process_function_args(function, node.args, self)

    def visitFunctionDef(self, node: FunctionDef) -> None:
        self._visitFunc(node)

    def visitAsyncFunctionDef(self, node: AsyncFunctionDef) -> None:
        self._visitFunc(node)

    def type_ref(self, ann: Optional[expr]) -> TypeRef:
        if not ann:
            return ResolvedTypeRef(DYNAMIC_TYPE)
        return TypeRef(self.module, ann)

    def visitImport(self, node: Import) -> None:
        self.module.decls.append((node, None))

    def visitImportFrom(self, node: ImportFrom) -> None:
        self.module.decls.append((node, None))

    # We don't pick up declarations in nested statements
    def visitFor(self, node: For) -> None:
        pass

    def visitAsyncFor(self, node: AsyncFor) -> None:
        pass

    def visitWhile(self, node: While) -> None:
        pass

    def visitIf(self, node: If) -> None:
        test = node.test
        if isinstance(test, Name) and test.id == "TYPE_CHECKING":
            self.visit(node.body)

    def visitWith(self, node: With) -> None:
        pass

    def visitAsyncWith(self, node: AsyncWith) -> None:
        pass

    def visitTry(self, node: Try) -> None:
        pass


class TypedSyntaxError(SyntaxError):
    pass


class LocalsBranch:
    """Handles branching and merging local variable types"""

    def __init__(self, scope: BindingScope) -> None:
        self.scope = scope
        self.entry_locals: Dict[str, Value] = dict(scope.local_types)

    def copy(self) -> Dict[str, Value]:
        """Make a copy of the current local state"""
        return dict(self.scope.local_types)

    def restore(self, state: Optional[Dict[str, Value]] = None) -> None:
        """Restore the locals to the state when we entered"""
        self.scope.local_types.clear()
        self.scope.local_types.update(state or self.entry_locals)

    def merge(self, entry_locals: Optional[Dict[str, Value]] = None) -> None:
        """Merge the entry locals, or a specific copy, into the current locals"""
        # TODO: What about del's?
        if entry_locals is None:
            entry_locals = self.entry_locals
        local_types = self.scope.local_types
        for key, value in entry_locals.items():
            if key in local_types:
                if value != local_types[key]:
                    widest = self._widest_type(value, local_types[key])
                    local_types[key] = (
                        widest or self.scope.decl_types[key].type.instance
                    )
                continue

        for key in local_types.keys():
            # If a value isn't definitely assigned we can safely turn it
            # back into the declared type
            if key not in entry_locals and key in self.scope.decl_types:
                local_types[key] = self.scope.decl_types[key].type.instance

    def _widest_type(self, *types: Value) -> Optional[Value]:
        # TODO: this should be a join, rather than just reverting to decl_type
        # if neither type is greater than the other
        if len(types) == 1:
            return types[0]

        widest_type = None
        for src in types:
            if src == DYNAMIC:
                return DYNAMIC

            if widest_type is None or src.klass.can_assign_from(widest_type.klass):
                widest_type = src
            elif widest_type is not None and not widest_type.klass.can_assign_from(
                src.klass
            ):
                return None

        return widest_type


class TypeDeclaration:
    def __init__(self, type: Class, node: AST) -> None:
        self.type = type
        self.node = node


class BindingScope:
    def __init__(self, node: AST, types: Optional[Dict[str, Value]] = None) -> None:
        self.node = node
        self.local_types: Dict[str, Value] = types or {}
        self.decl_types: Dict[str, TypeDeclaration] = {}

    def branch(self) -> LocalsBranch:
        return LocalsBranch(self)


class NarrowingEffect:
    """captures type narrowing effects on variables"""

    def union(self, other: NarrowingEffect) -> NarrowingEffect:
        if other is NoEffect:
            return self

        return CombinedEffect(self, other)

    def apply(self, local_types: Dict[str, Value]) -> None:
        """applies the given effect in the target scope"""
        pass

    def undo(self, local_types: Dict[str, Value]) -> None:
        """restores the type to its original value"""
        pass

    def reverse(self, local_types: Dict[str, Value]) -> None:
        """applies the reverse of the scope or reverts it if
        there is no reverse"""
        self.undo(local_types)


class CombinedEffect(NarrowingEffect):
    def __init__(self, *effects: NarrowingEffect) -> None:
        self.effects: Sequence[NarrowingEffect] = effects

    def union(self, other: NarrowingEffect) -> NarrowingEffect:
        if other is NoEffect:
            return self
        elif isinstance(other, CombinedEffect):
            return CombinedEffect(*self.effects, *other.effects)

        return CombinedEffect(*self.effects, other)

    def apply(self, local_types: Dict[str, Value]) -> None:
        for effect in self.effects:
            effect.apply(local_types)

    def undo(self, local_types: Dict[str, Value]) -> None:
        """restores the type to its original value"""
        for effect in self.effects:
            effect.undo(local_types)

    def reverse(self, local_types: Dict[str, Value]) -> None:
        """applies the reverse of the scope or reverts it if
        there is no reverse"""
        for effect in self.effects:
            effect.reverse(local_types)


class NoEffect(NarrowingEffect):
    def union(self, other: NarrowingEffect) -> NarrowingEffect:
        return other


# Singleton instance for no effects
NO_EFFECT = NoEffect()


class NegationEffect(NarrowingEffect):
    def __init__(self, negated: NarrowingEffect) -> None:
        self.negated = negated

    def apply(self, local_types: Dict[str, Value]) -> None:
        self.negated.reverse(local_types)

    def undo(self, local_types: Dict[str, Value]) -> None:
        self.negated.undo(local_types)

    def reverse(self, local_types: Dict[str, Value]) -> None:
        self.negated.apply(local_types)


class IsInstanceEffect(NarrowingEffect):
    def __init__(self, var: str, prev: Value, inst: Value, visitor: TypeBinder) -> None:
        self.var = var
        self.prev = prev
        self.inst = inst
        reverse = prev
        if isinstance(prev, UnionInstance):
            type_args = tuple(
                ta for ta in prev.klass.type_args if not inst.klass.can_assign_from(ta)
            )
            reverse = UNION_TYPE.make_generic_type(
                type_args, visitor.symtable.generic_types
            ).instance
        self.rev: Value = reverse

    def apply(self, local_types: Dict[str, Value]) -> None:
        local_types[self.var] = self.inst

    def undo(self, local_types: Dict[str, Value]) -> None:
        local_types[self.var] = self.prev

    def reverse(self, local_types: Dict[str, Value]) -> None:
        local_types[self.var] = self.rev


class TypeBinder(GenericVisitor):
    """Walks an AST and produces an optionally strongly typed AST, reporting errors when
    operations are occuring that are not sound.  Strong types are based upon places where
    annotations occur which opt-in the strong typing"""

    def __init__(
        self,
        symbols: SymbolVisitor,
        filename: str,
        symtable: SymbolTable,
        module_name: str,
    ) -> None:
        super().__init__(module_name, filename)
        self.symbols = symbols
        self.scopes: List[BindingScope] = []
        self.symtable = symtable
        self.cur_mod: ModuleTable = symtable[module_name]
        self.returns: Set[AST] = set()

    @property
    def local_types(self) -> Dict[str, Value]:
        return self.scopes[-1].local_types

    @property
    def decl_types(self) -> Dict[str, TypeDeclaration]:
        return self.scopes[-1].decl_types

    @property
    def scope(self) -> AST:
        return self.scopes[-1].node

    def visit(
        self, node: Union[AST, Sequence[AST]], *args: object
    ) -> Optional[NarrowingEffect]:
        """This override is only here to give Pyre the return type information."""
        return super().visit(node, *args)

    def declare_local(self, target: ast.Name, type: Class) -> TypeDeclaration:
        if target.id in self.decl_types:
            raise self.syntax_error(
                f"Cannot redefine local variable {target.id}", target
            )

        self.decl_types[target.id] = decl_type = TypeDeclaration(type, target)
        if isinstance(type, CType):
            self.check_primitive_scope(target)
        return decl_type

    def check_static_import_flags(self, node: Module) -> None:
        saw_doc_str = False
        for stmt in node.body:
            if isinstance(stmt, ast.Expr):
                val = stmt.value
                if isinstance(val, ast.Constant) and isinstance(val.value, str):
                    if saw_doc_str:
                        break
                    saw_doc_str = True
                else:
                    break
            elif isinstance(stmt, ast.Import):
                continue
            elif isinstance(stmt, ast.ImportFrom):
                if stmt.module == "__static__.compiler_flags":
                    for name in stmt.names:
                        if name.name == "nonchecked_dicts":
                            self.cur_mod.nonchecked_dicts = True
                        elif name.name == "tinyframe":
                            self.cur_mod.tinyframe = True
                        elif name.name == "noframe":
                            self.cur_mod.noframe = True

    def visitModule(self, node: Module) -> None:
        self.scopes.append(BindingScope(node, self.cur_mod.children))

        self.check_static_import_flags(node)

        for stmt in node.body:
            self.visit(stmt)

        self.scopes.pop()

    def set_param(self, arg: ast.arg, arg_type: Class, scope: BindingScope) -> None:
        scope.local_types[arg.arg] = arg_type.instance
        scope.decl_types[arg.arg] = TypeDeclaration(arg_type, arg)
        if isinstance(arg_type, CIntType):
            raise TypedSyntaxError("Cannot use primitives in function arguments")
        self.set_type(arg, arg_type.instance)

    def _visitFunc(self, node: Union[FunctionDef, AsyncFunctionDef]) -> None:
        scope = BindingScope(node)
        for decorator in node.decorator_list:
            self.visit(decorator)
        cur_scope = self.scope
        if (
            not node.decorator_list
            and isinstance(cur_scope, ClassDef)
            and node.args.args
        ):
            # Handle type of "self"
            klass = self.cur_mod.resolve_name(cur_scope.name)
            if isinstance(klass, Class):
                self.set_param(node.args.args[0], klass, scope)
            else:
                self.set_param(node.args.args[0], DYNAMIC_TYPE, scope)

        for arg in node.args.posonlyargs:
            ann = arg.annotation
            if ann:
                self.visit(ann)
                arg_type = self.cur_mod.resolve_annotation(ann) or DYNAMIC_TYPE
            elif arg.arg in scope.decl_types:
                # Already handled self
                continue
            else:
                arg_type = DYNAMIC_TYPE
            self.set_param(arg, arg_type, scope)

        for arg in node.args.args:
            ann = arg.annotation
            if ann:
                self.visit(ann)
                arg_type = self.cur_mod.resolve_annotation(ann) or DYNAMIC_TYPE
            elif arg.arg in scope.decl_types:
                # Already handled self
                continue
            else:
                arg_type = DYNAMIC_TYPE

            self.set_param(arg, arg_type, scope)

        if node.args.defaults:
            for default in node.args.defaults:
                self.visit(default)

        if node.args.kw_defaults:
            for default in node.args.kw_defaults:
                if default is not None:
                    self.visit(default)

        vararg = node.args.vararg
        if vararg:
            ann = vararg.annotation
            if ann:
                self.visit(ann)

            self.set_param(vararg, TUPLE_EXACT_TYPE, scope)

        for arg in node.args.kwonlyargs:
            ann = arg.annotation
            if ann:
                self.visit(ann)
                arg_type = self.cur_mod.resolve_annotation(ann) or DYNAMIC_TYPE
            else:
                arg_type = DYNAMIC_TYPE

            self.set_param(arg, arg_type, scope)

        kwarg = node.args.kwarg
        if kwarg:
            ann = kwarg.annotation
            if ann:
                self.visit(ann)
            self.set_param(kwarg, DICT_EXACT_TYPE, scope)

        returns = node.returns
        if returns:
            # We store the return type on the node for the function as we otherwise
            # don't need to store type information for it
            expected = self.cur_mod.resolve_annotation(returns) or DYNAMIC_TYPE
            self.set_type(node, expected.instance)
            self.visit(returns)
        else:
            self.set_type(node, DYNAMIC)

        self.scopes.append(scope)

        for stmt in node.body:
            self.visit(stmt)

        self.scopes.pop()

    def visitFunctionDef(self, node: FunctionDef) -> None:
        self._visitFunc(node)

    def visitAsyncFunctionDef(self, node: AsyncFunctionDef) -> None:
        self._visitFunc(node)

    def visitClassDef(self, node: ClassDef) -> None:
        for decorator in node.decorator_list:
            self.visit(decorator)

        for kwarg in node.keywords:
            self.visit(kwarg.value)

        for base in node.bases:
            self.visit(base)

        self.scopes.append(BindingScope(node))

        for stmt in node.body:
            self.visit(stmt)

        self.scopes.pop()

    def set_type(self, node: AST, type: Value) -> None:
        self.cur_mod.types[node] = type

    def get_type(self, node: AST) -> Value:
        assert node in self.cur_mod.types, f"node not found: {node}, {node.lineno}"
        return self.cur_mod.types[node]

    def check_primitive_scope(self, node: Name) -> None:
        cur_scope = self.symbols.scopes[self.scope]
        var_scope = cur_scope.check_name(node.id)
        if var_scope != SC_LOCAL or isinstance(self.scope, Module):
            raise self.syntax_error(
                "cannot use primitives in global or closure scope", node
            )

    def visitAnnAssign(self, node: AnnAssign) -> None:
        self.visit(node.annotation)

        target = node.target
        comp_type = self.cur_mod.resolve_annotation(node.annotation) or DYNAMIC_TYPE
        if isinstance(target, Name):
            self.declare_local(target, comp_type)
            self.local_types[target.id] = comp_type.instance

        self.visit(target)

        value = node.value
        if value:
            self.visit(value, comp_type.instance)
            if isinstance(target, Name) and comp_type.can_be_narrowed:
                # We could be narrowing the type after the assignment, so we update it here
                # even though we assigned it above (but we never narrow primtives)
                self.local_types[target.id] = self.get_type(value)

            self.check_can_assign_from(comp_type, self.get_type(value).klass, node)

    def visitAugAssign(self, node: AugAssign) -> None:
        self.visit(node.target)
        self.visit(node.value, self.get_type(node.target))
        self.set_type(node, self.get_type(node.target))

    def visitAssign(self, node: Assign) -> None:
        # Sometimes, we need to propagate types from the target to the value to allow primitives to be handled
        # correctly.  So we compute the narrowest target type. (Other checks do happen later).
        # e.g: `x: int8 = 1` means we need `1` to be of type `int8`
        narrowest_target_type = None
        for target in reversed(node.targets):
            cur_type = None
            if isinstance(target, ast.Name):
                # This is a name, it could be unassigned still
                decl_type = self.decl_types.get(target.id)
                if decl_type is not None:
                    cur_type = decl_type.type.instance
            elif isinstance(target, (ast.Tuple, ast.List)):
                # TODO: We should walk into the tuple/list and use it to infer
                # types down on the RHS if we can
                self.visit(target)
            else:
                # This is an attribute or subscript, the assignment can't change the type
                self.visit(target)
                cur_type = self.get_type(target)

            if cur_type is not None and (
                narrowest_target_type is None
                or narrowest_target_type.klass.can_assign_from(cur_type.klass)
            ):
                narrowest_target_type = cur_type

        self.visit(node.value, narrowest_target_type)
        value_type = self.get_type(node.value)
        for target in reversed(node.targets):
            self.assign_value(target, value_type, node.value)

        self.set_type(node, value_type)

    def check_can_assign_from(
        self, dest: Class, src: Class, node: AST, reason: str = "cannot be assigned to"
    ) -> None:
        if not dest.can_assign_from(src) and src is not DYNAMIC_TYPE:
            raise self.syntax_error(
                f"type mismatch: {src.name} {reason} {dest.name} ", node
            )

    def visitBoolOp(
        self, node: BoolOp, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        effect = NO_EFFECT
        final_type = None
        if isinstance(node.op, And):
            for value in node.values:
                new_effect = self.visit(value) or NO_EFFECT
                effect = effect.union(new_effect)
                final_type = self.widen(final_type, self.get_type(value))

                # apply the new effect as short circuiting would
                # eliminate it.
                new_effect.apply(self.local_types)

            # we undo the effect as we have no clue what context we're in
            # but then we return the combined effect in case we're being used
            # in a conditional context
            effect.undo(self.local_types)
        else:
            for value in node.values:
                self.visit(value)
                final_type = self.widen(final_type, self.get_type(value))

        self.set_type(node, final_type or DYNAMIC)
        return effect

    def visitBinOp(
        self, node: BinOp, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit(node.left, type_ctx)
        self.visit(node.right, type_ctx)
        ltype = self.get_type(node.left)
        rtype = self.get_type(node.right)

        tried_right = False
        if ltype.klass in rtype.klass.mro[1:]:
            if rtype.bind_reverse_binop(node, self, type_ctx):
                return NO_EFFECT
            tried_right = True

        if ltype.bind_binop(node, self, type_ctx):
            return NO_EFFECT

        if not tried_right:
            rtype.bind_reverse_binop(node, self, type_ctx)

        return NO_EFFECT

    def visitUnaryOp(
        self, node: UnaryOp, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        effect = self.visit(node.operand, type_ctx)
        self.get_type(node.operand).bind_unaryop(node, self, type_ctx)
        if (
            effect is not None
            and effect is not NO_EFFECT
            and isinstance(node.op, ast.Not)
        ):
            return NegationEffect(effect)
        return NO_EFFECT

    def visitLambda(
        self, node: Lambda, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit(node.body)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitIfExp(
        self, node: IfExp, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        effect = self.visit(node.test) or NO_EFFECT
        effect.apply(self.local_types)
        self.visit(node.body)
        effect.reverse(self.local_types)
        self.visit(node.orelse)
        effect.undo(self.local_types)

        # Select the most compatible types that we can, or fallback to
        # dynamic if we can coerce to dynamic, otherwise report an error.
        body_t = self.get_type(node.body)
        else_t = self.get_type(node.orelse)
        if body_t.klass.can_assign_from(else_t.klass):
            self.set_type(node, body_t)
        elif else_t.klass.can_assign_from(body_t.klass):
            self.set_type(node, else_t)
        elif DYNAMIC_TYPE.can_assign_from(
            body_t.klass
        ) and DYNAMIC_TYPE.can_assign_from(else_t.klass):
            self.set_type(node, DYNAMIC)
        else:
            raise self.syntax_error(
                f"if expression has incompatible types: {body_t.name} and {else_t.name}",
                node,
            )
        return NO_EFFECT

    def visitSlice(
        self, node: Slice, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        lower = node.lower
        if lower:
            self.visit(lower, type_ctx)
        upper = node.upper
        if upper:
            self.visit(upper, type_ctx)
        step = node.step
        if step:
            self.visit(step, type_ctx)
        self.set_type(node, SLICE_TYPE.instance)
        return NO_EFFECT

    def widen(self, existing: Optional[Value], new: Value) -> Value:
        if existing is None or new.klass.can_assign_from(existing.klass):
            return new
        elif existing.klass.can_assign_from(new.klass):
            return existing

        res = UNION_TYPE.make_generic_type(
            (existing.klass, new.klass), self.symtable.generic_types
        ).instance
        return res

    def visitDict(
        self, node: ast.Dict, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        key_type: Optional[Value] = None
        value_type: Optional[Value] = None
        for k, v in zip(node.keys, node.values):
            if k:
                self.visit(k)
                key_type = self.widen(key_type, self.get_type(k))
                self.visit(v)
                value_type = self.widen(value_type, self.get_type(v))
            else:
                self.visit(v, type_ctx)
                d_type = self.get_type(v).klass
                if (
                    d_type.generic_type_def is CHECKED_DICT_TYPE
                    or d_type.generic_type_def is CHECKED_DICT_EXACT_TYPE
                ):
                    assert isinstance(d_type, GenericClass)
                    key_type = self.widen(key_type, d_type.type_args[0].instance)
                    value_type = self.widen(value_type, d_type.type_args[1].instance)
                elif d_type in (DICT_TYPE, DICT_EXACT_TYPE, DYNAMIC_TYPE):
                    key_type = DYNAMIC
                    value_type = DYNAMIC

        self.set_dict_type(node, key_type, value_type, type_ctx, is_exact=True)
        return NO_EFFECT

    def set_dict_type(
        self,
        node: ast.expr,
        key_type: Optional[Value],
        value_type: Optional[Value],
        type_ctx: Optional[Class],
        is_exact: bool = False,
    ) -> Value:
        if self.cur_mod.nonchecked_dicts or type_ctx in (
            DICT_TYPE.instance,
            DICT_EXACT_TYPE.instance,
        ):
            # User opted out of generic dictionaries (for the file, or for this instance)
            if type_ctx:
                typ = type_ctx
            elif is_exact:
                typ = DICT_EXACT_TYPE.instance
            else:
                typ = DICT_TYPE.instance
            self.set_type(node, typ)
            return typ

        # Calculate the type that is inferred by the keys and values
        if key_type is None:
            key_type = OBJECT_TYPE.instance

        if value_type is None:
            value_type = OBJECT_TYPE.instance

        checked_dict_typ = CHECKED_DICT_EXACT_TYPE if is_exact else CHECKED_DICT_TYPE

        gen_type = checked_dict_typ.make_generic_type(
            (key_type.klass, value_type.klass), self.symtable.generic_types
        )

        if type_ctx is not None:
            type_class = type_ctx.klass
            if type_class.generic_type_def in (
                CHECKED_DICT_EXACT_TYPE,
                CHECKED_DICT_TYPE,
            ):
                assert isinstance(type_class, GenericClass)
                self.set_type(node, type_ctx)
                # We can use the type context to have a type which is wider than the
                # inferred types.  But we need to make sure that the keys/values are compatible
                # with the wider type, and if not, we'll report that the inferred type isn't
                # compatible.
                if not type_class.type_args[0].can_assign_from(
                    key_type.klass
                ) or not type_class.type_args[1].can_assign_from(value_type.klass):
                    self.check_can_assign_from(type_class, gen_type, node)
                return type_ctx
            else:
                # Otherwise we allow something that would assign to dynamic, but not something
                # that would assign to an unrelated type (e.g. int)
                self.set_type(node, gen_type.instance)
                self.check_can_assign_from(type_class, gen_type, node)
        else:
            self.set_type(node, gen_type.instance)

        return gen_type.instance

    def visitSet(
        self, node: ast.Set, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        for elt in node.elts:
            self.visit(elt)
        self.set_type(node, SET_EXACT_TYPE.instance)
        return NO_EFFECT

    def visitGeneratorExp(
        self, node: GeneratorExp, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit_comprehension(node, node.generators, node.elt)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitListComp(
        self, node: ListComp, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit_comprehension(node, node.generators, node.elt)
        self.set_type(node, LIST_EXACT_TYPE.instance)
        return NO_EFFECT

    def visitSetComp(
        self, node: SetComp, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit_comprehension(node, node.generators, node.elt)
        self.set_type(node, SET_EXACT_TYPE.instance)
        return NO_EFFECT

    def assign_value(
        self, target: expr, value: Value, src: Optional[expr] = None
    ) -> None:
        if isinstance(target, Name):
            decl_type = self.decl_types.get(target.id)
            if decl_type is None:
                # This is the first declaration of this variable

                # For an inferred exact type, we want to declare the inexact
                # type; the exact type is useful local inference information,
                # but we should still allow assignment of a subclass later.
                decl_type = self.declare_local(target, inexact(value.klass))
            else:
                self.check_can_assign_from(decl_type.type, value.klass, target)

            local_type = (
                value
                if decl_type.type.can_be_narrowed and value is not DYNAMIC
                else decl_type.type.instance
            )
            self.local_types[target.id] = local_type
            self.set_type(target, local_type)
        elif isinstance(target, (ast.Tuple, ast.List)):
            if isinstance(src, (ast.Tuple, ast.List)) and len(target.elts) == len(
                src.elts
            ):
                for target, inner_value in zip(target.elts, src.elts):
                    self.assign_value(target, self.get_type(inner_value), inner_value)
            elif isinstance(src, ast.Constant):
                t = src.value
                if isinstance(t, tuple) and len(t) == len(target.elts):
                    for target, inner_value in zip(target.elts, t):
                        self.assign_value(target, CONSTANT_TYPES[type(inner_value)])
                else:
                    for val in target.elts:
                        self.assign_value(val, DYNAMIC)
            else:
                for val in target.elts:
                    self.assign_value(val, DYNAMIC)
        else:
            self.check_can_assign_from(self.get_type(target).klass, value.klass, target)

    def visitDictComp(
        self, node: DictComp, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit(node.generators[0].iter)

        scope = BindingScope(node)
        self.scopes.append(scope)

        iter_type = self.get_type(node.generators[0].iter).get_iter_type(
            node.generators[0].iter, self
        )

        self.assign_value(node.generators[0].target, iter_type)
        for if_ in node.generators[0].ifs:
            self.visit(if_)

        for gen in node.generators[1:]:
            self.visit(gen.iter)
            iter_type = self.get_type(gen.iter).get_iter_type(gen.iter, self)
            self.assign_value(gen.target, iter_type)
            for if_ in node.generators[0].ifs:
                self.visit(if_)

        self.visit(node.key)
        self.visit(node.value)

        self.scopes.pop()

        key_type = self.get_type(node.key)
        value_type = self.get_type(node.value)
        self.set_dict_type(node, key_type, value_type, type_ctx, is_exact=True)
        return NO_EFFECT

    def visit_comprehension(
        self, node: ast.expr, generators: List[ast.comprehension], *elts: ast.expr
    ) -> None:
        self.visit(generators[0].iter)

        scope = BindingScope(node)
        self.scopes.append(scope)

        iter_type = self.get_type(generators[0].iter).get_iter_type(
            generators[0].iter, self
        )

        self.assign_value(generators[0].target, iter_type)
        for if_ in generators[0].ifs:
            self.visit(if_)

        for gen in generators[1:]:
            self.visit(gen.iter)
            iter_type = self.get_type(gen.iter).get_iter_type(gen.iter, self)
            self.assign_value(gen.target, iter_type)
            for if_ in generators[0].ifs:
                self.visit(if_)

        for elt in elts:
            self.visit(elt)

        self.scopes.pop()

    def visitAwait(
        self, node: Await, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit(node.value)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitYield(
        self, node: Yield, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        value = node.value
        if value is not None:
            self.visit(value)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitYieldFrom(
        self, node: YieldFrom, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit(node.value)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitIndex(
        self, node: Index, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        # pyre-fixme[16]: `Index` has no attribute `value`.
        self.visit(node.value, type_ctx)
        self.set_type(node, self.get_type(node.value))
        return NO_EFFECT

    def visitCompare(
        self, node: Compare, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        if len(node.ops) == 1 and isinstance(node.ops[0], (Is, IsNot)):
            left = node.left
            right = node.comparators[0]
            other = None

            self.set_type(node, BOOL_TYPE.instance)
            self.set_type(node.ops[0], BOOL_TYPE.instance)

            self.visit(left)
            self.visit(right)

            if isinstance(left, (Constant, NameConstant)) and left.value is None:
                other = right
            elif isinstance(right, (Constant, NameConstant)) and right.value is None:
                other = left

            if other is not None and isinstance(other, Name):
                var_type = self.get_type(other)

                if (
                    isinstance(var_type, UnionInstance)
                    and not var_type.klass.is_generic_type_definition
                ):
                    effect = IsInstanceEffect(
                        other.id, var_type, NONE_TYPE.instance, self
                    )
                    if isinstance(node.ops[0], IsNot):
                        effect = NegationEffect(effect)
                    return effect

        self.visit(node.left)
        left = node.left
        ltype = self.get_type(node.left)
        node.ops = [type(op)() for op in node.ops]
        for comparator, op in zip(node.comparators, node.ops):
            self.visit(comparator)
            rtype = self.get_type(comparator)

            tried_right = False
            if ltype.klass in rtype.klass.mro[1:]:
                if ltype.bind_reverse_compare(
                    node, left, op, comparator, self, type_ctx
                ):
                    continue
                tried_right = True

            if ltype.bind_compare(node, left, op, comparator, self, type_ctx):
                continue

            if not tried_right:
                rtype.bind_reverse_compare(node, left, op, comparator, self, type_ctx)

            ltype = rtype
            right = comparator
        return NO_EFFECT

    def visitCall(
        self, node: Call, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit(node.func)
        result = self.get_type(node.func).bind_call(node, self, type_ctx)
        return result

    def visitNum(self, node: Num, type_ctx: Optional[Class] = None) -> NarrowingEffect:
        if type_ctx is not None:
            type_ctx.bind_num(node, self)
        else:
            DYNAMIC.bind_num(node, self)

        return NO_EFFECT

    def visitStr(self, node: Str, type_ctx: Optional[Class] = None) -> NarrowingEffect:
        self.set_type(node, STR_TYPE.instance)
        return NO_EFFECT

    def visitFormattedValue(
        self, node: FormattedValue, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit(node.value)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitJoinedStr(
        self, node: JoinedStr, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        for value in node.values:
            self.visit(value)

        self.set_type(node, STR_TYPE.instance)
        return NO_EFFECT

    def visitBytes(
        self, node: Bytes, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.set_type(node, BYTES_TYPE.instance)
        return NO_EFFECT

    def visitNameConstant(
        self, node: NameConstant, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        if node.value is None:
            self.set_type(node, NONE_TYPE.instance)
        elif node.value in (True, False):
            self.set_type(node, BOOL_TYPE.instance)
        else:
            self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitEllipsis(
        self, node: Ellipsis, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitConstant(
        self, node: Constant, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        if type_ctx is not None:
            type_ctx.bind_constant(node, self)
        else:
            DYNAMIC.bind_constant(node, self)
        return NO_EFFECT

    def visitAttribute(
        self, node: Attribute, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit(node.value)
        self.get_type(node.value).bind_attr(node, self, type_ctx)
        return NO_EFFECT

    def visitSubscript(
        self, node: Subscript, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit(node.value)
        self.visit(node.slice)
        val_type = self.get_type(node.value)
        val_type.bind_subscr(node, self.get_type(node.slice), self)
        return NO_EFFECT

    def visitStarred(
        self, node: Starred, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        self.visit(node.value)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitName(
        self, node: Name, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        cur_scope = self.symbols.scopes[self.scope]
        scope = cur_scope.check_name(node.id)
        if scope == SC_LOCAL and not isinstance(self.scope, Module):
            var_type = self.local_types.get(node.id, DYNAMIC)
            self.set_type(node, var_type)
            if type_ctx is not None:
                self.check_can_assign_from(type_ctx.klass, var_type.klass, node)
        else:
            self.set_type(node, self.cur_mod.resolve_name(node.id) or DYNAMIC)
        return NO_EFFECT

    def visitList(
        self, node: ast.List, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        for elt in node.elts:
            self.visit(elt, DYNAMIC)
        self.set_type(node, LIST_EXACT_TYPE.instance)
        return NO_EFFECT

    def visitTuple(
        self, node: ast.Tuple, type_ctx: Optional[Class] = None
    ) -> NarrowingEffect:
        for elt in node.elts:
            self.visit(elt, DYNAMIC)
        self.set_type(node, TUPLE_EXACT_TYPE.instance)
        return NO_EFFECT

    def visitReturn(self, node: Return) -> None:
        self.returns.add(node)
        value = node.value
        if value is not None:
            cur_scope = self.scopes[-1]
            func = cur_scope.node
            expected = DYNAMIC
            if isinstance(func, (ast.FunctionDef, ast.AsyncFunctionDef)):
                func_returns = func.returns
                if func_returns:
                    expected = (
                        self.cur_mod.resolve_annotation(func_returns) or DYNAMIC_TYPE
                    ).instance

            self.visit(value, expected)
            returned = self.get_type(value).klass
            if returned is not DYNAMIC_TYPE and not expected.klass.can_assign_from(
                returned
            ):
                raise self.syntax_error(
                    f"return type must be {expected.name}, not "
                    + str(self.get_type(value).name),
                    node,
                )

    def visitImportFrom(self, node: ImportFrom) -> None:
        mod_name = node.module
        if node.level or not mod_name:
            raise NotImplementedError("relative imports aren't supported")

        if mod_name == "__static__":
            for alias in node.names:
                name = alias.name
                if name == "*":
                    raise TypedSyntaxError("from __static__ import * is disallowed")
                elif name not in self.symtable.statics.children:
                    raise TypedSyntaxError(f"unsupported static import {name}")

    def visit_until_return(self, nodes: List[ast.stmt]) -> bool:
        for stmt in nodes:
            self.visit(stmt)
            if stmt in self.returns:
                return True

        return False

    def visitIf(self, node: If) -> None:
        branch = self.scopes[-1].branch()

        effect = self.visit(node.test) or NO_EFFECT
        effect.apply(self.local_types)

        returns = self.visit_until_return(node.body)

        if node.orelse:
            if_end = branch.copy()
            branch.restore()

            effect.reverse(self.local_types)
            else_returns = self.visit_until_return(node.orelse)
            if else_returns:
                if returns:
                    self.returns.add(node)
                else:
                    branch.restore(if_end)
            elif not returns:
                # Merge end of orelse with end of if
                branch.merge(if_end)
        elif returns:
            effect.reverse(self.local_types)
        else:
            # Merge end of if w/ opening (with test effect reversed)
            branch.merge(effect.reverse(branch.entry_locals))

    def visitTry(self, node: Try) -> None:
        branch = self.scopes[-1].branch()
        self.visit(node.body)

        branch.merge()
        post_try = branch.copy()
        merges = []

        if node.orelse:
            self.visit(node.orelse)
            merges.append(branch.copy())

        for handler in node.handlers:
            branch.restore(post_try)
            self.visit(handler)
            merges.append(branch.copy())

        branch.restore(post_try)
        for merge in merges:
            branch.merge(merge)

        if node.finalbody:
            self.visit(node.finalbody)

    def visitExceptHandler(self, node: ast.ExceptHandler) -> None:
        htype = node.type
        hname = None
        if htype:
            self.visit(htype)
            handler_type = self.get_type(htype)
            hname = node.name
            if hname:
                if handler_type is DYNAMIC or not isinstance(handler_type, Class):
                    handler_type = DYNAMIC_TYPE

                decl_type = self.decl_types.get(hname)
                self.decl_types[hname] = decl_type = TypeDeclaration(handler_type, node)

                local_type = (
                    handler_type.instance
                    if decl_type.type.can_be_narrowed
                    else decl_type.type.instance
                )
                self.local_types[hname] = local_type

        self.visit(node.body)
        if hname is not None:
            del self.decl_types[hname]
            del self.local_types[hname]

    def visitWhile(self, node: While) -> None:
        branch = self.scopes[-1].branch()

        effect = self.visit(node.test) or NO_EFFECT
        effect.apply(self.local_types)

        while_returns = self.visit_until_return(node.body)

        if while_returns:
            branch.restore()
            effect.reverse(self.local_types)
        else:
            branch.merge(effect.reverse(branch.entry_locals))

        if node.orelse:
            # The or-else can happen after the while body, or without executing
            # it, but it can only happen after the while condition evaluates to
            # False.
            effect.reverse(self.local_types)
            self.visit(node.orelse)

            branch.merge()

    def visitFor(self, node: For) -> None:
        self.visit(node.iter)
        self.get_type(node.iter).get_iter_type(node.iter, self)
        self.visit(node.target)
        self.visit(node.body)
        self.visit(node.orelse)

    def syntax_error(self, msg: str, node: AST) -> TypedSyntaxError:
        source_line = linecache.getline(self.filename, node.lineno)
        return TypedSyntaxError(
            msg, (self.filename, node.lineno, node.col_offset, source_line or None)
        )


class PyFlowGraph38Static(PyFlowGraph38):
    opcode: Opcode = opcode38static.opcode


class Static38CodeGenerator(Python38CodeGenerator):
    flow_graph = PyFlowGraph38Static
    _default_cache: Dict[Type[ast.AST], typingCallable[[...], None]] = {}

    def __init__(
        self,
        parent: Optional[CodeGenerator],
        node: AST,
        symbols: SymbolVisitor,
        graph: PyFlowGraph,
        symtable: SymbolTable,
        modname: str,
        flags: int = 0,
        optimization_lvl: int = 0,
    ) -> None:
        super().__init__(parent, node, symbols, graph, flags, optimization_lvl)
        self.symtable = symtable
        self.modname = modname
        # Use this counter to allocate temporaries for loop indices
        self._tmpvar_loopidx_count = 0
        self.cur_mod: ModuleTable = self.symtable.modules[modname]

    def make_child_codegen(
        self, tree: AST, graph: PyFlowGraph
    ) -> Static38CodeGenerator:
        graph.setFlag(self.consts.CO_STATICALLY_COMPILED)
        if self.cur_mod.noframe:
            graph.setFlag(self.consts.CO_NO_FRAME)
        elif self.cur_mod.tinyframe:
            graph.setFlag(self.consts.CO_TINY_FRAME)
        return StaticCodeGenerator(
            self,
            tree,
            self.symbols,
            graph,
            symtable=self.symtable,
            modname=self.modname,
        )

    def get_type(self, node: Union[AST, AugName]) -> Value:
        return self.cur_mod.types[node]

    @classmethod
    def make_code_gen(
        cls,
        module_name: str,
        tree: AST,
        filename: str,
        flags: int,
        optimize: int,
        peephole_enabled: bool = True,
        ast_optimizer_enabled: bool = True,
    ) -> Static38CodeGenerator:
        # TODO: Parsing here should really be that we run declaration visitor over all nodes,
        # and then perform post processing on the symbol table, and then proceed to analysis
        # and compilation
        symtable = SymbolTable()
        decl_visit = DeclarationVisitor(module_name, filename, symtable)
        decl_visit.visit(tree)

        for module in symtable.modules.values():
            module.finish_bind()

        if ast_optimizer_enabled:
            tree = AstOptimizer(optimize=optimize > 0).visit(tree)

        s = symbols.SymbolVisitor()
        s.visit(tree)

        graph = cls.flow_graph(
            module_name, filename, s.scopes[tree], peephole_enabled=peephole_enabled
        )
        graph.setFlag(cls.consts.CO_STATICALLY_COMPILED)

        type_binder = TypeBinder(s, filename, symtable, module_name)
        type_binder.visit(tree)

        code_gen = cls(None, tree, s, graph, symtable, module_name, flags, optimize)
        code_gen.visit(tree)
        return code_gen

    def make_function_graph(
        self,
        func: FunctionDef,
        filename: str,
        scopes: Dict[AST, Scope],
        class_name: str,
        name: str,
        first_lineno: int,
    ) -> PyFlowGraph:
        graph = super().make_function_graph(
            func, filename, scopes, class_name, name, first_lineno
        )

        # we tagged the graph as CO_STATICALLY_COMPILED, and the last co_const entry
        # will inform the runtime of the return type for the code object.
        ret_type = self.get_type(func)
        type_descr = ret_type.klass.type_descr
        graph.extra_consts.append(type_descr)
        return graph

    @contextmanager
    def new_loopidx(self) -> Generator[str, None, None]:
        self._tmpvar_loopidx_count += 1
        try:
            yield f"{_TMP_VAR_PREFIX}.{self._tmpvar_loopidx_count}"
        finally:
            self._tmpvar_loopidx_count -= 1

    def walkClassBody(self, node: ClassDef, gen: CodeGenerator) -> None:
        super().walkClassBody(node, gen)
        cur_mod = self.symtable.modules[self.modname]
        klass = cur_mod.resolve_name(node.name)
        if not isinstance(klass, Class):
            raise gen.syntax_error("class missing or defined", node)

        for base_ in klass._bases:
            base = base_.resolved
            if base is NAMED_TUPLE_TYPE:
                return

        class_mems = [
            name for name, value in klass.members.items() if isinstance(value, Slot)
        ]

        # In the future we may want a compatibility mode where we add
        # __dict__ and __weakref__
        gen.emit("LOAD_CONST", tuple(class_mems))
        gen.emit("STORE_NAME", "__slots__")

        count = 0
        for name, value in klass.members.items():
            if not isinstance(value, Slot):
                continue

            if value.type_ref.resolved is DYNAMIC_TYPE:
                continue

            gen.emit("LOAD_CONST", name)
            gen.emit("LOAD_CONST", value.type_descr)
            count += 1

        if count:
            gen.emit("BUILD_MAP", count)
            gen.emit("STORE_NAME", "__slot_types__")

    def visitModule(self, node: Module) -> None:
        if not self.cur_mod.nonchecked_dicts:
            self.emit("LOAD_CONST", 0)
            self.emit("LOAD_CONST", ("chkdict",))
            self.emit("IMPORT_NAME", "_static")
            self.emit("IMPORT_FROM", "chkdict")
            self.emit("STORE_NAME", "dict")

        super().visitModule(node)

    def visitAugAttribute(self, node: AugAttribute, mode: str) -> None:
        if mode == "load":
            self.visit(node.value)
            self.emit("DUP_TOP")
            load = ast.Attribute(node.value, node.attr, ast.Load())
            load.lineno = node.lineno
            load.col_offset = node.col_offset
            self.get_type(node.value).emit_attr(load, self)
        elif mode == "store":
            self.emit("ROT_TWO")
            self.get_type(node.value).emit_attr(node, self)

    def visitAugSubscript(self, node: AugSubscript, mode: str) -> None:
        if mode == "load":
            self.get_type(node.value).emit_subscr(node.obj, 1, self)
        elif mode == "store":
            self.get_type(node.value).emit_store_subscr(node.obj, self)

    def visitAttribute(self, node: Attribute) -> None:
        self.update_lineno(node)
        self.visit(node.value)
        self.get_type(node.value).emit_attr(node, self)

    def emit_type_check(self, dest: Class, src: Class) -> None:
        if src is DYNAMIC_TYPE and dest is not OBJECT_TYPE and dest is not DYNAMIC_TYPE:
            self.emit("CAST", dest.type_descr)
        elif not dest.can_assign_from(src):
            raise TypedSyntaxError(f"Cannot assign a {src} to {dest}")

    def visitAssignTarget(self, elt: expr, value: Optional[expr] = None) -> None:
        if isinstance(elt, (ast.Tuple, ast.List)):
            self._visitUnpack(elt)
            if isinstance(value, ast.Tuple) and len(value.elts) == len(elt.elts):
                for target, inner_value in zip(elt.elts, value.elts):
                    self.visitAssignTarget(target, inner_value)
            else:
                for target in elt.elts:
                    self.visitAssignTarget(target)
        else:
            if value is not None:
                self.emit_type_check(
                    self.get_type(elt).klass, self.get_type(value).klass
                )
            else:
                self.emit_type_check(self.get_type(elt).klass, DYNAMIC_TYPE)
            self.visit(elt)

    def visitAssign(self, node: Assign) -> None:
        self.set_lineno(node)
        self.visit(node.value)
        dups = len(node.targets) - 1
        for i in range(len(node.targets)):
            elt = node.targets[i]
            if i < dups:
                self.emit("DUP_TOP")
            if isinstance(elt, ast.AST):
                self.visitAssignTarget(elt, node.value)

    def visitAnnAssign(self, node: ast.AnnAssign) -> None:
        self.set_lineno(node)
        value = node.value
        if value:
            self.visit(value)
            self.emit_type_check(
                self.get_type(node.target).klass, self.get_type(value).klass
            )
            self.visit(node.target)
        target = node.target
        if isinstance(target, ast.Name):
            # If we have a simple name in a module or class, store the annotation
            if node.simple and isinstance(self.tree, (ast.Module, ast.ClassDef)):
                self.emitStoreAnnotation(target.id, node.annotation)
        elif isinstance(target, ast.Attribute):
            if not node.value:
                self.checkAnnExpr(target.value)
        elif isinstance(target, ast.Subscript):
            if not node.value:
                self.checkAnnExpr(target.value)
                self.checkAnnSubscr(target.slice)
        else:
            raise SystemError(
                f"invalid node type {type(node).__name__} for annotated assignment"
            )

        if not node.simple:
            self.checkAnnotation(node)

    def visitNum(self, node: Num) -> None:
        self.get_type(node).emit_num(node, self)

    def visitConstant(self, node: Constant) -> None:
        self.get_type(node).emit_constant(node, self)

    def visitName(self, node: Name) -> None:
        self.get_type(node).emit_name(node, self)

    def visitAugAssign(self, node: AugAssign) -> None:
        self.get_type(node.target).emit_augassign(node, self)

    def visitAugName(self, node: AugName, mode: str) -> None:
        self.get_type(node).emit_augname(node, self, mode)

    def visitCompare(self, node: Compare) -> None:
        self.update_lineno(node)
        self.visit(node.left)
        cleanup = self.newBlock("cleanup")
        for op, code in zip(node.ops[:-1], node.comparators[:-1]):
            self.emitChainedCompareStep(op, code, cleanup)
        # now do the last comparison
        if node.ops:
            op = node.ops[-1]
            code = node.comparators[-1]
            self.visit(code)
            self.get_type(op).emit_compare(op, self)
        if len(node.ops) > 1:
            end = self.newBlock("end")
            self.emit("JUMP_FORWARD", end)
            self.nextBlock(cleanup)
            self.emit("ROT_TWO")
            self.emit("POP_TOP")
            self.nextBlock(end)

    def emitChainedCompareStep(
        self, op: cmpop, value: AST, cleanup: Block, jump: str = "JUMP_IF_ZERO_OR_POP"
    ) -> None:
        self.visit(value)
        self.emit("DUP_TOP")
        self.emit("ROT_THREE")
        self.get_type(op).emit_compare(op, self)
        self.emit(jump, cleanup)
        self.nextBlock(label="compare_or_cleanup")

    def visitBoolOp(self, node: BoolOp) -> None:
        end = self.newBlock()
        for child in node.values[:-1]:
            self.get_type(child).emit_jumpif_pop(
                child, end, type(node.op) == ast.Or, self
            )
            self.nextBlock()
        self.visit(node.values[-1])
        self.nextBlock(end)

    def visitBinOp(self, node: BinOp) -> None:
        self.get_type(node).emit_binop(node, self)

    def visitUnaryOp(self, node: UnaryOp, type_ctx: Optional[Class] = None) -> None:
        self.get_type(node).emit_unaryop(node, self)

    def visitCall(self, node: Call) -> None:
        self.get_type(node.func).emit_call(node, self)

    def visitSubscript(self, node: ast.Subscript, aug_flag: bool = False) -> None:
        self.get_type(node.value).emit_subscr(node, aug_flag, self)

    def get_expected_returntype(self, node: ast.Return) -> Optional[Class]:
        expected = None
        func = self.tree
        value = node.value
        self.checkReturn(node)
        if isinstance(func, (ast.FunctionDef, ast.AsyncFunctionDef)):
            func_returns = func.returns
            if func_returns:
                expected = self.cur_mod.resolve_annotation(func_returns)
        if value:
            return_type = self.get_type(value) or DYNAMIC
            if isinstance(return_type, CInstance):
                # We need better JIT typing support before we can start returning primitives from
                # functions.
                raise self.syntax_error(
                    f"Cannot return primitive type: {return_type.klass.name}", node
                )
        return expected

    def _visitReturnValue(self, value: ast.AST, expected: Optional[Class]) -> None:
        self.visit(value)
        if expected is not None and self.get_type(value) is DYNAMIC:
            self.emit("CAST", expected.type_descr)

    def visitReturn(self, node: ast.Return) -> None:
        expected = self.get_expected_returntype(node)
        self.set_lineno(node)
        value = node.value
        is_return_constant = isinstance(value, ast.Constant)

        if value:
            if not is_return_constant:
                self._visitReturnValue(value, expected)
                self.unwind_setup_entries(preserve_tos=True)
            else:
                self.unwind_setup_entries(preserve_tos=False)
                self._visitReturnValue(value, expected)
        else:
            self.unwind_setup_entries(preserve_tos=False)
            self.emit("LOAD_CONST", None)

        self.emit("RETURN_VALUE")

    def visitDictComp(self, node: DictComp) -> None:
        dict_type = self.get_type(node)
        if dict_type in (DICT_TYPE.instance, DICT_EXACT_TYPE.instance):
            return super().visitDictComp(node)
        klass = dict_type.klass

        assert isinstance(klass, GenericClass) and (
            klass.type_def is CHECKED_DICT_TYPE
            or klass.type_def is CHECKED_DICT_EXACT_TYPE
        ), dict_type
        self.compile_comprehension(
            node,
            sys.intern("<dictcomp>"),
            node.key,
            node.value,
            "BUILD_CHECKED_MAP",
            (dict_type.klass.type_descr, 0),
        )

    def compile_subgendict(
        self, node: ast.Dict, begin: int, end: int, dict_descr: TypeDescr
    ) -> None:
        n = end - begin
        for i in range(begin, end):
            k = node.keys[i]
            assert k is not None
            self.visit(k)
            self.visit(node.values[i])

        self.emit("BUILD_CHECKED_MAP", (dict_descr, n))

    def visitDict(self, node: ast.Dict) -> None:
        dict_type = self.get_type(node)
        if dict_type in (DICT_TYPE.instance, DICT_EXACT_TYPE.instance):
            return super().visitDict(node)
        klass = dict_type.klass

        assert isinstance(klass, GenericClass) and (
            klass.type_def is CHECKED_DICT_TYPE
            or klass.type_def is CHECKED_DICT_EXACT_TYPE
        ), dict_type

        self.update_lineno(node)
        elements = 0
        is_unpacking = False
        built_final_dict = False

        # This is similar to the normal dict code generation, but instead of relying
        # upon an opcode for BUILD_MAP_UNPACK we invoke the update method on the
        # underlying dict type.  Therefore the first dict that we create becomes
        # the final dict.  This allows us to not introduce a new opcode, but we should
        # also be able to dispatch the INVOKE_METHOD rather efficiently.
        dict_descr = dict_type.klass.type_descr
        update_descr = dict_descr + ("update",)
        for i, (k, v) in enumerate(zip(node.keys, node.values)):
            is_unpacking = k is None
            if elements == 0xFFFF or (elements and is_unpacking):
                self.compile_subgendict(node, i - elements, i, dict_descr)
                built_final_dict = True
                elements = 0

            if is_unpacking:
                if not built_final_dict:
                    # {**foo, ...}, we need to generate the empty dict
                    self.emit("BUILD_CHECKED_MAP", (dict_descr, 0))
                    built_final_dict = True
                self.emit("DUP_TOP")
                self.visit(v)

                self.emit_invoke_method(update_descr, 1)
                self.emit("POP_TOP")
            else:
                elements += 1

        if elements or not built_final_dict:
            if built_final_dict:
                self.emit("DUP_TOP")
            self.compile_subgendict(
                node, len(node.keys) - elements, len(node.keys), dict_descr
            )
            if built_final_dict:
                self.emit_invoke_method(update_descr, 1)
                self.emit("POP_TOP")

    def visitFor(self, node: ast.For) -> None:
        iter_type = self.get_type(node.iter)
        return iter_type.emit_forloop(node, self)

    def emit_invoke_method(self, descr: TypeDescr, arg_count: int) -> None:
        # Emit a zero EXTENDED_ARG before so that we can optimize and insert the
        # arg count
        self.emit("EXTENDED_ARG", 0)
        self.emit("INVOKE_METHOD", (descr, arg_count))

    def defaultVisit(self, node: object, *args: object) -> None:
        self.node = node
        klass = node.__class__
        meth = self._default_cache.get(klass, None)
        if meth is None:
            className = klass.__name__
            meth = getattr(
                super(Static38CodeGenerator, Static38CodeGenerator),
                "visit" + className,
                StaticCodeGenerator.generic_visit,
            )
            self._default_cache[klass] = meth
        return meth(self, node, *args)

    def compileJumpIf(self, test: AST, next: Block, is_if_true: bool) -> None:
        self.get_type(test).emit_jumpif(test, next, is_if_true, self)

    def _calculate_idx(
        self, arg_name: str, non_cellvar_pos: int, cellvars: List[str]
    ) -> int:
        try:
            offset = cellvars.index(arg_name)
        except ValueError:
            return non_cellvar_pos
        else:
            # the negative sign indicates to the runtime/JIT that this is a cellvar
            return -(offset + 1)

    def processBody(
        self,
        node: Union[ast.Lambda, ast.FunctionDef],
        body: Union[List[ast.stmt], ast.expr],
        gen: Static38CodeGenerator,
    ) -> None:
        arg_checks = []
        cellvars = gen.graph.cellvars
        for i, arg in enumerate(node.args.posonlyargs):
            t = self.get_type(arg)
            if t is not DYNAMIC and t is not OBJECT:
                arg_checks.append(self._calculate_idx(arg.arg, i, cellvars))
                arg_checks.append(t.klass.type_descr)

        for i, arg in enumerate(node.args.args):
            t = self.get_type(arg)
            if t is not DYNAMIC and t is not OBJECT:
                arg_checks.append(
                    self._calculate_idx(
                        arg.arg, i + len(node.args.posonlyargs), cellvars
                    )
                )
                arg_checks.append(t.klass.type_descr)

        for i, arg in enumerate(node.args.kwonlyargs):
            t = self.get_type(arg)
            if t is not DYNAMIC and t is not OBJECT:
                arg_checks.append(
                    self._calculate_idx(
                        arg.arg,
                        i + len(node.args.posonlyargs) + len(node.args.args),
                        cellvars,
                    )
                )
                arg_checks.append(t.klass.type_descr)

        gen.emit("CHECK_ARGS", tuple(arg_checks))

        super().processBody(node, body, gen)


StaticCodeGenerator = Static38CodeGenerator
