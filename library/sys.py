#!/usr/bin/env python3
"""The sys module"""


# These values are all injected by our boot process. flake8 has no knowledge
# about their definitions and will complain without these lines.
_Unbound = _Unbound  # noqa: F821
_base_dir = _base_dir  # noqa: F821
_patch = _patch  # noqa: F821
_stderr_fd = _stderr_fd  # noqa: F821
_stdout_fd = _stdout_fd  # noqa: F821
executable = executable  # noqa: F821


class _IOStream:
    """Simple io-stream implementation. We will replace this with
    _io.TextIOWrapper later."""

    def __init__(self, fd):
        self._fd = fd

    def flush(self):
        _fd_flush(self._fd)

    def write(self, text):
        # TODO(matthiasb): Use text.encode() once it is available and remove
        # hack from _fd_write().
        text_bytes = text
        _fd_write(self._fd, text_bytes)


@_patch
def _fd_flush(fd):
    pass


@_patch
def _fd_write(fd, bytes):
    pass


def exit(code=0):
    raise SystemExit(code)


def getfilesystemencoding():
    # TODO(T40363016): Allow arbitrary encodings instead of defaulting to utf-8.
    return "utf-8"


def getfilesystemencodeerrors():
    # TODO(T40363016): Allow arbitrary encodings and error handlings.
    return "surrogateescape"


meta_path = []


# TODO(T42692043) Put the standard library into the python binary instead.
path = ["", _base_dir + "/library", _base_dir + "/third-party/cpython/Lib"]


path_hooks = []


path_importer_cache = {}


stdout = _IOStream(_stdout_fd)
stderr = _IOStream(_stderr_fd)


# TODO(T39224400): Implement flags as a structsequence
class FlagsStructSeq:
    def __init__(self):
        self.verbose = 0
        self.bytes_warning = 0


flags = FlagsStructSeq()


# TODO(T40871632): Add sys.implementation as a namespace object
class ImplementationType:
    def __init__(self):
        # TODO(T40871490): Cache compiles *.py files to a __cache__ directory
        # Setting cache_tag to None avoids caching or searching for cached files
        self.cache_tag = None


implementation = ImplementationType()


dont_write_bytecode = False


@_patch
def displayhook(value):
    pass


@_patch
def excepthook(exc, value, tb):
    pass


@_patch
def exc_info():
    pass


def getsizeof(object, default=_Unbound):
    # It is possible (albeit difficult) to define a class without __sizeof__
    try:
        cls = type(object)
        size = cls.__sizeof__
    except AttributeError:
        if default is _Unbound:
            raise TypeError(f"Type {cls.__name__} doesn't define __sizeof__")
        return default
    result = size(object)
    if not isinstance(result, int):
        if default is _Unbound:
            raise TypeError("an integer is required")
        return default
    if result < 0:
        raise ValueError("__sizeof__() should return >= 0")
    return int(result)


warnoptions = []
