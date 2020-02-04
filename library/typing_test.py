#!/usr/bin/env python3
import unittest
from typing import Dict, List, Optional, Union


class TypingTests(unittest.TestCase):
    def test_typing_parameterized_generics(self):
        def fn(val: Optional[Union[Dict, List]]):
            return 5

        self.assertEqual(fn(None), 5)


if __name__ == "__main__":
    unittest.main()
