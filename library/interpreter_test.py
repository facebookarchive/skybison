#!/usr/bin/env python3
import sys
import unittest
from types import MethodType, ModuleType, SimpleNamespace
from unittest import skipIf
from unittest.mock import Mock


class IntepreterTest(unittest.TestCase):
    def test_binary_mul_smallint_returns_large_int(self):
        def mul(a, b):
            return a * b

        # Rewrite mul() to use BINARY_MUL_SMALLINT.
        self.assertEqual(mul(5, 6), 30)

        small_int = 1 << 62
        self.assertEqual(mul(small_int, small_int), small_int * small_int)

    def test_binary_mul_smallint_with_operand_zero_returns_zero(self):
        def mul(a, b):
            return a * b

        # Rewrite mul() to use BINARY_MUL_SMALLINT.
        self.assertEqual(mul(5, 6), 30)

        small_int = 1 << 62
        self.assertEqual(mul(small_int, 0), 0)
        self.assertEqual(mul(0, small_int), 0)

    @skipIf(
        sys.implementation.name == "cpython" and sys.version_info[:2] < (3, 7),
        "requires at least CPython 3.7",
    )
    def test_binary_subscr_calls_type_dunder_class_getitem(self):
        class C:
            def __class_getitem__(cls, item):
                return f"C:{cls.__name__}[{item.__name__}]"

        self.assertEqual(C[int], "C:C[int]")

    @skipIf(
        sys.implementation.name == "cpython" and sys.version_info[:2] < (3, 7),
        "requires at least CPython 3.7",
    )
    def test_binary_subscr_ignores_instance_dunder_class_getitem(self):
        class C:
            def __class_getitem__(cls, item):
                return f"C:{cls.__name__}[{item.__name__}]"

        with self.assertRaises(TypeError) as context:
            C()[int]
        self.assertEqual(str(context.exception), "'C' object is not subscriptable")

    @skipIf(
        sys.implementation.name == "cpython" and sys.version_info[:2] < (3, 7),
        "requires at least CPython 3.7",
    )
    def test_binary_subscr_prioritizes_metaclass_dunder_getitem(self):
        class M(type):
            def __getitem__(cls, item):
                return f"M:{cls.__name__}[{item.__name__}]"

        class C(metaclass=M):
            def __class_getitem__(cls, item):
                return f"C:{cls.__name__}[{item.__name__}]"

        self.assertEqual(C[int], "M:C[int]")

    def test_compare_op_in_propagetes_exception(self):
        class C:
            def __contains__(self, value):
                raise UserWarning("C.__contains__")

        c = C()
        with self.assertRaises(UserWarning) as context:
            1 in c
        self.assertEqual(str(context.exception), "C.__contains__")

    def test_compare_op_not_in_propagetes_exception(self):
        class C:
            def __contains__(self, value):
                raise UserWarning("C.__contains__")

        c = C()
        with self.assertRaises(UserWarning) as context:
            1 not in c
        self.assertEqual(str(context.exception), "C.__contains__")

    def test_for_iter_iterates_dict_by_insertion_order(self):
        d = {}
        d["a"] = 1
        d["c"] = 3
        d["b"] = 2
        result = []
        for key in d:
            result.append(key)
        self.assertEqual(result, ["a", "c", "b"])

    def test_store_name_calls_dunder_setitem(self):
        class C(dict):
            def __setitem__(self, key, value):
                self.result = (key, value)

        locals = C()
        exec("x = 44", None, locals)
        self.assertEqual(locals.result[0], "x")
        self.assertEqual(locals.result[1], 44)

    def test_store_name_setitem_propagates_exception(self):
        class C:
            def __setitem__(self, key, value):
                raise UserWarning("bar")

            def __getitem__(self, key):
                ...  # just here so this type is considered a mapping.

        locals = C()
        with self.assertRaises(UserWarning) as context:
            exec("x = 1", None, locals)
        self.assertEqual(str(context.exception), "bar")

    def test_store_name_setitem_propagates_descriptor_exception(self):
        class D:
            def __get__(self, instance, owner):
                self.result = (instance, owner)
                raise UserWarning("baz")

        d = D()

        class C:
            __setitem__ = d

            def __getitem__(self, key):
                ...  # just here so this type is considered a mapping.

        locals = C()
        with self.assertRaises(UserWarning) as context:
            exec("x = 1", None, locals)
        self.assertEqual(str(context.exception), "baz")
        self.assertEqual(d.result[0], locals)
        self.assertEqual(d.result[1], C)

    def test_load_attr_calls_dunder_get(self):
        class Descriptor:
            def __get__(self, *args):
                self.args = args
                return 42

        descriptor = Descriptor()

        class C:
            foo = descriptor

        self.assertEqual(C.foo, 42)
        self.assertEqual(descriptor.args, (None, C))
        c = C()
        self.assertEqual(c.foo, 42)
        self.assertEqual(descriptor.args, (c, C))

    def test_load_attr_calls_callable_dunder_get(self):
        class Callable:
            def __call__(self, *args):
                self.args = args
                return 42

        callable = Callable()

        class Descriptor:
            __get__ = callable

        descriptor = Descriptor()

        class C:
            foo = descriptor

        self.assertEqual(C.foo, 42)
        self.assertEqual(callable.args, (descriptor, None, C))
        c = C()
        self.assertEqual(c.foo, 42)
        self.assertEqual(callable.args, (descriptor, c, C))

    def test_load_attr_does_not_recursively_call_dunder_get(self):
        class Descriptor0:
            def __get__(self, *args):
                return 13

            def __call__(self, *args):
                return 42

        descriptor0 = Descriptor0()

        class Descriptor1:
            __get__ = descriptor0

        descriptor1 = Descriptor1()

        class C:
            foo = descriptor1

        self.assertEqual(C.foo, 42)
        self.assertEqual(C().foo, 42)
        self.assertIs(C.__dict__["foo"].__get__, 13)

    def test_load_attr_with_module_prioritizes_data_descr_than_instance_attr(self):
        from types import ModuleType

        class MyModule(ModuleType):
            @property
            def foo(self):
                return "data descriptor"

        m = MyModule("m")
        m.__dict__["foo"] = "instance attribute"

        self.assertEqual(m.foo, "data descriptor")

    def test_load_attr_with_module_prioritizes_instance_attr_than_non_data_descr(self):
        from types import ModuleType

        class NonDataDescriptor:
            def __get__(self, obj, obj_type):
                return "non-data descriptor"

        class MyModule(ModuleType):
            foo = NonDataDescriptor()

        m = MyModule("m")
        m.foo = "instance attribute"

        self.assertEqual(m.foo, "instance attribute")

    def test_load_attr_with_module_prioritizes_non_data_descr_than_instance_getattr(
        self,
    ):
        from types import ModuleType

        class NonDataDescriptor:
            def __get__(self, obj, obj_type):
                return "non-data descriptor"

        class MyModule(ModuleType):
            foo = NonDataDescriptor()

        m = MyModule("m")
        exec('def __getattr__(name): return name + " instance.__getattr__"', m.__dict__)
        self.assertEqual(m.foo, "non-data descriptor")

    def test_load_attr_with_module_prioritizes_instance_getattr_than_type_getattr(self):
        from types import ModuleType

        class MyModule(ModuleType):
            def __getattr__(self, name):
                return name + " type.__getattr__"

        m = MyModule("m")
        exec('def __getattr__(name): return name + " instance.__getattr__"', m.__dict__)
        self.assertEqual(m.foo, "foo instance.__getattr__")

    def test_load_attr_with_module_returns_type_getattr(self):
        from types import ModuleType

        class MyModule(ModuleType):
            def __getattr__(self, name):
                return name + " type.__getattr__"

        m = MyModule("m")
        self.assertEqual(m.foo, "foo type.__getattr__")

    def test_load_attr_with_module_returns_type_getattr(self):
        from types import ModuleType

        class MyModule(ModuleType):
            pass

        m = MyModule("m")
        with self.assertRaises(AttributeError):
            m.foo

    def test_load_deref_with_assigned_cell_returns_assigned_value(self):
        def foo(x):
            def bar():
                return x

            return bar

        bar = foo(10)
        self.assertEqual(bar(), 10)

    def test_load_deref_with_delete_cell_raises_name_error(self):
        def foo(x):
            def bar():
                return x  # noqa: F821

            del x
            return bar

        bar = foo(10)
        with self.assertRaises(NameError):
            bar()

    def test_load_name_calls_dunder_getitem(self):
        class C:
            def __getitem__(self, key):
                return (key, 17)

        globals = {"foo": None}
        locals = C()
        exec('assert(foo == ("foo", 17))', globals, locals)

    def test_load_name_continues_on_keyerror(self):
        class C:
            def __getitem__(self, key):
                self.result = key
                raise KeyError()

        globals = {}
        exec("foo = 99", globals)
        self.assertIn("foo", globals)

        locals = C()
        exec("assert(foo == 99)", globals, locals)  # should not raise
        self.assertEqual(locals.result, "foo")

    def test_load_name_getitem_propagates_exception(self):
        class C:
            def __getitem__(self, key):
                raise UserWarning("bar")

        globals = None
        locals = C()
        with self.assertRaises(UserWarning) as context:
            exec("foo", globals, locals)
        self.assertEqual(str(context.exception), "bar")

    def test_load_name_getitem_propagates_descriptor_exception(self):
        class D:
            def __get__(self, instance, owner):
                self.result = (instance, owner)
                raise UserWarning("baz")

        d = D()

        class C:
            __getitem__ = d

        globals = None
        locals = C()
        with self.assertRaises(UserWarning) as context:
            exec("foo", globals, locals)
        self.assertEqual(str(context.exception), "baz")
        self.assertEqual(d.result[0], locals)
        self.assertEqual(d.result[1], C)

    def test_load_global_returns_global(self):
        globals = {
            "foo": 42,
            "__builtins__": {"foo": 8},
        }
        locals = {"foo": 3}
        self.assertIs(
            exec("def get_foo(): global foo; return foo", globals, locals), None
        )
        self.assertEqual(locals["get_foo"](), 42)

    def test_load_global_returns_builtin(self):
        globals = {
            "foo": 42,
            "__builtins__": {"bar": 8},
        }
        locals = {"bar": 3}
        self.assertIs(
            exec("def get_bar(): global bar; return bar", globals, locals), None
        )
        self.assertEqual(locals["get_bar"](), 8)

    def test_load_global_does_not_cache_builtin(self):
        builtins = {"bar": -8}
        globals = {"foo": 42, "__builtins__": builtins}
        locals = {"bar": 3}
        self.assertIs(
            exec("def get_bar(): global bar; return bar", globals, locals), None
        )
        self.assertEqual(locals["get_bar"](), -8)
        builtins["bar"] = 9
        self.assertEqual(locals["get_bar"](), 9)

    def test_load_global_calls_builtins_getitem(self):
        class C:
            def __getitem__(self, key):
                return (self, key)

        builtins = C()
        globals = {"foo": 42, "__builtins__": builtins}
        locals = {"bar": 3}
        self.assertIs(
            exec("def get_bar(): global bar; return bar", globals, locals), None
        )
        self.assertEqual(locals["get_bar"](), (builtins, "bar"))

    def test_delete_attr_calls_dunder_delete(self):
        class Descriptor:
            def __delete__(self, *args):
                self.args = args
                return "return value ignored"

        descriptor = Descriptor()

        class C:
            foo = descriptor

        c = C()
        del c.foo
        self.assertEqual(descriptor.args, (c,))
        # Deletion on class should just delete descriptor
        del C.foo
        self.assertFalse(hasattr(C, "foo"))
        self.assertNotIn("foo", C.__dict__)

    def test_delete_attr_calls_callable_dunder_delete(self):
        class Callable:
            def __call__(self, *args):
                self.args = args
                return "return value ignored"

        callable = Callable()

        class Descriptor:
            __delete__ = callable

        descriptor = Descriptor()

        class C:
            foo = descriptor

        c = C()
        del c.foo
        self.assertEqual(callable.args, (c,))
        # Deletion on class should just delete descriptor
        del C.foo
        self.assertFalse(hasattr(C, "foo"))
        self.assertNotIn("foo", C.__dict__)

    def test_delete_attr_call_dunder_delete_descriptor(self):
        class Callable:
            def __call__(self, *args):
                self.args = args
                return "return value ignored"

        callable = Callable()

        class Descriptor0:
            def __get__(self, *args):
                self.args = args
                return callable

        descriptor0 = Descriptor0()

        class Descriptor1:
            __delete__ = descriptor0

        descriptor1 = Descriptor1()

        class C:
            foo = descriptor1

        c = C()
        del c.foo
        self.assertEqual(descriptor0.args, (descriptor1, Descriptor1))
        self.assertEqual(callable.args, (c,))

    def test_delete_fast_with_assigned_name_succeeds(self):
        def foo():
            a = 4
            del a

        foo()

    def test_delete_fast_with_deleted_name_raises_unbound_local_error(self):
        def foo():
            a = 4
            del a
            del a  # noqa: F821

        with self.assertRaises(UnboundLocalError) as context:
            foo()
        self.assertEqual(
            str(context.exception), "local variable 'a' referenced before assignment"
        )

    def test_delete_name_calls_dunder_delitem(self):
        class C(dict):
            def __delitem__(self, key):
                self.result = key
                return None

        locals = C()
        exec("del foo", None, locals)
        self.assertEqual(locals.result, "foo")

    def test_delete_name_raises_name_error_on_error(self):
        class C(dict):
            def __delitem__(self, key):
                self.result = key
                raise UserWarning("bar")

        globals = {}
        locals = C()
        with self.assertRaises(NameError) as context:
            exec("del foo", globals, locals)
        self.assertEqual(str(context.exception), "name 'foo' is not defined")
        self.assertEqual(locals.result, "foo")

    def test_delete_name_raises_name_error_on_descriptor_error(self):
        class D:
            def __get__(self, instance, owner):
                self.result = (instance, owner)
                raise UserWarning("baz")

        d = D()

        class C(dict):
            __delitem__ = d

        globals = {}
        locals = C()
        with self.assertRaises(NameError) as context:
            exec("del foo", globals, locals)
        self.assertEqual(str(context.exception), "name 'foo' is not defined")
        self.assertEqual(d.result[0], locals)
        self.assertEqual(d.result[1], C)

    def test_get_iter_with_range_with_bool(self):
        result = None
        for i in range(True):
            if result is None:
                result = i
            else:
                result += i
        self.assertEqual(result, 0)

        result = 0
        for i in range(True, 4):
            result += i
        self.assertEqual(result, 6)

        result = 0
        for i in range(0, 4, True):
            result += i
        self.assertEqual(result, 6)

    def test_import_performs_secondary_lookup(self):
        import sys

        class FakeModule:
            def __getattribute__(self, name):
                if name == "__name__":
                    return "test_import_performs_secondary_lookup_fake_too"
                raise AttributeError(name)

        sys.modules["test_import_performs_secondary_lookup_fake"] = FakeModule()
        sys.modules["test_import_performs_secondary_lookup_fake_too.foo"] = 42
        try:
            from test_import_performs_secondary_lookup_fake import foo
        finally:
            del sys.modules["test_import_performs_secondary_lookup_fake"]
            del sys.modules["test_import_performs_secondary_lookup_fake_too.foo"]
        self.assertEqual(foo, 42)

    def test_call_ex_kw_with_class_with_iterable_raises_type_error(self):
        def foo(a):
            return a

        mapping = [("hello", "world")]
        with self.assertRaisesRegex(TypeError, "must be a mapping, not list"):
            foo(**mapping)

    def test_call_ex_kw_with_class_without_keys_method_raises_type_error(self):
        class C:
            __getitem__ = Mock(name="__getitem__", return_value="mock")

        def foo(a):
            return a

        mapping = C()
        with self.assertRaisesRegex(TypeError, "must be a mapping, not C"):
            foo(**mapping)

    def test_call_ex_kw_with_dict_subclass_does_not_call_keys_or_dunder_getitem(self):
        class C(dict):
            __getitem__ = Mock(name="__getitem__", return_value="mock")
            keys = Mock(name="keys", return_value=("a",))

        def foo(a):
            return a

        mapping = C({"a": "foo"})
        result = foo(**mapping)
        self.assertEqual(result, "foo")
        C.keys.assert_not_called()
        C.__getitem__.assert_not_called()

    def test_call_ex_kw_with_non_dict_calls_keys_and_dunder_getitem(self):
        class C:
            __getitem__ = Mock(name="__getitem__", return_value="mock")
            keys = Mock(name="keys", return_value=("a",))

        def foo(a):
            return a

        mapping = C()
        result = foo(**mapping)
        self.assertEqual(result, "mock")
        C.keys.assert_called_once()
        C.__getitem__.assert_called_once()

    def test_call_ex_kw_with_non_dict_passes_dict_as_kwargs(self):
        class C:
            __getitem__ = Mock(name="__getitem__", return_value="mock")
            keys = Mock(name="keys", return_value=("hello", "world"))

        def foo(**kwargs):
            self.assertIs(type(kwargs), dict)
            self.assertIn("hello", kwargs)
            self.assertIn("world", kwargs)

        mapping = C()
        foo(**mapping)
        C.keys.assert_called_once()
        self.assertEqual(C.__getitem__.call_count, 2)

    def test_callfunction_kw_does_not_modify_actual_names(self):
        def foo(a, b):
            return (a, b)

        def bar():
            return foo(b="b", a="a")

        # Call the same function twice to see the actual names are preserved.
        self.assertEqual(bar(), ("a", "b"))
        self.assertEqual(bar(), ("a", "b"))

    def test_call_raises_recursion_error(self):
        def foo(x):
            return foo(x)

        with self.assertRaisesRegex(RecursionError, "maximum recursion depth exceeded"):
            foo(1)

    def test_cache_misses_after_dunder_class_update(self):
        class C:
            def foo(self):
                return 100

        class D:
            def foo(self):
                return 200

        def cache_attribute(c):
            return c.foo()

        c = C()
        # Load the cache
        result = cache_attribute(c)
        self.assertIs(result, 100)

        c.__class__ = D
        # The loaded cache doesn't match `c` since its layout id has changed.
        result = cache_attribute(c)
        self.assertIs(result, 200)


class InlineCacheTests(unittest.TestCase):
    def test_load_slot_descriptor(self):
        class C:
            __slots__ = "x"

        def read_x(c):
            return c.x

        c = C()
        c.x = 50

        # Cache C.x.
        self.assertEqual(read_x(c), 50)
        # Use the cached descriptor.
        self.assertEqual(read_x(c), 50)

        # The cached descriptor raises AttributeError after its corresponding
        # attribute gets deleted.
        del c.x
        with self.assertRaises(AttributeError):
            read_x(c)

    def test_load_slot_descriptor_invalidated_by_updating_type_attr(self):
        class C:
            __slots__ = "x"

        def read_x(c):
            return c.x

        c = C()
        c.x = 50

        # Cache C.x.
        self.assertEqual(read_x(c), 50)

        # Invalidate the cache.
        C.x = property(lambda self: 100)

        # Verify that the updated type attribute is used for `c.x`.
        self.assertEqual(read_x(c), 100)

    def test_store_attr_calls_dunder_set(self):
        class Descriptor:
            def __set__(self, *args):
                self.args = args
                return "return value ignored"

        descriptor = Descriptor()

        class C:
            foo = descriptor

        c = C()
        c.foo = 42
        self.assertEqual(descriptor.args, (c, 42))
        # Assignment on the class should replace descriptor.
        C.foo = 13
        self.assertEqual(C.foo, 13)
        self.assertEqual(C.__dict__["foo"], 13)

    def test_store_attr_calls_callable_dunder_set(self):
        class Callable:
            def __call__(self, *args):
                self.args = args
                return "return value ignored"

        callable = Callable()

        class Descriptor:
            __set__ = callable

        descriptor = Descriptor()

        class C:
            foo = descriptor

        c = C()
        c.foo = 42
        self.assertEqual(callable.args, (c, 42))
        # Assignment on the class should replace descriptor.
        C.foo = 13
        self.assertEqual(C.foo, 13)
        self.assertEqual(C.__dict__["foo"], 13)

    def test_store_attr_call_dunder_set_descriptor(self):
        class Callable:
            def __call__(self, *args):
                self.args = args
                return "return value ignored"

        callable = Callable()

        class Descriptor0:
            def __get__(self, *args):
                self.args = args
                return callable

        descriptor0 = Descriptor0()

        class Descriptor1:
            __set__ = descriptor0

        descriptor1 = Descriptor1()

        class C:
            foo = descriptor1

        c = C()
        c.foo = 42
        self.assertEqual(descriptor0.args, (descriptor1, Descriptor1))
        self.assertEqual(callable.args, (c, 42))

    def test_store_slot_descriptor(self):
        class C:
            __slots__ = "x"

        def write_x(c, v):
            c.x = v

        c = C()

        # Cache C.x.
        write_x(c, 1)
        self.assertEqual(c.x, 1)

        # Use the cached descriptor.
        write_x(c, 2)
        self.assertEqual(c.x, 2)

    def test_store_slot_descriptor_invalidated_by_updating_type_attr(self):
        class C:
            __slots__ = "x"

        def write_x(c, v):
            c.x = v

        c = C()

        # Cache C.x.
        write_x(c, 1)
        self.assertEqual(c.x, 1)

        # Invalidate the cache.
        def setter(self, value):
            value["setter_executed"] = True

        C.x = property(lambda self: 100, setter)

        # Verify that the updated type attribute is used for `c.x`.
        v = {"setter_executed": False}
        write_x(c, v)
        self.assertTrue(v["setter_executed"], True)

    def test_raise_oserror_with_errno_and_strerror(self):
        with self.assertRaises(FileNotFoundError) as context:
            open("/tmp/nonexisting_file")
        exc = context.exception
        self.assertEqual(exc.errno, 2)
        self.assertEqual(exc.strerror, "No such file or directory")
        # TODO(T67323177): Check filename.

    def test_call_with_bound_method_storing_arbitrary_callable_resolves_callable(self):
        class C:
            def __call__(self, arg):
                return self, arg

        func = C()
        instance = object()
        method = MethodType(func, instance)
        self.assertEqual((func, instance), method())


class ImportNameTests(unittest.TestCase):
    def test_import_name_calls_dunder_builtins_dunder_import(self):
        def my_import(name, globals, locals, fromlist, level):
            return SimpleNamespace(baz=42, bam=(name, fromlist, level))

        builtins = ModuleType("builtins")
        builtins.__import__ = my_import
        my_globals = {"__builtins__": builtins}

        exec("from ..foo.bar import baz, bam", my_globals)
        self.assertEqual(my_globals["baz"], 42)
        self.assertEqual(my_globals["bam"], ("foo.bar", ("baz", "bam"), 2))

    def test_import_name_without_dunder_import_raises_import_error(self):
        my_builtins = ModuleType("my_builtins")
        my_globals = {"__builtins__": my_builtins}
        with self.assertRaisesRegex(ImportError, "__import__ not found"):
            exec("import foo", my_globals)

    def test_import_from_module_with_raising_dunder_getattr_propagates_exception(self):
        import collections

        self.assertIn("__getattr__", collections.__dict__)
        with self.assertRaises(ImportError):
            from collections import definitely_does_not_exist


class ImportStarTests(unittest.TestCase):
    def test_import_star_imports_symbols(self):
        def my_import(name, globals, locals, fromlist, level):
            result = ModuleType("m")
            result.foo = 1
            result._bar = 2
            result.baz_ = 3
            result.__bam__ = 4
            return result

        builtins = ModuleType("builtins")
        builtins.__import__ = my_import
        my_globals = {"__builtins__": builtins}

        exec("from m import *", my_globals)
        self.assertEqual(my_globals["foo"], 1)
        self.assertEqual(my_globals["baz_"], 3)
        self.assertNotIn("_bar", my_globals)
        self.assertNotIn("__bam__", my_globals)

    def test_import_star_with_dunder_all_tuple_imports_symbols(self):
        def my_import(name, globals, locals, fromlist, level):
            result = ModuleType("m")
            result.foo = 1
            result._bar = 2
            result.baz = 99
            result.__all__ = ("foo", "_bar", "__repr__")
            return result

        builtins = ModuleType("builtins")
        builtins.__import__ = my_import
        my_globals = {"__builtins__": builtins}

        exec("from m import *", my_globals)
        self.assertEqual(my_globals["foo"], 1)
        self.assertEqual(my_globals["_bar"], 2)
        self.assertEqual(my_globals["__repr__"](), "<module 'm'>")
        self.assertNotIn("baz", my_globals)
        self.assertNotIn("__all__", my_globals)

    def test_import_star_with_dunder_all_list_imports_symbols(self):
        def my_import(name, globals, locals, fromlist, level):
            result = ModuleType("m")
            result.foo = 1
            result._bar = 2
            result.baz = 99
            result.__all__ = ["foo", "_bar", "__all__"]
            return result

        builtins = ModuleType("builtins")
        builtins.__import__ = my_import
        my_globals = {"__builtins__": builtins}

        exec("from m import *", my_globals)
        self.assertEqual(my_globals["foo"], 1)
        self.assertEqual(my_globals["_bar"], 2)
        self.assertEqual(my_globals["__all__"], ["foo", "_bar", "__all__"])
        self.assertNotIn("baz", my_globals)

    def test_import_star_with_dunder_all_sequence_imports_symbols(self):
        def my_import(name, globals, locals, fromlist, level):
            class C:
                def __getitem__(self, key):
                    if key == 0:
                        return "foo"
                    if key == 1:
                        return "_bar"
                    raise IndexError

            result = ModuleType("m")
            result.foo = 1
            result._bar = 2
            result.baz = 99
            result.__all__ = C()
            return result

        builtins = ModuleType("builtins")
        builtins.__import__ = my_import
        my_globals = {"__builtins__": builtins}

        exec("from m import *", my_globals)
        self.assertEqual(my_globals["foo"], 1)
        self.assertEqual(my_globals["_bar"], 2)
        self.assertNotIn("baz", my_globals)

    def test_import_star_calls_object_dunder_dict_keys(self):
        def my_import(name, globals, locals, fromlist, level):
            class C:
                def keys(self):
                    return ["foo", "_bar", "baz_"]

            class M:
                __dict__ = C()

                def __getattr__(self, key):
                    if key == "__all__":
                        raise AttributeError
                    return f"value for {key}"

            return M()

        builtins = ModuleType("builtins")
        builtins.__import__ = my_import
        my_globals = {"__builtins__": builtins}

        exec("from m import *", my_globals)
        self.assertEqual(my_globals["foo"], "value for foo")
        self.assertEqual(my_globals["baz_"], "value for baz_")
        self.assertNotIn("_bar", my_globals)

    def test_import_star_calls_implicit_globals_dunder_setitem(self):
        def my_import(name, globals, locals, fromlist, level):
            result = ModuleType("m")
            result.foo = 88
            result.bar = 77
            return result

        class G(dict):
            def __init__(self):
                self.setitem_keys = []

            def __setitem__(self, key, value):
                dict.__setitem__(self, key, value)
                self.setitem_keys.append(key)

        builtins = ModuleType("builtins")
        builtins.__import__ = my_import
        my_globals = {"__builtins__": builtins}
        my_locals = G()

        exec("from m import *", my_globals, my_locals)
        self.assertEqual(my_locals["foo"], 88)
        self.assertEqual(my_locals["bar"], 77)
        self.assertEqual(sorted(my_locals.setitem_keys), ["bar", "foo"])
        self.assertNotIn("foo", my_globals)
        self.assertNotIn("bar", my_globals)

    def test_import_star_with_object_without_dunder_dict_raises_import_errro(self):
        def my_import(name, globals, locals, fromlist, level):
            class C:
                def __getattribute__(self, key):
                    if key == "__dict__":
                        raise AttributeError
                    return object.__getattribute__(self, key)

            return C()

        builtins = ModuleType("builtins")
        builtins.__import__ = my_import
        my_globals = {"__builtins__": builtins}

        with self.assertRaisesRegex(
            ImportError, r"from-import-\* object has no __dict__ and no __all__"
        ):
            exec("from m import *", my_globals)


if __name__ == "__main__":
    unittest.main()
