#!/usr/bin/env python3
import asyncio
import unittest

from test_support import pyro_only


class AsyncioTaskTest(unittest.TestCase):
    class SpecialTask(asyncio.Task):
        def __init__(self, *args, **kwargs):
            self.step_counter = 0
            super().__init__(*args, **kwargs)

        def _step(self, exc=None):
            self.step_counter += 1
            super()._step(exc)

    @pyro_only
    def test_step_of_special_task_raises_exception(self):
        async def run():
            return "ok"

        loop = asyncio.new_event_loop()
        t = self.SpecialTask(run(), loop=loop, name="TestTask")
        self.assertEqual(t.step_counter, 0)
        loop.run_until_complete(t)
        self.assertEqual(t.step_counter, 1)


if __name__ == "__main__":
    unittest.main()
