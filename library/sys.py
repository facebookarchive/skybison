#!/usr/bin/env python3

from _builtins import (
    _builtin,
    _getframe_function,
    _int_check,
    _int_guard,
    _os_write,
    _Unbound,
)


# These values are all injected by our boot process. flake8 has no knowledge
# about their definitions and will complain without these lines.
_base_dir = _base_dir  # noqa: F821
_python_path = _python_path  # noqa: F821
_stderr_fd = _stderr_fd  # noqa: F821
_stdin_fd = _stdin_fd  # noqa: F821
_stdout_fd = _stdout_fd  # noqa: F821
_structseq_field = _structseq_field  # noqa: F821
executable = executable  # noqa: F821


class _Flags(tuple):
    debug = _structseq_field("debug", 0)
    inspect = _structseq_field("inspect", 1)
    interactive = _structseq_field("interactive", 2)
    optimize = _structseq_field("optimize", 3)
    dont_write_bytecode = _structseq_field("dont_write_bytecode", 4)
    no_user_site = _structseq_field("no_user_site", 5)
    no_site = _structseq_field("no_site", 6)
    ignore_environment = _structseq_field("ignore_environment", 7)
    verbose = _structseq_field("verbose", 8)
    bytes_warning = _structseq_field("bytes_warning", 9)
    quiet = _structseq_field("quiet", 10)
    hash_randomization = _structseq_field("hash_randomization", 11)
    isolated = _structseq_field("isolated", 12)
    dev_mode = _structseq_field("dev_mode", 13)
    utf8_mode = _structseq_field("utf8_mode", 14)


class _FloatInfo(tuple):
    max = _structseq_field("max", 0)
    max_exp = _structseq_field("max_exp", 1)
    max_10_exp = _structseq_field("max_10_exp", 2)
    min = _structseq_field("min", 3)
    min_exp = _structseq_field("min_exp", 4)
    min_10_exp = _structseq_field("min_10_exp", 5)
    dig = _structseq_field("dig", 6)
    mant_dig = _structseq_field("mant_dig", 7)
    epsilon = _structseq_field("epsilon", 8)
    radix = _structseq_field("radix", 9)
    rounds = _structseq_field("rounds", 10)


class _HashInfo(tuple):
    width = _structseq_field("width", 0)
    modulus = _structseq_field("modulus", 1)
    inf = _structseq_field("inf", 2)
    nan = _structseq_field("nan", 3)
    imag = _structseq_field("imag", 4)
    algorithm = _structseq_field("algorithm", 5)
    hash_bits = _structseq_field("hash_bits", 6)
    seed_bits = _structseq_field("seed_bits", 7)
    cutoff = _structseq_field("cutoff", 8)


class _IOStream:
    """Simple io-stream implementation. We will replace this with
    _io.TextIOWrapper later."""

    def __init__(self, fd):
        self._fd = fd

    def fileno(self):
        return self._fd

    def flush(self):
        pass

    def write(self, text):
        text_bytes = text.encode("utf-8")
        _os_write(self._fd, text_bytes)


class _ImplementationType:
    # TODO(T40871632): Add sys.implementation as a namespace object
    def __init__(self):
        self.cache_tag = "pyro-36"
        self.name = "pyro"


class _VersionInfo(tuple):
    major = _structseq_field("major", 0)
    minor = _structseq_field("minor", 1)
    micro = _structseq_field("micro", 2)
    releaselevel = _structseq_field("releaselevel", 3)
    serial = _structseq_field("serial", 4)


def _getframe(depth=0):
    _builtin()


abiflags = ""


# TODO(cshapiro): assign a meaningful value in the runtime
base_exec_prefix = ""


# TODO(cshapiro): assign a meaningful value in the runtime
base_prefix = ""


copyright = ""


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


def exc_info():
    _builtin()


def excepthook(exc, value, tb):
    _builtin()


# TODO(cshapiro): assign a meaningful value in the runtime
exec_prefix = ""


def exit(code=_Unbound):
    if code is _Unbound:
        raise SystemExit()
    raise SystemExit(code)


flags = _Flags((0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, False, 0))


float_info = _FloatInfo(
    (
        1.79769313486231570814527423731704357e308,
        1024,
        308,
        2.22507385850720138309023271733240406e-308,
        -1021,
        -307,
        15,
        53,
        2.22044604925031308084726333618164062e-16,
        2,
        1,
    )
)


def getfilesystemencodeerrors():
    # TODO(T40363016): Allow arbitrary encodings and error handlings.
    return "surrogateescape"


def getfilesystemencoding():
    # TODO(T40363016): Allow arbitrary encodings instead of defaulting to utf-8.
    return "utf-8"


# TODO(T62600497): Enforce the recursion limit
def getrecursionlimit():
    _builtin()


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
    if not _int_check(result):
        if default is _Unbound:
            raise TypeError("an integer is required")
        return default
    if result < 0:
        raise ValueError("__sizeof__() should return >= 0")
    return int(result)


implementation = _ImplementationType()


def intern(string):
    _builtin()


meta_path = []


# TODO(T42692043) Put the standard library into the python binary instead.
path = [*_python_path, _base_dir + "/library", _base_dir + "/third-party/cpython/Lib"]


path_hooks = []


path_importer_cache = {}


prefix = ""


ps1 = ">>> "


ps2 = "... "


# TODO(T62600497): Enforce the recursion limit
def setrecursionlimit(limit):
    _builtin()


stderr = _IOStream(_stderr_fd)


stdin = _IOStream(_stdin_fd)


stdout = _IOStream(_stdout_fd)


warnoptions = []
