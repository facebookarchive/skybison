#!/usr/bin/env python3
import signal
import time
import unittest


class SignalTest(unittest.TestCase):
    def test_import(self):
        self.assertEqual(signal.__name__, "signal")

    def test_alarm_sends_sigalrm(self):
        global x
        x = 0

        def handler(_signal_number, _stack_frame):
            global x
            x = 1

        signal.signal(signal.SIGALRM, handler)
        # No previous alarm exists.
        self.assertEqual(signal.alarm(1), 0)
        self.assertEqual(x, 0)
        # Sleep 2 seconds for safety against bad scheduling luck.
        time.sleep(2)
        self.assertEqual(x, 1)

    def test_alarm_with_negative_seconds_overflows(self):
        signal.alarm(-1)
        # The correct behavior of the old alarm is to overflow, and is per-platform,
        # so let's just check that no exception gets raised and that the stored data is
        # non-negative.
        self.assertGreaterEqual(signal.alarm(0), 0)

    def test_alarm_with_large_int_raises_overflow_error(self):
        with self.assertRaises(OverflowError):
            signal.alarm(2 ** 65)


if __name__ == "__main__":
    unittest.main()
