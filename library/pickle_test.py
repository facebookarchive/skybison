#!/usr/bin/env python3

import pickle
import unittest


class PickleTest(unittest.TestCase):
    def test_range_can_pickle_and_unpickle(self):
        r = range(2, 10, 3)
        self.assertEqual(r, pickle.loads(pickle.dumps(r, 0)))
        r = range(-(2 ** 65), 2 ** 65, 1)
        self.assertEqual(r, pickle.loads(pickle.dumps(r, 0)))

    def test_instance_proxy_can_pickle_and_unpickle(self):
        class C:
            def __init__(self, *args, **kwargs):
                for key, val in kwargs.items():
                    self.__setattr__(key, val)

        d = {"a": 1, "b": 2, "c": 3}
        c = C(**d)

        self.assertEqual(d, pickle.loads(pickle.dumps(c.__dict__)))


if __name__ == "__main__":
    unittest.main()
