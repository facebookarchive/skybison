#!/usr/bin/env python3
import argparse
import collections
import os
import re
import sys
from shutil import copyfile


# Tuple that specifies a regex and the position where a symbol will be found
SymbolRegex = collections.namedtuple("SymbolRegex", ["regex", "pos"])


# For sources where only the symbol is required
HEADER_SYMBOL_REGEX = {
    "typedef": SymbolRegex(
        regex=re.compile("^typedef.*;", re.MULTILINE), pos=2
    ),
    "multiline_typedef": SymbolRegex(
        regex=re.compile("^} .*;", re.MULTILINE), pos=-1
    ),
    "struct": SymbolRegex(regex=re.compile("^struct.*{", re.MULTILINE), pos=1),
    "macro": SymbolRegex(
        regex=re.compile("^#define.*[^\\\\]\n", re.MULTILINE), pos=1
    ),
    "multiline_macro": SymbolRegex(
        regex=re.compile("^#define.*\\\\", re.MULTILINE), pos=1
    ),
    "pytypeobject_macro": SymbolRegex(
        regex=re.compile("^#define.*_Type ", re.MULTILINE), pos=1
    ),
    "pyexc_macro": SymbolRegex(
        regex=re.compile("^#define PyExc_\w+ ", re.MULTILINE), pos=1
    ),
}

SOURCE_SYMBOL_REGEX = {
    "pytypeobject": SymbolRegex(
        regex=re.compile('^extern "C".*PyTypeObject.*_Type', re.MULTILINE),
        pos=3,
    ),
    "pyfunction": SymbolRegex(
        regex=re.compile('^extern "C"(?:[^{]|\n)*{', re.MULTILINE), pos=3
    ),
}

# For sources where the entire definition match is required
HEADER_DEFINITIONS_REGEX = {
    "typedef": SymbolRegex(
        regex=re.compile("^typedef.*;.*\n", re.MULTILINE), pos=2
    ),
    "multiline_typedef": SymbolRegex(
        regex=re.compile("^typedef.*{(.|\n)*?}.*;.*\n", re.MULTILINE), pos=-1
    ),
    "struct": SymbolRegex(
        regex=re.compile("^struct(.|\n)*?};.*\n", re.MULTILINE), pos=1
    ),
    "macro": SymbolRegex(
        regex=re.compile("^#define.*[^\\\\]\n", re.MULTILINE), pos=1
    ),
    "multiline_macro": SymbolRegex(
        regex=re.compile("^#define.*\\\\(\n.*\\\\)*\n.*\n", re.MULTILINE), pos=1
    ),
    "pytypeobject_macro": SymbolRegex(
        regex=re.compile("^PyAPI_DATA\(PyTypeObject.*;.*\n", re.MULTILINE),
        pos=2,
    ),
    "pyexc_macro": SymbolRegex(
        regex=re.compile(
            "^PyAPI_DATA\(PyObject.*(PyExc_\w+).*\n", re.MULTILINE
        ),
        pos=2,
    ),
}

SOURCE_DEFINITIONS_REGEX = {
    "pytypeobject": SymbolRegex(
        regex=re.compile("^PyTypeObject.*= {(.|\n)*?};.*\n", re.MULTILINE),
        pos=1,
    ),
    "pyfunction": SymbolRegex(
        regex=re.compile("^[a-zA-Z](.|.\n)*?{\n\s(.|\n)*?\n}", re.MULTILINE),
        pos=1,
    ),
}


SPECIAL_CHARS_REGEX = re.compile("[\*|,|;|\(|\)|]")


def remove_extra_chars(match):
    # Modify typedefs with the following signature:
    # type (*name)(variables) -> type (*name variables)
    modified_match = re.sub("\)\(", " ", match)
    # Modify return types longer than one
    # unsigned long long -> unsigned_long_long
    modified_match = re.sub("static ", "static_", modified_match)
    modified_match = re.sub("long long", "long_long", modified_match)
    modified_match = re.sub("unsigned ", "unsigned_", modified_match)
    modified_match = re.sub("const ", "const_", modified_match)
    # Remove extra characters to standardize symbol location
    # type (*name variables...) -> type name variables
    modified_match = re.sub(SPECIAL_CHARS_REGEX, " ", modified_match)
    return modified_match


# Given a source file, find the matched patterns
def find_symbols_in_file(lines, regex_dict):
    symbols_dict = {x: [] for x in regex_dict.keys()}
    for symbol_type, sr in regex_dict.items():
        matches = re.findall(sr.regex, lines)
        for match in matches:
            # Split and locate symbol based on its position
            modified_match = remove_extra_chars(match).split()
            if len(modified_match) > sr.pos:
                symbols_dict[symbol_type].append(modified_match[sr.pos])
    return symbols_dict


# Given a list of files, find all the defined symbols
def create_source_symbols_dict(modified_source_paths):
    source_dict = {}
    for path in modified_source_paths:
        if not path.endswith(".cpp"):
            continue
        with open(path, "r") as f:
            lines = f.read()
        symbols_dict = find_symbols_in_file(lines, SOURCE_SYMBOL_REGEX)
        if any(len(x) for x in symbols_dict.values()):
            c_path = os.path.basename(path).replace(".cpp", ".c")
            source_dict[c_path] = symbols_dict
    return source_dict


# Given a list of files, find all the defined symbols
def create_header_symbols_dict(modified_source_paths):
    symbols_dict = {}
    for path in modified_source_paths:
        if not path.endswith(".h"):
            continue
        with open(path, "r") as f:
            lines = f.read()
        result_dict = find_symbols_in_file(lines, HEADER_SYMBOL_REGEX)
        for k, v in result_dict.items():
            symbols_dict.setdefault(k, []).extend(v)
    return symbols_dict


# The set of heuristics to determine if a substitution should be performed
def replace_definition_if(match, symbol, pos):
    original_match = match.group(0)
    # Split and locate symbol based on its position
    modified_match = remove_extra_chars(original_match).split()
    # Verify that this is indeed the definition for symbol
    if symbol == modified_match[pos]:
        return ""
    return original_match


# Given a source file, replace the matched patterns
def modify_file(lines, symbols_dict, regex_dict):
    # Iterate dictionary of symbol types (i.e. macro, typedef, etc.)
    for symbol_type, symbols in symbols_dict.items():
        sr = regex_dict[symbol_type]
        # Iterate the symbols that will be replaced
        for symbol in symbols:
            lines = re.sub(
                sr.regex,
                lambda m: replace_definition_if(m, symbol, sr.pos),
                lines,
            )
    return lines


# Given a list of sources files, modify the patterns that were annotated
def create_output_file_dict(
    source_paths, header_symbols_dict, source_symbols_dict
):
    output_file_dict = {}
    # Iterate all source files
    for path in source_paths:
        output_file_dict[path] = None

        # Modules should not be patched. This quickly skips over them
        # to just copy the file directly.
        if "Modules" in path:
            continue

        if path.endswith(".h"):
            with open(path, "r") as f:
                lines = f.read()
            output_file_dict[path] = modify_file(
                lines, header_symbols_dict, HEADER_DEFINITIONS_REGEX
            )

        if path.endswith(".c"):
            file_name = os.path.basename(path)
            if file_name in source_symbols_dict.keys():
                with open(path, "r") as f:
                    lines = f.read()
                output_file_dict[path] = modify_file(
                    lines,
                    source_symbols_dict[file_name],
                    SOURCE_DEFINITIONS_REGEX,
                )
    return output_file_dict


# Given a dictionary of files, write the files to the output directory
def output_files_to_directory(output_directory, output_file_dict):
    for file_path, lines in output_file_dict.items():
        # Create directory
        output_file_path = file_path.split("cpython")[-1]

        # Special handle Python.h
        if "Python.h" in file_path:
            if "ext" not in file_path:
                continue
            output_file_path = file_path.split("ext")[-1]

        output_gen_path = f"{output_directory}/cpython{output_file_path}"
        if not os.path.exists(os.path.dirname(output_gen_path)):
            os.makedirs(os.path.dirname(output_gen_path))

        # Copy those files that were not modified
        if lines is None:
            copyfile(file_path, output_gen_path)
            continue

        # Write files
        with open(output_gen_path, "w+", encoding="utf-8") as f:
            f.writelines(lines)


def main(args):
    parser = argparse.ArgumentParser(
        description="This strips out the unneeded definitions in the CPython Extension Subsystem"
    )
    parser.add_argument(
        "-sources",
        nargs="*",
        required=True,
        help="A list of sources from a clean CPython source",
    )
    parser.add_argument(
        "-modified",
        nargs="*",
        required=True,
        help="A list of sources that modify the clean CPython sources",
    )
    parser.add_argument(
        "-output_dir",
        required=True,
        help="The directory in which to output the processed sources",
    )
    args = parser.parse_args()

    header_symbols_dict = create_header_symbols_dict(args.modified)
    source_symbols_dict = create_source_symbols_dict(args.modified)
    output_file_dict = create_output_file_dict(
        args.sources, header_symbols_dict, source_symbols_dict
    )
    output_files_to_directory(args.output_dir, output_file_dict)


if __name__ == "__main__":
    main(sys.argv)
