#!/usr/bin/env python3
import argparse
import collections
import os
import re
import sys

# Tuple that specifies a regex, its replacement, and a file filter
SourceRegex = collections.namedtuple(
    "SourceRegex", ["regex", "repl", "file_filter"]
)


def replace_object_to_type_match(match):
    # Check if a the accessor is via '->' or '.'
    # i.e: Obj.ob_type => Py_Type(&Obj) - obj->ob_type => Py_TYPE(obj)
    ref = "&" if match.group("ref") == "." else ""
    return f"Py_TYPE({ref}{match.group('object')})"


object_to_type_map = {
    # Match variables that are contained within parenthesis
    # i.e: (obj)->ob_type->tp_as_buffer => Py_TYPE(obj)
    "object_parenthesis_to_type_regex": SourceRegex(
        regex=re.compile(
            r"\((?P<object>[a-zA-Z0-9_]+?)\)(?P<ref>->|\.)ob_type", re.MULTILINE
        ),
        repl=replace_object_to_type_match,
        file_filter=None,
    ),
    # Match variables that are accessing ob_type
    # i.e: obj->ob_type => Py_TYPE(obj)
    "object_to_type_regex": SourceRegex(
        regex=re.compile(
            r"(?P<object>[a-zA-Z0-9_\->\.\[\]]+?)(?P<ref>->|\.)ob_type",
            re.MULTILINE,
        ),
        repl=replace_object_to_type_match,
        file_filter=None,
    ),
    # Special case for sha512module.c
    "object_to_type_shamodule_regex": SourceRegex(
        regex=re.compile(r"\(\(PyObject\*\)self\)->ob_type", re.MULTILINE),
        repl=r"Py_TYPE(self)",
        file_filter="sha512module.c",
    ),
}


def object_to_type(lines, cpython_file):
    for sr in object_to_type_map.values():
        if (sr.file_filter or "") in cpython_file:
            lines = re.sub(sr.regex, sr.repl, lines)
    return lines


def modify_cpython_source(source_dir):
    filter_source_files = lambda p: p.endswith(".c") or p.endswith(".h")
    for subdir, _dirs, files in os.walk(source_dir):
        list_of_files = [os.path.join(subdir, x) for x in files]

        for cpython_file in list(filter(filter_source_files, list_of_files)):
            # Read file
            with open(cpython_file, "r", encoding="utf-8") as f:
                original_lines = f.read()

            # Run Transformations
            lines = object_to_type(original_lines, cpython_file)

            # Save File
            if lines != original_lines:
                with open(cpython_file, "w+", encoding="utf-8") as f:
                    f.writelines(lines)


def main(args):
    parser = argparse.ArgumentParser(
        description="Cleanup the invalid object accesses in CPython code"
    )
    parser.add_argument(
        "-s",
        "--source_dir",
        required=True,
        help="The directory with source files that will be modified",
    )
    args = parser.parse_args()
    modify_cpython_source(args.source_dir)


if __name__ == "__main__":
    main(sys.argv)
