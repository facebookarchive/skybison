#!/usr/bin/env python3

from builtins import _index, _str_array

from _builtins import (
    _builtin,
    _bytes_check,
    _bytes_decode,
    _bytes_decode_ascii,
    _bytes_decode_utf_8,
    _bytes_len,
    _byteslike_guard,
    _int_check,
    _object_type_hasattr,
    _str_array_iadd,
    _str_check,
    _str_encode,
    _str_encode_ascii,
    _str_guard,
    _str_len,
    _tuple_check,
    _tuple_len,
    _type,
    _Unbound,
    _unimplemented,
    maxunicode as _maxunicode,
)


codec_search_path = []


codec_search_cache = {}


def register(search_func):
    if not callable(search_func):
        raise TypeError("argument must be callable")
    codec_search_path.append(search_func)


def lookup(encoding):
    cached = codec_search_cache.get(encoding)
    if cached is not None:
        return cached
    # Make sure that we loaded the standard codecs.
    if not codec_search_path:
        import encodings  # noqa: F401

    normalized_encoding = encoding.lower().replace(" ", "-")
    result = None
    for search_func in codec_search_path:
        result = search_func(normalized_encoding)
        if result is None:
            continue
        if not _tuple_check(result) or _tuple_len(result) != 4:
            raise TypeError("codec search functions must return 4-tuples")
        break
    if result is None:
        raise LookupError(f"unknown encoding: {encoding}")

    codec_search_cache[encoding] = result
    return result


def _lookup_text(encoding, alternate_command):
    codec = lookup(encoding)
    if _type(codec) != tuple:
        try:
            if not codec._is_text_encoding:
                raise LookupError(
                    f"{encoding} is not a text encoding; "
                    f"use {alternate_command} to handle arbitrary codecs"
                )
        except AttributeError:
            pass
    return codec


def decode(data, encoding: str = "utf-8", errors: str = _Unbound) -> str:
    result = _bytes_decode(data, encoding)
    if result is not _Unbound:
        return result
    try:
        return _codec_decode_table[encoding.lower()](
            data, "strict" if errors is _Unbound else errors
        )[0]
    except KeyError:
        try:
            decoder = lookup(encoding)[1]
        except LookupError:
            raise LookupError(f"unknown encoding: {encoding}")
        if errors is _Unbound:
            result = decoder(data)
        else:
            result = decoder(data, errors)
        if _tuple_check(result) and _tuple_len(result) == 2:
            return result[0]
        # CPython does not check to make sure that the second element is an int
        raise TypeError("decoder must return a tuple (object,integer)")


def encode(data, encoding: str = "utf-8", errors: str = _Unbound) -> bytes:
    result = _str_encode(data, encoding)
    if result is not _Unbound:
        return result
    try:
        return _codec_encode_table[encoding.lower()](
            data, "strict" if errors is _Unbound else errors
        )[0]
    except KeyError:
        try:
            encoder = lookup(encoding)[0]
        except LookupError:
            raise LookupError(f"unknown encoding: {encoding}")
        if errors is _Unbound:
            result = encoder(data)
        else:
            result = encoder(data, errors)
        if _tuple_check(result) and _tuple_len(result) == 2:
            return result[0]
        # CPython does not check to make sure that the second element is an int
        raise TypeError("encoder must return a tuple (object, integer)")


def _ascii_decode(data: str, errors: str, index: int, out: _str_array):
    _builtin()


def ascii_decode(data: bytes, errors: str = "strict"):
    _byteslike_guard(data)
    if not _str_check(errors):
        raise TypeError(
            "ascii_decode() argument 2 must be str or None, not "
            f"'{_type(errors).__name__}'"
        )
    result = _bytes_decode_ascii(data)
    if result is not _Unbound:
        return result, _bytes_len(data)
    result = _str_array()
    i = 0
    encoded = ""
    length = len(data)
    while i < length:
        encoded, i = _ascii_decode(data, errors, i, result)
        if _int_check(encoded):
            data, i = _call_decode_errorhandler(
                errors, data, result, "ordinal not in range(128)", "ascii", encoded, i
            )
    if _str_check(encoded):
        return encoded, i
    # The error handler was the last to write to the result
    return str(result), i


def _ascii_encode(data: str, errors: str, index: int, out: bytearray):
    """Tries to encode `data`, starting from `index`, into the `out` bytearray.
    If it encounters any codepoints above 127, it tries using the `errors`
    error handler to fix it internally, but returns the a tuple of the first
    and last index of the error on failure.
    If it finishes encoding, it returns a tuple of the final bytes and length.
    """
    _builtin()


def ascii_encode(data: str, errors: str = "strict"):
    if not _str_check(data):
        raise TypeError(
            f"ascii_encode() argument 1 must be str, not {_type(data).__name__}"
        )
    if not _str_check(errors):
        raise TypeError(
            "ascii_encode() argument 2 must be str or None, not "
            f"{_type(errors).__name__}"
        )
    result = _str_encode_ascii(data)
    if result is not _Unbound:
        return result, _str_len(data)
    result = bytearray()
    i = 0
    encoded = b""
    length = _str_len(data)
    while i < length:
        encoded, i = _ascii_encode(data, errors, i, result)
        if _int_check(encoded):
            unicode, pos = _call_encode_errorhandler(
                errors, data, "ordinal not in range(128)", "ascii", encoded, i
            )
            if _bytes_check(unicode):
                result += unicode
                i = pos
                continue
            for char in unicode:
                if char > "\x7F":
                    raise UnicodeEncodeError(
                        "ascii", data, encoded, i, "ordinal not in range(128)"
                    )
            _bytearray_string_append(result, unicode)
            i = pos
    if _bytes_check(encoded):
        return encoded, i
    # _ascii_encode encountered an error and _call_encode_errorhandler was the
    # last function to write to `result`.
    return bytes(result), i


def charmap_decode(data, errors="strict", mapping=None):
    _byteslike_guard(data)
    _str_guard(errors)
    if errors != "strict":
        _unimplemented()

    result = _str_array()
    data_len = _bytes_len(data)
    i = 0
    while i < data_len:
        try:
            mapped = mapping[data[i]]
            if mapped is None or mapped == "\ufffe":
                raise UnicodeDecodeError(
                    "charmap", data, data[i], i, "character maps to <undefined>"
                )
            if _int_check(mapped):
                if mapped < 0 or mapped > _maxunicode:
                    raise TypeError(
                        f"character mapping must be in range ({_maxunicode + 1:#x})"
                    )
                mapped = chr(mapped)
            elif not _str_check(mapped):
                raise TypeError("character mapping must return integer, None or str")
            _str_array_iadd(result, mapped)
        except (IndexError, KeyError):
            raise UnicodeDecodeError(
                "charmap", data, data[i], i, "character maps to <undefined>"
            )
        i += 1

    return str(result), data_len


def _escape_decode(data: bytes, errors: str, recode_encoding: str):
    """Tries to decode `data`.
    If it runs into any errors, it raises and returns the message to throw.
    If it finishes encoding, it returns a tuple of
    (decoded, length, first_invalid_escape)
    where the first_invalid_escape is either the index into the data of the first
    invalid escape sequence, or -1 if none occur.
    Will eventually have to handle the recode_encoding argument.
    """
    _builtin()


def _escape_decode_stateful(
    data: bytes, errors: str = "strict", recode_encoding: str = ""
):
    if not _str_check(data):
        _byteslike_guard(data)
    if not _str_check(errors):
        raise TypeError(
            "escape_decode() argument 2 must be str or None, not "
            f"{type(errors).__name__}"
        )
    decoded = _escape_decode(data, errors, recode_encoding)
    if _str_check(decoded):
        raise ValueError(decoded)
    return decoded


def escape_decode(data, errors: str = "strict"):
    escaped, length, _ = _escape_decode_stateful(data, errors)
    return escaped, length


def _latin_1_decode(data: bytes):
    _builtin()


def latin_1_decode(data: bytes, errors: str = "strict"):
    _byteslike_guard(data)
    if not _str_check(errors):
        raise TypeError(
            "latin_1_decode() argument 2 must be str or None, not "
            f"'{_type(errors).__name__}'"
        )
    return _latin_1_decode(data)


def _latin_1_encode(data: str, errors: str, index: int, out: bytearray):
    """Tries to encode `data`, starting from `index`, into the `out` bytearray.
    If it encounters any codepoints above 255, it tries using the `errors`
    error handler to fix it internally, but returns the a tuple of the first
    and last index of the error on failure.
    If it finishes encoding, it returns a tuple of the final bytes and length.
    """
    _builtin()


def latin_1_encode(data: str, errors: str = "strict"):
    if not _str_check(data):
        raise TypeError(
            f"latin_1_encode() argument 1 must be str, not {_type(data).__name__}"
        )
    if not _str_check(errors):
        raise TypeError(
            "latin_1_encode() argument 2 must be str or None, not "
            f"{_type(errors).__name__}"
        )
    result = bytearray()
    i = 0
    encoded = b""
    length = _str_len(data)
    while i < length:
        encoded, i = _latin_1_encode(data, errors, i, result)
        if _int_check(encoded):
            unicode, pos = _call_encode_errorhandler(
                errors, data, "ordinal not in range(256)", "latin-1", encoded, i
            )
            if _bytes_check(unicode):
                result += unicode
                i = pos
                continue
            for char in unicode:
                if char > "\xFF":
                    raise UnicodeEncodeError(
                        "latin-1", data, encoded, i, "ordinal not in range(256)"
                    )
            result += latin_1_encode(unicode, errors)[0]
            i = pos
    if _bytes_check(encoded):
        return encoded, i
    # _latin_1_encode encountered an error and _call_encode_errorhandler was the
    # last function to write to `result`.
    return bytes(result), i


def raw_unicode_escape_decode(data, errors: str = "strict"):
    _unimplemented()


def raw_unicode_escape_encode(data, errors: str = "strict"):
    _unimplemented()


def _unicode_escape_decode(data: bytes, errors: str, index: int, out: _str_array):
    """Tries to decode `data`, starting from `index`, into the `out` _str_array.
    If it runs into any errors, it returns a tuple of
    (error_start, error_end, error_message, first_invalid_escape),
    where the first_invalid_escape is either the index into the data of the first
    invalid escape sequence, or -1 if none occur.
    If it finishes encoding, it returns a tuple of
    (decoded, length, "", first_invalid_escape)
    """
    _builtin()


def _unicode_escape_decode_stateful(data: bytes, errors: str = "strict"):
    if not _str_check(data):
        _byteslike_guard(data)
    if not _str_check(errors):
        raise TypeError(
            "unicode_escape_decode() argument 2 must be str or None, not "
            f"{type(errors).__name__}"
        )
    result = _str_array()
    i = 0
    decoded = ""
    length = len(data)
    while i < length:
        decoded, i, error_msg, first_invalid = _unicode_escape_decode(
            data, errors, i, result
        )
        if error_msg:
            data, i = _call_decode_errorhandler(
                errors, data, result, error_msg, "unicodeescape", decoded, i
            )
    if _str_check(decoded):
        return decoded, i, first_invalid
    # The error handler was the last to write to the result
    return str(result), i, first_invalid


def unicode_escape_decode(data, errors: str = "strict"):
    escaped, length, _ = _unicode_escape_decode_stateful(data, errors)
    return escaped, length


def unicode_escape_encode(data, errors: str = "strict"):
    _unimplemented()


def _utf_8_decode(
    data: bytes, errors: str, index: int, out: _str_array, is_final: bool
):
    """Tries to decode `data`, starting from `index`, into the `out` _str_array.
    If it runs into any errors, it returns a tuple of
    (error_start, error_end, error_message),
    If it finishes encoding, it returns a tuple of
    (decoded, length, "")
    """
    _builtin()


def utf_8_decode(data: bytes, errors: str = "strict", is_final: bool = False):
    _byteslike_guard(data)
    if not _str_check(errors) and not None:
        raise TypeError(
            "utf_8_decode() argument 2 must be str or None, not "
            f"'{_type(errors).__name__}'"
        )
    result = _bytes_decode_utf_8(data)
    if result is not _Unbound:
        return result, _bytes_len(data)
    result = _str_array()
    i = 0
    encoded = ""
    length = len(data)
    while i < length:
        encoded, i, errmsg = _utf_8_decode(data, errors, i, result, is_final)
        if _int_check(encoded):
            data, i = _call_decode_errorhandler(
                errors, data, result, errmsg, "utf-8", encoded, i
            )
            continue
        # If encoded isn't an int, utf_8_decode returned because it ran into
        # an error it could potentially recover from and is_final is true.
        # We should stop decoding in this case.
        break
    if _str_check(encoded):
        return encoded, i
    # The error handler was the last to write to the result
    return str(result), i


def _utf_8_encode(data: str, errors: str, index: int, out: bytearray):
    """Tries to encode `data`, starting from `index`, into the `out` bytearray.
    If it encounters an error, it tries using the `errors` error handler to
    fix it internally, but returns the a tuple of the first and last index of
    the error.
    If it finishes encoding, it returns a tuple of the final bytes and length.
    """
    _builtin()


def utf_8_encode(data: str, errors: str = "strict"):
    if not _str_check(data):
        raise TypeError(
            f"utf_8_encode() argument 1 must be str, not {_type(data).__name__}"
        )
    if not _str_check(errors):
        raise TypeError(
            "utf_8_encode() argument 2 must be str or None, not "
            f"{_type(errors).__name__}"
        )
    result = bytearray()
    i = 0
    encoded = bytes()
    length = _str_len(data)
    while i < length:
        encoded, i = _utf_8_encode(data, errors, i, result)
        if _int_check(encoded):
            unicode, pos = _call_encode_errorhandler(
                errors, data, "surrogates not allowed", "utf-8", encoded, i
            )
            if _bytes_check(unicode):
                result += unicode
                i = pos
                continue
            for char in unicode:
                if char > "\x7F":
                    raise UnicodeEncodeError(
                        "utf-8", data, encoded, i, "surrogates not allowed"
                    )
            _bytearray_string_append(result, unicode)
            i = pos
    if _bytes_check(encoded):
        return encoded, i
    # _utf_8_encode encountered an error and _call_encode_errorhandler was the
    # last function to write to `result`.
    return bytes(result), i


def _utf_16_encode(data: str, errors: str, index: int, out: bytearray, byteorder: int):
    _builtin()


def utf_16_encode(data: str, errors: str = "strict", byteorder: int = 0):  # noqa: C901
    if byteorder < 0:
        h_encoding = "utf-16-le"
        u_encoding = "utf_16_le"
    elif byteorder < 0:
        h_encoding = "utf-16-be"
        u_encoding = "utf_16_be"
    else:
        h_encoding = "utf-16"
        u_encoding = "utf_16"
    if not _str_check(data):
        raise TypeError(
            f"{u_encoding}_encode() argument 1 must be str, not {_type(data).__name__}"
        )
    if not _str_check(errors):
        raise TypeError(
            f"{u_encoding}_encode() argument 2 must be str or None, not "
            f"{_type(errors).__name__}"
        )
    result = bytearray()
    if byteorder == 0:
        result += b"\xFF"
        result += b"\xFE"
    i = 0
    length = _str_len(data)
    encoded = bytes(result)
    while i < length:
        encoded, i = _utf_16_encode(data, errors, i, result, byteorder)
        if _int_check(encoded):
            unicode, pos = _call_encode_errorhandler(
                errors, data, "surrogates not allowed", h_encoding, encoded, i
            )
            if _bytes_check(unicode):
                if _bytes_len(unicode) & 1:
                    raise UnicodeEncodeError(
                        h_encoding, data, encoded, i, "surrogates not allowed"
                    )
                result += unicode
                i = pos
                continue
            for char in unicode:
                if char > "\x7F":
                    raise UnicodeEncodeError(
                        h_encoding, data, encoded, i, "surrogates not allowed"
                    )
            result += utf_16_encode(
                unicode, errors, -1 if byteorder == 0 else byteorder
            )[0]
            i = pos
    if _bytes_check(encoded):
        return encoded, i
    # _utf_16_encode encountered an error and _call_encode_errorhandler was the
    # last function to write to `result`.
    return bytes(result), i


def utf_16_le_decode(data: str, errors: str = "strict"):
    _unimplemented()


def utf_16_le_encode(data: str, errors: str = "strict"):
    return utf_16_encode(data, errors, -1)


def utf_16_be_decode(data: str, errors: str = "strict"):
    _unimplemented()


def utf_16_be_encode(data: str, errors: str = "strict"):
    return utf_16_encode(data, errors, 1)


def _utf_32_encode(data: str, errors: str, index: int, out: bytearray, byteorder: int):
    _builtin()


def utf_32_encode(data: str, errors: str = "strict", byteorder: int = 0):  # noqa: C901
    if byteorder < 0:
        hEncoding = "utf-32-le"
        uEncoding = "utf_32_le"
    elif byteorder < 0:
        hEncoding = "utf-32-be"
        uEncoding = "utf_32_be"
    else:
        hEncoding = "utf-32"
        uEncoding = "utf_32"
    if not _str_check(data):
        raise TypeError(
            f"{uEncoding}_encode() argument 1 must be str, not {_type(data).__name__}"
        )
    if not _str_check(errors):
        raise TypeError(
            f"{uEncoding}_encode() argument 2 must be str or None, not "
            f"{_type(errors).__name__}"
        )
    result = bytearray()
    if byteorder == 0:
        result += b"\xFF\xFE\x00\x00"
    i = 0
    length = _str_len(data)
    encoded = bytes(result)
    while i < length:
        encoded, i = _utf_32_encode(data, errors, i, result, byteorder)
        if _int_check(encoded):
            unicode, pos = _call_encode_errorhandler(
                errors, data, "surrogates not allowed", hEncoding, encoded, i
            )
            if _bytes_check(unicode):
                if _bytes_len(unicode) & 3:
                    raise UnicodeEncodeError(
                        hEncoding, data, encoded, i, "surrogates not allowed"
                    )
                result += unicode
                i = pos
                continue
            for char in unicode:
                if char > "\x7f":
                    raise UnicodeEncodeError(
                        hEncoding, data, encoded, i, "surrogates not allowed"
                    )
            result += utf_32_encode(
                unicode, errors, -1 if byteorder == 0 else byteorder
            )[0]
            i = pos
    if _bytes_check(encoded):
        return encoded, i
    # _utf_32_encode encountered an error and _call_encode_errorhandler was the
    # last function to write to `result`.
    return bytes(result), i


def utf_32_le_encode(data: str, errors: str = "strict"):
    return utf_32_encode(data, errors, -1)


def utf_32_be_encode(data: str, errors: str = "strict"):
    return utf_32_encode(data, errors, 1)


_codec_decode_table = {
    "ascii": ascii_decode,
    "us_ascii": ascii_decode,
    "latin1": latin_1_decode,
    "latin 1": latin_1_decode,
    "latin-1": latin_1_decode,
    "latin_1": latin_1_decode,
    "utf_8": utf_8_decode,
    "utf-8": utf_8_decode,
    "utf8": utf_8_decode,
}

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
    "utf_16": utf_16_encode,
    "utf-16": utf_16_encode,
    "utf16": utf_16_encode,
    "utf_16_le": utf_16_le_encode,
    "utf-16-le": utf_16_le_encode,
    "utf_16_be": utf_16_be_encode,
    "utf-16-be": utf_16_be_encode,
    "utf_32": utf_32_encode,
    "utf-32": utf_32_encode,
    "utf32": utf_32_encode,
    "utf_32_le": utf_32_le_encode,
    "utf-32-le": utf_32_le_encode,
    "utf_32_be": utf_32_be_encode,
    "utf-32-be": utf_32_be_encode,
}


def strict_errors(error):
    if not isinstance(error, Exception):
        raise TypeError("codecs must pass exception instance")
    raise error


def ignore_errors(error):
    if not isinstance(error, UnicodeError):
        raise TypeError(
            f"don't know how to handle {_type(error).__name__} in error callback"
        )
    return ("", error.end)


def lookup_error(error: str):
    if not _str_check(error):
        raise TypeError(
            f"lookup_error() argument must be str, not {_type(error).__name__}"
        )
    try:
        return _codec_error_registry[error]
    except KeyError:
        raise LookupError(f"unknown error handler name '{error}'")


def register_error(name: str, error_func):
    if not _str_check(name):
        raise TypeError(
            f"register_error() argument 1 must be str, not {_type(name).__name__}"
        )
    if not callable(error_func):
        raise TypeError("handler must be callable")
    _codec_error_registry[name] = error_func


def _call_decode_errorhandler(
    errors: str,
    input: bytes,
    output: _str_array,
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
    writes the str to the output _str_array passed in.
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
    if not _tuple_check(result) or _tuple_len(result) != 2:
        raise TypeError("decoding error handler must return (str, int) tuple")

    replacement, pos = result
    if not _str_check(replacement) or not _object_type_hasattr(pos, "__index__"):
        raise TypeError("decoding error handler must return (str, int) tuple")

    pos = _index(pos)
    input = exception.object
    if not _bytes_check(input):
        raise TypeError("exception attribute object must be bytes")
    if pos < 0:
        pos += _bytes_len(input)
    if not 0 <= pos <= _bytes_len(input):
        raise IndexError(f"position {pos} from error handler out of bounds")
    _str_array_iadd(output, replacement)

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
    if not _tuple_check(result) or _tuple_len(result) != 2:
        raise TypeError("encoding error handler must return (str/bytes, int) tuple")

    unicode, pos = result
    if (
        not _str_check(unicode)
        and not _bytes_check(unicode)
        or not _object_type_hasattr(pos, "__index__")
    ):
        raise TypeError("encoding error handler must return (str/bytes, int) tuple")

    pos = _index(pos)
    length = len(input)
    if pos < 0:
        pos += length
    if not 0 <= pos <= length:
        raise IndexError(f"position {pos} from error handler out of bounds")

    return unicode, pos


# TODO(T61927696): Support surrogatepass errors for utf-8 decode
_codec_error_registry = {"strict": strict_errors, "ignore": ignore_errors}


def _bytearray_string_append(dst: bytearray, data: str):
    _builtin()


# NOTE: This should behave the same as codecs.IncrementalEncoder.
# TODO(T61720167): Should be removed once we can freeze encodings
class IncrementalEncoder(object):
    def __init__(self, errors="strict"):
        self.errors = errors
        self.buffer = ""

    def encode(self, input, final=False):
        raise NotImplementedError

    def reset(self):
        pass

    def getstate(self):
        return 0

    def setstate(self, state):
        pass


# NOTE: This should behave the same as codecs.IncrementalDecoder.
# TODO(T61720167): Should be removed once we can freeze encodings
class IncrementalDecoder(object):
    def __init__(self, errors="strict"):
        self.errors = errors

    def decode(self, input, final=False):
        raise NotImplementedError

    def reset(self):
        pass

    def getstate(self):
        return (b"", 0)

    def setstate(self, state):
        pass


# NOTE: This should behave the same as codecs.BufferedIncrementalDecoder.
# TODO(T61720167): Should be removed once we can freeze encodings
class BufferedIncrementalDecoder(IncrementalDecoder):
    def __init__(self, errors="strict"):
        IncrementalDecoder.__init__(self, errors)
        self.buffer = b""

    def _buffer_decode(self, input, errors, final):
        raise NotImplementedError

    def decode(self, input, final=False):
        data = self.buffer + input
        (result, consumed) = self._buffer_decode(data, self.errors, final)
        self.buffer = data[consumed:]
        return result

    def reset(self):
        IncrementalDecoder.reset(self)
        self.buffer = b""

    def getstate(self):
        return (self.buffer, 0)

    def setstate(self, state):
        self.buffer = state[0]


# TODO(T61720167): Should be removed once we can freeze encodings
class UTF8IncrementalEncoder(IncrementalEncoder):
    def encode(self, input, final=False):
        return utf_8_encode(input, self.errors)[0]


# TODO(T61720167): Should be removed once we can freeze encodings
class UTF8IncrementalDecoder(BufferedIncrementalDecoder):
    @staticmethod
    def _buffer_decode(input, errors, final):
        return utf_8_decode(input, errors, final)


# TODO(T61720167): Should be removed once we can freeze encodings
def getincrementaldecoder(encoding):
    if encoding == "UTF-8" or encoding == "utf-8":
        return UTF8IncrementalDecoder
    decoder = lookup(encoding).incrementaldecoder
    if decoder is None:
        raise LookupError(encoding)
    return decoder


# TODO(T61720167): Should be removed once we can freeze encodings
def getincrementalencoder(encoding):
    if encoding == "UTF-8" or encoding == "utf-8":
        return UTF8IncrementalEncoder
    encoder = lookup(encoding).incrementalencoder
    if encoder is None:
        raise LookupError(encoding)
    return encoder
