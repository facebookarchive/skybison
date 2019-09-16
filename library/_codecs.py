#!/usr/bin/env python3

# These values are injected by our boot process. flake8 has no knowledge about
# their definitions and will complain without this gross circular helper here.
_bytearray_check = _bytearray_check  # noqa: F821
_bytes_check = _bytes_check  # noqa: F821
_byteslike_guard = _byteslike_guard  # noqa: F821
_index = _index  # noqa: F821
_int_check = _int_check  # noqa: F821
_patch = _patch  # noqa: F821
_str_check = _str_check  # noqa: F821
_strarray = _strarray  # noqa: F821
_strarray_iadd = _strarray_iadd  # noqa: F821
_tuple_check = _tuple_check  # noqa: F821
_type = _type  # noqa: F821
_Unbound = _Unbound  # noqa: F821


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
        if not _tuple_check(result) or not len(result) == 4:
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
        if _tuple_check(result) and len(result) == 2:
            return result[0]
        # CPython does not check to make sure that the second element is an int
        raise TypeError("decoder must return a tuple (object,integer)")


def encode(data, encoding: str = "utf-8", errors: str = _Unbound) -> bytes:
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
        if _tuple_check(result) and len(result) == 2:
            return result[0]
        # CPython does not check to make sure that the second element is an int
        raise TypeError("encoder must return a tuple (object, integer)")


@_patch
def _ascii_decode(data: str, errors: str, index: int, out: _strarray):
    pass


def ascii_decode(data: bytes, errors: str = "strict"):
    _byteslike_guard(data)
    if not _str_check(errors):
        raise TypeError(
            "ascii_decode() argument 2 must be str or None, not "
            f"'{_type(errors).__name__}'"
        )
    result = _strarray()
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
    if not _str_check(data):
        raise TypeError(
            f"ascii_encode() argument 1 must be str, not {_type(data).__name__}"
        )
    if not _str_check(errors):
        raise TypeError(
            "ascii_encode() argument 2 must be str or None, not "
            f"{_type(errors).__name__}"
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


@_patch
def _escape_decode(data: bytes, errors: str, recode_encoding: str):
    """Tries to decode `data`.
    If it runs into any errors, it raises and returns the message to throw.
    If it finishes encoding, it returns a tuple of
    (decoded, length, first_invalid_escape)
    where the first_invalid_escape is either the index into the data of the first
    invalid escape sequence, or -1 if none occur.
    Will eventually have to handle the recode_encoding argument.
    """
    pass


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


def escape_decode(data: bytes, errors: str = "strict") -> str:
    return _escape_decode_stateful(data, errors)[:2]


@_patch
def _latin_1_decode(data: bytes):
    pass


def latin_1_decode(data: bytes, errors: str = "strict"):
    _byteslike_guard(data)
    if not _str_check(errors):
        raise TypeError(
            "latin_1_decode() argument 2 must be str or None, not "
            f"'{_type(errors).__name__}'"
        )
    return _latin_1_decode(data)


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
    length = len(data)
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


@_patch
def _unicode_escape_decode(data: bytes, errors: str, index: int, out: _strarray):
    """Tries to decode `data`, starting from `index`, into the `out` _strarray.
    If it runs into any errors, it returns a tuple of
    (error_start, error_end, error_message, first_invalid_escape),
    where the first_invalid_escape is either the index into the data of the first
    invalid escape sequence, or -1 if none occur.
    If it finishes encoding, it returns a tuple of
    (decoded, length, "", first_invalid_escape)
    """
    pass


def _unicode_escape_decode_stateful(data: bytes, errors: str = "strict"):
    if not _str_check(data):
        _byteslike_guard(data)
    if not _str_check(errors):
        raise TypeError(
            "unicode_escape_decode() argument 2 must be str or None, not "
            f"{type(errors).__name__}"
        )
    result = _strarray()
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


def unicode_escape_decode(data: bytes, errors: str = "strict") -> str:
    return _unicode_escape_decode_stateful(data, errors)[:2]


@_patch
def _utf_8_decode(
    data: bytes, errors: str, index: int, out: _strarray, is_stateful: bool
):
    """Tries to decode `data`, starting from `index`, into the `out` _strarray.
    If it runs into any errors, it returns a tuple of
    (error_start, error_end, error_message),
    If it finishes encoding, it returns a tuple of
    (decoded, length, "")
    """
    pass


def _utf_8_decode_stateful(
    data: bytes, errors: str = "strict", is_stateful: bool = False
):
    _byteslike_guard(data)
    if not _str_check(errors) and not None:
        raise TypeError(
            "utf_8_decode() argument 2 must be str or None, not "
            f"'{_type(errors).__name__}'"
        )
    result = _strarray()
    i = 0
    encoded = ""
    length = len(data)
    while i < length:
        encoded, i, errmsg = _utf_8_decode(data, errors, i, result, is_stateful)
        if _int_check(encoded):
            data, i = _call_decode_errorhandler(
                errors, data, result, errmsg, "utf-8", encoded, i
            )
            continue
        # If encoded isn't an int, _utf_8_decode returned because it ran into
        # an error it could potentially recover from and is_stateful is true.
        # We should stop decoding in this case.
        break
    if _str_check(encoded):
        return encoded, i
    # The error handler was the last to write to the result
    return str(result), i


def utf_8_decode(data: bytes, errors: str = "strict"):
    return _utf_8_decode_stateful(data, errors, True)


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
    length = len(data)
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


@_patch
def _utf_16_encode(data: str, errors: str, index: int, out: bytearray, byteorder: int):
    pass


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
    length = len(data)
    encoded = bytes(result)
    while i < length:
        encoded, i = _utf_16_encode(data, errors, i, result, byteorder)
        if _int_check(encoded):
            unicode, pos = _call_encode_errorhandler(
                errors, data, "surrogates not allowed", h_encoding, encoded, i
            )
            if _bytes_check(unicode):
                if len(unicode) & 1:
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


def utf_16_le_encode(data: str, errors: str = "strict"):
    return utf_16_encode(data, errors, -1)


def utf_16_be_encode(data: str, errors: str = "strict"):
    return utf_16_encode(data, errors, 1)


@_patch
def _utf_32_encode(data: str, errors: str, index: int, out: bytearray, byteorder: int):
    pass


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
    length = len(data)
    encoded = bytes(result)
    while i < length:
        encoded, i = _utf_32_encode(data, errors, i, result, byteorder)
        if _int_check(encoded):
            unicode, pos = _call_encode_errorhandler(
                errors, data, "surrogates not allowed", hEncoding, encoded, i
            )
            if _bytes_check(unicode):
                if len(unicode) & 3:
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
    output: _strarray,
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
    writes the str to the output _strarray passed in.
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
        not _tuple_check(result)
        or len(result) != 2
        or not _str_check(result[0])
        or not hasattr(result[1], "__index__")
    ):
        raise TypeError("decoding error handler must return (str, int) tuple")
    replacement = result[0]
    pos = _index(result[1])
    input = exception.object
    if not _bytes_check(input):
        raise TypeError("exception attribute object must be bytes")
    if pos < 0:
        pos += len(input)
    if not 0 <= pos <= len(input):
        raise IndexError(f"position {pos} from error handler out of bounds")
    _strarray_iadd(output, replacement)

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
        not _tuple_check(result)
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
