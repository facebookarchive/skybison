#!/usr/bin/env python3
import unittest
import weakref
from unittest.mock import MagicMock, Mock


class WeakRefTests(unittest.TestCase):
    def test_ref_dunder_callback_readonly(self):
        class C:
            pass

        def callback(*args):
            pass

        obj = C()
        ref = weakref.ref(obj)
        with self.assertRaises(AttributeError):
            ref.__callback__ = callback

    def test_ref_dunder_callback_with_callback_returns_callback(self):
        class C:
            pass

        def callback(*args):
            pass

        obj = C()
        ref = weakref.ref(obj, callback)
        self.assertIs(ref.__callback__, callback)

    def test_ref_dunder_callback_without_callback_returns_none(self):
        class C:
            pass

        obj = C()
        ref = weakref.ref(obj)
        self.assertIsNone(ref.__callback__)

    def test_ref_dunder_call_with_non_ref_raises_type_error(self):
        with self.assertRaisesRegex(
            TypeError, "'__call__' requires a 'weakref' object but received a 'str'"
        ):
            weakref.ref.__call__("not a weakref")

    def test_dunder_eq_proxies_dunder_eq(self):
        class C:
            def __eq__(self, other):
                return self is other

        obj1 = C()
        obj2 = C()
        ref1 = weakref.ref(obj1)
        ref2 = weakref.ref(obj2)
        obj1.__eq__ = MagicMock()
        self.assertIs(ref1.__eq__(ref2), False)
        obj1.__eq__.called_once_with(obj1, obj2)
        self.assertIs(ref1.__eq__(ref1), True)
        obj1.__eq__.called_once_with(obj1, obj1)

    def test_dunder_eq_with_non_ref_returns_not_implemented(self):
        class C:
            pass

        obj = C()
        ref = weakref.ref(obj)
        not_a_ref = object()
        self.assertIs(ref.__eq__(not_a_ref), NotImplemented)

    def test_dunder_ge_always_returns_not_implemented(self):
        class C:
            pass

        obj = C()
        ref = weakref.ref(obj)
        not_a_ref = object()
        self.assertIs(ref.__ge__(ref), NotImplemented)
        self.assertIs(ref.__ge__(not_a_ref), NotImplemented)

    def test_dunder_gt_always_returns_not_implemented(self):
        class C:
            pass

        obj = C()
        ref = weakref.ref(obj)
        not_a_ref = object()
        self.assertIs(ref.__gt__(ref), NotImplemented)
        self.assertIs(ref.__gt__(not_a_ref), NotImplemented)

    def test_dunder_le_always_returns_not_implemented(self):
        class C:
            pass

        obj = C()
        ref = weakref.ref(obj)
        not_a_ref = object()
        self.assertIs(ref.__le__(ref), NotImplemented)
        self.assertIs(ref.__le__(not_a_ref), NotImplemented)

    def test_dunder_lt_always_returns_not_implemented(self):
        class C:
            pass

        obj = C()
        ref = weakref.ref(obj)
        not_a_ref = object()
        self.assertIs(ref.__lt__(ref), NotImplemented)
        self.assertIs(ref.__lt__(not_a_ref), NotImplemented)

    def test_dunder_ne_proxies_dunder_ne(self):
        class C:
            def __ne__(self, other):
                return self is other

        obj1 = C()
        obj2 = C()
        ref1 = weakref.ref(obj1)
        ref2 = weakref.ref(obj2)
        obj1.__ne__ = MagicMock()
        self.assertIs(ref1.__ne__(ref2), False)
        obj1.__ne__.called_once_with(obj1, obj2)
        self.assertIs(ref1.__ne__(ref1), True)
        obj1.__ne__.called_once_with(obj1, obj1)

    def test_dunder_ne_with_non_ref_returns_not_implemented(self):
        class C:
            pass

        obj = C()
        ref = weakref.ref(obj)
        not_a_ref = object()
        self.assertIs(ref.__ne__(not_a_ref), NotImplemented)

    def test_dunder_new_with_subtype_return_subtype_instance(self):
        class SubRef(weakref.ref):
            pass

        class C:
            def __eq__(self, other):
                return "C.__eq__"

        c = C()
        sub_ref = SubRef(c)
        self.assertIsInstance(sub_ref, SubRef)
        self.assertIsInstance(sub_ref, weakref.ref)

        ref = weakref.ref(c)
        self.assertEqual(sub_ref.__eq__(ref), "C.__eq__")

        sub_ref.new_attribute = 50
        self.assertEqual(sub_ref.new_attribute, 50)

    def test_dunder_new_with_non_type_raises_type_error(self):
        with self.assertRaises(TypeError):
            weakref.ref.__new__("not a type object")

    def test_dunder_new_with_non_ref_subtype_raises_type_error(self):
        with self.assertRaises(TypeError):
            weakref.ref.__new__(list)

    def test_hash_on_proxy_not_callable_object_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            class NotCallable:
                def get_name(self):
                    return "NotCallableObject"

            not_callable = NotCallable()
            proxy = weakref.proxy(not_callable)
            hash(proxy)
        self.assertEqual(str(context.exception), "unhashable type: 'weakproxy'")

    def test_proxy_not_callable_object_returns_proxy_type(self):
        class NotCallable:
            def get_name(self):
                return "NotCallableObject"

        not_callable = NotCallable()
        proxy = weakref.proxy(not_callable)
        self.assertEqual(type(proxy), weakref.ProxyType)

    def test_proxy_calls_to_dunder_functions(self):
        class C:
            def __add__(self, another):
                return 50 + another

        c = C()
        proxy = weakref.proxy(c)
        self.assertEqual(proxy + 5, 55)

    def test_proxy_with_hash_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            class C:
                pass

            c = C()
            hash(weakref.proxy(c))
        self.assertEqual(str(context.exception), "unhashable type: 'weakproxy'")

    def test_proxy_dunder_hash_function_access_suceeds(self):
        class C:
            pass

        c = C()
        m = c.__hash__()
        self.assertNotEqual(m, 0)

    def test_proxy_field_access(self):
        class C:
            def __init__(self):
                self.field = "field_value"

        c = C()
        proxy = weakref.proxy(c)
        self.assertEqual(proxy.field, "field_value")

    def test_proxy_instance_method_call(self):
        class C:
            def method(self):
                return "method_return"

        c = C()
        proxy = weakref.proxy(c)
        self.assertEqual(proxy.method(), "method_return")

    def test_hash_on_proxy_callable_object_raises_type_error(self):
        with self.assertRaises(TypeError) as context:

            class Callable:
                def __call__(self):
                    return "CallableObject"

            callable = Callable()
            proxy = weakref.proxy(callable)
            hash(proxy)
        self.assertEqual(str(context.exception), "unhashable type: 'weakcallableproxy'")

    def test_proxy_callable_object_returns_callable_proxy_type(self):
        class Callable:
            def __call__(self):
                return "CallableObject"

        callable = Callable()
        proxy = weakref.proxy(callable)
        self.assertTrue(isinstance(proxy, weakref.CallableProxyType))

    def test_proxy_callable_object_returns_callable_object(self):
        class Callable:
            def __call__(self):
                return "CallableObject"

        callable_obj = Callable()
        proxy = weakref.proxy(callable_obj)
        self.assertEqual(proxy(), "CallableObject")

    def test_hash_returns_referent_hash(self):
        class C:
            def __hash__(self):
                return 123456

        i = C()
        r = weakref.ref(i)
        self.assertEqual(hash(r), hash(i))

    def test_hash_picks_correct_dunder_hash(self):
        class C:
            def __hash__(self):
                raise Exception("Should not pick this")

        r = weakref.ref(C)
        self.assertEqual(hash(r), hash(C))

    def test_hash_caches_referent_hash(self):
        m = Mock()
        m.__hash__ = MagicMock(return_value=99)
        r = weakref.ref(m)
        self.assertEqual(hash(r), 99)
        self.assertEqual(hash(r), 99)
        m.__hash__.assert_called_once()

    # TODO(T43270097): Add a test for callback.


if __name__ == "__main__":
    unittest.main()
