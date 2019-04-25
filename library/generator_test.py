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
