#!/usr/bin/env python3
from libfb.py.asyncio.unittest import async_test
from libfb.py.testutil import BaseFacebookTestCase
from pyro.benchmarks.server_handler import PyroBenchmarksServer


class MockBlobStorage:
    def __init__(self, blob_handle):
        self.blob_handle = blob_handle

    async def store_blob(self, file_path):
        return self.blob_handle

    async def download_blob(self, blob_handle, _file_path):
        return self.blob_handle == blob_handle


class PyroBenchmarksTestCase(BaseFacebookTestCase):
    def setUp(self):
        self.mock_blob_handle = "foo_bar_baz"
        storage_client = MockBlobStorage(self.mock_blob_handle)
        self.controller = PyroBenchmarksServer(storage_client)
        BaseFacebookTestCase.setUp(self)

    @async_test
    async def test_run_completes(self):
        result = await self.controller.run(self.mock_blob_handle)
        self.assertEqual(result, "success")

    @async_test
    async def test_run_fails(self):
        result = await self.controller.run("bad blob")
        self.assertEqual(result, "failure")
