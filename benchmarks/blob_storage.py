#!/usr/bin/env python3
import os

from everstore.client.py import EverstoreClient


EVERSTORE_TIER = "dfsrouter.common"
EVERSTORE_PYRO_EXT = "txt"
EVERSTORE_PYRO_CONTEXT = "pyro/binaries"
EVERSTORE_PYRO_BINARIES = 27299  # FbType for EVERSTORE_PYRO_BINARIES


class PyroBlobStorage:
    def __init__(self, client):
        self.client = client
        self.context_id = EVERSTORE_PYRO_CONTEXT
        self.fbtype = EVERSTORE_PYRO_BINARIES
        self.extension = EVERSTORE_PYRO_EXT

    @staticmethod
    def create_everstore_client():
        return EverstoreClient(tier_name=EVERSTORE_TIER, use_srproxy=False)

    async def store_blob(self, file_path):
        with open(file_path, "rb") as f:
            content = f.read()
        return await self.client.async_write(
            context_id=self.context_id,
            content=content,
            fbtype=self.fbtype,
            extension=self.extension,
        )

    async def download_blob(self, blob_handle, file_path):
        try:
            content = await self.client.async_read(
                context_id=self.context_id, handle=blob_handle
            )
        except RuntimeError:
            print("Invalid everstore handle: ", blob_handle)
            return False

        if not os.path.exists(os.path.dirname(file_path)):
            os.makedirs(os.path.dirname(file_path))

        with open(file_path, "wb") as f:
            f.write(content)
        os.chmod(file_path, 0o775)  # Make executable
        return True
