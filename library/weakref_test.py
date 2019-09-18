#!/usr/bin/env python3
import unittest
import weakref


class WeakRefTests(unittest.TestCase):
    def test_ref_dunder_call_with_non_ref_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            weakref.ref.__call__("not a weakref")
        self.assertTrue(
            str(context.exception) == "'__call__' requires a 'ref' object but got 'str'"
            or str(context.exception)
            == "descriptor '__call__' requires a 'weakref' object but received a 'str'"
        )

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

    # TODO(T43270097): Add a test for callback.


if __name__ == "__main__":
    unittest.main()
