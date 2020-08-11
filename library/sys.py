#!/usr/bin/env python3
# $builtin-init-module$

from builtins import SimpleNamespace as _SimpleNamespace, _structseq_new_type

from _builtins import _builtin, _get_asyncgen_hooks, _int_check, _os_write, _Unbound
from _path import dirname as _dirname, join as _join


# These values are all injected by our boot process. flake8 has no knowledge
# about their definitions and will complain without these lines.
_stderr_fd = _stderr_fd  # noqa: F821
_stdin_fd = _stdin_fd  # noqa: F821
_stdout_fd = _stdout_fd  # noqa: F821
_version_releaselevel = _version_releaselevel  # noqa: F821
hexversion = hexversion  # noqa: F821


_Flags = _structseq_new_type(
    "sys.flags",
    (
        "debug",
        "inspect",
        "interactive",
        "optimize",
        "dont_write_bytecode",
        "no_user_site",
        "no_site",
        "ignore_environment",
        "verbose",
        "bytes_warning",
        "quiet",
        "hash_randomization",
        "isolated",
        "dev_mode",
        "utf8_mode",
    ),
)


_FloatInfo = _structseq_new_type(
    "sys.float_info",
    (
        "max",
        "max_exp",
        "max_10_exp",
        "min",
        "min_exp",
        "min_10_exp",
        "dig",
        "mant_dig",
        "epsilon",
        "radix",
        "rounds",
    ),
)


_HashInfo = _structseq_new_type(
    "sys.hash_info",
    (
        "width",
        "modulus",
        "inf",
        "nan",
        "imag",
        "algorithm",
        "hash_bits",
        "seed_bits",
        "cutoff",
    ),
)

_AsyncGenHooks = _structseq_new_type("sys.asyncgen_hooks", ("firstiter", "finalizer"))


class _IOStream:
    """Simple io-stream implementation. We will replace this with
    _io.TextIOWrapper later."""

    def __init__(self, fd):
        self._fd = fd
        self.encoding = "utf-8"

    def fileno(self):
        return self._fd

    def flush(self):
        pass

    def write(self, text):
        text_bytes = text.encode("utf-8")
        _os_write(self._fd, text_bytes)


_VersionInfo = _structseq_new_type(
    "sys.version_info", ("major", "minor", "micro", "releaselevel", "serial")
)


def _init(_executable, _python_path, _flags_data):
    global executable
    executable = _executable

    global prefix
    prefix = _join(_dirname(executable), "..")
    global base_exec_prefix
    base_exec_prefix = prefix
    global base_prefix
    base_prefix = prefix
    global exec_prefix
    exec_prefix = prefix

    global path
    path = _python_path
    stdlib_dir = _join(
        prefix, "lib", f"{implementation.name}{_version.major}.{_version.minor}"
    )
    path.append(stdlib_dir)

    global flags
    flags = _Flags(_flags_data)


def __displayhook__(value):
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


_framework = ""


_version = _VersionInfo(
    (
        (hexversion >> 24) & 0xFF,  # major
        (hexversion >> 16) & 0xFF,  # minor
        (hexversion >> 8) & 0xFF,  # micro
        _version_releaselevel,  # releaselevel
        hexversion & 0x0F,  # serial
    )
)


def _getframe(depth=0):
    _builtin()


abiflags = ""


base_exec_prefix = None  # will be set by _init


base_prefix = None  # will be set by _init


copyright = ""


displayhook = __displayhook__


dont_write_bytecode = False


def exc_info():
    _builtin()


def excepthook(exc, value, tb):
    _builtin()


exec_prefix = None  # will be set by _init


executable = None  # will be set by _init


def exit(code=_Unbound):
    if code is _Unbound:
        raise SystemExit()
    raise SystemExit(code)


flags = None  # will be set by _init


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
    return "surrogatepass"


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


implementation = _SimpleNamespace(
    cache_tag=f"pyro-{_version.major}{_version.minor}", name="pyro", version=_version
)


def intern(string):
    _builtin()


def is_finalizing():
    _builtin()


meta_path = []


path = None  # will be set by _init


path_hooks = []


path_importer_cache = {}


platlibdir = "lib"


prefix = None  # will be set by _init


ps1 = ">>> "


ps2 = "... "


# TODO(T62600497): Enforce the recursion limit
def setrecursionlimit(limit):
    _builtin()


stderr = _IOStream(_stderr_fd)


stdin = _IOStream(_stdin_fd)


stdout = _IOStream(_stdout_fd)


version_info = _version


warnoptions = []


def _program_name():
    _builtin()


def _calculate_path():
    """Returns a tuple representing (prefix, exec_prefix, module_search_path)"""
    # TODO(T61328507): Implement the path lookup algorithm. In the meantime, return
    # the compiled-in defaults that CPython returns when run out of a build directory.
    return "/usr/local", "/usr/local", ""


def get_asyncgen_hooks():
    return _AsyncGenHooks(_get_asyncgen_hooks())


def set_asyncgen_hooks(firstiter=_Unbound, finalizer=_Unbound):
    _builtin()
