#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates. (http://www.facebook.com)
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

    def test_valid_signals(self):
        valid_signals = signal.valid_signals()
        self.assertIsInstance(valid_signals, set)
        self.assertIn(signal.SIGINT, valid_signals)
        self.assertNotIn(1001, valid_signals)

    def test_siginterrupt_with_string_signal_raises_type_error(self):
        with self.assertRaises(TypeError):
            signal.siginterrupt("invalid", True)

    def test_siginterrupt_with_string_flag_raises_type_error(self):
        with self.assertRaises(TypeError):
            signal.siginterrupt(signal.SIGCHLD, "invalid")

    def test_siginterrupt_with_too_large_signal_raises_value_error(self):
        with self.assertRaises(ValueError):
            signal.siginterrupt(123456, True)

    def test_siginterrupt_with_negative_signal_raises_value_error(self):
        with self.assertRaises(ValueError):
            signal.siginterrupt(-1, True)

    def test_siginterrupt_with_valid_signal_sets_interrupt(self):
        self.assertEqual(signal.siginterrupt(signal.SIGCHLD, True), None)

    def test_siginterrupt_with_integer_flag_sets_interrupt(self):
        self.assertEqual(signal.siginterrupt(signal.SIGCHLD, 0), None)


if __name__ == "__main__":
    unittest.main()
