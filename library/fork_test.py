#!/usr/bin/env python3

import os
import time
import unittest


class ForkTest(unittest.TestCase):
    def test_basic_fork(self):
        pid = os.fork()
        if pid == 0:
            # Child
            time.sleep(1)
            return
        else:
            # Parent
            # Ensure that the pid exists.
            self.assertGreaterEqual(pid, 1)
            os.kill(pid, 0)
            os.waitpid(pid, 0)
            # Ensure process is reaped.
            with self.assertRaises(OSError):
                os.kill(pid, 0)


if __name__ == "__main__":
    unittest.main()
