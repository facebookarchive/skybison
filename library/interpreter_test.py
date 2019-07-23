#!/usr/bin/env python3
import unittest


class IntepreterTest(unittest.TestCase):
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

        # TODO(matthiasb): Pyro does not accept normal dicts as globals, so we
        # need this workaround instead of `globals = {"foo": 99}`.
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

        globals = {"bar": 42}
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

        globals = {"bar": 42}
        locals = C()
        with self.assertRaises(NameError) as context:
            exec("del foo", globals, locals)
        self.assertEqual(str(context.exception), "name 'foo' is not defined")
        self.assertEqual(d.result[0], locals)
        self.assertEqual(d.result[1], C)


if __name__ == "__main__":
    unittest.main()
