from __future__ import annotations

import ast
import linecache
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
    List,
    ListComp,
    Module,
    Name,
    NameConstant,
    Num,
    Return,
    Set,
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
    stmt,
)
from types import CodeType, MethodDescriptorType
from typing import (
    Dict,
    Generic,
    Iterable,
    List,
    Optional,
    Tuple,
    Type,
    TypeVar,
    Union,
    cast,
)

from . import symbols
from .consts import CO_STATICALLY_COMPILED, SC_LOCAL
from .optimizer import AstOptimizer
from .pyassem import PyFlowGraph
from .pycodegen import AugName, CodeGenerator, Python37CodeGenerator, compile, wrap_aug
from .symbols import Scope, SymbolVisitor
from .visitor import ASTVisitor


try:
    from xxclassloader import spamobj  # pyre-ignore[21]: unknown module
except Exception:
    spamobj = None


def exec_static(
    source: str, locals: dict, globals: dict, modname="<module>"  # noqa: P210
):
    code = compile(
        source, "<module>", "exec", compiler=StaticCodeGenerator, modname=modname
    )
    exec(code, locals, globals)


INT8_TYPE: IntType
INT16_TYPE: IntType
INT32_TYPE: IntType
INT64_TYPE: IntType
INT64_VALUE: IntInstance
INT_TYPES: List[IntType]
INT_TYPE: NumType
FLOAT_TYPE: NumType
COMPLEX_TYPE: NumType
BOOL_TYPE: Class
ARRAY_TYPE: Class
DICT_TYPE: Class

OBJECT_TYPE: Class
OBJECT: Instance

DYNAMIC_TYPE: DynamicClass
DYNAMIC: Instance
FUNCTION_TYPE: Class
METHOD_TYPE: Class
MEMBER_TYPE: Class
NONE_TYPE: Class
TYPE_TYPE: Class
SLICE_TYPE: Class

CHAR_TYPE: IntType
DOUBLE_TYPE: DoubleType

RESOLVED_OBJECT_TYPE: ResolvedTypeRef

# Prefix for temporary var names. It's illegal in normal
# Python, so there's no chance it will ever clash with a
# user defined name.
_TMP_VAR_PREFIX = "_pystatic_.0._tmp__"


class TypeRef:
    """Stores unresolved typed references, capturing the referring module
    as well as the annotation"""

    def __init__(self, module, ref: ast.expr):
        self.module = module
        self.ref = ref

    @property
    def resolved(self):
        res = self.module.resolve_annotation(self.ref)
        if res is None:
            return DYNAMIC_TYPE
        return res

    def __repr__(self):
        return f"TypeRef({self.module.name}, {self.ref})"


class ResolvedTypeRef(TypeRef):
    def __init__(self, type: Class):
        self._resolved = type

    @property
    def resolved(self):
        return self._resolved

    def __repr__(self):
        return f"ResolvedTypeRef({self.resolved})"


class GenericTypeParamRef(TypeRef):
    def __init__(self, index: int):
        self.index = index

    @property
    def resolved(self):
        return GenericParameter("T" + str(self.index), self.index)

    def __repr__(self):
        return f"GenericTypeParamRef({self.index})"


class TypeName:
    def __init__(self, module: str, name: str):
        self.module = module
        self.name = name

    @property
    def type_descr(self):
        return (self.module, self.name)

    @property
    def friendly_name(self):
        if self.module:
            return f"{self.module}.{self.name}"
        return self.name


class GenericTypeName(TypeName):
    def __init__(self, module: str, name: str, args: Tuple[Class, ...]):
        super().__init__(module, name)
        self.args = args

    @property
    def type_descr(self):
        descr = [self.module, self.name]
        gen_args = []
        for arg in self.args:
            gen_args.append(arg.type_descr)
        descr.append(tuple(gen_args))
        return tuple(descr)

    @property
    def friendly_name(self):
        args = ", ".join(arg.type_name.friendly_name for arg in self.args)
        return f"{self.module}.{self.name}[{args}]"


class SymbolTable:
    def __init__(self):
        self.modules: Dict[str, ModuleTable] = {}
        builtins_children = {
            "object": OBJECT_TYPE,
            "type": TYPE_TYPE,
            "None": NONE_TYPE.instance,
            "int": INT_TYPE,
            "str": STR_TYPE,
            "bytes": BYTES_TYPE,
            "bool": BOOL_TYPE,
            "float": FLOAT_TYPE,
            "len": LenFunction(Function),
            "Exception": EXCEPTION_TYPE,
            "BaseException": BASE_EXCEPTION_TYPE,
            "isinstance": IsInstanceFunction(),
            "issubclass": IsSubclassFunction(),
        }
        strict_builtins = StrictBuiltins(builtins_children)
        typing_children = {"Optional": OPTIONAL_CLASS, "NamedTuple": NAMED_TUPLE_TYPE}

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
                "Array": ARRAY_TYPE,
                "box": BoxFunction(Function),
                "cast": CastFunction(Function),
                "size_t": INT64_TYPE,
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
                "unbox": UnboxFunction(Function),
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
        self.generic_types = {k: dict(v) for k, v in BUILTIN_GENERICS.items()}

    def __getitem__(self, name):
        return self.modules[name]

    def __setitem__(self, name, value):
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
        graph.setFlag(CO_STATICALLY_COMPILED)

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
        self.types: Dict[AST, Value] = {}
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

    def resolve_annotation(self, node, type_ctx=None) -> Optional[Class]:  # noqa: C901
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
            mod = ast.parse(node.s, "", "eval")
            assert isinstance(mod, ast.Expression), type(mod)
            return self.resolve_annotation(mod.body)
        elif isinstance(node, ast.Constant):
            sval = node.value
            if isinstance(sval, str):
                mod = ast.parse(sval, "", "eval")
                assert isinstance(mod, ast.Expression), type(mod)
                return self.resolve_annotation(mod.body)
        elif isinstance(node, NameConstant):
            if node.value in (True, False):
                return BOOL_TYPE
            elif node.value is None:
                return NONE_TYPE
            # should never happen
            raise TypedSyntaxError("invalid annotation")

    def resolve_name(self, name: str) -> Optional[Value]:
        return self.children.get(name) or self.symtable.builtins.children.get(name)


TClass = TypeVar("TClass", bound="Class")


class Value(Generic[TClass]):
    """base class for all values tracked at compile time."""

    def __init__(self, klass: TClass):
        """name: the name of the value, for instances this is used solely for
        debug/reporting purposes.  In Class subclasses this will be the
        qualified name (e.g. module.Foo).
        klass: the Class of this object"""
        self.klass = klass

    @property
    def name(self) -> str:
        return type(self).__name__

    def finish_bind(self):
        pass

    def make_generic_type(self, index, generic_types):
        pass

    def get_iter_type(self) -> Value:
        """returns the type that is produced when iterating over this value"""
        return DYNAMIC

    def bind_attr(self, node: ast.Attribute, visitor: TypeBinder, type_ctx):
        visitor.visit(node.value)
        if node.attr == "__class__":
            visitor.set_type(node, self.klass)
        else:
            visitor.set_type(node, DYNAMIC)

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        visitor.set_type(node, DYNAMIC)
        for arg in node.args:
            visitor.visit(arg)

        for arg in node.keywords:
            visitor.visit(arg.value)
        return NO_EFFECT

    def bind_descr_get(
        self,
        node: ast.Attribute,
        inst: Optional[Instance],
        ctx: Class,
        visitor: TypeBinder,
        type_ctx,
    ):
        visitor.set_type(node, DYNAMIC)

    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        visitor.set_type(node, DYNAMIC)

    def emit_call(self, node: ast.Call, code_gen: StaticCodeGenerator):
        code_gen.defaultVisit(node)

    def emit_attr(self, node: ast.Attribute, code_gen: StaticCodeGenerator):
        if isinstance(node.ctx, ast.Store):
            code_gen.emit("STORE_ATTR", code_gen.mangle(node.attr))
        elif isinstance(node.ctx, ast.Del):
            code_gen.emit("DELETE_ATTR", code_gen.mangle(node.attr))
        else:
            code_gen.emit("LOAD_ATTR", code_gen.mangle(node.attr))

    def emit_check_self(self, code_gen: "StaticCodeGenerator", name: str) -> None:
        pass

    def bind_compare(
        self,
        node: ast.Compare,
        left: expr,
        op: cmpop,
        right: expr,
        visitor: TypeBinder,
        type_ctx,
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
        type_ctx,
    ) -> bool:
        visitor.set_type(op, DYNAMIC)
        visitor.set_type(node, DYNAMIC)
        return False

    def emit_compare(self, op: cmpop, code_gen: StaticCodeGenerator) -> None:
        code_gen.defaultEmitCompare(op)

    def bind_binop(self, node: ast.BinOp, visitor: TypeBinder, type_ctx) -> bool:
        return False

    def bind_reverse_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx
    ) -> bool:
        # we'll set the type in case we're the only one called
        visitor.set_type(node, DYNAMIC)
        return False

    def bind_unaryop(self, node: ast.UnaryOp, visitor: TypeBinder, type_ctx):
        if isinstance(node.op, ast.Not):
            visitor.set_type(node, BOOL_TYPE.instance)
        else:
            visitor.set_type(node, DYNAMIC)

    def emit_binop(self, node: ast.BinOp, code_gen: StaticCodeGenerator) -> None:
        code_gen.defaultVisit(node)

    def emit_unaryop(self, node: ast.UnaryOp, code_gen: StaticCodeGenerator) -> None:
        code_gen.defaultVisit(node)

    def emit_augassign(
        self, node: ast.AugAssign, code_gen: StaticCodeGenerator
    ) -> None:
        code_gen.defaultVisit(node)

    def emit_augname(self, node: AugName, code_gen: StaticCodeGenerator, mode: str):
        code_gen.defaultVisit(node, mode)

    def bind_constant(self, node: ast.Constant, visitor: TypeBinder) -> None:
        visitor.set_type(node, CONSTANT_TYPES[type(node.value)])
        visitor.check_can_assign_from(
            self.klass, CONSTANT_TYPES[type(node.value)].klass, node
        )

    def emit_constant(self, node: ast.Constant, code_gen: StaticCodeGenerator) -> None:
        return code_gen.defaultVisit(node)

    def bind_num(self, node: ast.Num, visitor: TypeBinder) -> None:
        visitor.set_type(node, CONSTANT_TYPES[type(node.n)])
        visitor.check_can_assign_from(
            self.klass, CONSTANT_TYPES[type(node.n)].klass, node
        )

    def emit_num(self, node: ast.Num, code_gen: StaticCodeGenerator) -> None:
        return code_gen.defaultVisit(node)

    def emit_name(self, node: ast.Name, code_gen: StaticCodeGenerator) -> None:
        return code_gen.defaultVisit(node)

    def emit_jumpif(
        self, test, next, is_if_true, code_gen: StaticCodeGenerator
    ) -> None:
        Python37CodeGenerator.compileJumpIf(code_gen, test, next, is_if_true)

    def emit_jumpif_pop(
        self, test, next, is_if_true, code_gen: StaticCodeGenerator
    ) -> None:
        Python37CodeGenerator.compileJumpIfPop(code_gen, test, next, is_if_true)

    def emit_box(self, node: expr, code_gen: StaticCodeGenerator) -> None:
        raise RuntimeError(f"Unsupported box type: {code_gen.get_type(node)}")

    def emit_unbox(self, node: expr, code_gen: StaticCodeGenerator) -> None:
        raise RuntimeError("Unsupported unbox type")

    def emit_len(self, node: ast.Call, code_gen: StaticCodeGenerator) -> None:
        return self.emit_call(node, code_gen)

    def make_generic(self, name: GenericTypeName):
        return self


class Class(Value):
    """Represents a type object at compile time"""

    def __init__(
        self,
        type_name: TypeName,
        bases: Optional[List[TypeRef]] = None,
        instance: Optional[Instance] = None,
        klass: Optional[Class] = None,
        members: Optional[Dict[str, Value]] = None,
        pytype: Optional[Type] = None,
    ):
        super().__init__(klass or TYPE_TYPE)
        assert isinstance(bases, (type(None), list))
        self.type_name = type_name
        self.instance = instance or Instance(self)
        self._bases = bases or []
        self._mro = None
        self.members = members or {}
        if pytype:
            self.members.update(make_type_dict(self, pytype))

    @property
    def name(self) -> str:
        return self.type_name.friendly_name

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
        generic_types: Dict[Class, Dict[Tuple[Class, ...], Class]],
    ) -> Optional[Class]:
        """Binds the generic type parameters to a generic type definition"""
        return None

    def bind_attr(self, node: ast.Attribute, visitor: TypeBinder, type_ctx) -> None:
        for base in self.mro:
            member = base.members.get(node.attr)
            if member is not None:
                member.bind_descr_get(node, None, self, visitor, type_ctx)
                return

        super().bind_attr(node, visitor, type_ctx)

    @property
    def can_be_narrowed(self):
        return True

    @property
    def type_descr(self):
        """gets the type descriptor which is the metadata emitted into the const pool.
        For normal types this is just the fully qualified type name (e.g. mypackage.mymod.C).
        For optional types we have an extra ? suffix in the name.  In the future this will
        likely need to be extended to support some kind of generics (e.g. mypackage.mymod.T[mypackage.mymod.X]).
        """
        return self.type_name.type_descr

    @property
    def bases(self):
        return [base.resolved for base in self._bases]

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        visitor.set_type(node, self.instance)
        for arg in node.args:
            visitor.visit(arg)
        for arg in node.keywords:
            visitor.visit(arg.value)
        return NO_EFFECT

    def can_assign_from(self, src: Class) -> bool:
        """checks to see if the src value can be assigned to this value.  Currently
        you can assign a derived type to a base type.  You cannot assign a primitive
        type to an object type.

        At some point we may also support some form of interfaces via protocols if we
        implement a more efficient form of interface dispatch than doing the dictionary
        lookup for the member."""
        return src is self or (
            self.issubclass(src) and not isinstance(src, PrimitiveType)
        )

    def __repr__(self):
        return f"<{self.name} class>"

    def isinstance(self, src: Value) -> bool:
        return self in src.klass.mro

    def issubclass(self, src: Class) -> bool:
        return self in src.mro

    def finish_bind(self):
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
        # If we don't yet have a slot we allow
        if existing is None:
            self.members[name] = Slot(
                type_ref or ResolvedTypeRef(DYNAMIC_TYPE), name, self
            )
        elif isinstance(existing, Slot):
            if existing.type_ref.resolved == DYNAMIC_TYPE:
                # Replacing the dynamic type with a more specific type
                self.members[name] = Slot(
                    type_ref or ResolvedTypeRef(DYNAMIC_TYPE), name, self
                )
            elif (
                type_ref is not None and type_ref.resolved != existing.type_ref.resolved
            ):
                raise TypedSyntaxError(
                    f"conflicting type definitions for slot {name} in class {self.name}"
                )
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

        function = self.members[name] = Function(
            f"{name}", self.type_name, func, visitor.type_ref(func.returns)
        )
        process_function_args(function, func.args, visitor, self_type=self)

    @property
    def mro(self):
        if self._mro is None:
            if not all(self.bases):
                # TODO: We can't compile w/ unknown bases
                self._mro = []
            else:
                self._mro = _mro(self)

        return self._mro

    def bind_generics(
        self,
        name: GenericTypeName,
        generic_types: Dict[Class, Dict[Tuple[Class, ...], Class]],
    ) -> Class:
        return self

    def emit_type_check(self, code_gen: StaticCodeGenerator, src: Class) -> None:
        assert self.can_assign_from(src)


class GenericClass(Class):
    def __init__(
        self,
        name: GenericTypeName,
        bases: Optional[List[TypeRef]] = None,
        instance: Optional[Instance] = None,
        klass: Optional[Class] = None,
        members: Optional[Dict[str, Value]] = None,
        type_def: Optional[GenericClass] = None,
        pytype: Optional[Type] = None,
    ):
        super().__init__(name, bases, instance, klass, members, pytype)
        self.gen_name = name
        self.type_def = type_def

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
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
            if len(val.elts) != len(self.gen_name.args):
                raise visitor.syntax_error(
                    "incorrect number of generic arguments", node
                )
        else:
            if len(self.gen_name.args) != 1:
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
    def type_args(self):
        return self.type_name.args

    @property
    def contains_generic_parameters(self):
        for arg in self.gen_name.args:
            if arg.is_generic_parameter:
                return True
        return False

    @property
    def is_generic_type(self):
        return True

    @property
    def is_generic_type_definition(self):
        return self.type_def is None

    @property
    def generic_type_def(self) -> Optional[Class]:
        """Gets the generic type definition that defined this class"""
        return self.type_def

    def make_generic_type(
        self,
        index: Tuple[Class, ...],
        generic_types: Dict[Class, Dict[Tuple[Class, ...], Class]],
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
        bases: List[TypeRef] = [
            ResolvedTypeRef(base.resolved.make_generic_type(self, index, generic_types))
            for base in self.bases
        ]
        instance = type(self)(type_name, bases, None, self.klass, {}, type_def=self)

        instantiations[index] = instance
        instance.members.update(
            {
                k: v.make_generic(type_name, generic_types)
                for k, v in self.members.items()
            }
        )
        return instance


class GenericParameter(Class):
    def __init__(self, name: str, index: int):
        super().__init__(TypeName("", name), [], None, None, {})
        self.index = index

    @property
    def name(self) -> str:
        return self.type_name.name

    @property
    def is_generic_parameter(self):
        return True

    def bind_generics(
        self,
        name: GenericTypeName,
        generic_types: Dict[Class, Dict[Tuple[Class, ...], Class]],
    ) -> Class:
        return name.args[self.index]


class PrimitiveType(Class):
    """base class for primitives that aren't heap allocated"""

    @property
    def can_be_narrowed(self):
        return False


class Instance(Generic[TClass], Value[TClass]):
    """Represents an instance of a type at compile time"""

    @property
    def name(self) -> str:
        return self.klass.name + " instance"

    def emit_call(self, node: ast.Call, code_gen: StaticCodeGenerator) -> None:
        code_gen.defaultVisit(node)

    def bind_attr(self, node: ast.Attribute, visitor: TypeBinder, type_ctx) -> None:
        for base in self.klass.mro:
            member = base.members.get(node.attr)
            if member is not None:
                member.bind_descr_get(node, self, self.klass, visitor, type_ctx)
                return

        super().bind_attr(node, visitor, type_ctx)

    def emit_attr(self, node: ast.Attribute, code_gen: StaticCodeGenerator):
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

    def __repr__(self) -> str:
        return f"<{self.klass.name} instance>"


class DynamicClass(Class):
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
    def type_descr(self):
        # Any references to dynamic at runtime are object
        return OBJECT_TYPE.type_descr

    def can_assign_from(self, src: Class) -> bool:
        # No automatic boxing to the dynamic type
        return not isinstance(src, PrimitiveType)

    def emit_type_check(self, code_gen: StaticCodeGenerator, src: Class) -> None:
        code_gen.emit("CAST", self.type_descr)


class DynamicInstance(Instance):
    def __init__(self, klass: DynamicClass) -> None:
        super().__init__(klass)

    @property
    def name(self) -> str:
        return "dynamic"

    def bind_constant(self, node: ast.Constant, visitor: TypeBinder) -> None:
        n = node.value
        if isinstance(n, int):
            visitor.set_type(node, INT_TYPE.instance)
        elif isinstance(n, float):
            visitor.set_type(node, FLOAT_TYPE.instance)
        elif isinstance(n, complex):
            visitor.set_type(node, COMPLEX_TYPE.instance)
        elif isinstance(n, str):
            visitor.set_type(node, STR_TYPE.instance)
        else:
            # could be a tuple
            visitor.set_type(node, DYNAMIC)

    def bind_num(self, node: ast.Num, visitor: TypeBinder) -> None:
        n = node.n
        if isinstance(n, int):
            visitor.set_type(node, INT_TYPE.instance)
        elif isinstance(n, float):
            visitor.set_type(node, FLOAT_TYPE.instance)
        elif isinstance(n, complex):
            visitor.set_type(node, COMPLEX_TYPE.instance)


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
    ):
        self.name = name
        self.type_ref = type_ref
        self.index = idx
        self.has_default = has_default
        self.default_val = default_val
        self.is_kwonly = is_kwonly

    def __repr__(self):
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
            return Parameter(
                self.name,
                self.index,
                ResolvedTypeRef(name.args[klass.index]),
                self.has_default,
                self.default_val,
                self.is_kwonly,
            )
        elif klass.contains_generic_parameters:
            # map the generic type parameters for the type to the parameters provided
            bind_args = tuple(name.args[arg.index] for arg in klass.type_name.args)
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
            pass

        return self


def is_subsequence(a: Iterable, b: Iterable):
    # for loops go brrrr :)
    # https://ericlippert.com/2020/03/27/new-grad-vs-senior-dev/
    itr = iter(a)
    for each in b:
        if each not in itr:
            return False
    return True


class Callable(Instance):
    def __init__(
        self,
        klass: Class,
        func_name: str,
        args: List[Parameter],
        args_by_name: Dict[str, Parameter],
        num_required_args: int,
        return_type: TypeRef,
    ) -> None:
        super().__init__(klass)
        self.func_name = func_name
        self.args = args
        self.args_by_name = args_by_name
        self.num_required_args = num_required_args
        assert return_type is not None
        self.return_type = return_type

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx: Optional[Class]
    ) -> NarrowingEffect:
        return self.bind_call_self(node, visitor, type_ctx)

    def bind_call_self(
        self,
        node: ast.Call,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
        self_type=None,
    ) -> NarrowingEffect:
        visitor.set_type(node, self.return_type.resolved.instance)

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

            visitor.visit(call_arg)
            resolved_type = defined_arg.type_ref.resolved
            if isinstance(call_arg, Starred):
                # Skip type verification here, f(a, b, *something)
                # TODO: add support for this by implementing type constrained tuples
                continue
            if resolved_type is None:
                resolved_type = DYNAMIC_TYPE
            visitor.check_can_assign_from(
                resolved_type,
                visitor.get_type(call_arg).klass,
                node,
                "positional argument type mismatch",
            )

        for kw_arg in node.keywords:
            call_arg = kw_arg.value
            visitor.visit(call_arg)
            argname = kw_arg.arg
            if argname is None:
                # Gotta skip this, we cannot handle verification of types for `f(**something)`.
                # TODO: add support for this by implementing type constrained dicts
                continue

            if argname not in self.args_by_name:
                raise visitor.syntax_error(
                    f"Given argument {kw_arg.arg} "
                    f"does not exist in the definition of {self.func_name}",
                    node,
                )

            defined_arg = self.args_by_name[argname]
            resolved_type = defined_arg.type_ref.resolved
            if resolved_type is None:
                resolved_type = DYNAMIC_TYPE
            visitor.check_can_assign_from(
                resolved_type,
                visitor.get_type(call_arg).klass,
                node,
                "keyword argument type mismatch",
            )
        return NO_EFFECT

    def _emit_kwarg_temps(
        self, keywords: List[ast.keyword], code_gen: StaticCodeGenerator
    ) -> Dict[str, str]:
        temporaries = {}
        for _idx, each in enumerate(keywords):
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

    def emit_call_self(  # noqa: C901
        self,
        node: ast.Call,
        code_gen: StaticCodeGenerator,
        self_type: Optional[TypeName] = None,
    ) -> None:
        assert self.can_call_self(node, self_type is not None)

        if not self_type:
            code_gen.visit(node.func)

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

        # For all args that are not passed positionally, push NULL to signify which ones should use
        # the defaults
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
            type_descr = self_type.type_descr
            type_descr += (self.func_name,)
            code_gen.emit_invoke_method(type_descr, nseen - 1)
            pass
        else:
            code_gen.emit("INVOKE_FUNCTION", nseen)


class Function(Callable):
    def __init__(
        self,
        name: str,
        decl_type: Optional[TypeName],
        node: Union[AsyncFunctionDef, FunctionDef],
        ret_type: TypeRef,
    ):
        super().__init__(FUNCTION_TYPE, name, [], {}, 0, ret_type)
        self.decl_type = decl_type
        self.node = node

    @property
    def name(self) -> str:
        return f"function {self.func_name}"

    def emit_call(self, node: ast.Call, code_gen: StaticCodeGenerator) -> None:
        if not self.can_call_self(node, False):
            return super().emit_call(node, code_gen)

        return self.emit_call_self(node, code_gen, None)

    def bind_descr_get(
        self,
        node: ast.Attribute,
        inst: Optional[Instance],
        ctx: Class,
        visitor: TypeBinder,
        type_ctx,
    ):
        decl_type = self.decl_type
        if decl_type is None:
            # This is probably impossible, we have no tracking of when a non-declared function
            # would end up in a class.
            visitor.set_type(node, DYNAMIC)
        elif inst is None:
            visitor.set_type(node, self)
        else:
            visitor.set_type(node, MethodType(decl_type, self.node, node, self))

    def register_arg(self, name, idx, ref, has_default, default_val, is_kwonly):
        parameter = Parameter(name, idx, ref, has_default, default_val, is_kwonly)
        self.args.append(parameter)
        self.args_by_name[name] = parameter
        if not has_default:
            self.num_required_args += 1

    def __repr__(self):
        return f"<{self.klass.name} '{self.name}' instance, args={self.args}>"


class MethodType(Value):
    def __init__(
        self,
        decl_type: TypeName,
        node: Union[AsyncFunctionDef, FunctionDef],
        target: ast.Attribute,
        function: Function,
    ):
        super().__init__(METHOD_TYPE)
        self.decl_type = decl_type
        self.node = node
        self.target = target
        self.function = function

    @property
    def name(self) -> str:
        return "method " + self.function.func_name

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        return self.function.bind_call_self(
            node, visitor, type_ctx, visitor.get_type(self.target.value)
        )

    def emit_call(self, node: ast.Call, code_gen: StaticCodeGenerator):
        if not self.function.can_call_self(node, True):
            return super().emit_call(node, code_gen)

        code_gen.update_lineno(node)
        code_gen.visit(self.target.value)

        code_gen.get_type(self.target.value).emit_check_self(code_gen, self.target.attr)

        self.function.emit_call_self(node, code_gen, self.decl_type)


class BuiltinMethodDescriptor(Callable):
    def __init__(
        self,
        decl_type: TypeName,
        func_name: str,
        args: Optional[Tuple[Parameter, ...]] = None,
        return_type: Optional[TypeRef] = None,
    ):
        assert isinstance(return_type, (TypeRef, type(None)))
        super().__init__(
            BUILTIN_METHOD_DESC_TYPE,
            func_name,
            args,
            {},
            0,
            return_type or ResolvedTypeRef(DYNAMIC_TYPE),
        )
        self.decl_type = decl_type
        self.func_name = func_name

    def bind_call_self(
        self,
        node: ast.Call,
        visitor: TypeBinder,
        type_ctx: Optional[Class],
        self_type=None,
    ):
        if self.args is not None:
            return super().bind_call_self(node, visitor, type_ctx, self_type)
        elif node.keywords:
            return super().bind_call(node, visitor, type_ctx)

        visitor.set_type(node, DYNAMIC)
        for arg in node.args:
            visitor.visit(arg)

    def bind_descr_get(
        self,
        node: ast.Attribute,
        inst: Optional[Instance],
        ctx: Class,
        visitor: TypeBinder,
        type_ctx,
    ):
        if inst is None:
            visitor.set_type(node, self)
        else:
            visitor.set_type(node, BuiltinMethod(self, node))

    def make_generic(
        self,
        name: GenericTypeName,
        generic_types: Dict[Class, Dict[Tuple[Class, ...], Class]],
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


class BuiltinMethod(Callable):
    def __init__(self, desc: BuiltinMethodDescriptor, target: ast.Attribute):
        super().__init__(
            BUILTIN_METHOD_TYPE, desc.func_name, desc.args, None, 0, desc.return_type
        )
        self.desc = desc
        self.target = target

    @property
    def name(self):
        return self.func_name

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        if self.args:
            return super().bind_call_self(
                node, visitor, type_ctx, visitor.get_type(self.target.value)
            )
        if node.keywords:
            return Instance.bind_call(self, node, visitor, type_ctx)

        visitor.set_type(node, self)
        visitor.visit(self.target.value)
        for arg in node.args:
            visitor.visit(arg)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: StaticCodeGenerator):
        if node.keywords or (
            self.args is not None and self.desc.can_call_self(node, True)
        ):
            return super().emit_call(node, code_gen)

        code_gen.update_lineno(node)
        code_gen.visit(self.target.value)

        code_gen.get_type(self.target.value).emit_check_self(code_gen, self.target.attr)

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


class StrictBuiltins(Instance):
    def __init__(self, builtins: Dict[str, Value]):
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


def get_default_value(default: expr):
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
):
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
    for idx, (argument, default) in enumerate(
        zip(arguments.kwonlyargs, arguments.kw_defaults)
    ):
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
        func.register_arg(
            argument.arg, base_idx + idx + 1, ref, has_default, default_val, True
        )

    kwarg = arguments.kwarg
    if kwarg:
        raise visitor.syntax_error(
            f"Cannot support generic keyword arguments: **{kwarg.arg}", kwarg
        )

    vararg = arguments.vararg
    if vararg:
        raise visitor.syntax_error(
            f"Cannot support generic varargs: *{vararg.arg}", vararg
        )


# Bringing up the type system is a little special as we have dependencies
# amongst type and object
TYPE_TYPE = Class.__new__(Class)
TYPE_TYPE.type_name = TypeName("builtins", "type")
TYPE_TYPE.klass = TYPE_TYPE
TYPE_TYPE.instance = TYPE_TYPE
TYPE_TYPE.members = {}
TYPE_TYPE._mro = None


class Slot(Value):
    def __init__(self, type_ref: TypeRef, name: str, decl_type: Class):
        super().__init__(MEMBER_TYPE)
        self.decl_type = decl_type
        self.slot_name = name
        self.type_ref = type_ref

    def bind_descr_get(
        self,
        node: ast.Attribute,
        inst: Optional[Instance],
        ctx: Class,
        visitor: TypeBinder,
        type_ctx,
    ):
        if inst is None:
            visitor.set_type(node, self)
            return

        resolved = self.type_ref.resolved
        if resolved is not None:
            visitor.set_type(node, resolved.instance)
        else:
            visitor.set_type(node, DYNAMIC)

    @property
    def type_descr(self):
        resolved = self.type_ref.resolved
        if resolved == DYNAMIC_TYPE or resolved is None:
            return None

        return resolved.type_descr


# TODO (aniketpanse): move these to a better place
OBJECT_TYPE = Class(TypeName("builtins", "object"))
OBJECT = OBJECT_TYPE.instance

DYNAMIC_TYPE = DynamicClass()
DYNAMIC = DYNAMIC_TYPE.instance


class BoxFunction(Value):
    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        if len(node.args) != 1:
            raise visitor.syntax_error("box only accepts a single argument", node)

        for arg in node.args:
            visitor.visit(arg)
            if not isinstance(visitor.get_type(arg), Primitive):
                raise visitor.syntax_error(
                    f"can't box non-primitive: {visitor.get_type(arg).name}", node
                )
        visitor.set_type(node, DYNAMIC)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: StaticCodeGenerator):
        code_gen.get_type(node.args[0]).emit_box(node.args[0], code_gen)


class UnboxFunction(Value):
    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        if len(node.args) != 1:
            raise visitor.syntax_error("unbox only accepts a single argument", node)

        for arg in node.args:
            visitor.visit(arg, DYNAMIC)
        visitor.set_type(node, INT64_VALUE)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: StaticCodeGenerator):
        code_gen.get_type(node).emit_unbox(node.args[0], code_gen)


class LenFunction(Value):
    def __init__(self, klass: TClass):
        super().__init__(klass)
        self._is_array_len = False

    @property
    def name(self) -> str:
        return "len function"

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        if len(node.args) != 1:
            visitor.syntax_error(
                f"len() does not accept more than one arguments ({len(node.args)} given)",
                node,
            )
        visitor.visit(node.args[0])
        visitor.set_type(node, INT_TYPE.instance)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: StaticCodeGenerator) -> None:
        code_gen.get_type(node.args[0]).emit_len(node, code_gen)


class IsInstanceFunction(Value):
    def __init__(self):
        super().__init__(FUNCTION_TYPE)

    @property
    def name(self) -> str:
        return "isinstance function"

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        if node.keywords:
            visitor.syntax_error("isinstance() does not accept keyword arguments", node)
        for arg in node.args:
            visitor.visit(arg)
        visitor.set_type(node, BOOL_TYPE.instance)
        if len(node.args) == 2:
            arg0 = node.args[0]
            if not isinstance(arg0, ast.Name):
                return NO_EFFECT

            klass_type = visitor.get_type(node.args[1])
            if isinstance(klass_type, Class):
                return IsInstanceEffect(
                    arg0.id, visitor.get_type(arg0), klass_type.instance
                )

        return NO_EFFECT


class IsSubclassFunction(Value):
    def __init__(self):
        super().__init__(FUNCTION_TYPE)

    @property
    def name(self) -> str:
        return "issubclass function"

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        if node.keywords:
            visitor.syntax_error("issubclass() does not accept keyword arguments", node)
        for arg in node.args:
            visitor.visit(arg)
        visitor.set_type(node, BOOL_TYPE.instance)
        return NO_EFFECT


class NumInstance(Instance):
    def bind_unaryop(self, node: ast.UnaryOp, visitor: TypeBinder, type_ctx) -> None:
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
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx
    ) -> bool:
        visitor.set_type(node, self)
        return False


class NumType(Class):
    def __init__(self, name: TypeName, pytype: Optional[Type] = None):
        super().__init__(name, [RESOLVED_OBJECT_TYPE], NumInstance(self), pytype=pytype)


def parse_type(info: Dict[str, object]) -> Class:
    optional = info.get("optional", False)
    type = info.get("type")
    if type:
        if type == "object":
            klass = OBJECT_TYPE
        elif type == "str":
            klass = STR_TYPE
        elif type == "int":
            klass = INT_TYPE
        elif type == "NoneType":
            klass = NONE_TYPE
        else:
            raise NotImplementedError("unsupported type: " + str(type))
    else:
        type_param = info.get("type_param")
        assert isinstance(type_param, int)
        klass = GenericParameter("T" + str(type_param), type_param)

    if optional:
        return OPTIONAL_CLASS.make_generic_type((klass,), BUILTIN_GENERICS)

    return klass


def parse_param(info: Dict[str, object], idx: int) -> Parameter:
    name = info.get("name", "")
    assert isinstance(name, str)

    return Parameter(
        name, idx, ResolvedTypeRef(parse_type(info)), "default" in info, None, False
    )


def make_type_dict(klass: Class, t: Type) -> Dict[str, Value]:
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


FUNCTION_TYPE = Class(TypeName("builtins", "function"))
METHOD_TYPE = Class(TypeName("builtins", "method"))
MEMBER_TYPE = Class(TypeName("builtins", "member"))
BUILTIN_METHOD_DESC_TYPE = Class(TypeName("builtins", "method_descriptor"))
BUILTIN_METHOD_TYPE = Class(TypeName("builtins", "builtin_function_or_method"))
SLICE_TYPE = Class(TypeName("builtins", "slice"))

# builtin types
NONE_TYPE = Class(TypeName("builtins", "None"))
STR_TYPE = Class(TypeName("builtins", "str"), pytype=str)

RESOLVED_OBJECT_TYPE = ResolvedTypeRef(OBJECT_TYPE)

INT_TYPE = NumType(TypeName("builtins", "int"), pytype=int)
FLOAT_TYPE = NumType(TypeName("builtins", "float"), pytype=float)
COMPLEX_TYPE = NumType(TypeName("builtins", "complex"))
BYTES_TYPE = Class(TypeName("builtins", "bytes"), pytype=bytes)
BOOL_TYPE = Class(TypeName("builtins", "bool"), pytype=bool)
DICT_TYPE = Class(TypeName("builtins", "dict"), pytype=dict)
BASE_EXCEPTION_TYPE = Class(TypeName("builtins", "BaseException"), pytype=BaseException)
EXCEPTION_TYPE = Class(
    TypeName("builtins", "Exception"),
    bases=[ResolvedTypeRef(BASE_EXCEPTION_TYPE)],
    pytype=Exception,
)

RESOLVED_INT_TYPE = ResolvedTypeRef(INT_TYPE)
RESOLVED_STR_TYPE = ResolvedTypeRef(STR_TYPE)
RESOLVED_NONE_TYPE = ResolvedTypeRef(NONE_TYPE)

TYPE_TYPE._bases = [RESOLVED_OBJECT_TYPE]

CONSTANT_TYPES = {
    str: STR_TYPE.instance,
    int: INT_TYPE.instance,
    float: FLOAT_TYPE.instance,
    complex: COMPLEX_TYPE.instance,
    bytes: BYTES_TYPE.instance,
    bool: BOOL_TYPE.instance,
    type(None): NONE_TYPE.instance,
}

NAMED_TUPLE_TYPE = Class(TypeName("typing", "NamedTuple"))


class OptionalInstance(Instance["OptionalClass"]):
    def bind_attr(self, node: ast.Attribute, visitor: TypeBinder, type_ctx):
        index = self.klass.index
        if index.is_generic_parameter:
            raise visitor.syntax_error(
                "cannot access attribute from bare Optional", node
            )

        index.instance.bind_attr(node, visitor, type_ctx)

    def emit_attr(self, node: ast.Attribute, code_gen: StaticCodeGenerator):
        index = self.klass.index
        if index is None:
            raise TypeError("index shouldn't be None")

        for base in index.mro:
            member = base.members.get(node.attr)
            if member is not None and isinstance(member, Slot):
                self.emit_check_self(code_gen, node.attr)
                # Fall into the base klass which will do the same
                # scan and emit the appropriate attibute lookup
                break

        index.instance.emit_attr(node, code_gen)

    def emit_check_self(self, code_gen: StaticCodeGenerator, name: str) -> None:
        code_gen.emit("RAISE_IF_NONE", name)

    def __repr__(self) -> str:
        return f"Optional[{self.klass.index.name!r}]"


class ArrayInstance(Instance["ArrayClass"]):
    def emit_len(self, node: ast.Call, code_gen: StaticCodeGenerator) -> None:
        if len(node.args) != 1:
            raise code_gen.syntax_error(
                "Can only pass a single argument when checking array length", node
            )
        code_gen.visit(node.args[0])
        code_gen.emit("FAST_LEN")

    def bind_subscr(
        self, node: ast.Subscript, type: Value, visitor: TypeBinder
    ) -> None:
        if type == SLICE_TYPE.instance:
            # Slicing preserves type
            return visitor.set_type(node, self)

        index = self.klass.index
        if index is not None:
            visitor.set_type(node, index.instance)
        else:
            visitor.set_type(node, DYNAMIC)

    def __repr__(self) -> str:
        return f"Array[{self.klass.index.name!r}]"


class ArrayClass(Class):
    @property
    def name(self) -> str:
        index = self.index
        if index is not None:
            return f"Array[{index.name}]"
        return "Array[T]"

    def __init__(self, name: TypeName, index: Optional[Class] = None):
        super().__init__(name, [RESOLVED_OBJECT_TYPE], instance=ArrayInstance(self))
        self.index = index

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        try:
            array_type = visitor.symtable.generic_types[ARRAY_TYPE][
                self.index,
            ].instance
        except KeyError:
            # Should never happen, we check this while looking at the subscript
            raise visitor.syntax_error("Invalid Array type", node)
        else:
            visitor.set_type(node, array_type)

        for arg in node.args:
            visitor.visit(arg)
        return NO_EFFECT

    def bind_subscr(self, node: ast.Subscript, type: Value, visitor: TypeBinder):
        if (type,) not in visitor.symtable.generic_types[ARRAY_TYPE]:
            raise visitor.syntax_error(f"Invalid array element type: {type.name}", node)

        visitor.set_type(
            node,
            visitor.symtable.generic_types[ARRAY_TYPE][
                type,
            ],
        )
        assert isinstance(type, Class)

    def make_generic_type(
        self,
        index: Tuple[Class, ...],
        generic_types: Dict[Class, Dict[Tuple[Class, ...], Class]],
    ) -> Optional[Class]:
        # For arrays, generic types are pre-populated
        # TODO: We should be able to report an error in the future when we
        # flow the TypeBinder down instead of just returning None
        return generic_types[self].get(index)


class OptionalClass(GenericClass):
    def __init__(
        self,
        name: GenericTypeName,
        bases: Optional[List[TypeRef]] = None,
        instance: Optional[Instance] = None,
        klass: Optional[Class] = None,
        members: Optional[Dict[str, Value]] = None,
        type_def: Optional[GenericClass] = None,
        pytype: Optional[Type] = None,
    ):
        super().__init__(
            name, bases, OptionalInstance(self), klass, members, type_def, pytype
        )

    @property
    def index(self) -> Class:
        return self.type_args[0]

    @property
    def name(self) -> str:
        index = self.index
        if index.is_generic_parameter:
            return f"Optional[{index.type_name.name}]"

        return f"Optional[{index.name}]"

    @property
    def type_descr(self):
        if not self.type_def:
            # Optional can't be instantiated w/o a type, so we'll
            # treat it as dynamic if someone tries to refer to it
            # anywhere.
            return ("builtins", "object")
        return self.index.type_descr + ("?",)

    def can_assign_from(self, src: Class) -> bool:
        # We can only assign None or instances of T
        if self == src or src == NONE_TYPE or src == self.index:
            return True

        index = self.index
        if index.is_generic_parameter:
            # can't use bare Optional as a target to assign to
            return False

        if isinstance(src, OptionalClass):
            src_index = src.index
            if src_index is None:
                return False
            return index.issubclass(src_index)
        else:
            return index.issubclass(src)


BUILTIN_GENERICS: Dict[Class, Dict[Union[Class, Tuple[Class, ...]], Class]] = {}
OPTIONAL_CLASS = OptionalClass(
    GenericTypeName("typing", "Optional", (GenericParameter("T", 0),))
)


class CastFunction(Value):
    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        if len(node.args) != 2:
            raise visitor.syntax_error(
                "cast requires two parameters: type and value", node
            )

        for arg in node.args:
            visitor.visit(arg)

        cast_type = visitor.cur_mod.resolve_annotation(node.args[0])
        if cast_type is None:
            raise visitor.syntax_error("cast to unknown type", node)

        visitor.set_type(node, cast_type.instance)
        return NO_EFFECT

    def emit_call(self, node: ast.Call, code_gen: StaticCodeGenerator):
        code_gen.visit(node.args[1])
        code_gen.emit("CAST", code_gen.get_type(node).klass.type_descr)


# These need to be kept in sync with the version in classloader.h:
TYPED_INT_8BIT = 0
TYPED_INT_16BIT = 1
TYPED_INT_32BIT = 2
TYPED_INT_64BIT = 3

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


class Primitive(Instance):
    pass


class IntInstance(Primitive):
    def __init__(self, klass: IntType, size: int, signed: bool):
        super().__init__(klass)
        self.size = size
        self.signed = signed

    @property
    def name(self) -> str:
        return self.klass.name

    _int_binary_opcode_signed = {
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

    _int_binary_opcode_unsigned = {
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

    def get_op_id(self, op: AST):
        return (
            self._int_binary_opcode_signed[type(op)]
            if self.signed
            else (self._int_binary_opcode_unsigned[type(op)])
        )

    def validate_mixed_math(self, other: Value) -> Optional[Value]:
        if other is self:
            return self
        elif isinstance(other, IntInstance):
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
                    return INT_TYPES[new_size].instance

        return None

    def bind_compare(
        self,
        node: ast.Compare,
        left: expr,
        op: cmpop,
        right: expr,
        visitor: TypeBinder,
        type_ctx,
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
        type_ctx,
    ) -> bool:
        if not isinstance(visitor.get_type(left), IntInstance):
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

    def emit_compare(self, op: cmpop, code_gen: StaticCodeGenerator) -> None:
        code_gen.emit("INT_COMPARE_OP", self.get_op_id(op))

    def emit_binop(self, node: ast.BinOp, code_gen: StaticCodeGenerator) -> None:
        code_gen.update_lineno(node)
        code_gen.visit(node.left)
        code_gen.visit(node.right)
        code_gen.emit("INT_BINARY_OP", self.get_op_id(node.op))

    def emit_augassign(
        self, node: ast.AugAssign, code_gen: StaticCodeGenerator
    ) -> None:
        code_gen.set_lineno(node)
        aug_node = wrap_aug(node.target)
        code_gen.visit(aug_node, "load")
        code_gen.visit(node.value)
        code_gen.emit("INT_BINARY_OP", self.get_op_id(node.op))
        code_gen.visit(aug_node, "store")

    def emit_augname(
        self, node: AugName, code_gen: StaticCodeGenerator, mode: str
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

    def emit_constant(self, node: ast.Constant, code_gen: StaticCodeGenerator) -> None:
        if node.value < 0:
            code_gen.emit("INT_LOAD_CONST", -node.value)
            code_gen.emit("INT_UNARY_OP", PRIM_OP_NEG)
        elif node.value > 0x7FFFFFFF:
            code_gen.emit("LOAD_CONST", node.value)
            code_gen.emit("INT_UNBOX", int(self.signed))
        else:
            code_gen.emit("INT_LOAD_CONST", node.value)

    def bind_num(self, node: ast.Num, visitor: TypeBinder) -> None:
        self.validate_int(node.n, node, visitor)
        visitor.set_type(node, self)

    def emit_num(self, node: ast.Num, code_gen: StaticCodeGenerator) -> None:
        n = node.n
        assert isinstance(n, int)
        if n > 0x7FFFFFFF:
            code_gen.emit("LOAD_CONST", n)
            code_gen.emit("INT_UNBOX", int(self.signed))
        else:
            code_gen.emit("INT_LOAD_CONST", n)

    def emit_name(self, node: ast.Name, code_gen: StaticCodeGenerator) -> None:
        if isinstance(node.ctx, ast.Load):
            code_gen.emit("LOAD_LOCAL", (node.id, ("__static__", self.klass.name)))
        elif isinstance(node.ctx, ast.Store):
            code_gen.emit("STORE_LOCAL", (node.id, ("__static__", self.klass.name)))
        else:
            raise TypedSyntaxError("unsupported op")

    def emit_jumpif(
        self, test, next, is_if_true, code_gen: StaticCodeGenerator
    ) -> None:
        code_gen.visit(test)
        code_gen.emit("POP_JUMP_IF_NONZERO" if is_if_true else "POP_JUMP_IF_ZERO", next)

    def emit_jumpif_pop(
        self, test, next, is_if_true, code_gen: StaticCodeGenerator
    ) -> None:
        code_gen.visit(test)
        code_gen.emit(
            "JUMP_IF_NONZERO_OR_POP" if is_if_true else "JUMP_IF_ZERO_OR_POP", next
        )

    def bind_binop(self, node: ast.BinOp, visitor: TypeBinder, type_ctx) -> bool:
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

    def binop_error(self, left: Value, right: Value, op):
        return f"cannot {self._op_name[type(op)]} {left.name} and {right.name}"

    def bind_reverse_binop(
        self, node: ast.BinOp, visitor: TypeBinder, type_ctx
    ) -> bool:
        try:
            visitor.visit(node.left, self)
        except TypedSyntaxError:
            raise visitor.syntax_error(
                self.binop_error(visitor.get_type(node.left), self, node.op), node
            )
        visitor.set_type(node, self)
        return True

    def emit_box(self, node: expr, code_gen: StaticCodeGenerator) -> None:
        code_gen.visit(node)
        type = code_gen.get_type(node)
        if isinstance(type, IntInstance):
            code_gen.emit("INT_BOX", int(type.signed))
        else:
            raise RuntimeError("unsupported box type: " + type.name)

    def emit_unbox(self, node: expr, code_gen: StaticCodeGenerator) -> None:
        code_gen.visit(node)
        code_gen.emit("INT_UNBOX")

    def bind_unaryop(self, node: ast.UnaryOp, visitor: TypeBinder, type_ctx) -> None:
        if isinstance(node.op, (ast.USub, ast.Invert, ast.UAdd)):
            visitor.set_type(node, self)
        else:
            assert isinstance(node.op, ast.Not)
            visitor.set_type(node, BOOL_TYPE.instance)

    def emit_unaryop(self, node: ast.UnaryOp, code_gen: StaticCodeGenerator) -> None:
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


class IntType(PrimitiveType):
    def __init__(self, size: int, signed: bool, name_override: Optional[str] = None):
        if name_override is None:
            name = ("" if signed else "u") + "int" + str(8 << size)
        else:
            name = name_override
        self.size = size
        self.signed = signed
        super().__init__(
            TypeName("__static__", name),
            [RESOLVED_OBJECT_TYPE],
            IntInstance(self, size, signed),
        )

    @property
    def name(self) -> str:
        return self.type_name.name

    def can_assign_from(self, src: Class) -> bool:
        if isinstance(src, IntType):
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
    def type_descr(self):
        return ("__static__", self.name)

    def bind_call(
        self, node: ast.Call, visitor: TypeBinder, type_ctx
    ) -> NarrowingEffect:
        if len(node.args) != 1:
            raise visitor.syntax_error(
                f"{self.name} requires a single argument ({len(node.args)} given)", node
            )

        return super().bind_call(node, visitor, type_ctx)

    def emit_call(self, node: ast.Call, code_gen: StaticCodeGenerator) -> None:
        if len(node.args) != 1:
            raise code_gen.syntax_error(
                f"{self.name} requires a single argument ({len(node.args)} given)", node
            )

        arg = node.args[0]
        arg_type = code_gen.get_type(arg)
        if isinstance(arg, Num):
            return self.instance.emit_num(arg, code_gen)
        elif isinstance(arg, Constant):
            return self.instance.emit_constant(arg, code_gen)
        elif isinstance(arg_type, IntInstance):
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


class DoubleInstance(Instance):
    pass


class DoubleType(PrimitiveType):
    def __init__(self):
        name = "double"
        super().__init__(
            TypeName("__static__", name), [RESOLVED_OBJECT_TYPE], DoubleInstance(self)
        )


INT8_TYPE = IntType(TYPED_INT_8BIT, True)
INT16_TYPE = IntType(TYPED_INT_16BIT, True)
INT32_TYPE = IntType(TYPED_INT_32BIT, True)
INT64_TYPE = IntType(TYPED_INT_64BIT, True)

UINT8_TYPE = IntType(TYPED_INT_8BIT, False)
UINT16_TYPE = IntType(TYPED_INT_16BIT, False)
UINT32_TYPE = IntType(TYPED_INT_32BIT, False)
UINT64_TYPE = IntType(TYPED_INT_64BIT, False)

INT64_VALUE = INT64_TYPE.instance

CHAR_TYPE = IntType(TYPED_INT_8BIT, True, name_override="char")
DOUBLE_TYPE = DoubleType()
ARRAY_TYPE = ArrayClass(GenericTypeName("__static__", "Array", ()), None)

INT_TYPES = [INT8_TYPE, INT16_TYPE, INT32_TYPE, INT64_TYPE]
UNSIGNED_INT_TYPES = [UINT8_TYPE, UINT16_TYPE, UINT32_TYPE, UINT64_TYPE]
ALL_INT_TYPES = INT_TYPES + UNSIGNED_INT_TYPES


if spamobj is not None:
    SPAM_OBJ = GenericClass(
        GenericTypeName("xxclassloader", "spamobj", (GenericParameter("T", 0),)),
        pytype=spamobj,
    )
else:
    SPAM_OBJ: Optional[GenericClass] = None


BUILTIN_GENERICS.update(
    {
        ARRAY_TYPE: {
            (prim,): ArrayClass(
                GenericTypeName("__static__", "Array", (prim,)), index=prim
            )
            for prim in (
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
            )
        }
    }
)


class GenericVisitor(ASTVisitor):
    def __init__(self, module_name: str, filename: str):
        super().__init__()
        self.module_name = module_name
        self.filename = filename

    def syntax_error(self, msg: str, node: AST):
        source_line = linecache.getline(self.filename, node.lineno)
        return TypedSyntaxError(
            msg, (self.filename, node.lineno, node.col_offset, source_line or None)
        )


class DeclarationVisitor(GenericVisitor):
    def __init__(self, mod_name: str, filename: str, symbols: SymbolTable):
        super().__init__(mod_name, filename)
        self.module = symbols[mod_name] = ModuleTable(mod_name, symbols)

    def visitClassDef(self, node: ClassDef):  # noqa: C901
        bases = [TypeRef(self.module, base) for base in node.bases]
        if not bases:
            bases.append(RESOLVED_OBJECT_TYPE)
        klass = self.module.children[node.name] = Class(
            TypeName(self.module_name, node.name), bases
        )
        self.module.decls.append((node, klass))
        for item in node.body:
            if isinstance(item, (AsyncFunctionDef, FunctionDef)):
                if item.decorator_list:
                    continue

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

    def visitFunctionDef(self, node: Union[FunctionDef, AsyncFunctionDef]):
        # Ignore functions with decorators, we don't know what they return yet
        if not node.decorator_list:
            function = self.module.children[node.name] = Function(
                f"{self.module_name}.{node.name}",
                None,
                node,
                self.type_ref(node.returns),
            )
            self.module.decls.append((node, function))
            process_function_args(function, node.args, self)

    visitAsyncFunctionDef = visitFunctionDef

    def visitImport(self, node: Import) -> None:
        self.module.decls.append((node, None))

    def visitImportFrom(self, node: ImportFrom) -> None:
        self.module.decls.append((node, None))

    def type_ref(self, ann: Optional[expr]):
        if not ann:
            return ResolvedTypeRef(DYNAMIC_TYPE)
        return TypeRef(self.module, ann)

    # We don't pick up declarations in nested statements
    def visitFor(self, node: For):
        pass

    def visitAsyncFor(self, node: AsyncFor):
        pass

    def visitWhile(self, node: While):
        pass

    def visitIf(self, node: If):
        pass

    def visitWith(self, node: With):
        pass

    def visitAsyncWith(self, node: AsyncWith):
        pass

    def visitTry(self, node: Try):
        pass


class TypedSyntaxError(SyntaxError):
    pass


class LocalsBranch:
    """Handles branching and merging local variable types"""

    def __init__(self, scope: BindingScope):
        self.scope = scope
        self.entry_locals = dict(scope.local_types)

    def copy(self):
        """Make a copy of the current local state"""
        return dict(self.scope.local_types)

    def restore(self, state=None):
        """Restore the locals to the state when we entered"""
        self.scope.local_types.clear()
        self.scope.local_types.update(state or self.entry_locals)

    def merge(self, entry_locals=None):
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

        for key, _value in local_types.items():
            # If a value isn't definitely assigned we can safely turn it
            # back into the declared type
            if key not in entry_locals and key in self.scope.decl_types:
                local_types[key] = self.scope.decl_types[key].type.instance

    def _widest_type(self, *types: Value) -> Optional[Value]:
        if len(types) == 1:
            return types[0]

        widest_type = None
        for src in types:
            if src == DYNAMIC:
                continue

            if widest_type is None or src.klass.can_assign_from(widest_type.klass):
                widest_type = src
            elif widest_type is not None and not widest_type.klass.can_assign_from(
                src.klass
            ):
                return None

        return widest_type


class TypeDeclaration:  # noqa: B903
    def __init__(self, type: Class, node: AST):
        self.type = type
        self.node = node


class BindingScope:
    def __init__(self, node: AST, types: Optional[Dict[str, Value]] = None):
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

    def apply(self, binder: TypeBinder) -> None:
        """applies the given effect in the target scope"""
        pass

    def undo(self, binder: TypeBinder) -> None:
        """restores the type to its original value"""
        pass

    def reverse(self, binder: TypeBinder) -> None:
        """applies the reverse of the scope or reverts it if
        there is no reverse"""
        self.undo(binder)


class CombinedEffect(NarrowingEffect):
    def __init__(self, *effects: NarrowingEffect) -> None:
        self.effects = effects

    def union(self, other: NarrowingEffect) -> NarrowingEffect:
        if other is NoEffect:
            return self
        elif isinstance(other, CombinedEffect):
            return CombinedEffect(*self.effects, *other.effects)

        return CombinedEffect(*self.effects, other)

    def apply(self, binder: TypeBinder) -> None:
        for effect in self.effects:
            effect.apply(binder)

    def undo(self, binder: TypeBinder) -> None:
        """restores the type to its original value"""
        for effect in self.effects:
            effect.undo(binder)

    def reverse(self, binder: TypeBinder) -> None:
        """applies the reverse of the scope or reverts it if
        there is no reverse"""
        for effect in self.effects:
            effect.reverse(binder)


class NoEffect(NarrowingEffect):
    def union(self, other: NarrowingEffect):
        return other


# Singleton instance for no effects
NO_EFFECT = NoEffect()


class NotNoneEffect(NarrowingEffect):
    def __init__(self, var: str, prev: OptionalInstance):
        self.var = var
        self.prev = prev

    def apply(self, binder: TypeBinder) -> None:
        index = self.prev.klass.index
        if index is None:
            raise TypeError("shouldn't have None instance")
        binder.local_types[self.var] = index.instance

    def undo(self, binder: TypeBinder) -> None:
        binder.local_types[self.var] = self.prev

    def reverse(self, binder: TypeBinder) -> None:
        binder.local_types[self.var] = NONE_TYPE.instance


class NoneEffect(NarrowingEffect):
    def __init__(self, var: str, prev: OptionalInstance):
        self.var = var
        self.prev = prev

    def apply(self, binder: TypeBinder) -> None:
        binder.local_types[self.var] = NONE_TYPE.instance

    def undo(self, binder: TypeBinder) -> None:
        binder.local_types[self.var] = self.prev

    def reverse(self, binder: TypeBinder) -> None:
        index = self.prev.klass.index
        if index is None:
            raise TypeError("shouldn't have None instance")
        binder.local_types[self.var] = index.instance


class IsInstanceEffect(NarrowingEffect):
    def __init__(self, var: str, prev: Value, inst: Value):
        self.var = var
        self.prev = prev
        self.inst = inst

    def apply(self, binder: TypeBinder) -> None:
        binder.local_types[self.var] = self.inst

    def undo(self, binder: TypeBinder) -> None:
        binder.local_types[self.var] = self.prev

    def reverse(self, binder: TypeBinder) -> None:
        binder.local_types[self.var] = self.prev


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
        self.returns = set()

    @property
    def local_types(self) -> Dict[str, Value]:
        return self.scopes[-1].local_types

    @property
    def decl_types(self) -> Dict[str, TypeDeclaration]:
        return self.scopes[-1].decl_types

    @property
    def scope(self) -> AST:
        return self.scopes[-1].node

    def declare_local(self, target: ast.Name, type: Class) -> TypeDeclaration:
        if target.id in self.decl_types:
            raise self.syntax_error(
                f"Cannot redefine local variable {target.id}", target
            )

        self.decl_types[target.id] = decl_type = TypeDeclaration(type, target)
        if isinstance(type, PrimitiveType):
            self.check_primitive_scope(target)
        return decl_type

    def visitModule(self, node: Module):
        self.scopes.append(BindingScope(node, self.cur_mod.children))

        for statement in node.body:
            self.visit(statement)

        self.scopes.pop()

    def set_param(self, arg: ast.arg, arg_type: Class, scope: BindingScope) -> None:
        scope.local_types[arg.arg] = arg_type.instance
        scope.decl_types[arg.arg] = TypeDeclaration(arg_type, arg)
        self.set_type(arg, arg_type.instance)

    def visitFunctionDef(  # noqa: C901
        self, node: Union[FunctionDef, AsyncFunctionDef]
    ):
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

            self.set_param(vararg, DYNAMIC_TYPE, scope)

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

            self.set_param(kwarg, DYNAMIC_TYPE, scope)

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

        for statement in node.body:
            self.visit(statement)

        self.scopes.pop()

    visitAsyncFunctionDef = visitFunctionDef

    def visitClassDef(self, node: ClassDef):
        for decorator in node.decorator_list:
            self.visit(decorator)

        for kwarg in node.keywords:
            self.visit(kwarg.value)

        for base in node.bases:
            self.visit(base)

        self.scopes.append(BindingScope(node))

        for statement in node.body:
            self.visit(statement)

        self.scopes.pop()

    def set_type(self, node: AST, type: Value) -> None:
        self.cur_mod.types[node] = type

    def get_type(self, node: AST) -> Value:
        return self.cur_mod.types[node]

    def check_primitive_scope(self, node: Name):
        cur_scope = self.symbols.scopes[self.scope]
        var_scope = cur_scope.check_name(node.id)
        if var_scope != SC_LOCAL or isinstance(self.scope, Module):
            raise self.syntax_error(
                "cannot use primitives in global or closure scope", node
            )

    def visitAnnAssign(self, node: AnnAssign):
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

    def visitAugAssign(self, node: AugAssign):
        self.visit(node.target)
        self.visit(node.value, self.get_type(node.target))
        self.set_type(node, self.get_type(node.target))

    def visitAssign(self, node: Assign):
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
            if isinstance(target, ast.Name):
                decl_type = self.decl_types.get(target.id)
                if decl_type is None:
                    # This is the first declaration of this variable
                    decl_type = self.declare_local(target, value_type.klass)
                else:
                    self.check_can_assign_from(decl_type.type, value_type.klass, target)

                local_type = (
                    value_type
                    if decl_type.type.can_be_narrowed and value_type is not DYNAMIC
                    else decl_type.type.instance
                )
                self.local_types[target.id] = local_type
                self.set_type(target, decl_type.type.instance)
            else:
                target_type = self.get_type(target)
                self.check_can_assign_from(target_type.klass, value_type.klass, target)

        self.set_type(node, value_type)

    def check_can_assign_from(
        self, dest: Class, src: Class, node: AST, reason="cannot be assigned to"
    ) -> None:
        if not dest.can_assign_from(src) and src is not DYNAMIC_TYPE:
            raise self.syntax_error(
                f"type mismatch: {src.name} {reason} {dest.name} ", node
            )

    def visitBoolOp(self, node: BoolOp, type_ctx=None) -> NarrowingEffect:
        effect = NO_EFFECT
        final_type = None
        if isinstance(node.op, And):
            for value in node.values:
                new_effect = self.visit(value)
                effect = effect.union(new_effect)
                final_type = self.widen(final_type, self.get_type(value))

                # apply the new effect as short circuiting would
                # eliminate it.
                new_effect.apply(self)

            # we undo the effect as we have no clue what context we're in
            # but then we return the combined effect in case we're being used
            # in a conditional context
            effect.undo(self)
        else:
            for value in node.values:
                self.visit(value)
                final_type = self.widen(final_type, self.get_type(value))

        self.set_type(node, final_type or DYNAMIC)
        return effect

    def visitBinOp(self, node: BinOp, type_ctx=None) -> NarrowingEffect:
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

    def visitUnaryOp(self, node: UnaryOp, type_ctx=None) -> NarrowingEffect:
        self.visit(node.operand, type_ctx)
        self.get_type(node.operand).bind_unaryop(node, self, type_ctx)
        return NO_EFFECT

    def visitLambda(self, node: Lambda, type_ctx=None) -> NarrowingEffect:
        self.visit(node.body)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitIfExp(self, node: IfExp, type_ctx=None) -> NarrowingEffect:
        effect = self.visit(node.test)
        effect.apply(self)
        self.visit(node.body)
        effect.reverse(self)
        self.visit(node.orelse)
        effect.undo(self)

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

    def visitSlice(self, node: Slice, type_ctx=None) -> NarrowingEffect:
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

        return OBJECT_TYPE.instance

    def visitDict(self, node: ast.Dict, type_ctx=None) -> NarrowingEffect:
        for k in node.keys:
            assert k is not None
            self.visit(k)
        for v in node.values:
            self.visit(v)

        self.set_type(node, DICT_TYPE.instance)
        return NO_EFFECT

    def visitSet(self, node: Set, type_ctx=None) -> NarrowingEffect:
        for elt in node.elts:
            self.visit(elt)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def assign_value(self, target: expr, value: Value) -> None:
        if isinstance(target, Name):
            decl_type = self.decl_types.get(target.id)
            if decl_type is None:
                # This is the first declaration of this variable
                decl_type = self.declare_local(target, value.klass)
            else:
                self.check_can_assign_from(decl_type.type, value.klass, target)

            local_type = (
                value if decl_type.type.can_be_narrowed else decl_type.type.instance
            )
            self.local_types[target.id] = local_type
            self.set_type(target, local_type)
        elif isinstance(target, ast.Tuple):
            for val in target.elts:
                self.assign_value(val, DYNAMIC)
        else:
            target_type = self.get_type(target)
            self.check_can_assign_from(target_type.klass, value.klass, target)

    def visitListComp(self, node: ListComp, type_ctx=None) -> NarrowingEffect:
        self.visit_comprehension(node, node.generators, node.elt)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitSetComp(self, node: SetComp, type_ctx=None) -> NarrowingEffect:
        self.visit_comprehension(node, node.generators, node.elt)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitDictComp(self, node: DictComp, type_ctx=None) -> NarrowingEffect:
        self.visit_comprehension(node, node.generators, node.key, node.value)
        self.set_type(node, DICT_TYPE.instance)
        return NO_EFFECT

    def visitGeneratorExp(self, node: GeneratorExp, type_ctx=None) -> NarrowingEffect:
        self.visit_comprehension(node, node.generators, node.elt)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visit_comprehension(
        self, node: ast.expr, generators: List[ast.comprehension], *elts: ast.expr
    ) -> None:
        self.visit(generators[0].iter)

        scope = BindingScope(node)
        self.scopes.append(scope)

        iter_type = self.get_type(generators[0].iter).get_iter_type()

        self.assign_value(generators[0].target, iter_type)
        for if_ in generators[0].ifs:
            self.visit(if_)

        for gen in generators[1:]:
            self.visit(gen.iter)
            iter_type = self.get_type(gen.iter).get_iter_type()
            self.assign_value(gen.target, iter_type)
            for if_ in generators[0].ifs:
                self.visit(if_)

        for elt in elts:
            self.visit(elt)

        self.scopes.pop()

    def visitAwait(self, node: Await, type_ctx=None) -> NarrowingEffect:
        self.visit(node.value)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitYield(self, node: Yield, type_ctx=None) -> NarrowingEffect:
        value = node.value
        if value is not None:
            self.visit(value)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitYieldFrom(self, node: YieldFrom, type_ctx=None) -> NarrowingEffect:
        self.visit(node.value)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitIndex(self, node: Index, type_ctx=None) -> NarrowingEffect:
        # pyre-ignore[16]: `Index` has no attribute `value`.
        self.visit(node.value, type_ctx)
        self.set_type(node, self.get_type(node.value))
        return NO_EFFECT

    def visitCompare(self, node: Compare, type_ctx=None) -> NarrowingEffect:
        if len(node.ops) == 1 and isinstance(node.ops[0], (Is, IsNot)):
            left = node.left
            right = node.comparators[0]
            other = None

            self.set_type(node, BOOL_TYPE.instance)
            self.set_type(node.ops[0], BOOL_TYPE.instance)

            if isinstance(left, NameConstant) and left.value is None:
                other = right
            elif isinstance(right, NameConstant) and right.value is None:
                other = left

            if other is not None and isinstance(other, Name):
                self.visitName(other)
                var_type = self.get_type(other)

                if (
                    isinstance(var_type, OptionalInstance)
                    and not var_type.klass.index.is_generic_parameter
                ):
                    if isinstance(node.ops[0], IsNot):
                        return NotNoneEffect(other.id, var_type)
                    else:
                        return NoneEffect(other.id, var_type)

        self.visit(node.left, type_ctx)
        left = node.left
        ltype = self.get_type(node.left)
        node.ops = [type(op)() for op in node.ops]
        for comparator, op in zip(node.comparators, node.ops):
            self.visit(comparator, type_ctx)
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

    def visitCall(self, node: Call, type_ctx=None) -> NarrowingEffect:
        self.visit(node.func)
        return self.get_type(node.func).bind_call(node, self, type_ctx)

    def visitNum(self, node: Num, type_ctx=None) -> NarrowingEffect:
        if type_ctx is not None:
            type_ctx.bind_num(node, self)
        else:
            DYNAMIC.bind_num(node, self)

        return NO_EFFECT

    def visitStr(self, node: Str, type_ctx=None) -> NarrowingEffect:
        self.set_type(node, STR_TYPE.instance)
        return NO_EFFECT

    def visitFormattedValue(
        self, node: FormattedValue, type_ctx=None
    ) -> NarrowingEffect:
        self.visit(node.value)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitJoinedStr(self, node: JoinedStr, type_ctx=None) -> NarrowingEffect:
        for value in node.values:
            self.visit(value)

        self.set_type(node, STR_TYPE.instance)
        return NO_EFFECT

    def visitBytes(self, node: Bytes, type_ctx=None) -> NarrowingEffect:
        self.set_type(node, BYTES_TYPE.instance)
        return NO_EFFECT

    def visitNameConstant(self, node: NameConstant, type_ctx=None) -> NarrowingEffect:
        if node.value is None:
            self.set_type(node, NONE_TYPE.instance)
        elif node.value in (True, False):
            self.set_type(node, BOOL_TYPE.instance)
        else:
            self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitEllipsis(self, node: Ellipsis, type_ctx=None) -> NarrowingEffect:
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitConstant(self, node: Constant, type_ctx=None) -> NarrowingEffect:
        if type_ctx is not None:
            type_ctx.bind_constant(node, self)
        else:
            self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitAttribute(self, node: Attribute, type_ctx=None) -> NarrowingEffect:
        self.visit(node.value)
        self.get_type(node.value).bind_attr(node, self, type_ctx)
        return NO_EFFECT

    def visitSubscript(self, node: Subscript, type_ctx=None) -> NarrowingEffect:
        self.visit(node.value)
        self.visit(node.slice, DYNAMIC)
        val_type = self.get_type(node.value)
        val_type.bind_subscr(node, self.get_type(node.slice), self)
        return NO_EFFECT

    def visitStarred(self, node: Starred, type_ctx=None) -> NarrowingEffect:
        self.visit(node.value)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitName(self, node: Name, type_ctx=None) -> NarrowingEffect:
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

    def visitList(self, node: ast.List, type_ctx=None) -> NarrowingEffect:
        for elt in node.elts:
            self.visit(elt, DYNAMIC)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitTuple(self, node: ast.Tuple, type_ctx=None) -> NarrowingEffect:
        for elt in node.elts:
            self.visit(elt, DYNAMIC)
        self.set_type(node, DYNAMIC)
        return NO_EFFECT

    def visitReturn(self, node: Return):
        self.returns.add(node)
        value = node.value
        if value is not None:
            self.visit(value)
            cur_scope = self.scopes[-1]
            expected = DYNAMIC
            func = cur_scope.node
            if not isinstance(func, (ast.FunctionDef, ast.AsyncFunctionDef)):
                pass
            elif func.returns:
                expected = self.cur_mod.resolve_annotation(func.returns)
                if expected is None:
                    expected = DYNAMIC
                else:
                    expected = expected.instance

    def visitImportFrom(self, node: ImportFrom):
        mod_name = node.module
        if node.level or not mod_name:
            raise NotImplementedError("relative imports aren't supported")

        if mod_name == "__static__":
            for alias in node.names:
                name = alias.name
                if name == "*":
                    raise TypedSyntaxError("from __static__ import * is disallowed")
                elif name not in self.symtable.statics.children:
                    raise TypedSyntaxError(f"unsuported static import {name}")

    def visit_until_return(self, nodes: List[stmt]) -> bool:
        for statement in nodes:
            self.visit(statement)
            if statement in self.returns:
                return True

        return False

    def visitIf(self, node: If):
        branch = self.scopes[-1].branch()

        effect = self.visit(node.test)
        effect.apply(self)

        returns = self.visit_until_return(node.body)

        if node.orelse:
            if_end = branch.copy()
            branch.restore()

            effect.reverse(self)
            else_returns = self.visit_until_return(node.orelse)
            if else_returns:
                if returns:
                    self.returns.add(node)
            else:
                # Merge end of orelse with end of if
                branch.merge(if_end)
        else:
            # Merge end of if w/ opening
            branch.merge()

        # We don't need to undo the effect, as the type merging handles
        # it for us, and it could overwrite an assignment that
        # happened in the body.  But if the if body returns, then we know
        # the effect is reversed.
        if returns:
            effect.reverse(self)

    def visitTry(self, node: Try):
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

    def visitExceptHandler(self, node: ast.ExceptHandler):
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

    def visitWhile(self, node: While):
        branch = self.scopes[-1].branch()

        effect = self.visit(node.test)
        effect.apply(self)

        self.visit(node.body)
        effect.undo(self)

        if node.orelse:
            # The or-else can happen after the while body, or without executing it.
            branch.merge()
            self.visit(node.orelse)

        branch.merge()

    def syntax_error(self, msg: str, node: AST):
        source_line = linecache.getline(self.filename, node.lineno)
        return TypedSyntaxError(
            msg, (self.filename, node.lineno, node.col_offset, source_line or None)
        )


class StaticCodeGenerator(Python37CodeGenerator):
    _default_cache = {}

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
    ):
        super().__init__(parent, node, symbols, graph, flags, optimization_lvl)
        self.symtable = symtable
        self.modname = modname
        self.cur_mod = self.symtable.modules[modname]

    def make_child_codegen(self, tree: AST, graph: PyFlowGraph) -> StaticCodeGenerator:
        graph.setFlag(CO_STATICALLY_COMPILED)
        return StaticCodeGenerator(
            self,
            tree,
            self.symbols,
            graph,
            symtable=self.symtable,
            modname=self.modname,
        )

    def get_type(self, node: AST) -> Value:
        return self.cur_mod.types[node]

    @classmethod
    def make_code_gen(
        cls, module_name: str, tree: AST, filename: str, flags: int, optimize: int
    ) -> StaticCodeGenerator:
        # TODO: Parsing here should really be that we run declaration visitor over all nodes,
        # and then perform post processing on the symbol table, and then proceed to analysis
        # and compilation
        symtable = SymbolTable()
        decl_visit = DeclarationVisitor(module_name, filename, symtable)
        decl_visit.visit(tree)

        for module in symtable.modules.values():
            module.finish_bind()

        tree = AstOptimizer(optimize=optimize > 0).visit(tree)
        s = symbols.SymbolVisitor()
        s.visit(tree)

        graph = cls.flow_graph(module_name, filename, s.scopes[tree])
        graph.setFlag(CO_STATICALLY_COMPILED)

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
        optimization_lvl: int = 0,
    ):
        graph = super().make_function_graph(
            func, filename, scopes, class_name, name, optimization_lvl
        )

        # we tagged the graph as CO_STATICALLY_COMPILED, and the last co_const entry
        # will inform the runtime of the return type for the code object.
        ret_type = self.get_type(func)
        type_descr = ret_type.klass.type_descr
        graph.extra_consts.append(type_descr)
        return graph

    def walkClassBody(self, node: ClassDef, gen: CodeGenerator):
        super().walkClassBody(node, gen)
        cur_mod = self.symtable.modules[self.modname]
        klass = cur_mod.resolve_name(node.name)
        if not isinstance(klass, Class):
            raise SyntaxError("class missing or defined")

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

            type_descr = value.type_descr
            if type_descr is None:
                continue

            gen.emit("LOAD_CONST", name)
            gen.emit("LOAD_CONST", type_descr)
            count += 1

        if count:
            gen.emit("BUILD_MAP", count)
            gen.emit("STORE_NAME", "__slot_types__")

    def visitAugAttribute(self, node, mode):
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

    def visitAttribute(self, node: Attribute):
        self.update_lineno(node)
        self.visit(node.value)
        self.get_type(node.value).emit_attr(node, self)

    def emit_type_check(self, dest: Class, src: Class):
        if src is DYNAMIC_TYPE and dest is not OBJECT_TYPE and dest is not DYNAMIC_TYPE:
            self.emit("CAST", dest.type_descr)
        else:
            assert dest.can_assign_from(src)

    def visitAssignTarget(self, elt: expr, value: Optional[expr] = None):
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

    def visitAssign(self, node: Assign):
        self.set_lineno(node)
        self.visit(node.value)
        dups = len(node.targets) - 1
        for i in range(len(node.targets)):
            elt = node.targets[i]
            if i < dups:
                self.emit("DUP_TOP")
            if isinstance(elt, ast.AST):
                self.visitAssignTarget(elt, node.value)

    def visitAnnAssign(self, node: ast.AnnAssign):
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

    def visitNum(self, node):
        self.get_type(node).emit_num(node, self)

    def visitConstant(self, node):
        self.get_type(node).emit_constant(node, self)

    def visitName(self, name):
        self.get_type(name).emit_name(name, self)

    def visitAugAssign(self, node):
        self.get_type(node.target).emit_augassign(node, self)

    def visitAugName(self, node, mode):
        self.get_type(node).emit_augname(node, self, mode)

    def visitCompare(self, node):
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

    def emitChainedCompareStep(self, op, value, cleanup, jump="JUMP_IF_ZERO_OR_POP"):
        self.visit(value)
        self.emit("DUP_TOP")
        self.emit("ROT_THREE")
        self.get_type(op).emit_compare(op, self)
        self.emit(jump, cleanup)
        self.nextBlock(label="compare_or_cleanup")

    def visitBoolOp(self, node: BoolOp):
        end = self.newBlock()
        for child in node.values[:-1]:
            self.get_type(child).emit_jumpif_pop(
                child, end, type(node.op) == ast.Or, self
            )
            self.nextBlock()
        self.visit(node.values[-1])
        self.nextBlock(end)

    def visitBinOp(self, node):
        self.get_type(node).emit_binop(node, self)

    def visitUnaryOp(self, node: UnaryOp, type_ctx=None):
        self.get_type(node).emit_unaryop(node, self)

    def visitCall(self, node):
        self.get_type(node.func).emit_call(node, self)

    def visitReturn(self, node):
        func = self.tree
        if not isinstance(func, (ast.FunctionDef, ast.AsyncFunctionDef)):
            raise SyntaxError(
                "'return' outside function", self.syntax_error_position(node)
            )
        elif self.scope.coroutine and self.scope.generator and node.value:
            raise SyntaxError(
                "'return' with value in async generator",
                self.syntax_error_position(node),
            )

        self.set_lineno(node)
        expected = None
        if isinstance(func, (ast.FunctionDef, ast.AsyncFunctionDef)) and func.returns:
            expected = self.cur_mod.resolve_annotation(func.returns)
        if node.value:
            return_type = self.get_type(node.value) or DYNAMIC
            if return_type.klass in ALL_INT_TYPES:
                # When returning primitives from functions, we must box them.
                return_type.emit_box(node.value, self)
            else:
                self.visit(node.value)

            if expected is not None and not expected.can_assign_from(
                self.get_type(node.value).klass
            ):
                self.emit("CAST", expected.type_descr)
        else:
            self.emit("LOAD_CONST", None)
        self.emit("RETURN_VALUE")

    def emit_invoke_method(self, descr, arg_count):
        # Emit a zero EXTENDED_ARG before so that we can optimize and insert the
        # arg count
        self.emit("EXTENDED_ARG", 0)
        self.emit("INVOKE_METHOD", (descr, arg_count))

    def defaultVisit(self, node, *args):
        self.node = node
        klass = node.__class__
        meth = self._default_cache.get(klass, None)
        if meth is None:
            className = klass.__name__
            meth = getattr(
                super(StaticCodeGenerator, StaticCodeGenerator),
                "visit" + className,
                StaticCodeGenerator.generic_visit,
            )
            self._default_cache[klass] = meth
        return meth(self, node, *args)

    def compileJumpIf(self, test, next, is_if_true):
        self.get_type(test).emit_jumpif(test, next, is_if_true, self)

    def processBody(
        self,
        node: Union[ast.Lambda, ast.FunctionDef],
        body: Union[List[ast.stmt], ast.expr],
        gen: StaticCodeGenerator,
    ):
        arg_checks = []
        for i, arg in enumerate(node.args.args):
            t = self.get_type(arg)
            if t is not DYNAMIC and t is not OBJECT:
                arg_checks.append(i)
                arg_checks.append(t.klass.type_descr)

        for i, arg in enumerate(node.args.kwonlyargs):
            t = self.get_type(arg)
            if t is not DYNAMIC and t is not OBJECT:
                arg_checks.append(i + len(node.args.args))
                arg_checks.append(t.klass.type_descr)

        gen.emit("CHECK_ARGS", tuple(arg_checks))

        super().processBody(node, body, gen)
