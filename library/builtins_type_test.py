#!/usr/bin/env python3
import sys
import unittest

from test_support import pyro_only

try:
    from builtins import _non_heaptype

    from _builtins import _gc
except ImportError:
    pass


Py_TPFLAGS_HEAPTYPE = 1 << 9
Py_TPFLAGS_BASETYPE = 1 << 10
Py_TPFLAGS_READY = 1 << 12
Py_TPFLAGS_READYING = 1 << 13
Py_TPFLAGS_IS_ABSTRACT = 1 << 20
Py_TPFLAGS_LONG_SUBCLASS = 1 << 24
Py_TPFLAGS_LIST_SUBCLASS = 1 << 25
Py_TPFLAGS_TUPLE_SUBCLASS = 1 << 26
Py_TPFLAGS_BYTES_SUBCLASS = 1 << 27
Py_TPFLAGS_UNICODE_SUBCLASS = 1 << 28
Py_TPFLAGS_DICT_SUBCLASS = 1 << 29
Py_TPFLAGS_BASE_EXC_SUBCLASS = 1 << 30
Py_TPFLAGS_TYPE_SUBCLASS = 1 << 31


class TypeTests(unittest.TestCase):
    def test_abstract_methods_get_with_builtin_type_raises_attribute_error(self):
        with self.assertRaises(AttributeError) as context:
            type.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_abstract_methods_get_with_type_subclass_raises_attribute_error(self):
        class Foo(type):
            pass

        with self.assertRaises(AttributeError) as context:
            Foo.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_abstract_methods_get_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__abstractmethods__"].__get__(42)

    def test_abstract_methods_set_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            int.__abstractmethods__ = ["foo"]
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'int'",
        )

    def test_abstract_methods_get_with_type_subclass_sets_attribute(self):
        class Foo(type):
            pass

        Foo.__abstractmethods__ = 1
        self.assertEqual(Foo.__abstractmethods__, 1)

    def test_abstract_methods_del_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            del str.__abstractmethods__
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'str'",
        )

    def test_abstract_methods_del_unset_with_type_subclass_raises_attribute_error(self):
        class Foo(type):
            pass

        with self.assertRaises(AttributeError) as context:
            del Foo.__abstractmethods__
        self.assertEqual(str(context.exception), "__abstractmethods__")

    def test_abstract_methods_del_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__abstractmethods__"].__delete__(42)

    def test_abstract_methods_set_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__abstractmethods__"].__set__(42, [])

    def test_builtin_types_have_no_module_attribute(self):
        from types import FrameType

        self.assertNotIn("__module__", int.__dict__)
        self.assertNotIn("__module__", object.__dict__)
        self.assertNotIn("__module__", tuple.__dict__)
        self.assertNotIn("__module__", FrameType.__dict__)

    def test_delattr_deletes_class_attribute(self):
        class C:
            fld = 4

        self.assertTrue(hasattr(C, "fld"))
        self.assertIs(type.__delattr__(C, "fld"), None)
        self.assertFalse(hasattr(C, "fld"))

    def test_delattr_raises_type_error_with_instance(self):
        class C:
            fld = 4

        c = C()
        self.assertRaisesRegex(
            TypeError,
            "'__delattr__' .* 'type' object.* a 'C'",
            type.__delattr__,
            c,
            "fld",
        )

    def test_dunder_base_with_object_type_returns_none(self):
        self.assertIs(object.__base__, None)

    def test_dunder_base_with_builtin_type_returns_supertype(self):
        self.assertIs(bool.__base__, int)
        self.assertIs(int.__base__, object)
        self.assertIs(str.__base__, object)
        self.assertIs(type.__base__, object)

    def test_dunder_base_with_user_type_returns_best_base(self):
        class A:
            pass

        class B(A, str):
            pass

        class C(B):
            pass

        self.assertIs(A.__base__, object)
        self.assertIs(B.__base__, str)
        self.assertIs(C.__base__, B)

    def test_dunder_bases_del_with_builtin_type_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            del object.__bases__
        self.assertEqual(
            str(context.exception),
            "can't set attributes of built-in/extension type 'object'",
        )

    def test_dunder_bases_del_with_user_type_raises_type_error(self):
        class C:
            pass

        with self.assertRaises(TypeError) as context:
            del C.__bases__
        self.assertEqual(str(context.exception), "can't delete C.__bases__")

    def test_dunder_bases_del_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__bases__"].__delete__(42)

    def test_dunder_bases_get_with_builtin_type_returns_tuple(self):
        self.assertEqual(object.__bases__, ())
        self.assertEqual(type.__bases__, (object,))
        self.assertEqual(int.__bases__, (object,))
        self.assertEqual(bool.__bases__, (int,))

    def test_dunder_bases_get_with_user_type_returns_tuple(self):
        class A:
            pass

        class B:
            pass

        class C(A):
            pass

        class D(C, B):
            pass

        self.assertEqual(A.__bases__, (object,))
        self.assertEqual(B.__bases__, (object,))
        self.assertEqual(C.__bases__, (A,))
        self.assertEqual(D.__bases__, (C, B))

    def test_dunder_bases_get_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__bases__"].__get__(42)

    def test_dunder_basicsize_get_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__basicsize__"].__get__(42)

    def test_dunder_call_with_non_type_self_raises_type_error(self):
        self.assertRaisesRegex(
            TypeError,
            "'__call__' .* 'type' object.* a 'int'",
            type.__call__,
            5,
        )

    def test_dunder_dir_with_non_type_object_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dir__(None)

    def test_dunder_doc_on_empty_class_is_none(self):
        class C:
            pass

        self.assertIsNone(C.__doc__)
        instance = C()
        self.assertIsNone(instance.__doc__)

    def test_dunder_doc_accessible_via_instance(self):
        class C:
            """docstring"""

            pass

        self.assertEqual(C.__doc__, "docstring")
        instance = C()
        self.assertEqual(instance.__doc__, "docstring")

    def test_type_dunder_doc_is_not_inheritable(self):
        class C:
            """docstring"""

            pass

        class D(C):
            pass

        self.assertEqual(C.__doc__, "docstring")
        self.assertIsNone(D.__doc__)

    def test_dunder_flags_returns_basetype_set(self):
        class C:
            pass

        self.assertTrue(object.__flags__ & Py_TPFLAGS_BASETYPE)
        self.assertTrue(float.__flags__ & Py_TPFLAGS_BASETYPE)
        self.assertTrue(C.__flags__ & Py_TPFLAGS_BASETYPE)

    def test_dunder_flags_returns_basetype_clear(self):
        self.assertFalse(bool.__flags__ & Py_TPFLAGS_BASETYPE)
        str_iter_type = type(iter(""))
        self.assertFalse(str_iter_type.__flags__ & Py_TPFLAGS_BASETYPE)

    def test_dunder_flags_with_managed_type_is_heap_type(self):
        class C:
            pass

        self.assertTrue(C.__flags__ & Py_TPFLAGS_HEAPTYPE)

    def test_dunder_flags_with_managed_type_is_ready(self):
        class C:
            pass

        self.assertTrue(C.__flags__ & Py_TPFLAGS_READY)
        self.assertFalse(C.__flags__ & Py_TPFLAGS_READYING)

    def test_dunder_flags_without_dunder_abstractmethods_returns_false(self):
        class C:
            pass

        with self.assertRaises(AttributeError):
            C.__abstractmethods__

        self.assertFalse(C.__flags__ & Py_TPFLAGS_IS_ABSTRACT)

    def test_dunder_flags_with_empty_dunder_abstractmethods_returns_false(self):
        import abc

        class C(metaclass=abc.ABCMeta):
            pass

        self.assertEqual(len(C.__abstractmethods__), 0)
        self.assertFalse(C.__flags__ & Py_TPFLAGS_IS_ABSTRACT)

    def test_dunder_flags_with_non_empty_dunder_abstractmethods_returns_true(self):
        import abc

        class C(metaclass=abc.ABCMeta):
            @abc.abstractmethod
            def foo(self):
                pass

        self.assertEqual(len(C.__abstractmethods__), 1)
        self.assertTrue(C.__flags__ & Py_TPFLAGS_IS_ABSTRACT)

    def test_dunder_flags_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__dict__["__flags__"].__get__(42)

    def test_dunder_flags_sets_long_subclass_if_int_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_LONG_SUBCLASS)

        class D(int):
            pass

        self.assertTrue(int.__flags__ & Py_TPFLAGS_LONG_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_LONG_SUBCLASS)

    def test_dunder_flags_sets_list_subclass_if_list_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_LIST_SUBCLASS)

        class D(list):
            pass

        self.assertTrue(list.__flags__ & Py_TPFLAGS_LIST_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_LIST_SUBCLASS)

    def test_dunder_flags_sets_tuple_subclass_if_tuple_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_TUPLE_SUBCLASS)

        class D(tuple):
            pass

        self.assertTrue(tuple.__flags__ & Py_TPFLAGS_TUPLE_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_TUPLE_SUBCLASS)

    def test_dunder_flags_sets_bytes_subclass_if_bytes_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_BYTES_SUBCLASS)

        class D(bytes):
            pass

        self.assertTrue(bytes.__flags__ & Py_TPFLAGS_BYTES_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_BYTES_SUBCLASS)

    def test_dunder_flags_sets_unicode_subclass_if_str_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_UNICODE_SUBCLASS)

        class D(str):
            pass

        self.assertTrue(str.__flags__ & Py_TPFLAGS_UNICODE_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_UNICODE_SUBCLASS)

    def test_dunder_flags_sets_dict_subclass_if_dict_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_DICT_SUBCLASS)

        class D(dict):
            pass

        self.assertTrue(dict.__flags__ & Py_TPFLAGS_DICT_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_DICT_SUBCLASS)

    def test_dunder_flags_sets_base_exc_subclass_if_base_exception_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_BASE_EXC_SUBCLASS)

        class D(BaseException):
            pass

        self.assertTrue(BaseException.__flags__ & Py_TPFLAGS_BASE_EXC_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_BASE_EXC_SUBCLASS)
        self.assertTrue(MemoryError.__flags__ & Py_TPFLAGS_BASE_EXC_SUBCLASS)

    def test_dunder_flags_sets_type_subclass_if_type_subclass(self):
        class C:
            pass

        self.assertFalse(C.__flags__ & Py_TPFLAGS_TYPE_SUBCLASS)

        class D(type):
            pass

        self.assertTrue(type.__flags__ & Py_TPFLAGS_TYPE_SUBCLASS)
        self.assertTrue(D.__flags__ & Py_TPFLAGS_TYPE_SUBCLASS)

    def test_dunder_init_with_no_name_or_object_param_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            type.__init__(type)

        self.assertIn("type.__init__() takes 1 or 3 arguments", str(context.exception))

    def test_dunder_init_with_too_many_args_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            type.__init__(type, "C", (), {}, 1, 2, 3, 4, 5, foobar=42)

        self.assertIn("type.__init__() takes 1 or 3 arguments", str(context.exception))

    def test_dunder_init_with_kwargs_does_not_raise(self):
        type.__init__(type, "C", (), {}, foobar=42)

    def test_dunder_instancecheck_with_instance_returns_true(self):
        self.assertIs(int.__instancecheck__(3), True)
        self.assertIs(int.__instancecheck__(False), True)
        self.assertIs(object.__instancecheck__(type), True)
        self.assertIs(str.__instancecheck__("123"), True)
        self.assertIs(type.__instancecheck__(type, int), True)
        self.assertIs(type.__instancecheck__(type, object), True)

    def test_dunder_instancecheck_with_non_instance_returns_false(self):
        self.assertIs(bool.__instancecheck__(3), False)
        self.assertIs(int.__instancecheck__("123"), False)
        self.assertIs(str.__instancecheck__(b"123"), False)
        self.assertIs(type.__instancecheck__(type, 3), False)

    def test_dunder_subclasscheck_with_subclass_returns_true(self):
        self.assertIs(int.__subclasscheck__(int), True)
        self.assertIs(int.__subclasscheck__(bool), True)
        self.assertIs(object.__subclasscheck__(int), True)
        self.assertIs(object.__subclasscheck__(type), True)

    def test_dunder_subclasscheck_with_non_subclass_returns_false(self):
        self.assertIs(bool.__subclasscheck__(int), False)
        self.assertIs(int.__subclasscheck__(object), False)
        self.assertIs(str.__subclasscheck__(object), False)
        self.assertIs(type.__subclasscheck__(type, object), False)

    def test_dunder_module_set_on_builtin_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, ".*int.*"):
            int.__module__ = "foo"
        with self.assertRaisesRegex(TypeError, ".*int.*"):
            type.__dict__["__module__"].__set__(int, "foo")

    def test_dunder_module_sets_module(self):
        class C:
            pass

        type.__dict__["__module__"].__set__(C, "foo")
        self.assertEqual(type.__dict__["__module__"].__get__(C), "foo")
        self.assertEqual(C.__dict__["__module__"], "foo")
        self.assertEqual(C.__module__, "foo")

    def test_dunder_module_set_accepts_anything(self):
        class C:
            pass

        type.__dict__["__module__"].__set__(C, 42.42)
        self.assertEqual(C.__module__, 42.42)

        C.__module__ = None
        self.assertIsNone(type.__dict__["__module__"].__get__(C))

    def test_dunder_module_with_builtin_type_returns_builtins(self):
        self.assertEqual(int.__module__, "builtins")
        self.assertEqual(dict.__module__, "builtins")
        self.assertEqual(OSError.__module__, "builtins")
        self.assertEqual(type.__module__, "builtins")

    def test_dunder_name_returns_name(self):
        class FooBar:
            pass

        self.assertEqual(FooBar.__name__, "FooBar")
        self.assertEqual(type.__dict__["__name__"].__get__(FooBar), "FooBar")

    def test_dunder_name_sets_name(self):
        class C:
            pass

        type.__dict__["__name__"].__set__(C, "foo")
        self.assertEqual(type.__dict__["__name__"].__get__(C), "foo")
        self.assertEqual(C.__name__, "foo")

    def test_dunder_name_set_with_non_string_raises_type_error(self):
        class C:
            pass

        with self.assertRaisesRegex(
            TypeError, "can only assign string to C.__name__, not 'int'"
        ):
            C.__name__ = 42
        with self.assertRaisesRegex(
            TypeError, "can only assign string to C.__name__, not 'float'"
        ):
            type.__dict__["__name__"].__set__(C, 1.2)

    def test_dunder_name_set_on_builtin_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, ".*int.*"):
            int.__name__ = "foo"
        with self.assertRaisesRegex(TypeError, ".*int.*"):
            type.__dict__["__name__"].__set__(int, "foo")

    def test_dunder_new_with_one_arg_returns_type_of_arg(self):
        class C:
            pass

        self.assertIs(type.__new__(type, 1), int)
        self.assertIs(type.__new__(type, "hello"), str)
        self.assertIs(type.__new__(type, C()), C)

    def test_dunder_new_with_non_type_cls_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__new__(1, "X", (object,), {})

    def test_dunder_new_with_non_str_name_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__new__(type, 1, (object,), {})

    def test_dunder_new_with_non_tuple_bases_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__new__(type, "X", [object], {})

    def test_dunder_new_with_non_basetype_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "type 'bool' is not an acceptable base type"
        ):
            type.__new__(type, "X", (bool,), {})

    def test_dunder_new_with_non_dict_type_dict_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__new__(type, "X", (object,), 1)

    def test_dunder_new_with_duplicated_base_raises_type_error(self):
        class C:
            pass

        error_msg = "duplicate base class C"
        with self.assertRaisesRegex(TypeError, error_msg):
            type.__new__(type, "X", (C, C), {})

    def test_dunder_new_returns_type_instance(self):
        X = type.__new__(type, "X", (object,), {})
        self.assertIsInstance(X, type)
        self.assertEqual(X.__name__, "X")
        self.assertEqual(X.__qualname__, "X")
        self.assertTrue(X.__flags__ & Py_TPFLAGS_HEAPTYPE)

    def test_dunder_new_sets_dunder_module(self):
        globals = {"__name__": 8.13}
        X = eval("type('X', (), {})", globals)  # noqa: P204
        self.assertEqual(X.__module__, 8.13)
        self.assertEqual(X.__dict__["__module__"], 8.13)

    def test_dunder_new_does_not_override_dunder_module(self):
        X = type.__new__(type, "X", (), {"__module__": "foobar"})
        self.assertEqual(X.__module__, "foobar")
        self.assertEqual(X.__dict__["__module__"], "foobar")

    def test_dunder_new_sets_qualname(self):
        X = type.__new__(type, "foo.bar", (), {})
        self.assertEqual(X.__qualname__, "foo.bar")
        self.assertNotIn("__qualname__", X.__dict__)

    def test_dunder_new_sets_qualname_from_dict(self):
        namespace = {"__qualname__": "quux"}
        X = type.__new__(type, "X", (), namespace)
        self.assertEqual(X.__qualname__, "quux")
        self.assertNotIn("__qualname__", X.__dict__)
        self.assertEqual(namespace["__qualname__"], "quux")

    def test_dunder_new_with_non_string_qualname_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "type __qualname__ must be a str, not float"
        ):
            type.__new__(type, "X", (), {"__qualname__": 2.3})

    def test_dunder_new_adds_to_base_dunder_subclasses(self):
        A = type.__new__(type, "A", (object,), {})
        B = type.__new__(type, "B", (object,), {})
        C = type.__new__(type, "C", (A, B), {})
        D = type.__new__(type, "D", (A,), {})
        self.assertEqual(A.__subclasses__(), [C, D])
        self.assertEqual(B.__subclasses__(), [C])
        self.assertEqual(C.__subclasses__(), [])
        self.assertEqual(D.__subclasses__(), [])

    @unittest.skipIf(
        sys.version_info < (3, 10) and sys.implementation.name != "skybison",
        "Union requrires CPython 3.10",
    )
    def test_dunder_or_returns_union(self):
        from types import Union

        t = int | float
        self.assertIs(type(t), Union)
        self.assertEqual(t.__args__, (int, float))
        t = float | int
        self.assertIs(type(t), Union)
        self.assertEqual(t.__args__, (float, int))
        t = str | None
        self.assertIs(type(t), Union)
        self.assertEqual(t.__args__, (str, type(None)))

    @unittest.skipIf(
        sys.version_info < (3, 10) and sys.implementation.name != "skybison",
        "Union requrires CPython 3.10",
    )
    def test_dunder_ror_returns_union(self):
        from types import Union

        t = None | bytes
        self.assertIs(type(t), Union)
        self.assertEqual(t.__args__, (type(None), bytes))

    def test_dunder_qualname_returns_qualname(self):
        class C:
            __qualname__ = "bar"

        self.assertEqual(C.__qualname__, "bar")
        self.assertEqual(type.__dict__["__qualname__"].__get__(C), "bar")

    def test_dunder_qualname_sets_qualname(self):
        class C:
            pass

        type.__dict__["__qualname__"].__set__(C, "baz")
        self.assertEqual(C.__qualname__, "baz")
        C.__qualname__ = "bam"
        self.assertEqual(type.__dict__["__qualname__"].__get__(C), "bam")

    def test_dunder_qualname_set_with_non_string_raises_type_error(self):
        class C:
            pass

        with self.assertRaisesRegex(
            TypeError, "can only assign string to C.__qualname__, not 'int'"
        ):
            C.__qualname__ = 42
        with self.assertRaisesRegex(
            TypeError, "can only assign string to C.__qualname__, not 'float'"
        ):
            type.__dict__["__qualname__"].__set__(C, 1.2)

    def test_dunder_qualname_set_on_builtin_raises_type_error(self):
        with self.assertRaisesRegex(TypeError, ".*list.*"):
            list.__qualname__ = "foo"
        with self.assertRaisesRegex(TypeError, ".*list.*"):
            type.__dict__["__qualname__"].__set__(list, "foo")

    def test_dunder_repr_with_bytes_raises_type_error(self):
        with self.assertRaises(TypeError):
            type.__repr__(b"")

    def test_dunder_repr_returns_string_with_module_and_name(self):
        class C:
            pass

        self.assertEqual(
            type.__repr__(C),
            "<class '__main__.TypeTests.test_dunder_repr_returns_string_with_module_and_name.<locals>.C'>",
        )

    def test_dunder_repr_for_builtins_returns_string_with_only_name(self):
        self.assertEqual(type.__repr__(list), "<class 'list'>")
        self.assertEqual(type.__repr__(type), "<class 'type'>")
        self.assertEqual(type.__repr__(TypeError), "<class 'TypeError'>")

    def test_dunder_repr_for_imported_class_returns_string_with_module_and_name(self):
        self.assertEqual(
            type.__repr__(unittest.case.TestCase), "<class 'unittest.case.TestCase'>"
        )

    def test_dunder_subclasses_with_leaf_type_returns_empty_list(self):
        class C:
            pass

        self.assertEqual(C.__subclasses__(), [])

    def test_dunder_subclasses_with_supertype_returns_list(self):
        class C:
            pass

        class D(C):
            pass

        self.assertEqual(C.__subclasses__(), [D])

    def test_dunder_subclasses_returns_new_list(self):
        class C:
            pass

        self.assertIsNot(C.__subclasses__(), C.__subclasses__())

    @pyro_only
    def test_dunder_subclasses_does_not_return_dead_types(self):
        class C:
            pass

        class D(C):
            pass

        D = None  # noqa: F811, F841
        _gc()
        self.assertEqual(type.__subclasses__(C), [])

    @pyro_only
    def test_dunder_subclasses_with_multiple_subclasses_returns_list(self):
        class B:
            pass

        class S0(B):
            pass

        class S1(B):
            pass

        class S2(B):
            pass

        class S3(B):
            pass

        class S4(B):
            pass

        S1 = None  # noqa: F811, F841
        S3 = None  # noqa: F811, F841
        _gc()
        self.assertEqual(len(type.__subclasses__(B)), 3)
        S0 = None  # noqa: F811, F841
        _gc()
        self.assertEqual(type.__subclasses__(B), [S2, S4])

    def test_mro_returns_list(self):
        class C:
            pass

        mro = C.mro()
        self.assertIsInstance(mro, list)
        self.assertEqual(mro, [C, object])

    def test_mro_with_multiple_inheritance_returns_linearization(self):
        class A:
            pass

        class B:
            pass

        class C(A, B):
            pass

        mro = type.mro(C)
        self.assertIsInstance(mro, list)
        self.assertEqual(mro, [C, A, B, object])

    def test_mro_with_invalid_linearization_raises_type_error(self):
        class A:
            pass

        class B(A):
            pass

        with self.assertRaisesRegex(
            TypeError,
            r"Cannot create a consistent method resolution\s+order \(MRO\) for bases A, B",
        ):

            class C(A, B):
                pass

    def test_mro_with_custom_method_propagates_exception(self):
        class Meta(type):
            def mro(cls):
                raise KeyError

        with self.assertRaises(KeyError):

            class Foo(metaclass=Meta):
                pass

    def test_new_calculates_metaclass_and_calls_dunder_new(self):
        class M0(type):
            def __new__(metacls, name, bases, type_dict, **kwargs):
                metacls.new_args = (metacls, name, bases, kwargs)
                return type.__new__(metacls, name, bases, type_dict)

        class M1(M0):
            foo = 7

        class A(metaclass=M0):
            pass

        class B(metaclass=M1):
            pass

        C = type.__new__(type, "C", (A, B), {}, bar=8)
        self.assertIs(type(C), M1)
        self.assertEqual(C.new_args, (M1, "C", (A, B), {"bar": 8}))
        self.assertEqual(C.foo, 7)

    def test_new_with_mro_entries_base_raises_type_error(self):
        class X:
            def __mro_entries__(self, bases):
                return (object,)

        pseudo_base = X()
        with self.assertRaises(TypeError) as ctx:
            type.__new__(type, "C", (pseudo_base,), {})
        self.assertEqual(
            str(ctx.exception),
            "type() doesn't support MRO entry resolution; use types.new_class()",
        )

    def test_mro_returning_iterable_returns_class_with_mro_tuple(self):
        class A:
            foo = 42

        class X:
            def __init__(self, cls):
                self.cls = cls

            def __iter__(self):
                return iter((self.cls, A))

        class Meta(type):
            def mro(cls):
                return X(cls)

        C = Meta.__new__(Meta, "C", (object,), {})
        self.assertIs(type(C.__mro__), tuple)
        self.assertEqual(C.__mro__, (C, A))
        self.assertEqual(C.foo, 42)

    def test_new_calls_init_subclass(self):
        class Foo:
            def __init_subclass__(cls, *args, **kwargs):
                cls.called_init = True

                self.assertIsInstance(cls, type)
                self.assertIn("foo", kwargs)

        class Bar(Foo, foo=True):
            pass

        self.assertTrue(Bar.called_init)

    def test_new_duplicates_dict(self):
        d = {"foo": 42, "bar": 17}
        T = type("T", (object,), d)
        d["foo"] = -7
        del d["bar"]
        self.assertEqual(T.foo, 42)
        self.assertEqual(T.bar, 17)
        T.foo = 20
        self.assertEqual(d["foo"], -7)
        self.assertFalse("bar" in d)

    @pyro_only
    def test_non_heaptype_returns_non_heaptype(self):
        X = _non_heaptype("X", (), {})
        self.assertFalse(X.__flags__ & Py_TPFLAGS_HEAPTYPE)
        with self.assertRaises(TypeError):
            X.foo = 1

    def test_non_heaptype_has_no_module_attribute(self):
        from types import SimpleNamespace

        self.assertNotIn("__module__", SimpleNamespace.__dict__)
        self.assertNotIn("__module__", zip.__dict__)
        self.assertNotIn("__module__", map.__dict__)

    def test_setattr_with_metaclass_does_not_abort(self):
        class Meta(type):
            pass

        class C(metaclass=Meta):
            __slots__ = "attr"

            def __init__(self, data):
                self.attr = data

        m = C("foo")
        self.assertEqual(m.attr, "foo")
        m.attr = "bar"
        self.assertEqual(m.attr, "bar")

    def test_type_new_sets_name_on_attributes(self):
        class Descriptor:
            def __set_name__(self, owner, name):
                self.owner = owner
                self.name = name

        class A:
            d = Descriptor()

        self.assertEqual(A.d.name, "d")
        self.assertIs(A.d.owner, A)

    def test_type_new_propagates_set_name_error(self):
        class Descriptor:
            def __set_name__(self, owner, name):
                raise Exception("I prefer to remain unnamed.")

        with self.assertRaises(RuntimeError) as context:

            class A:
                d = Descriptor()

        self.assertIn("A", str(context.exception))
        self.assertIn("Descriptor", str(context.exception))

    def test_type_new_with_metaclass_sets_name(self):
        class Meta(type):
            def __new__(metacls, name, bases, ns):
                ret = super().__new__(metacls, name, bases, ns)
                self.assertEqual(ret.d.name, "d")
                self.assertIs(ret.d.owner, ret)
                return 0

        class Descriptor:
            def __set_name__(self, owner, name):
                self.owner = owner
                self.name = name

        class A(metaclass=Meta):
            d = Descriptor()

        self.assertEqual(A, 0)

    def test_type_new_with_set_name_raising_error_propagates_exception(self):
        class SetNameDescriptor:
            def __get__(self, instance, owner):
                raise ValueError("Don't call, please.")

        class Descriptor:
            __set_name__ = SetNameDescriptor()

        with self.assertRaises(ValueError):

            class A:
                d = Descriptor()

    def test_non_type_with_type_getattribute(self):
        class C:
            __getattribute__ = type.__getattribute__

        c = C()
        with self.assertRaises(TypeError):
            c.foo


class TypeProxyTests(unittest.TestCase):
    def setUp(self):
        class A:
            placeholder = "placeholder_value"

        class B(A):
            pass

        def make_placeholder():
            b = B()
            return b.placeholder

        self.tested_type = B
        self.type_proxy = B.__dict__
        self.assertEqual(make_placeholder(), "placeholder_value")

    def test_dunder_contains_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).__contains__(None, None)

    def test_dunder_contains_returns_true_for_existing_item(self):
        self.tested_type.x = 40
        self.assertTrue(self.type_proxy.__contains__("x"))

    def test_dunder_contains_returns_false_for_not_existing_item(self):
        self.assertFalse(self.type_proxy.__contains__("x"))

    def test_dunder_contains_returns_false_for_placeholder(self):
        self.assertFalse(self.type_proxy.__contains__("placeholder"))

    def test_copy_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).copy(None)

    def test_copy_returns_dict_copy(self):
        self.tested_type.x = 40
        result = self.type_proxy.copy()
        self.assertEqual(type(result), dict)
        self.assertEqual(result["x"], 40)
        self.tested_type.y = 50
        self.assertNotIn("y", result)

    def test_dunder_getitem_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).__getitem__(None, None)

    def test_dunder_getitem_for_existing_key_returns_that_item(self):
        self.tested_type.x = 40
        self.assertEqual(self.type_proxy.__getitem__("x"), 40)

    def test_dunder_getitem_for_not_existing_key_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            self.type_proxy.__getitem__("x")
        self.assertIn("'x'", str(context.exception))

    def test_dunder_getitem_for_placeholder_raises_key_error(self):
        with self.assertRaises(KeyError) as context:
            self.type_proxy.__getitem__("placeholder")
        self.assertIn("'placeholder'", str(context.exception))

    def test_dunder_iter_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).__iter__(None)

    def test_dunder_iter_returns_key_iterator(self):
        self.tested_type.x = 40
        self.tested_type.y = 50
        result = self.type_proxy.__iter__()
        self.assertTrue(hasattr(result, "__next__"))
        result_list = list(result)
        self.assertIn("x", result_list)
        self.assertIn("y", result_list)

    def test_dunder_len_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).__len__(None)

    def test_dunder_len_returns_num_items(self):
        length = self.type_proxy.__len__()
        self.tested_type.x = 40
        self.assertEqual(self.type_proxy.__len__(), length + 1)

    def test_dunder_len_returns_num_items_excluding_placeholder(self):
        length = self.type_proxy.__len__()
        # Overwrite the existing placeholder by creating a real one under the same name.
        self.tested_type.placeholder = 1
        self.assertEqual(self.type_proxy.__len__(), length + 1)

    def test_dunder_repr_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).__repr__(None)

    def test_dunder_repr_returns_str_containing_existing_items(self):
        self.tested_type.x = 40
        self.tested_type.y = 50
        result = self.type_proxy.__repr__()
        self.assertIsInstance(result, str)
        self.assertIn("'x': 40", result)
        self.assertIn("'y': 50", result)

    def test_dunder_repr_returns_str_not_containing_placeholder(self):
        result = self.type_proxy.__repr__()
        self.assertNotIn("'placeholder'", result)

    def test_get_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).get(None, None)

    def test_get_returns_existing_item_value(self):
        self.tested_type.x = 40
        self.assertEqual(self.type_proxy.get("x"), 40)

    def test_get_with_default_for_non_existing_item_value_returns_that_default(self):
        self.assertEqual(self.type_proxy.get("x", -1), -1)

    def test_get_for_non_existing_item_returns_none(self):
        self.assertIs(self.type_proxy.get("x"), None)

    def test_get_for_placeholder_returns_none(self):
        self.assertIs(self.type_proxy.get("placeholder"), None)

    def test_items_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).items(None)

    def test_items_returns_container_for_key_value_pairs(self):
        self.tested_type.x = 40
        self.tested_type.y = 50
        result = self.type_proxy.items()
        self.assertTrue(hasattr(result, "__iter__"))
        result_list = list(iter(result))
        self.assertIn(("x", 40), result_list)
        self.assertIn(("y", 50), result_list)

    def test_keys_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).keys(None)

    def test_keys_returns_container_for_keys(self):
        self.tested_type.x = 40
        self.tested_type.y = 50
        result = self.type_proxy.keys()
        self.assertTrue(hasattr(result, "__iter__"))
        result_list = list(iter(result))
        self.assertIn("x", result_list)
        self.assertIn("y", result_list)

    def test_keys_returns_key_iterator_excluding_placeholder(self):
        result = self.type_proxy.keys()
        self.assertNotIn("placeholder", result)

    def test_values_with_non_type_proxy_raises_type_error(self):
        with self.assertRaises(TypeError):
            type(self.type_proxy).values(None)

    def test_values_returns_container_for_values(self):
        self.tested_type.x = 1243314135
        self.tested_type.y = -1243314135
        result = self.type_proxy.values()
        self.assertTrue(hasattr(result, "__iter__"))
        result_list = list(iter(result))
        self.assertIn(1243314135, result_list)
        self.assertIn(-1243314135, result_list)

    def test_values_returns_iterator_excluding_placeholder_value(self):
        result = self.type_proxy.values()
        self.assertNotIn("placeholder_value", result)


class DunderSlotsTests(unittest.TestCase):
    def test_with_non_str_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            class C:
                __slots__ = "a", 1

        self.assertIn(
            "__slots__ items must be strings, not 'int'", str(context.exception)
        )

    def test_with_non_identifier_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            class C:
                __slots__ = "a", ";;"

        self.assertIn("__slots__ must be identifiers", str(context.exception))

    def test_with_duplicated_dunder_dict_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            class C:
                __slots__ = "a", "__dict__", "__dict__"

        self.assertIn(
            "__dict__ slot disallowed: we already got one", str(context.exception)
        )

    def test_with_conflicting_name_raises_value_error(self):
        with self.assertRaises(ValueError) as context:

            class C:
                __slots__ = "a"
                a = "conflicting"

        self.assertIn(
            "'a' in __slots__ conflicts with class variable", str(context.exception)
        )

    def test_without_dunder_dict_removes_dunder_dict_type_attribute(self):
        class C:
            __slots__ = "a"

        self.assertNotIn("__dict__", C.__dict__)

    def test_with_dunder_dict_keeps_dunder_dict_type_attribute(self):
        class C:
            __slots__ = "a", "__dict__"

        self.assertIn("__dict__", C.__dict__)

    def test_slot_descriptor_dunder_get_with_none_returns_type(self):
        class C:
            __slots__ = "a"

        descriptor = C.__dict__["a"]
        slot_descriptor_type = type(descriptor)
        self.assertIs(descriptor.__get__(None, slot_descriptor_type), descriptor)

    def test_slot_descriptor_dunder_get_with_instance_returns_attribute(self):
        class C:
            __slots__ = "a"

            def __init__(self):
                self.a = 10

        descriptor = C.__dict__["a"]
        instance = C()
        self.assertEqual(descriptor.__get__(instance), 10)

    def test_slot_descriptor_dunder_get_with_instance_and_none_owner_returns_attribute(
        self,
    ):
        class C:
            __slots__ = "a"

            def __init__(self):
                self.a = 10

        descriptor = C.__dict__["a"]
        instance = C()
        self.assertEqual(descriptor.__get__(instance, None), 10)

    def test_slot_descriptor_dunder_get_with_none_instance_and_none_owner_raises_type_error(
        self,
    ):
        class C:
            __slots__ = "a"

        descriptor = C.__dict__["a"]

        with self.assertRaises(TypeError) as context:
            descriptor.__get__(None, None)
        self.assertEqual(
            str(context.exception),
            "__get__(None, None) is invalid",
        )

    def test_slot_descriptor_dunder_delete_returns_none(self):
        class C:
            __slots__ = "a"

            def __init__(self):
                self.a = 10

        descriptor = C.__dict__["a"]
        instance = C()
        self.assertEqual(descriptor.__delete__(instance), None)

    def test_slot_descriptor_dunder_delete_none_raises_type_error(self):
        class C:
            __slots__ = "a"

        descriptor = C.__dict__["a"]

        with self.assertRaises(TypeError) as context:
            descriptor.__delete__(None)
        self.assertEqual(
            str(context.exception),
            "descriptor 'a' for 'C' objects doesn't apply to a 'NoneType' object",
        )

    def test_creates_type_attributes(self):
        class C:
            __slots__ = "a", "b", "c"

        self.assertIn("a", C.__dict__)
        self.assertIn("b", C.__dict__)
        self.assertIn("c", C.__dict__)

        slot_descriptor_type = type(C.a)
        self.assertTrue(hasattr(slot_descriptor_type, "__get__"))
        self.assertTrue(hasattr(slot_descriptor_type, "__set__"))
        self.assertIsInstance(C.a, slot_descriptor_type)
        self.assertIsInstance(C.b, slot_descriptor_type)
        self.assertIsInstance(C.c, slot_descriptor_type)

    def test_sharing_same_layout_base_can_servce_as_bases(self):
        class C:
            __slots__ = "x"

        class D(C):
            pass

        class E(C):
            __slots__ = "y"

        class F(D, E):
            pass

        self.assertTrue(hasattr(F, "x"))
        self.assertTrue(hasattr(F, "y"))

    def test_attributes_raises_attribute_error_before_set(self):
        class C:
            __slots__ = "x"

        c = C()
        self.assertFalse(hasattr(c, "x"))
        with self.assertRaises(AttributeError):
            c.x

    def test_attributes_return_set_attributes(self):
        class C:
            __slots__ = "x"

        c = C()
        c.x = 500
        self.assertEqual(c.x, 500)

    def test_attributes_raises_attribute_error_after_deletion(self):
        class C:
            __slots__ = "x"

        c = C()
        c.x = 50
        del c.x
        with self.assertRaises(AttributeError):
            c.x

    def test_attributes_delete_raises_attribute_before_set(self):
        class C:
            __slots__ = "x"

        c = C()
        with self.assertRaises(AttributeError):
            del c.x

    def test_with_sealed_base_seals_type(self):
        class C:
            __slots__ = ()

        c = C()
        with self.assertRaises(AttributeError):
            c.x = 500
        self.assertFalse(hasattr(c, "__dict__"))

    def test_with_unsealed_base_unseals_type(self):
        class C:
            pass

        class D(C):
            __slots__ = ()

        d = D()
        d.x = 500
        self.assertEqual(d.x, 500)
        self.assertTrue(hasattr(d, "__dict__"))

    def test_inherit_from_bases(self):
        class C:
            __slots__ = ()

        class D(C):
            __slots__ = "x", "y"

        class E(D, C):
            __slots__ = "z"

        e = E()
        e.x = 1
        e.y = 2
        e.z = 3
        self.assertEqual(e.x, 1)
        self.assertEqual(e.y, 2)
        self.assertEqual(e.z, 3)
        with self.assertRaises(AttributeError):
            e.w = 500
        self.assertFalse(hasattr(e, "__dict__"))

    def test_inherit_from_builtin_type(self):
        class C(dict):
            __slots__ = "x"

        c = C()
        c.x = 500
        c["p"] = 4
        c["q"] = 5
        self.assertEqual(c.x, 500)
        self.assertEqual(c["p"], 4)
        self.assertEqual(c["q"], 5)

    def test_are_independent_from_inherited_slots(self):
        class C:
            __slots__ = ("a", "c")

        class D(C):
            __slots__ = ("b", "c")

        obj = D()
        obj.a = 1
        obj.b = 2
        obj.c = 3
        C.c.__set__(obj, 33)

        self.assertEqual(obj.a, 1)
        self.assertEqual(C.a.__get__(obj), 1)
        self.assertEqual(D.a.__get__(obj), 1)
        self.assertEqual(obj.b, 2)
        with self.assertRaises(AttributeError):
            C.b
        self.assertEqual(D.b.__get__(obj), 2)
        self.assertEqual(obj.c, 3)
        self.assertEqual(C.c.__get__(obj), 33)
        self.assertEqual(D.c.__get__(obj), 3)

    def test_member_descriptor_works_only_for_subtypes(self):
        class C:
            __slots__ = "x"

        class D(C):
            __slots__ = "y"

        class E:
            __slots__ = "x"

        d = D()
        # C's member_descriptor for "x" works for D instances.
        C.x.__set__(d, 500)
        self.assertEqual(C.x.__get__(d), 500)
        self.assertEqual(d.x, 500)

        e = E()
        with self.assertRaises(TypeError) as context:
            C.x.__set__(e, 500)
        self.assertEqual(
            str(context.exception),
            "descriptor 'x' for 'C' objects doesn't apply to a 'E' object",
        )

        e.x = 600
        with self.assertRaises(TypeError) as context:
            C.x.__get__(e)
        self.assertEqual(
            str(context.exception),
            "descriptor 'x' for 'C' objects doesn't apply to a 'E' object",
        )

    def test_private_names_are_mangled(self):
        class C:
            __slots__ = ("__priv", "__priv_")

            def __init__(self):
                self.__priv = 42
                self.__priv_ = 8

        c = C()
        self.assertEqual(c._C__priv, 42)
        self.assertEqual(c._C__priv_, 8)

    def test_names_are_not_mangled(self):
        class C:
            __slots__ = ("_notpriv", "__notpriv__")

            def __init__(self):
                self._notpriv = "foo"
                self.__notpriv__ = "bar"

        c = C()
        self.assertEqual(c._notpriv, "foo")
        self.assertEqual(c.__notpriv__, "bar")


if __name__ == "__main__":
    unittest.main()
