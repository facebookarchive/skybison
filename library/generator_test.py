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


if __name__ == "__main__":
    unittest.main()
