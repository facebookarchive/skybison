#!/usr/bin/env python3

# These values are injected by our boot process. flake8 has no knowledge about
# their definitions and will complain without this gross circular helper here.
_int_check = _int_check  # noqa: F821
_index = _index  # noqa: F821
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
        if _int_check(encoded):
            data, i = _call_decode_errorhandler(
                errors, data, result, "ordinal not in range(128)", "ascii", encoded, i
            )
    if isinstance(encoded, str):
        return encoded, i
    # The error handler was the last to write to the result
    return _bytearray_to_string(result), i


@_patch
def _ascii_encode(data: str, errors: str, index: int, out: bytearray):
    """Tries to encode `data`, starting from `index`, into the `out` bytearray.
    If it encounters any codepoints above 127, it tries using the `errors`
    error handler to fix it internally, but returns the a tuple of the first
    and last index of the error on failure.
    If it finishes encoding, it returns a tuple of the final bytes and length.
    """
    pass


def ascii_encode(data: str, errors: str = "strict"):
    if not isinstance(data, str):
        raise TypeError(
            f"ascii_encode() argument 1 must be str, not {type(data).__name__}"
        )
    if not isinstance(errors, str):
        raise TypeError(
            "ascii_encode() argument 2 must be str or None, not "
            f"{type(errors).__name__}"
        )
    result = bytearray()
    i = 0
    encoded = b""
    length = len(data)
    while i < length:
        encoded, i = _ascii_encode(data, errors, i, result)
        if _int_check(encoded):
            unicode, pos = _call_encode_errorhandler(
                errors, data, "ordinal not in range(128)", "ascii", encoded, i
            )
            if isinstance(unicode, bytes):
                result += unicode
                i = pos
                continue
            for char in unicode:
                if char > "\x7F":
                    raise UnicodeEncodeError(
                        "ascii", unicode, encoded, i, "ordinal not in range(128)"
                    )
            _bytearray_string_append(result, unicode)
            i = pos
    if isinstance(encoded, bytes):
        return encoded, i
    # _ascii_encode encountered an error and _call_encode_errorhandler was the
    # last function to write to `result`.
    return bytes(result), i


@_patch
def _latin_1_encode(data: str, errors: str, index: int, out: bytearray):
    """Tries to encode `data`, starting from `index`, into the `out` bytearray.
    If it encounters any codepoints above 255, it tries using the `errors`
    error handler to fix it internally, but returns the a tuple of the first
    and last index of the error on failure.
    If it finishes encoding, it returns a tuple of the final bytes and length.
    """
    pass


def latin_1_encode(data: str, errors: str = "strict"):
    if not isinstance(data, str):
        raise TypeError(
            f"latin_1_encode() argument 1 must be str, not {type(data).__name__}"
        )
    if not isinstance(errors, str):
        raise TypeError(
            "latin_1_encode() argument 2 must be str or None, not "
            f"{type(errors).__name__}"
        )
    result = bytearray()
    i = 0
    encoded = b""
    length = len(data)
    while i < length:
        encoded, i = _latin_1_encode(data, errors, i, result)
        if _int_check(encoded):
            unicode, pos = _call_encode_errorhandler(
                errors, data, "ordinal not in range(256)", "latin-1", encoded, i
            )
            if isinstance(unicode, bytes):
                result += unicode
                i = pos
                continue
            for char in unicode:
                if char > "\xFF":
                    raise UnicodeEncodeError(
                        "latin-1", unicode, encoded, i, "ordinal not in range(256)"
                    )
            result += latin_1_encode(unicode, errors)[0]
            i = pos
    if isinstance(encoded, bytes):
        return encoded, i
    # _latin_1_encode encountered an error and _call_encode_errorhandler was the
    # last function to write to `result`.
    return bytes(result), i


@_patch
def _utf_8_encode(data: str, errors: str, index: int, out: bytearray):
    """Tries to encode `data`, starting from `index`, into the `out` bytearray.
    If it encounters an error, it tries using the `errors` error handler to
    fix it internally, but returns the a tuple of the first and last index of
    the error.
    If it finishes encoding, it returns a tuple of the final bytes and length.
    """
    pass


def utf_8_encode(data: str, errors: str = "strict"):
    if not isinstance(data, str):
        raise TypeError(
            f"utf_8_encode() argument 1 must be str, not {type(data).__name__}"
        )
    if not isinstance(errors, str):
        raise TypeError(
            "utf_8_encode() argument 2 must be str or None, not "
            f"{type(errors).__name__}"
        )
    result = bytearray()
    i = 0
    encoded = bytes()
    while i < len(data):
        encoded, i = _utf_8_encode(data, errors, i, result)
        if _int_check(encoded):
            unicode, pos = _call_encode_errorhandler(
                errors, data, "surrogates not allowed", "utf-8", encoded, i
            )
            if isinstance(unicode, bytes):
                result += unicode
                i = pos
                continue
            for char in unicode:
                if ord(char) > 127:
                    raise UnicodeEncodeError(
                        "utf-8", unicode, encoded, i, "surrogates not allowed"
                    )
            _bytearray_string_append(result, unicode)
            i = pos
    if isinstance(encoded, bytes):
        return encoded, i
    # _utf_8_encode encountered an error and _call_encode_errorhandler was the
    # last function to write to `result`.
    return bytes(result), i


_codec_decode_table = {"ascii": ascii_decode, "us_ascii": ascii_decode}

_codec_encode_table = {
    "ascii": ascii_encode,
    "us_ascii": ascii_encode,
    "latin_1": latin_1_encode,
    "latin-1": latin_1_encode,
    "iso-8859-1": latin_1_encode,
    "iso_8859_1": latin_1_encode,
    "utf_8": utf_8_encode,
    "utf-8": utf_8_encode,
    "utf8": utf_8_encode,
}


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


def _call_encode_errorhandler(
    errors: str, input: str, reason: str, encoding: str, start: int, end: int
):
    """
    Generic encoding errorhandling function
    Creates a UnicodeEncodeError, looks up an error handler, and calls the
    error handler with the UnicodeEncodeError.
    Makes sure the error handler returns a (str/bytes, int) tuple and returns it

    errors: The name of the error handling function to call
    input: The input to be encoded
    reason: The reason the errorhandler was called
    encoding: The encoding being used
    start: The index of the first non-erroneus byte
    end: The index of the first non-erroneous byte
    """
    exception = UnicodeEncodeError(encoding, input, start, end, reason)
    result = lookup_error(errors)(exception)
    if (
        not isinstance(result, tuple)
        or len(result) != 2
        or not isinstance(result[0], (str, bytes))
        or not hasattr(result[1], "__index__")
    ):
        raise TypeError("encoding error handler must return (str/bytes, int) tuple")
    unicode = result[0]
    pos = _index(result[1])
    if pos < 0:
        pos += len(input)
    if not 0 <= pos <= len(input):
        raise IndexError(f"position {pos} from error handler out of bounds")

    return unicode, pos


_codec_error_registry = {"strict": strict_errors, "ignore": ignore_errors}


@_patch
def _bytearray_string_append(dst: bytearray, data: str):
    pass


# TODO(T42244617): Replace with a _strarray type
@_patch
def _bytearray_to_string(src: bytearray) -> str:
    pass
