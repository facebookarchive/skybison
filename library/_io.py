#!/usr/bin/env python3
"""The io module provides the Python interfaces to stream handling. The
builtin open function is defined in this module.

At the top of the I/O hierarchy is the abstract base class IOBase. It
defines the basic interface to a stream. Note, however, that there is no
separation between reading and writing to streams; implementations are
allowed to raise an IOError if they do not support a given operation.

Extending IOBase is RawIOBase which deals simply with the reading and
writing of raw bytes to a stream. FileIO subclasses RawIOBase to provide
an interface to OS files.

BufferedIOBase deals with buffering on a raw byte stream (RawIOBase). Its
subclasses, BufferedWriter, BufferedReader, and BufferedRWPair buffer
streams that are readable, writable, and both respectively.
BufferedRandom provides a buffered interface to random access
streams. BytesIO is a simple stream of in-memory bytes.

Another IOBase subclass, TextIOBase, deals with the encoding and decoding
of streams into text. TextIOWrapper, which extends it, is a buffered text
interface to a buffered raw stream (`BufferedIOBase`). Finally, StringIO
is an in-memory stream for text.

Argument names are not part of the specification, and only the arguments
of open() are intended to be used as keyword arguments."""

# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_patch = _patch  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


DEFAULT_BUFFER_SIZE = None


class BlockingIOError:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class BufferedRWPair:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class BufferedRandom:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class BufferedReader:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class BufferedWriter:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class BytesIO:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class FileIO:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class IncrementalNewlineDecoder:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class StringIO:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class TextIOWrapper:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class UnsupportedOperation:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class _BufferedIOBase:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class _IOBase:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class _RawIOBase(_IOBase):
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class _TextIOBase(_IOBase):
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


def open(
    file,
    mode="r",
    buffering=-1,
    encoding=None,
    errors=None,
    newline=None,
    closefd=True,
    opener=None,
):
    _unimplemented()


@_patch
def _readfile(path):
    pass


@_patch
def _readbytes(bytes):
    pass
