#!/usr/bin/env python3

# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_patch = _patch  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


codec_search_path = []


codec_search_cache = {}


def register(search_func):
    if not callable(search_func):
        raise TypeError("argument must be callable")
    codec_search_path.append(search_func)


def lookup(encoding):
    encoding = encoding.lower().replace(" ", "-")
    if encoding in codec_search_cache:
        return codec_search_cache[encoding]
    result = None
    for search_func in codec_search_path:
        result = search_func(encoding)
        if result is None:
            continue
        if not isinstance(result, tuple) or not len(result) == 4:
            raise TypeError("codec search functions must return 4-tuples")
        break
    if result is None:
        raise LookupError(f"unknown encoding: {encoding}")

    codec_search_cache[encoding] = result
    return result


def _lookup_text(encoding, alternate_command):
    codec = lookup(encoding)
    if type(codec) != tuple:
        try:
            if not codec._is_text_encoding:
                raise LookupError(
                    f"{encoding} is not a text encoding; "
                    f"use {alternate_command} to handle arbitrary codecs"
                )
        except AttributeError:
            pass
    return codec


def decode(data, encoding: str = "utf-8", errors: str = "strict") -> str:
    try:
        return _codec_decode_table[encoding.lower()](data, errors)[0]
    except KeyError:
        # TODO(T39917465): Call the encoding search function
        _unimplemented()


def encode(data, encoding: str = "utf-8", errors: str = "strict") -> bytes:
    try:
        return _codec_encode_table[encoding.lower()](data, errors)[0]
    except KeyError:
        # TODO(T39917465): Call the encodings search function
        _unimplemented()


@_patch
def _ascii_decode(data: str, errors: str, index: int, out: bytearray):
    pass


def ascii_decode(data: bytes, errors: str = "strict"):
    if not isinstance(data, bytes):
        raise TypeError(f"a bytes-like object is required, not '{type(data).__name__}'")
    if not isinstance(errors, str):
        raise TypeError(
            "ascii_decode() argument 2 must be str or None, not "
            f"'{type(errors).__name__}'"
        )
    result = bytearray()
    i = 0
    encoded = ""
    while i < len(data):
        encoded, i = _ascii_decode(data, errors, i, result)
        if isinstance(encoded, int):
            data, pos = _call_decode_errorhandler(
                errors, data, result, "ordinal not in range(128)", "ascii", encoded, i
            )
            i += pos
    if isinstance(encoded, str):
        return encoded, i
    # The error handler was the last to write to the result
    return _bytearray_to_string(result), i


_codec_decode_table = {"ascii": ascii_decode, "us_ascii": ascii_decode}

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
    return ("", 0)


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


def _call_decode_errorhandler(
    errors: str,
    input: bytes,
    output: bytearray,
    reason: str,
    encoding: str,
    start: int,
    end: int,
):
    """
    Generic decoding errorhandling function
    Creates a UnicodeDecodeError, looks up an error handler, and calls the
    error handler with the UnicodeDecodeError.
    Makes sure the error handler returns a (str, int) tuple and returns it and
    writes the str to the output bytearray passed in.
    Since the error handler can change the object that's being decoded by
    replacing the object of the UnicodeDecodeError, this function returns the
    Error's object field, along with the integer returned from the function
    call that's been normalized to fit within the length of the object.

    errors: The name of the error handling function to call
    input: The input to be decoded
    output: The string builder that the error handling result should be appended to
    reason: The reason the errorhandler was called
    encoding: The encoding being used
    start: The index of the first non-erroneus byte
    end: The index of the first non-erroneous byte
    """
    exception = UnicodeDecodeError(encoding, input, start, end, reason)
    result = lookup_error(errors)(exception)
    if (
        not isinstance(result, tuple)
        or len(result) != 2
        or not isinstance(result[0], str)
        or not hasattr(result[1], "__index__")
    ):
        raise TypeError("decoding error handler must return (str, int) tuple")
    replacement = result[0]
    pos = _index(result[1])
    input = exception.object
    if not isinstance(input, bytes):
        raise TypeError("exception attribute object must be bytes")
    if pos < 0:
        pos += len(input)
    if not 0 <= pos <= len(input):
        raise IndexError(f"position {pos} from error handler out of bounds")
    _bytearray_string_append(output, replacement)

    return (input, pos)


def _index(num):
    if not isinstance(num, int):
        try:
            # TODO(T41077650): Truncate the result of __index__ to Py_ssize_t
            return num.__index__()
        except AttributeError:
            raise TypeError(
                f"'{type(num).__name__}' object cannot be interpreted as" " an integer"
            )
    return num


_codec_error_registry = {"strict": strict_errors, "ignore": ignore_errors}


@_patch
def _bytearray_string_append(dst: bytearray, data: str):
    pass


# TODO(T42244617): Replace with a _strarray type
@_patch
def _bytearray_to_string(src: bytearray) -> str:
    pass
