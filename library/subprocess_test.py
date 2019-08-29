#!/usr/bin/env python3
import unittest


class SubprocessTest(unittest.TestCase):
    def test_import(self):
        import subprocess

        self.assertEqual(subprocess.__name__, "subprocess")


if __name__ == "__main__":
    unittest.main()
