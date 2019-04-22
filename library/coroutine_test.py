#!/usr/bin/env python3
import unittest


class AttributesTest(unittest.TestCase):
    def test_cr_running(self):
        async def coro():
            return 1

        # TODO(bsimmers): Test more once we have coroutine.__await__()
        cr = coro()
        self.assertFalse(cr.cr_running)


if __name__ == "__main__":
    unittest.main()
