#!/usr/bin/env python3
from _builtins import _int_check_exact, _profiler_exclude, _profiler_install


next_id = 0


class CodeInfo:
    """Profile information about a single code object."""

    def __init__(self):
        global next_id
        self.id = next_id
        next_id += 1

        self.called = 0
        self.function = None
        self.self = 0
        self.inclusive = 0
        # List of callees: This list alternates between code object and
        # corresponding call count.
        self.callees = []

    def clear(self):
        self.called = 0
        self.function = None
        self.self = 0
        self.inclusive = 0
        self.callees = []

    def is_empty(self):
        return (
            self.called == 0
            and self.self == 0
            and self.inclusive == 0
            and len(self.callees) == 0
        )


class CodeInfoDict(dict):
    """Dictionary mapping code object to corresponding CodeInfo instances.
    Note that this is per-thread information. There are code info dictionaries
    for each thread, and one code object can have multiple CodeInfos associated;
    at most 1 per thread."""

    def __missing__(self, key):
        value = CodeInfo()
        dict.__setitem__(self, key, value)
        return value


class FrameInfo:
    """Profiling information about a currently active call/callframe. Instances
    of this type are pushed to the `ThreadInfo.shadow_stack` as the program
    is executing."""

    def __init__(self, begin):
        self.begin = begin
        self.self_begin = begin
        self.self_amount = 0


class ThreadInfo:
    """Profiling information about a single thread."""

    def __init__(self):
        self.code_infos = CodeInfoDict()
        self.shadow_stack = []


_main_thread_info = None


def _new_thread():
    global _main_thread_info
    assert _main_thread_info is None
    _main_thread_info = ThreadInfo()
    return _main_thread_info


def _is_native(code):
    return _int_check_exact(code.co_code)


def _call(thread_data, from_function, to_function, opcodes):
    to_code = to_function.__code__
    code_infos = thread_data.code_infos
    to_info = code_infos[to_code]
    to_info.called += 1

    to_info_function = to_info.function
    if to_info_function is None:
        to_info.function = to_function
    elif to_info_function is not to_function:
        # Multiple functions appear to share the same code object
        to_info.function = "multiple"

    if from_function is not None:
        from_code = from_function.__code__
        from_info = code_infos[from_code]
        # Add entry with callee code and call count to callees list.
        callees = from_info.callees
        i = 0
        callees_len = len(callees)
        while i < callees_len:
            if callees[i] is to_code:
                callees[i + 1] += 1
                break
            i += 2
        else:
            callees.append(to_code)
            callees.append(1)

    if _is_native(to_code):
        return

    shadow_stack = thread_data.shadow_stack
    if shadow_stack:
        last = shadow_stack[-1]
        assert last.self_begin >= 0
        last.self_amount += opcodes - last.self_begin
        last.self_begin = -1

    frame_info = FrameInfo(opcodes)
    shadow_stack.append(frame_info)


def _return(thread_data, from_function, to_function, opcodes):
    from_code = from_function.__code__
    if _is_native(from_code):
        return

    shadow_stack = thread_data.shadow_stack
    if not shadow_stack:
        return

    frame_info = shadow_stack.pop()

    from_info = thread_data.code_infos[from_code]
    from_info.inclusive += opcodes - frame_info.begin
    from_info.self += frame_info.self_amount + (opcodes - frame_info.self_begin)

    if shadow_stack:
        to_frame_info = shadow_stack[-1]
        to_frame_info.self_begin = opcodes


def install():
    _profiler_install(_new_thread, _call, _return)


def uninstall():
    _profiler_install(None, None, None)


def _info_name(code, code_info):
    name = code.co_name
    function = code_info.function
    if function is not None and "multiple" != function:
        module = getattr(function, "__module__", "?")
        qualname = getattr(function, "__qualname__", name)
        return f"{module}.{qualname}"
    return name


def _clear():
    code_infos = _main_thread_info.code_infos
    for info in code_infos.values():
        info.clear()


class _DumpCallgrind:
    def __init__(self, fp):
        self.fp = fp
        self.current_file = {}
        self.file_ids = {}
        self.func_ids = {}

    def file(self, prefix, filename):
        if filename.strip() == "":
            filename = "<empty>"
        file_ids = self.file_ids
        file_id = file_ids.get(filename)
        if file_id is None:
            file_id = len(file_ids)
            file_ids[filename] = file_id
            self.fp.write(f"{prefix}=({file_id}) {filename}\n")
        elif self.current_file.get(prefix) != file_id:
            self.fp.write(f"{prefix}=({file_id})\n")
        self.current_file[prefix] = file_id

    def func(self, prefix, code, info):
        func_ids = self.func_ids
        func_id = func_ids.get(info.id)
        if func_id is None:
            func_id = len(func_ids)
            func_ids[info.id] = func_id
            name = _info_name(code, info)
            self.fp.write(f"{prefix}=({func_id}) {name}\n")
        else:
            self.fp.write(f"{prefix}=({func_id})\n")

    def write(self, s):
        self.fp.write(s)


def _dump_callgrind_to_fp(fp):
    code_infos = _main_thread_info.code_infos
    to_print = []
    for code, info in code_infos.items():
        if info.is_empty():
            continue
        to_print.append((code, info))
    to_print.sort(key=lambda d: d[0].co_name)

    env = _DumpCallgrind(fp)
    env.write(
        """\
# callgrind format
version: 1
creator: _profiler
positions: line
events: Op

"""
    )
    for code, info in to_print:
        env.file("fl", code.co_filename)
        env.func("fn", code, info)
        if not _is_native(code):
            value = info.self
        else:
            value = info.called
        lineno = code.co_firstlineno
        env.write(f"{lineno} {value}\n")

        callees = info.callees
        callees_len = len(callees)
        # Iterate through callees: Note that the list alternates between
        # code object and corresponding call count (which is more efficient
        # than storing tuples).
        for c in range(0, callees_len, 2):
            callee = callees[c]
            called = callees[c + 1]
            callee_info = code_infos[callee]
            env.file("cfi", callee.co_filename)
            env.func("cfn", callee, callee_info)
            env.write(f"calls={called} {callee.co_firstlineno}\n")
            if not _is_native(callee):
                inclusive = callee_info.inclusive
            else:
                inclusive = callee_info.called
            if inclusive > 0:
                inclusive = int(called * (inclusive / callee_info.called))
            env.write(f"{lineno} {inclusive}\n")
        env.write("\n")


def _dump_callgrind_impl(filename, clear):
    with open(filename, "w") as fp:
        _dump_callgrind_to_fp(fp)
    if clear:
        _clear()


def dump_callgrind(filename, clear=True):
    _profiler_exclude(lambda: _dump_callgrind_impl(filename, clear))
