#!/usr/bin/env python3
import unittest


class FcntlModuleTest(unittest.TestCase):
    def test_it_imports(self):
        import fcntl

        self.assertEqual(fcntl.__name__, "fcntl")


if __name__ == "__main__":
    unittest.main()
