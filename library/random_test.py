#!/usr/bin/env python3

import random
import unittest


class RandomTest(unittest.TestCase):
    def test_seed_succeeds(self):
        random.seed(0xDEADBEEF)

    def test_seeded_randint(self):
        random.seed(0xBA5EBA11, version=2)
        self.assertEqual(
            [random.randint(0, 1000) for _i in range(10)],
            [519, 88, 712, 325, 296, 529, 436, 708, 381, 454],
        )


if __name__ == "__main__":
    unittest.main()
