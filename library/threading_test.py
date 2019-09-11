#!/usr/bin/env python3
import unittest
from threading import Event


class EventTests(unittest.TestCase):
    def test_dunder_init_clears_flag(self):
        e = Event()
        self.assertFalse(e.is_set())

    def test_clear_clears_flag(self):
        e = Event()
        e.set()
        self.assertTrue(e.is_set())
        e.clear()
        self.assertFalse(e.is_set())

    def test_set_sets_flag(self):
        e = Event()
        self.assertFalse(e.is_set())
        e.set()
        self.assertTrue(e.is_set())

    def wait_on_set_flag_returns(self):
        e = Event()
        e.set()
        e.wait()
        e.wait()


if __name__ == "__main__":
    unittest.main()
