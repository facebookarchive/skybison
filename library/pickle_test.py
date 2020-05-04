#!/usr/bin/env python3

import pickle
import unittest


class PickleTest(unittest.TestCase):
    def test_range_can_pickle_and_unpickle(self):
        r = range(2, 10, 3)
        self.assertEqual(r, pickle.loads(pickle.dumps(r, 0)))
        r = range(-(2 ** 65), 2 ** 65, 1)
        self.assertEqual(r, pickle.loads(pickle.dumps(r, 0)))


if __name__ == "__main__":
    unittest.main()
