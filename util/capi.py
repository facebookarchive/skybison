#!/usr/bin/env python3
"""
Generate a report of C-API completeness for Pyro.
"""

import argparse
import functools
import itertools
import os
import pickle
import re
import subprocess
import sys
from pathlib import Path


CAPI_BLACKLIST = {
    "_Py_NoneStruct",  # expansion of Py_None
    "_Py_TrueStruct",  # expansion of Py_True
    "_Py_FalseStruct",  # expansion of Py_False
    "_Py_NotImplementedStruct",  # expansion of Py_NotImplemented
    "_PyLong_Zero",  # a const
    "_PyLong_One",  # a const
}

FBCODE_PLATFORM = "gcc-5-glibc-2.23"


# Yield all .o files in the given cpython directory that look like they're from
# a Module.
def module_object_files(dirname):
    for path, _dirs, filenames in os.walk(dirname):
        for filename in filenames:
            if filename.endswith(".o") and "Modules" in path:
                yield os.path.join(path, filename)


TEXT_RE = re.compile(r"\w+\sT (.+)$")


# Yield all externally-visible symbols defined in the text section of the given
# object file.
def find_defined(obj_file):
    output = subprocess.run(
        ["nm", "--defined-only", obj_file], stdout=subprocess.PIPE
    ).stdout.decode(sys.stdout.encoding)
    for line in output.strip().split("\n"):
        match = TEXT_RE.match(line)
        if not match:
            continue
        yield match[1]


# Return true if and only if the given function is exported by cpython and not
# defined by a C module.
def is_capi(name):
    return name in CAPI_DEFS and name not in CAPI_BLACKLIST


# One use of a C-API function, along with the filename it's used by (usually a
# .o or .so file).
class FunctionUse:
    def __init__(self, filename, name, raw_filename=False):
        self.filename = filename if raw_filename else self._process_filename(filename)
        self.name = name

    MODULES_RE = re.compile(r"Modules/(.+\.o)$")
    TP2_RE = re.compile(fr"/([^/]+)/(?:[^/]+)/{FBCODE_PLATFORM}/.+/([^/]+\.so)$")
    INSTAGRAM_RE = re.compile(r"/((?:distillery|site-packages|lib-dynload)/.+\.so)$")
    UWSGI_RE = re.compile(r"uWSGI/master/src/build-.+/([^/]+\.o)")

    @classmethod
    def _process_filename(cls, filename):
        match = cls.MODULES_RE.search(filename)
        if match:
            return f"cpython/{match[1]}"
        match = cls.TP2_RE.search(filename)
        if match:
            return f"{match[1]}/{match[2]}"
        match = cls.INSTAGRAM_RE.search(filename)
        if match:
            return match[1]
        match = cls.UWSGI_RE.search(filename)
        if match:
            return f"uwsgi/{match[1]}"
        raise ValueError(f"Unknown path format: '{filename}'")

    def __repr__(self):
        return f"{self.filename}: {self.name} {self.status}"

    def __hash__(self):
        return hash((self.filename, self.name))

    def __eq__(self, other):
        return self.filename == other.filename and self.name == other.name


UNDEF_RE = re.compile(r"\s*U (_?Py.+)$")


# Yield all C-API functions used by the given object file. This includes
# symbols that meet all of the following:
# - Marked as undefined by nm
# - Starts with _Py or Py
# - In CAPI_DEFS
# - Not in CAPI_BLACKLIST
def find_used_capi(obj_file):
    output = subprocess.run(
        ["nm", "--undefined-only", obj_file], stdout=subprocess.PIPE
    ).stdout.decode(sys.stdout.encoding)
    for line in output.strip().split("\n"):
        match = UNDEF_RE.match(line)
        if not match:
            continue
        function = match[1]
        if not is_capi(function):
            continue
        yield FunctionUse(obj_file, function)


# Read a precomputed list of C-API functions used by Instagram from the given
# fbsource checkout.
def read_insta_used_capi(fbsource_path):
    with open(
        os.path.join(
            fbsource_path,
            "fbcode/experimental/pyro/cpython/tools/instagram_cpython_symbols.pkl",
        ),
        "rb",
    ) as pkl_file:
        for name in filter(is_capi, pickle.load(pkl_file)):
            yield FunctionUse("instagram.so", name)


# Yield C-API functions used by any .so or .o file recursively found in
# root. If platform is given, restrict the search to object files that look
# like they're from the given fbcode platform.
def find_obj_used_capi(root, platform=None):
    if not Path(root).exists():
        raise RuntimeError(f"Path '{root}' does not exist")
    for path, _dirs, files in os.walk(root):
        for file in files:
            if (file.endswith(".so") or file.endswith(".o")) and (
                platform is None or f"/{FBCODE_PLATFORM}/" in path
            ):
                yield from find_used_capi(os.path.join(path, file))


def read_raw_csv(filename):
    result = []
    with open(filename, "r") as file:
        lines = iter(file)
        next(lines)
        for line in lines:
            parts = line.strip().split(",")
            if len(parts) != 5:
                sys.exit("Malformed raw.csv.")
            result.append(FunctionUse(parts[0], parts[3], raw_filename=True))
    return result


def read_func_list(filename):
    return [
        FunctionUse("ext.so", line.strip(), raw_filename=True)
        for line in open(filename, "r")
    ]


def process_modules(args):
    global CAPI_BLACKLIST, CAPI_DEFS
    cpython_path = args.cpython_path
    if not cpython_path or not Path(cpython_path).exists():
        sys.exit("Please provide a valid cpython path.")

    # Remove functions defined in the Modules directory from the search
    # space. This includes things like hash table implementations, etc.
    module_objs = list(module_object_files(cpython_path))
    for obj in module_objs:
        CAPI_BLACKLIST |= set(find_defined(obj))
    assert "_Py_hashtable_clear" in CAPI_BLACKLIST

    CAPI_DEFS = set(find_defined(os.path.join(cpython_path, "python")))
    assert CAPI_DEFS

    used_capi = set()
    if args.scan_modules:
        used_capi |= set(itertools.chain(*[find_used_capi(obj) for obj in module_objs]))
    if args.tp2:
        used_capi |= set(find_obj_used_capi(args.tp2, platform=FBCODE_PLATFORM))
    for so_dir in args.objs:
        used_capi |= set(find_obj_used_capi(so_dir))
    if not used_capi:
        sys.exit("No C-API functions found.")
    return list(used_capi)


GROUP_RE = re.compile(r"_?Py([A-Za-z]+)(_|$)")


# Return the group of the given function, which we've defined as the characters
# between 'Py' and '_' in PyFoo_Mumble.
def group_of(function):
    found = GROUP_RE.match(function.name)
    return found.group(1) if found else "<nogroup>"


# Return true if a function needs to be implemented in Pyro, meaning at least
# one of the following is true:
# - An UNIMPLEMENTED line with the given name was found in the source.
# - The funtion's name does not appear in the source.
@functools.lru_cache(maxsize=None, typed=False)
def is_todo(name):
    has_unimplemented = (
        subprocess.run(
            ["grep", "-rqF", f'UNIMPLEMENTED("{name}")', *PYRO_CPP_FILES],
            stdout=subprocess.PIPE,
        ).returncode
        == 0
    )  # 0 means it has been found
    # TODO: Use a better regular expression to actually check for signature
    definition_found = (
        subprocess.run(
            ["grep", "-qF", f"{name}(", *PYRO_CPP_FILES], stdout=subprocess.PIPE
        ).returncode
        == 0
    )  # 0 means it has been found
    return has_unimplemented or not definition_found


def list_functions(funcs):
    for func in sorted(set(map(lambda f: f.name, funcs))):
        print(func)


def print_summary(funcs):
    funcs = set(map(lambda f: f.name, funcs))
    completed_funcs = list(filter(lambda f: not is_todo(f), funcs))
    percent = len(completed_funcs) * 100 / len(funcs)
    print(f"{len(completed_funcs)} / {len(funcs)} complete ({percent:.1f}%)")


def write_grouped_csv(filename, funcs, group_key, summary=False):
    grouped = itertools.groupby(sorted(funcs, key=group_key), key=group_key)
    grouped_dict = {k: sorted(list(g), key=lambda x: x.name) for k, g in grouped}

    total_completed = 0
    total_functions = 0

    # group_name -> (num_completed, num_functions, percent_complete)
    group_completion = {}

    for group_name, functions in grouped_dict.items():
        functions = {f.name for f in functions}
        completed_functions = set(filter(lambda f: not is_todo(f), functions))

        num_completed = len(completed_functions)
        num_functions = len(functions)
        percent_complete = 100 * num_completed // num_functions
        group_completion[group_name] = (num_completed, num_functions, percent_complete)

        total_completed += num_completed
        total_functions += num_functions

    total_percent_complete = 100 * total_completed // total_functions

    with open(filename, "w") as out_file:
        print("name,num_completed,total_functions,percent_complete", file=out_file)
        if summary:
            print(
                f"SUMMARY,{total_completed},{total_functions},{total_percent_complete}",
                file=out_file,
            )

        for group_name, stats in group_completion.items():
            print(f"{group_name}," + ",".join(map(str, stats)), file=out_file)

    print(f"Wrote {filename}", file=sys.stderr)


def write_raw_csv(filename, funcs):
    funcs.sort(key=lambda f: f.name)
    funcs.sort(key=lambda f: f.filename)
    with open(filename, "w") as out_file:
        print("module,module_dir,group,function,todo", file=out_file)
        for func in funcs:
            todo = "1" if is_todo(func.name) else "0"
            module_dir = "/".join(func.filename.split("/")[0:-1])
            print(
                f"{func.filename},{module_dir},{group_of(func)},{func.name},{todo}",
                file=out_file,
            )

    print(f"Wrote {filename}", file=sys.stderr)


def parse_args():
    parser = argparse.ArgumentParser(
        description="""
Analyze the completeness of Pyro's C-API implementation. Unless one of the
Actions arguments is given, print a summary line showing how many C-API
functions are implemented out of the total needed.
"""
    )

    parser.add_argument("--show-args", action="store_true", help=argparse.SUPPRESS)

    libs = parser.add_argument_group("Source/library arguments")
    libs.add_argument(
        "cpython_path",
        nargs="?",
        help="Root of a built cpython tree. Must be provided unless "
        "--update-csv is also given.",
    )
    libs.add_argument(
        "--pyro",
        help="Pyro source tree. Defaults to the checkout containing this script.",
    )
    libs.add_argument(
        "--no-modules",
        action="store_false",
        dest="scan_modules",
        help="Don't look for C-API uses in cpython/Modules.",
    )
    libs.add_argument(
        "--tp2",
        help=f"Root of a tp2 checkout to scan for .so files. Only versions "
        "from platform {FBCODE_PLATFORM} will be considered.",
    )
    libs.add_argument(
        "--objs",
        action="append",
        default=[],
        help="Root of any directory to scan for .so or .o files. May be given "
        "multiple times.",
    )
    libs.add_argument(
        "--read-csv",
        help="Read function uses from raw.csv from a previous run, rather "
        "than inspecting object files. Proceed as usual after, including "
        "writing out new .csv files if -csv is given.",
    )
    libs.add_argument(
        "--read-func-list",
        help="Like --read-csv, but read a newline-separate list of functions,"
        " rather than a full raw csv file. All functions will appear to be "
        "used by the same module.",
    )

    actions = parser.add_argument_group("Actions")
    actions.add_argument(
        "--csv",
        help="""
In addition to printing the summary line, write out a few csv files: 1)
groups.csv: One line per function group, with completion stats. 2)
modules.csv: Like groups.csv, but grouped by module. 3) raw.csv: Raw
data, one line per function use.
""",
        action="store_true",
    )
    actions.add_argument(
        "--output",
        "-o",
        help="Set the output directory for the --csv option. Defaults to cwd.",
        default=".",
    )
    actions.add_argument(
        "--list-funcs", action="store_true", help="List all C-API functions."
    )
    actions.add_argument(
        "--list-todo",
        action="store_true",
        help="List all unimplemented C-API functions.",
    )
    return parser.parse_args()


def find_pyro_impl_files(args):
    if args.pyro:
        path = os.path.join(args.pyro, "ext")
    else:
        path = os.path.join(os.path.dirname(os.path.realpath(__file__)), "..", "ext")
    if not Path(path).exists():
        raise RuntimeError(f"Pyro source path {path} does not exist")

    return subprocess.run(
        ["find", path, "-name", "*.cpp"],
        stdout=subprocess.PIPE,
        encoding=sys.stdout.encoding,
        check=True,
    ).stdout.split()


def main():
    args = parse_args()
    if args.list_funcs + args.list_todo + args.csv > 1:
        sys.exit("Don't supply more than one Action argument")
    if args.show_args:
        sys.exit(args)

    global PYRO_CPP_FILES
    PYRO_CPP_FILES = find_pyro_impl_files(args)
    if not PYRO_CPP_FILES:
        # Tolerate running on revs with no cpp files in ext/
        PYRO_CPP_FILES = ["/dev/null"]

    if args.read_csv:
        used_capi = read_raw_csv(args.read_csv)
    elif args.read_func_list:
        used_capi = read_func_list(args.read_func_list)
    else:
        used_capi = process_modules(args)

    if args.list_funcs:
        list_functions(used_capi)
        return 0

    if args.list_todo:
        list_functions(filter(lambda f: is_todo(f.name), used_capi))
        return 0

    if args.csv:
        output_dir = Path(args.output)
        output_dir.mkdir(parents=True, exist_ok=True)

        def outfile(f):
            return os.path.join(output_dir, f)

        write_grouped_csv(outfile("groups.csv"), used_capi, group_of)
        write_grouped_csv(outfile("modules.csv"), used_capi, lambda f: f.filename)
        write_raw_csv(outfile("raw.csv"), used_capi)

        print_summary(used_capi)
        return 0

    print_summary(used_capi)
    return 0


if __name__ == "__main__":
    sys.exit(main())
