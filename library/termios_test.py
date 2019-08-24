#!/usr/bin/env python3
import unittest


class TermiosModuleTest(unittest.TestCase):
    def test_it_imports(self):
        import termios

        self.assertEqual(termios.__name__, "termios")


if __name__ == "__main__":
    unittest.main()
