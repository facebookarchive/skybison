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
            os._exit(0)
        else:
            # Parent
            # Ensure that the pid exists.
            self.assertGreaterEqual(pid, 1)
            os.kill(pid, 0)
            os.waitpid(pid, 0)
            # Ensure process is reaped.
            with self.assertRaises(OSError):
                os.kill(pid, 0)

    def test_register_before_fork(self):
        self.x = 0

        def f():
            self.x = 1

        os.register_at_fork(before=f)
        self.assertEqual(self.x, 0)
        pid = os.fork()
        if pid == 0:
            self.assertEqual(self.x, 1)
            os._exit(0)
        else:
            self.assertEqual(self.x, 1)

    def test_register_after_fork_parent(self):
        self.x = 0

        def f():
            self.x = 1

        os.register_at_fork(after_in_parent=f)
        self.assertEqual(self.x, 0)
        pid = os.fork()
        if pid == 0:
            self.assertEqual(self.x, 0)
            os._exit(0)
        else:
            self.assertEqual(self.x, 1)

    def test_register_after_fork_child(self):
        self.x = 0

        def f():
            self.x = 1

        os.register_at_fork(after_in_child=f)
        self.assertEqual(self.x, 0)
        pid = os.fork()
        if pid == 0:
            os._exit(0)
        else:
            self.assertEqual(self.x, 0)


if __name__ == "__main__":
    unittest.main()
