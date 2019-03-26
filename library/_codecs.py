#!/usr/bin/env python3

# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_patch = _patch  # noqa: F821


def decode(data, encoding: str = "utf-8", errors: str = "strict") -> str:
    try:
        return _codec_decode_table[encoding.lower()](data, errors)[0]
    except KeyError:
        # TODO(T39917465): Call the encoding search function
        raise NotImplementedError("Non-fastpass codecs are unimplemented")


def encode(data, encoding: str = "utf-8", errors: str = "strict") -> bytes:
    try:
        return _codec_encode_table[encoding.lower()](data, errors)[0]
    except KeyError:
        # TODO(T39917465): Call the encodings search function
        raise NotImplementedError("Non-fastpass codecs are unimplemented")


_codec_decode_table = {}  # noqa: T484

_codec_encode_table = {}  # noqa: T484


def strict_errors(error):
    if not isinstance(error, Exception):
        raise TypeError("codecs must pass exception instance")
    raise error


def ignore_errors(error):
    if not isinstance(error, UnicodeError):
        raise TypeError(
            f"don't know how to handle {type(error).__name__} in error callback"
        )
    return ("", error.end)


def lookup_error(error: str):
    if not isinstance(error, str):
        raise TypeError(
            f"lookup_error() argument must be str, not {type(error).__name__}"
        )
    try:
        return _codec_error_registry[error]
    except KeyError:
        raise LookupError(f"unknown error handler name '{error}'")


def register_error(name: str, error_func):
    if not isinstance(name, str):
        raise TypeError(
            f"register_error() argument 1 must be str, not {type(name).__name__}"
        )
    if not callable(error_func):
        raise TypeError("handler must be callable")
    _codec_error_registry[name] = error_func


_codec_error_registry = {"strict": strict_errors, "ignore": ignore_errors}
