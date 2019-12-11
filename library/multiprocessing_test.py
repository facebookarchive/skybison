#!/usr/bin/env python3
import unittest


try:
    import multiprocessing
except ImportError:
    multiprocessing = None

try:
    import multiprocessing.connection as m_conn
except ImportError:
    m_conn = None


class MultiprocessingImportTests(unittest.TestCase):
    def test_succesful_import_of_multiprocessing(self):
        """Baseline test to import multiprocessing"""
        self.assertIsNotNone(multiprocessing)

    def test_succesful_import_of_multiprocessing_connection(self):
        """Importing multiprocessing.connection relies on _multiprocessing"""
        self.assertIsNotNone(m_conn)


if __name__ == "__main__":
    unittest.main()
