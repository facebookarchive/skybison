from _thread import RLock
from types import MethodType

################################################################################
### LRU Cache function decorator
################################################################################


class _HashedSeq(list):
    """This class guarantees that hash() will be called no more than once
    per element.  This is important because the lru_cache() will hash
    the key multiple times on a cache miss.

    """

    __slots__ = "hashvalue"

    def __init__(self, tup, hash=hash):
        self[:] = tup
        self.hashvalue = hash(tup)

    def __hash__(self):
        return self.hashvalue


kwd_mark = (object(),)
fasttypes = {int, str}


def _make_key(args, kwds, typed):
    """Make a cache key from optionally typed positional and keyword arguments

    The key is constructed in a way that is flat as possible rather than
    as a nested structure that would take more memory.

    If there is only a single argument and its data type is known to cache
    its hash value, then that argument is returned without a wrapper.  This
    saves space and improves lookup speed.

    """
    # All of code below relies on kwds preserving the order input by the user.
    # Formerly, we sorted() the kwds before looping.  The new way is *much*
    # faster; however, it means that f(x=1, y=2) will now be treated as a
    # distinct call from f(y=2, x=1) which will be cached separately.
    key = args
    if kwds:
        key += kwd_mark
        for item in kwds.items():
            key += item
    if typed:
        key += tuple(type(v) for v in args)
        if kwds:
            key += tuple(type(v) for v in kwds.values())
    elif len(key) == 1 and type(key[0]) in fasttypes:
        return key[0]
    return _HashedSeq(key)


_sentinel = object()


class _lru_cache_wrapper_base:
    def __init__(self, user_function, typed, _CacheInfo):
        self._user_function = user_function
        self._typed = typed
        self._CacheInfo = _CacheInfo
        self._cache = {}
        self._cache_get = self._cache.get  # bound method to lookup a key or return None
        self._hits = 0
        self._misses = 0

    def cache_info(self, maxsize, cache_len):
        """Report cache statistics"""
        return self._CacheInfo(self._hits, self._misses, maxsize, cache_len)

    def __get__(self, instance, owner):
        if instance is None:
            return self
        return MethodType(self, instance)


class _uncached_lru_cache_wrapper(_lru_cache_wrapper_base):
    def __init__(self, user_function, typed, _CacheInfo):
        super().__init__(user_function, typed, _CacheInfo)

    def cache_info(self):
        return super().cache_info(0, 0)

    def cache_clear(self):
        """Clear the cache and cache statistics"""
        self._misses = 0

    def __call__(self, *args, **kwds):
        # No caching -- just a statistics update
        self._misses += 1
        result = self._user_function(*args, **kwds)
        return result


class _infinite_lru_cache_wrapper(_lru_cache_wrapper_base):
    def __init__(self, user_function, typed, _CacheInfo):
        super().__init__(user_function, typed, _CacheInfo)
        self._cache_len = self._cache.__len__  # get cache size without calling len()

    def cache_info(self):
        return super().cache_info(None, self._cache_len())

    def cache_clear(self):
        """Clear the cache and cache statistics"""
        self._cache.clear()
        self._hits = 0
        self._misses = 0

    def __call__(self, *args, **kwds):
        # Simple caching without ordering or size limit
        key = _make_key(args, kwds, self._typed)
        result = self._cache_get(key, _sentinel)
        if result is not _sentinel:
            self._hits += 1
            return result
        self._misses += 1
        result = self._user_function(*args, **kwds)
        self._cache[key] = result
        return result


class _bounded_lru_cache_wrapper(_lru_cache_wrapper_base):
    def __init__(self, user_function, maxsize, typed, _CacheInfo):
        super().__init__(user_function, typed, _CacheInfo)
        self._maxsize = maxsize
        self._full = False
        self._cache_len = self._cache.__len__  # get cache size without calling len()
        self._lock = RLock()  # because linkedlist updates aren't threadsafe
        self._root = []  # root of the circular doubly linked list
        self._root[:] = [
            self._root,
            self._root,
            None,
            None,
        ]  # initialize by pointing to self

    def cache_info(self):
        return super().cache_info(self._maxsize, self._cache_len())

    def cache_clear(self):
        """Clear the cache and cache statistics"""
        with self._lock:
            self._cache.clear()
            self._root[:] = [self._root, self._root, None, None]
            self._hits = self._misses = 0
            self._full = False

    def __call__(self, *args, **kwds):
        # Size limited caching that tracks accesses by recency
        PREV, NEXT, KEY, RESULT = 0, 1, 2, 3
        key = _make_key(args, kwds, self._typed)
        with self._lock:
            link = self._cache_get(key)
            if link is not None:
                # Move the link to the front of the circular queue
                link_prev, link_next, _key, result = link
                link_prev[NEXT] = link_next
                link_next[PREV] = link_prev
                last = self._root[PREV]
                last[NEXT] = self._root[PREV] = link
                link[PREV] = last
                link[NEXT] = self._root
                self._hits += 1
                return result
            self._misses += 1
        result = self._user_function(*args, **kwds)
        with self._lock:
            if key in self._cache:
                # Getting here means that this same key was added to the
                # cache while the lock was released.  Since the link
                # update is already done, we need only return the
                # computed result and update the count of misses.
                pass
            elif self._full:
                # Use the old root to store the new key and result.
                oldroot = self._root
                oldroot[KEY] = key
                oldroot[RESULT] = result
                # Empty the oldest link and make it the new root.
                # Keep a reference to the old key and old result to
                # prevent their ref counts from going to zero during the
                # update. That will prevent potentially arbitrary object
                # clean-up code (i.e. __del__) from running while we're
                # still adjusting the links.
                self._root = oldroot[NEXT]
                oldkey = self._root[KEY]
                self._root[KEY] = self._root[RESULT] = None
                # Now update the cache dictionary.
                del self._cache[oldkey]
                # Save the potentially reentrant cache[key] assignment
                # for last, after the root and links have been put in
                # a consistent state.
                self._cache[key] = oldroot
            else:
                # Put result in a new link at the front of the queue.
                last = self._root[PREV]
                link = [last, self._root, key, result]
                last[NEXT] = self._root[PREV] = self._cache[key] = link
                # Use the cache_len bound method instead of the len() function
                # which could potentially be wrapped in an lru_cache itself.
                self._full = self._cache_len() >= self._maxsize
        return result


def _lru_cache_wrapper(user_function, maxsize, typed, _CacheInfo):
    if maxsize == 0:
        return _uncached_lru_cache_wrapper(user_function, typed, _CacheInfo)
    elif maxsize is None:
        return _infinite_lru_cache_wrapper(user_function, typed, _CacheInfo)
    else:
        return _bounded_lru_cache_wrapper(user_function, maxsize, typed, _CacheInfo)
