#!/usr/bin/env python3
import _functools
import functools
import unittest
from collections import namedtuple

_CacheInfo = functools._CacheInfo


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

    def test_lru_cache_in_class_binds_self(self):
        def func(*args, **kwargs):
            return (args, kwargs)

        class C:
            foo = _functools._lru_cache_wrapper(func, 10, False, _CacheInfo)

        c = C()
        result = c.foo(1, "two", bar="baz")
        self.assertEqual(result, ((c, 1, "two"), {"bar": "baz"}))

    def test_lru_cache_unbounded_in_class_binds_self(self):
        def func(*args, **kwargs):
            return (args, kwargs)

        class C:
            foo = _functools._lru_cache_wrapper(func, None, False, _CacheInfo)

        c = C()
        result = c.foo(1, "two", bar="baz")
        self.assertEqual(result, ((c, 1, "two"), {"bar": "baz"}))

    def test_cache_clean_with_uncached_cleans_cache(self):
        wrapper = _functools._lru_cache_wrapper(pow, 0, False, _CacheInfo)

        info = wrapper.cache_info()
        self.assertEqual(info.hits, 0)
        self.assertEqual(info.misses, 0)
        self.assertEqual(info.currsize, 0)
        wrapper(1, 2)
        info = wrapper.cache_info()
        self.assertEqual(info.hits, 0)
        self.assertEqual(info.misses, 1)
        self.assertEqual(info.currsize, 0)
        wrapper.cache_clear()
        info = wrapper.cache_info()
        self.assertEqual(info.hits, 0)
        self.assertEqual(info.misses, 0)
        self.assertEqual(info.currsize, 0)

    def test_cache_clean_with_infinite_cleans_cache(self):
        wrapper = _functools._lru_cache_wrapper(pow, None, False, _CacheInfo)

        info = wrapper.cache_info()
        self.assertEqual(info.hits, 0)
        self.assertEqual(info.misses, 0)
        self.assertEqual(info.currsize, 0)
        wrapper(1, 2)
        wrapper(1, 2)
        info = wrapper.cache_info()
        self.assertEqual(info.hits, 1)
        self.assertEqual(info.misses, 1)
        self.assertEqual(info.currsize, 1)
        wrapper.cache_clear()
        info = wrapper.cache_info()
        self.assertEqual(info.hits, 0)
        self.assertEqual(info.misses, 0)
        self.assertEqual(info.currsize, 0)

    def test_cache_clean_with_bounded_cleans_cache(self):
        wrapper = _functools._lru_cache_wrapper(pow, 2, False, _CacheInfo)

        info = wrapper.cache_info()
        self.assertEqual(info.hits, 0)
        self.assertEqual(info.misses, 0)
        self.assertEqual(info.currsize, 0)
        wrapper(1, 2)
        wrapper(1, 2)
        info = wrapper.cache_info()
        self.assertEqual(info.hits, 1)
        self.assertEqual(info.misses, 1)
        self.assertEqual(info.currsize, 1)
        wrapper.cache_clear()
        info = wrapper.cache_info()
        self.assertEqual(info.hits, 0)
        self.assertEqual(info.misses, 0)
        self.assertEqual(info.currsize, 0)


if __name__ == "__main__":
    unittest.main()
