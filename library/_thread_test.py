#!/usr/bin/env python3
import _io
import _thread
import contextlib
import time
import unittest


_thread._enable_threads = True


class UnderThreadTest(unittest.TestCase):
    def test_start_new_thread_returns_new_thread(self):
        def bootstrap():
            pass

        current_id = _thread.get_ident()
        new_id = _thread.start_new_thread(bootstrap, ())
        time.sleep(1.0)  # TODO(T66337218): remove when we can join on threads

        self.assertIsInstance(new_id, int)
        self.assertNotEqual(new_id, current_id)

    def test_start_new_thread_with_exception_prints_error(self):
        def bootstrap():
            raise RuntimeError

        with _io.StringIO() as stderr, contextlib.redirect_stderr(stderr):
            _thread.start_new_thread(bootstrap, ())
            time.sleep(1.0)  # TODO(T66337218): remove when we can join on threads

            self.maxDiff = 10000
            self.assertRegex(
                stderr.getvalue(),
                r"""Unhandled exception in thread started by <function .*bootstrap at 0x[0-9a-f]+>
Traceback \(most recent call last\):
  File ".*/_thread_test.py", line \d+, in bootstrap
    raise RuntimeError
RuntimeError
""",
            )


if __name__ == "__main__":
    unittest.main()
