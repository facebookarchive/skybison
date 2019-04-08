#!/usr/bin/env python3
"""The sys module"""


# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_Unbound = _Unbound  # noqa: F821
_patch = _patch  # noqa: F821
_stderr_fd = _stderr_fd  # noqa: F821
_stdout_fd = _stdout_fd  # noqa: F821
executable = executable  # noqa: F821


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


_base_dir = executable[: executable.rfind("/")] + "/.."
# TODO(T42692043) Put the standard library into the python binary instead.
path = ["", _base_dir + "/library", _base_dir + "/third-party/cpython/Lib"]


path_hooks = []


path_importer_cache = {}


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


def getsizeof(obj, default=_Unbound):
    dunder_sizeof = type(obj).__sizeof__
    result = dunder_sizeof(obj)
    if not isinstance(result, int):
        if default is _Unbound:
            raise TypeError("an integer is required")
        return default
    if result < 0:
        raise ValueError("__sizeof__() should return >= 0")
    return result


warnoptions = []
