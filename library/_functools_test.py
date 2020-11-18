#!/usr/bin/env python3
import _functools
import unittest
from collections import namedtuple

_CacheInfo = namedtuple("CacheInfo", ["hits", "misses", "maxsize", "currsize"])


class FunctoolsLRUCacheWrapperTests(unittest.TestCase):
    def test_uncached_lru_wrapper(self):
        wrapper = _functools._lru_cache_wrapper(pow, 0, False, _CacheInfo)

        self.assertEqual(wrapper(2, 3), 8)
        self.assertEqual(wrapper(2, 4), 16)
        self.assertEqual(wrapper(2, 3), 8)
        self.assertEqual(wrapper(2, 4), 16)

        info = wrapper.cache_info()
        self.assertEqual(info.hits, 0)
        self.assertEqual(info.misses, 4)
        self.assertEqual(info.currsize, 0)

    def test_infinite_lru_wrapper(self):
        wrapper = _functools._lru_cache_wrapper(pow, None, False, _CacheInfo)

        self.assertEqual(wrapper(2, 3), 8)
        self.assertEqual(wrapper(2, 4), 16)
        self.assertEqual(wrapper(2, 3), 8)
        self.assertEqual(wrapper(2, 4), 16)

        info = wrapper.cache_info()
        self.assertEqual(info.hits, 2)
        self.assertEqual(info.misses, 2)
        self.assertEqual(info.currsize, 2)

    def test_bounded_lru_wrapper(self):
        wrapper = _functools._lru_cache_wrapper(pow, 2, False, _CacheInfo)

        self.assertEqual(wrapper(2, 3), 8)
        self.assertEqual(wrapper(2, 4), 16)
        self.assertEqual(wrapper(2, 3), 8)
        self.assertEqual(wrapper(2, 4), 16)
        self.assertEqual(wrapper(2, 5), 32)

        info = wrapper.cache_info()
        self.assertEqual(info.hits, 2)
        self.assertEqual(info.misses, 3)
        self.assertEqual(info.currsize, 2)


if __name__ == "__main__":
    unittest.main()
