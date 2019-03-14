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
