#!/usr/bin/env python3
import unittest
import warnings


def raisewarning(msg):
    raise UserWarning(msg)


class WarningsTests(unittest.TestCase):
    def test_warn_calls_showwarnmsg(self):
        orig = warnings._showwarnmsg
        warnings._showwarnmsg = raisewarning
        with self.assertRaises(UserWarning):
            warnings.warn("hello")
        warnings._showwarnmsg = orig

    def test_warn_sets_fields(self):
        orig = warnings._showwarnmsg
        warnings._showwarnmsg = raisewarning
        try:
            warnings.warn("hello")
        except UserWarning as exc:
            msg = exc.args[0]
            self.assertEqual(msg.message.args[0], "hello")
            self.assertEqual(msg.category, UserWarning)
        warnings._showwarnmsg = orig


if __name__ == "__main__":
    unittest.main()
