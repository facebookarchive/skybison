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
        if not sr.file_filter or sr.file_filter in cpython_file:
            lines = re.sub(sr.regex, sr.repl, lines)
    return lines


slot_to_func_type_map = {
    "tp_dealloc": "destructor",
    "tp_print": "printfunc",
    "tp_getattr": "getattrfunc",
    "tp_setattr": "setattrfunc",
    "tp_repr": "reprfunc",
    "tp_hash": "hashfunc",
    "tp_call": "ternaryfunc",
    "tp_str": "reprfunc",
    "tp_getattro": "getattrofunc",
    "tp_setattro": "setattrofunc",
    "tp_traverse": "traverseproc",
    "tp_clear": "inquiry",
    "tp_richcompare": "richcmpfunc",
    "tp_iter": "getiterfunc",
    "tp_iternext": "iternextfunc",
    "tp_descr_set": "descrgetfunc",
    "tp_descr_get": "descrsetfunc",
    "tp_alloc": "allocfunc",
    "tp_init": "initproc",
    "tp_new": "newfunc",
    "tp_free": "freefunc",
    "tp_is_gc": "inquiry",
    "tp_del": "destructor",
    "tp_finalize": "destructor",
    "nb_add": "binaryfunc",
    "nb_subtract": "binaryfunc",
    "nb_multiply": "binaryfunc",
    "nb_remainder": "binaryfunc",
    "nb_divmod": "binaryfunc",
    "nb_power": "ternaryfunc",
    "nb_negative": "unaryfunc",
    "nb_positive": "unaryfunc",
    "nb_absolute": "unaryfunc",
    "nb_bool": "inquiry",
    "nb_invert": "unaryfunc",
    "nb_lshift": "binaryfunc",
    "nb_rshift": "binaryfunc",
    "nb_and": "binaryfunc",
    "nb_xor": "binaryfunc",
    "nb_or": "binaryfunc",
    "nb_int": "unaryfunc",
    "nb_float": "unaryfunc",
    "nb_inplace_add": "binaryfunc",
    "nb_inplace_subtract": "binaryfunc",
    "nb_inplace_multiply": "binaryfunc",
    "nb_inplace_remainder": "binaryfunc",
    "nb_inplace_power": "ternaryfunc",
    "nb_inplace_lshift": "binaryfunc",
    "nb_inplace_rshift": "binaryfunc",
    "nb_inplace_and": "binaryfunc",
    "nb_inplace_xor": "binaryfunc",
    "nb_inplace_or": "binaryfunc",
    "nb_floor_divide": "binaryfunc",
    "nb_true_divide": "binaryfunc",
    "nb_inplace_floor_divide": "binaryfunc",
    "nb_inplace_true_divide": "binaryfunc",
    "nb_index": "unaryfunc",
    "nb_matrix_multiply": "binaryfunc",
    "nb_inplace_matrix_multiply": "binaryfunc",
    "sq_length": "lenfunc",
    "sq_concat": "binaryfunc",
    "sq_repeat": "ssizeargfunc",
    "sq_item": "ssizeargfunc",
    "sq_ass_item": "ssizeobjargproc",
    "sq_contains": "objobjproc",
    "sq_inplace_concat": "binaryfunc",
    "sq_inplace_repeat": "ssizeargfunc",
    "mp_length": "lenfunc",
    "mp_subscript": "binaryfunc",
    "mp_ass_subscript": "objobjargproc",
    "am_await": "unaryfunc",
    "am_aiter": "unaryfunc",
    "am_anext": "unaryfunc",
    "bf_getbuffer": "getbufferproc",
    "bf_releasebuffer": "releasebufferproc",
}


def replace_type_to_slot_match(match):
    # Check if a the accessor is via '->' or '.'
    ref = "&" if match.group("ref") == "." else ""
    return (
        f"PyType_GetSlot({ref}{match.group('type')}, Py_{match.group('slot')})"
    )


def replace_slot_to_callable_match(match):
    # Find the correct casting type for the slot
    func_type = slot_to_func_type_map[match.group("func_type")]
    return f"(({func_type}){match.group('void_ptr')}){match.group('extra_paren')}{match.group('args')}"


def replace_type_to_member_match(match):
    slot = match.group("slot")
    # Check if a the accessor is via '->' or '.'
    ref = "&" if match.group("ref") == "." else ""
    # Special handle tp_name and tp_dict to access through GetAttr
    attr_accessors = ["tp_name", "tp_dict", "tp_base", "tp_doct"]
    if slot in attr_accessors:
        if "tp_name" in slot:
            return f"((const char*)PyObject_GetAttrString({ref}{match.group('type')}, \"__name__\"))"
        return f"PyObject_GetAttrString({ref}{match.group('type')}, \"__{slot[3:]}__\")"
    return replace_type_to_slot_match(match)


slot_to_macro_map = {
    # Match variables that are accessing a slot
    # i.e: type->slot->nb_add => PyType_GetSlot(type, Py_nb_add)
    "type_to_slot_regex": SourceRegex(
        regex=re.compile(
            r"(?P<type>[a-zA-Z0-9_]+)(?P<ref>->|\.)(?P<slot_type>[a-zA-Z0-9_]+)->(?P<slot>(nb|sq|mp|am)_[a-z_]+)",
            re.MULTILINE,
        ),
        repl=replace_type_to_slot_match,
        file_filter=None,
    ),
    # Match variables that are accessing a slot through Py_TYPE
    # i.e: Py_TYPE(o)->slot->nb_add => PyType_GetSlot(Py_TYPE(o), Py_nb_add)
    "type_macro_to_slot_regex": SourceRegex(
        regex=re.compile(
            r"(?P<type>Py_TYPE\([a-zA-Z0-9_]+\))(?P<ref>->|\.)(?P<slot_type>[a-zA-Z0-9_]+)->(?P<slot>(nb|sq|mp|am)_[a-z_]+)",
            re.MULTILINE,
        ),
        repl=replace_type_to_slot_match,
        file_filter=None,
    ),
    # Match variables that are accessing a type slot
    # i.e: type->tp_doc => PyType_GetSlot(type, Py_tp_doc)
    "type_to_member_regex": SourceRegex(
        regex=re.compile(
            r"(?P<type>[a-zA-Z0-9_]+)(?P<ref>->|\.)(?P<slot>tp_[a-z_]+)",
            re.MULTILINE,
        ),
        repl=replace_type_to_member_match,
        file_filter=None,
    ),
    # Match variables that are accessing a type slot through Py_TYPE
    # i.e: Py_TYPE(o)->tp_doc => PyType_GetSlot(Py_TYPE(o), Py_tp_doc)
    "type_macro_to_member_regex": SourceRegex(
        regex=re.compile(
            r"(?P<type>Py_TYPE\([a-zA-Z0-9_]+\))(?P<ref>->|\.)(?P<slot>tp_[a-z_]+)",
            re.MULTILINE,
        ),
        repl=replace_type_to_member_match,
        file_filter=None,
    ),
    # Cast the slots that are doing a function call to the function type
    # i.e: PyType_GetSlot(type, Py_tp_alloc)(v, w) -> ((allocfunc)PyType_GetSlot(type, Py_tp_alloc))(v, w)
    "slot_to_callable_regex": SourceRegex(
        regex=re.compile(
            r"(?P<deref>\*)?(?P<void_ptr>PyType_GetSlot\(.*?, Py_(?P<func_type>[a-z_]*)\))(?P<extra_paren>.*)(?P<args>\(.*\))",
            re.MULTILINE,
        ),
        repl=replace_slot_to_callable_match,
        file_filter=None,
    ),
}


def slot_to_macro(lines, cpython_file):
    for sr in slot_to_macro_map.values():
        if not sr.file_filter or sr.file_filter in cpython_file:
            lines = re.sub(sr.regex, sr.repl, lines)
    return lines


def modify_cpython_source(source_dir):
    filter_source_files = lambda p: p.endswith(".c") or p.endswith(".h")
    file_transforms = [object_to_type, slot_to_macro]
    for subdir, _dirs, files in os.walk(source_dir):
        list_of_files = [os.path.join(subdir, x) for x in files]

        for cpython_file in list(filter(filter_source_files, list_of_files)):
            # Read file
            with open(cpython_file, "r", encoding="utf-8") as f:
                original_lines = f.read()

            # Run Transformations
            lines = original_lines
            for t in file_transforms:
                lines = t(lines, cpython_file)

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
