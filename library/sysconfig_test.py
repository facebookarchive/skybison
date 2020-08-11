#!/usr/bin/env python3
import sysconfig
import unittest


class SysconfigTest(unittest.TestCase):
    def test_variables_present(self):
        self.assertIsNot(sysconfig.get_config_var("BINLIBDEST"), None)
        self.assertIsNot(sysconfig.get_config_var("INCLUDEPY"), None)
        self.assertIsNot(sysconfig.get_config_var("LIBDEST"), None)


if __name__ == "__main__":
    unittest.main()
