#!/usr/bin/env python3
import unittest


class VenvTest(unittest.TestCase):
    def test_it_imports(self):
        import venv

        self.assertEqual(venv.__name__, "venv")


if __name__ == "__main__":
    unittest.main()
