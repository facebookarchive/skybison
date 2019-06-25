#!/usr/bin/env python3
from libfb.py.asyncio.unittest import async_test
from libfb.py.testutil import BaseFacebookTestCase
from pyro.benchmarks.server_handler import PyroBenchmarksServer


class PyroBenchmarksTestCase(BaseFacebookTestCase):
    def setUp(self):
        self.controller = PyroBenchmarksServer()
        BaseFacebookTestCase.setUp(self)

    @async_test
    async def test_hello(self):
        result = await self.controller.hello()
        self.assertEqual(result, "world")
