#!/usr/bin/env python3

import faulthandler
import tempfile
import unittest
from unittest.mock import Mock


class FaulthandlerTest(unittest.TestCase):
    def test_dump_traceback_with_non_int_calls_fileno_and_flush(self):
        with tempfile.TemporaryFile() as file:

            class C:
                fileno = Mock(return_value=file.fileno())
                flush = Mock()

            C.fileno.assert_not_called()
            C.flush.assert_not_called()
            faulthandler.dump_traceback(C(), False)
            C.fileno.assert_called_once()
            C.flush.assert_called_once()


if __name__ == "__main__":
    unittest.main()
