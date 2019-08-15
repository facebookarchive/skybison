#!/usr/bin/env python3


# These values are all injected by our boot process. flake8 has no knowledge
# about their definitions and will complain without these lines.
_Unbound = _Unbound  # noqa: F821
_base_dir = _base_dir  # noqa: F821
_os_write = _os_write  # noqa: F821
_patch = _patch  # noqa: F821
_stderr_fd = _stderr_fd  # noqa: F821
_stdout_fd = _stdout_fd  # noqa: F821
executable = executable  # noqa: F821


class _FlagsStructSeq:
    # TODO(T39224400): Implement flags as a structsequence
    def __init__(self):
        self.bytes_warning = 0
        self.no_site = 0
        self.no_user_site = 0
        self.verbose = 0


class _IOStream:
    """Simple io-stream implementation. We will replace this with
    _io.TextIOWrapper later."""

    def __init__(self, fd):
        self._fd = fd

    def flush(self):
        pass

    def write(self, text):
        text_bytes = text.encode("utf-8")
        _os_write(self._fd, text_bytes)


class _ImplementationType:
    # TODO(T40871632): Add sys.implementation as a namespace object
    def __init__(self):
        # TODO(T40871490): Cache compiles *.py files to a __cache__ directory
        # Setting cache_tag to None avoids caching or searching for cached files
        self.cache_tag = None
        self.name = "pyro"


class _VersionInfo(tuple):
    # TODO(cshapiro): switch to using builtins._structseq_field
    # instead of a custom descriptor
    class _VersionInfoField:
        def __get__(self, instance, owner):
            return instance[self._index]

        def __init__(self, index):
            self._index = index

        def __set__(self, instance, value):
            raise AttributeError("readonly attribute")

    major = _VersionInfoField(0)
    minor = _VersionInfoField(1)
    micro = _VersionInfoField(2)
    releaselevel = _VersionInfoField(3)
    serial = _VersionInfoField(4)


@_patch
def _getframe_code(depth=0):
    pass


@_patch
def _getframe_globals(depth=0):
    pass


@_patch
def _getframe_lineno(depth=0) -> int:
    pass


abiflags = ""


# TODO(cshapiro): assign a meaningful value in the runtime
base_exec_prefix = ""


# TODO(cshapiro): assign a meaningful value in the runtime
base_prefix = ""


def displayhook(value):
    if value is None:
        return
    # Set '_' to None to avoid recursion
    import builtins

    builtins._ = None
    text = repr(value)
    try:
        stdout.write(text)
    except UnicodeEncodeError:
        bytes = text.encode(stdout.encoding, "backslashreplace")
        if hasattr(stdout, "buffer"):
            stdout.buffer.write(bytes)
        else:
            text = bytes.decode(stdout.encoding, "strict")
            stdout.write(text)
    stdout.write("\n")
    builtins._ = value


dont_write_bytecode = False


@_patch
def exc_info():
    pass


@_patch
def excepthook(exc, value, tb):
    pass


# TODO(cshapiro): assign a meaningful value in the runtime
exec_prefix = ""


def exit(code=0):
    raise SystemExit(code)


flags = _FlagsStructSeq()


def getfilesystemencodeerrors():
    # TODO(T40363016): Allow arbitrary encodings and error handlings.
    return "surrogateescape"


def getfilesystemencoding():
    # TODO(T40363016): Allow arbitrary encodings instead of defaulting to utf-8.
    return "utf-8"


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


implementation = _ImplementationType()


meta_path = []


# TODO(T42692043) Put the standard library into the python binary instead.
path = ["", _base_dir + "/library", _base_dir + "/third-party/cpython/Lib"]


path_hooks = []


path_importer_cache = {}


prefix = ""


ps1 = ">>> "


ps2 = "... "


stderr = _IOStream(_stderr_fd)


stdout = _IOStream(_stdout_fd)


version = "3.6.8+"


warnoptions = []


version_info = _VersionInfo((3, 6, 8, "final", 0))
