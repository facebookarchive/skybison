#!/usr/bin/env python3

# These tests cannot be on contextvars as they need access to Pyro-specific
# internal functions.

import _contextvars
import unittest

from test_support import pyro_only


class CopyContextTests(unittest.TestCase):
    @pyro_only
    def test_produces_context_with_same_data_as_current_context(self):
        empty_ctx = _contextvars.Context()

        def f_with_empty_ctx():
            var1 = _contextvars.ContextVar("foo")
            var1.set(1)
            var2 = _contextvars.ContextVar("bar")
            var2.set(2)
            ctx_copy = _contextvars.copy_context()
            self.assertEqual(
                list(ctx_copy.items()), list(_contextvars._thread_context().items())
            )

        empty_ctx.run(f_with_empty_ctx)

    @pyro_only
    def test_produces_new_context(self):
        empty_ctx = _contextvars.Context()

        def f_with_empty_ctx():
            ctx_copy = _contextvars.copy_context()
            self.assertIsNot(_contextvars._thread_context(), ctx_copy)

        empty_ctx.run(f_with_empty_ctx)


if __name__ == "__main__":
    unittest.main()
