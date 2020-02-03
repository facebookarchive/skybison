#!/usr/bin/env python3
"""Weak-reference support module."""

from builtins import _index

from _builtins import (
    _patch,
    _property,
    _unimplemented,
    _weakref_callback,
    _weakref_check,
    _weakref_guard,
    _weakref_referent,
)


__all__ = ["CallableProxyType", "ProxyType", "ReferenceType", "ref", "proxy"]


# TODO(djang): Define these in native code.
def _proxy_check(object):
    return isinstance(object, (weakproxy, weakcallableproxy))


def _proxy_referent(proxy_obj):
    referent = object.__getattribute__(proxy_obj, "ref_obj")()
    if referent is None:
        raise ReferenceError("weakly-referenced object no longer exists")
    return referent


def _proxy_unwrap(object):
    return (
        _proxy_referent(object)
        if isinstance(object, (weakproxy, weakcallableproxy))
        else object
    )


def _remove_dead_weakref(object):
    _unimplemented()


@_patch
def _weakref_hash(self):
    pass


@_patch
def _weakref_set_hash(self, hash):
    pass


def getweakrefs(object):
    _unimplemented()


def getweakrefcount(object):
    _unimplemented()


def proxy(object, callback=None):
    reference = ref(object, callback)
    if callable(object):
        return weakcallableproxy(reference)
    return weakproxy(reference)


class ref(bootstrap=True):
    def __call__(self):
        pass

    __callback__ = _property(_weakref_callback)

    def __eq__(self, other):
        _weakref_guard(self)
        if not _weakref_check(other):
            return NotImplemented
        self_referent = _weakref_referent(self)
        other_referent = _weakref_referent(other)
        if self_referent is None or other_referent is None:
            return self is other
        return self_referent == other_referent

    def __ge__(self, other):
        _weakref_guard(self)
        return NotImplemented

    def __gt__(self, other):
        _weakref_guard(self)
        return NotImplemented

    def __le__(self, other):
        _weakref_guard(self)
        return NotImplemented

    def __lt__(self, other):
        _weakref_guard(self)
        return NotImplemented

    def __ne__(self, other):
        _weakref_guard(self)
        if not _weakref_check(other):
            return NotImplemented
        self_referent = _weakref_referent(self)
        other_referent = _weakref_referent(other)
        if self_referent is None or other_referent is None:
            return self is not other
        return self_referent != other_referent

    def __new__(cls, referent, callback=None):
        pass

    def __init__(self, referent, callback=None):
        return None

    def __hash__(self):
        result = _weakref_hash(self)
        if result is not None:
            return result
        obj = _weakref_referent(self)
        if obj is None:
            raise TypeError("weak object has gone away")
        result = hash(obj)
        _weakref_set_hash(self, result)
        return result


class weakcallableproxy:
    __hash__ = None

    def __init__(self, ref_obj):
        object.__setattr__(self, "ref_obj", ref_obj)

    def __abs__(self):
        return abs(_proxy_unwrap(self))

    def __add__(self, other):
        return _proxy_unwrap(self) + _proxy_unwrap(other)

    def __and__(self, other):
        return _proxy_unwrap(self) & _proxy_unwrap(other)

    def __bytes__(self):
        return bytes(_proxy_unwrap(self))

    @property
    def __call__(self):
        return _proxy_unwrap(self)

    def __divmod__(self, other):
        return divmod(_proxy_unwrap(self), _proxy_unwrap(other))

    def __float__(self):
        return float(_proxy_unwrap(self))

    def __getattr__(self, other):
        return getattr(_proxy_unwrap(self), _proxy_unwrap(other))

    def __getitem__(self, other):
        return _proxy_unwrap(self)[_proxy_unwrap(other)]

    def __iadd__(self, other):
        referent = _proxy_unwrap(self)
        referent += _proxy_unwrap(other)

    def __iand__(self, other):
        referent = _proxy_unwrap(self)
        referent &= _proxy_unwrap(other)

    def __ifloor_div__(self, other):
        referent = _proxy_unwrap(self)
        referent /= _proxy_unwrap(other)

    def __ilshift__(self, other):
        referent = _proxy_unwrap(self)
        referent <<= _proxy_unwrap(other)

    def __imod__(self, other):
        referent = _proxy_unwrap(self)
        referent %= _proxy_unwrap(other)

    def __imul__(self, other):
        referent = _proxy_unwrap(self)
        referent *= _proxy_unwrap(other)

    def __index__(self):
        return _index(_proxy_unwrap(self))

    def __int__(self):
        return int(_proxy_unwrap(self))

    def __invert__(self):
        return ~_proxy_unwrap(self)

    def __ior__(self, other):
        referent = _proxy_unwrap(self)
        referent |= _proxy_unwrap(other)

    def __ipow__(self, other):
        referent = _proxy_unwrap(self)
        referent **= _proxy_unwrap(other)

    def __irshift__(self, other):
        referent = _proxy_unwrap(self)
        referent >>= _proxy_unwrap(other)

    def __isub__(self, other):
        referent = _proxy_unwrap(self)
        referent -= _proxy_unwrap(other)

    def __itrue_div__(self, other):
        referent = _proxy_unwrap(self)
        referent /= _proxy_unwrap(other)

    def __ixor__(self, other):
        referent = _proxy_unwrap(self)
        referent ^= _proxy_unwrap(other)

    def __lshift__(self, other):
        return _proxy_unwrap(self) << _proxy_unwrap(other)

    def __mod__(self, other):
        return _proxy_unwrap(self) % _proxy_unwrap(other)

    def __mul__(self, other):
        return _proxy_unwrap(self) * _proxy_unwrap(other)

    def __neg__(self):
        return -_proxy_unwrap(self)

    def __or__(self, other):
        return _proxy_unwrap(self) | _proxy_unwrap(other)

    def __pos__(self):
        return +_proxy_unwrap(self)

    def __pow__(self, other, modulo=None):
        return pow(
            _proxy_unwrap(self),
            _proxy_unwrap(other),
            None if modulo is None else _proxy_unwrap(modulo),
        )

    def __rshift__(self, other):
        return _proxy_unwrap(self) >> _proxy_unwrap(other)

    def __setattr__(self, name, value):
        if not _proxy_check(self):
            raise TypeError(
                "descriptor '__setattr__' requires a 'weakproxy' object but received a "
                "{type(self).__name__}"
            )
        return setattr(_proxy_unwrap(self), name, value)

    def __str__(self):
        return str(_proxy_unwrap(self))

    def __sub__(self, other):
        return _proxy_unwrap(self) - _proxy_unwrap(other)

    def __truediv__(self, other):
        return _proxy_unwrap(self) / _proxy_unwrap(other)

    def __xor__(self, other):
        return _proxy_unwrap(self) ^ _proxy_unwrap(other)


class weakproxy:
    __hash__ = None

    def __init__(self, ref_obj):
        object.__setattr__(self, "ref_obj", ref_obj)

    def __abs__(self):
        return abs(_proxy_unwrap(self))

    def __add__(self, other):
        return _proxy_unwrap(self) + _proxy_unwrap(other)

    def __and__(self, other):
        return _proxy_unwrap(self) & _proxy_unwrap(other)

    def __bytes__(self):
        return bytes(_proxy_unwrap(self))

    def __divmod__(self, other):
        return divmod(_proxy_unwrap(self), _proxy_unwrap(other))

    def __float__(self):
        return float(_proxy_unwrap(self))

    def __getattr__(self, other):
        return getattr(_proxy_unwrap(self), _proxy_unwrap(other))

    def __getitem__(self, other):
        return _proxy_unwrap(self)[_proxy_unwrap(other)]

    def __iadd__(self, other):
        referent = _proxy_unwrap(self)
        referent += _proxy_unwrap(other)

    def __iand__(self, other):
        referent = _proxy_unwrap(self)
        referent &= _proxy_unwrap(other)

    def __ifloor_div__(self, other):
        referent = _proxy_unwrap(self)
        referent /= _proxy_unwrap(other)

    def __ilshift__(self, other):
        referent = _proxy_unwrap(self)
        referent <<= _proxy_unwrap(other)

    def __imod__(self, other):
        referent = _proxy_unwrap(self)
        referent %= _proxy_unwrap(other)

    def __imul__(self, other):
        referent = _proxy_unwrap(self)
        referent *= _proxy_unwrap(other)

    def __index__(self):
        return _index(_proxy_unwrap(self))

    def __int__(self):
        return int(_proxy_unwrap(self))

    def __invert__(self):
        return ~_proxy_unwrap(self)

    def __ior__(self, other):
        referent = _proxy_unwrap(self)
        referent |= _proxy_unwrap(other)

    def __ipow__(self, other):
        referent = _proxy_unwrap(self)
        referent **= _proxy_unwrap(other)

    def __irshift__(self, other):
        referent = _proxy_unwrap(self)
        referent >>= _proxy_unwrap(other)

    def __isub__(self, other):
        referent = _proxy_unwrap(self)
        referent -= _proxy_unwrap(other)

    def __itrue_div__(self, other):
        referent = _proxy_unwrap(self)
        referent /= _proxy_unwrap(other)

    def __ixor__(self, other):
        referent = _proxy_unwrap(self)
        referent ^= _proxy_unwrap(other)

    def __lshift__(self, other):
        return _proxy_unwrap(self) << _proxy_unwrap(other)

    def __mod__(self, other):
        return _proxy_unwrap(self) % _proxy_unwrap(other)

    def __mul__(self, other):
        return _proxy_unwrap(self) * _proxy_unwrap(other)

    def __neg__(self):
        return -_proxy_unwrap(self)

    def __or__(self, other):
        return _proxy_unwrap(self) | _proxy_unwrap(other)

    def __pos__(self):
        return +_proxy_unwrap(self)

    def __pow__(self, other, modulo=None):
        return pow(
            _proxy_unwrap(self),
            _proxy_unwrap(other),
            None if modulo is None else _proxy_unwrap(modulo),
        )

    def __rshift__(self, other):
        return _proxy_unwrap(self) >> _proxy_unwrap(other)

    def __setattr__(self, name, value):
        if not _proxy_check(self):
            raise TypeError(
                "descriptor '__setattr__' requires a 'weakproxy' object but received a "
                "{type(self).__name__}"
            )
        return setattr(_proxy_unwrap(self), name, value)

    def __str__(self):
        return str(_proxy_unwrap(self))

    def __sub__(self, other):
        return _proxy_unwrap(self) - _proxy_unwrap(other)

    def __truediv__(self, other):
        return _proxy_unwrap(self) / _proxy_unwrap(other)

    def __xor__(self, other):
        return _proxy_unwrap(self) ^ _proxy_unwrap(other)


CallableProxyType = weakcallableproxy
ProxyType = weakproxy
ReferenceType = ref
