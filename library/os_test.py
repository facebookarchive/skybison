#!/usr/bin/env python3
import os
import subprocess
import unittest


class OsTest(unittest.TestCase):
    def test_modifying_os_environ_affects_spawn(self):
        var_name = "TEST_MODIFYING_OS_ENVIRON_AFFECTS_SPAWN"
        self.assertNotIn(var_name, os.environ)
        try:
            os.environ[var_name] = "foo42"
            p = subprocess.run(
                ["/usr/bin/printenv", var_name], check=True, capture_output=True
            )
        finally:
            del os.environ[var_name]
        self.assertEqual(p.stdout.strip(), b"foo42")


if __name__ == "__main__":
    unittest.main()
