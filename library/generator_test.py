#!/usr/bin/env python3
import unittest


class YieldFromTest(unittest.TestCase):
    def test_managed_stop_iteration(self):
        def inner_gen():
            yield None
            raise StopIteration("hello")

        def outer_gen():
            val = yield from inner_gen()
            yield val

        g = outer_gen()
        self.assertIs(next(g), None)
        self.assertEqual(next(g), "hello")


class AttributesTest(unittest.TestCase):
    def test_gi_running(self):
        def gen():
            self.assertTrue(g.gi_running)
            yield 1
            self.assertTrue(g.gi_running)
            yield 2

        g = gen()
        self.assertFalse(g.gi_running)
        next(g)
        self.assertFalse(g.gi_running)
        next(g)
        self.assertFalse(g.gi_running)

    def test_gi_running_readonly(self):
        def gen():
            yield None

        g = gen()
        self.assertRaises(AttributeError, setattr, g, "gi_running", 1234)

    def test_running_gen_raises(self):
        def gen():
            self.assertRaises(ValueError, next, g)
            yield "done"

        g = gen()
        self.assertEqual(next(g), "done")


class MyError(Exception):
    pass


class ThrowTest(unittest.TestCase):
    @staticmethod
    def simple_gen():
        yield 1
        yield 2

    @staticmethod
    def catching_gen():
        try:
            yield 1
        except MyError:
            yield "caught"

    @staticmethod
    def catching_returning_gen():
        try:
            yield 1
        except MyError:
            return "all done!"  # noqa

    @staticmethod
    def delegate_gen(g):
        r = yield from g
        yield r

    def test_throw(self):
        g = self.simple_gen()
        self.assertRaises(MyError, g.throw, MyError())

    def test_throw_caught(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(MyError()), "caught")

    def test_throw_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(MyError), "caught")

    def test_throw_type_and_value(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(MyError, MyError()), "caught")

    def test_throw_uncaught_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(RuntimeError, g.throw, RuntimeError)

    def test_throw_finished(self):
        g = self.catching_returning_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(StopIteration, g.throw, MyError)

    def test_throw_two_values(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(TypeError, g.throw, MyError(), MyError())

    def test_throw_bad_traceback(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(TypeError, g.throw, MyError, MyError(), 5)

    def test_throw_bad_type(self):
        g = self.catching_gen()
        self.assertEqual(next(g), 1)
        self.assertRaises(TypeError, g.throw, 1234)

    def test_throw_not_started(self):
        g = self.simple_gen()
        self.assertRaises(MyError, g.throw, MyError())
        self.assertRaises(StopIteration, next, g)

    def test_throw_stopped(self):
        g = self.simple_gen()
        self.assertEqual(next(g), 1)
        self.assertEqual(next(g), 2)
        self.assertRaises(StopIteration, next, g)
        self.assertRaises(MyError, g.throw, MyError())

    def test_throw_yield_from(self):
        g = self.delegate_gen(self.simple_gen())
        self.assertEqual(next(g), 1)
        self.assertRaises(MyError, g.throw, MyError)

    def test_throw_yield_from_caught(self):
        g = self.delegate_gen(self.catching_gen())
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(MyError), "caught")

    def test_throw_yield_from_finishes(self):
        g = self.delegate_gen(self.catching_returning_gen())
        self.assertEqual(next(g), 1)
        self.assertEqual(g.throw(MyError), "all done!")

    def test_throw_yield_from_non_gen(self):
        g = self.delegate_gen([1, 2, 3, 4])
        self.assertEqual(next(g), 1)
        self.assertRaises(RuntimeError, g.throw, RuntimeError)


class MethodTest(unittest.TestCase):
    def test_dunder_repr(self):
        def foo():
            yield 5

        self.assertTrue(
            foo()
            .__repr__()
            .startswith(
                "<generator object MethodTest.test_dunder_repr.<locals>.foo at "
            )
        )


if __name__ == "__main__":
    unittest.main()
