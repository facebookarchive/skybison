#!/usr/bin/env python3.6

import argparse
import importlib
import os
import pathlib
import re
import sys
import tempfile
from collections import namedtuple
from types import CodeType


INIT_MODULE_NAME = "__init_module__"


# Pyro specific flag to mark code objects for builtin functions.
# (see also `runtime/objects.h`)
CO_BUILTIN = 0x200000


def builtin_function_template():
    _builtin()  # noqa: F821


def builtin_function_template_with_docu():
    """template with docu comment"""
    _builtin()  # noqa: F821


def mark_native_functions(code, builtins, module_name, fqname=()):
    """Checks whether the code object or recursively any code objects in the
    constants tuple only contains a `_builtin()` call and should be marked
    as a native function. If so an entry is appended to the `builtins` list
    and the code object is marked with the CO_BUILTIN flag."""
    if code.co_names == ("_builtin",) and (
        code.co_code == builtin_function_template.__code__.co_code
        or code.co_code == builtin_function_template_with_docu.__code__.co_code
    ):
        assert code.co_stacksize == 1
        assert code.co_freevars == ()
        assert code.co_cellvars == ()
        # INIT_MODULE_NAME is magic and should not be used.
        assert code.co_name != INIT_MODULE_NAME

        if len(fqname) == 1:
            builtin_cpp_id = f"FUNC({module_name}, {fqname[0]})"
        elif len(fqname) == 2:
            builtin_cpp_id = f"METH({fqname[0]}, {fqname[1]})"
        else:
            raise Exception(f"Don't know how to create C++ name for {fqname}")
        builtin_num = len(builtins)
        builtins.append(builtin_cpp_id)

        # We abuse the stacksize field to store the index of the builtin
        # function. This way we do not need to invent a new marshaling type
        # just for this.
        stacksize = builtin_num
        flags = code.co_flags | CO_BUILTIN
        bytecode = b""
        constants = ()
        names = ()
        filename = ""
        firstlineno = 0
        lnotab = b""
        freevars = ()
        cellvars = ()
        new_code = CodeType(
            code.co_argcount,
            code.co_kwonlyargcount,
            code.co_nlocals,
            stacksize,
            flags,
            bytecode,
            constants,
            names,
            code.co_varnames,
            filename,
            code.co_name,
            firstlineno,
            lnotab,
            freevars,
            cellvars,
        )
        return new_code

    # Recursively transform code objects in the constants tuple.
    new_consts = []
    changed = False
    for c in code.co_consts:
        if isinstance(c, CodeType):
            assert c.co_name != "<module>"
            sub_name = fqname + (c.co_name,)
            new_c = mark_native_functions(c, builtins, module_name, sub_name)
        else:
            new_c = c
        changed |= new_c is c
        new_consts.append(new_c)
    if not changed:
        return code

    new_consts = tuple(new_consts)
    return CodeType(
        code.co_argcount,
        code.co_kwonlyargcount,
        code.co_nlocals,
        code.co_stacksize,
        code.co_flags,
        code.co_code,
        new_consts,
        code.co_names,
        code.co_varnames,
        code.co_filename,
        code.co_name,
        code.co_firstlineno,
        code.co_lnotab,
        code.co_freevars,
        code.co_cellvars,
    )


def to_char_array(byte_array):
    """Returns a string with a brace-enclosed list of C char array elements"""
    members = ", ".join(f"0x{int(byte):02x}" for byte in byte_array)
    return f"{{{members}}}"


ModuleData = namedtuple("ModuleData", ("name", "initializer", "builtin_init"))


def process_module(filename, builtins):
    name = os.path.splitext(os.path.basename(filename))[0]
    with open(filename) as fp:
        source = fp.read()
    builtin_init = "$builtin-init-module$" in source
    module_code = compile(source, filename, "exec", dont_inherit=True, optimize=0)
    marked_code = mark_native_functions(module_code, builtins, name)
    bytecode = importlib._bootstrap_external._code_to_bytecode(marked_code)
    initializer = to_char_array(bytecode)
    return ModuleData(name, initializer, builtin_init)


def write_source(fp, modules):
    """Writes the frozen module definitions to a C++ source file."""
    fp.write(
        f"""\
#include "frozen-modules.h"

#include "builtins.h"
#include "globals.h"
#include "modules.h"

namespace py {{
"""
    )

    for module in modules:
        fp.write(
            f"""
static const byte kData_{module.name}[] = {module.initializer};
"""
        )

    fp.write(
        f"""
const FrozenModule kFrozenModules[] = {{
"""
    )

    for module in modules:
        name = module.name
        init = f"FUNC({name}, {INIT_MODULE_NAME})" if module.builtin_init else "nullptr"
        fp.write(
            f"""\
    {{"{name}", ARRAYSIZE(kData_{name}), kData_{name}, {init}}},
"""
        )

    fp.write(
        f"""\
}};
const word kNumFrozenModules = ARRAYSIZE(kFrozenModules);

}}  // namespace py
"""
    )


def write_header(fp, modules):
    """Writes the frozen module declarations to a C++ header file."""
    fp.write(
        f"""\
#include "modules.h"

namespace py {{

extern const FrozenModule kFrozenModules[];
extern const word kNumFrozenModules;

}}  // namespace py
"""
    )


def write_builtins_source(fp, builtins):
    fp.write(
        f"""\
#include "builtins.h"

namespace py {{

const Function::Entry kBuiltinFunctions[] = {{
"""
    )

    for builtin in builtins:
        fp.write(f"    {builtin},\n")

    fp.write(
        f"""
}};
const word kNumBuiltinFunctions = ARRAYSIZE(kBuiltinFunctions);
}}  // namespace py
"""
    )


def write_builtins_header(fp, modules, builtins):
    fp.write(
        f"""\
#include "globals.h"
#include "handles-decl.h"
#include "modules.h"
#include "view.h"

namespace py {{

class Thread;

extern const Function::Entry kBuiltinFunctions[];
extern const word kNumBuiltinFunctions;

"""
    )

    for builtin in builtins:
        fp.write(
            f"""\
RawObject {builtin}(Thread*, Frame*, word);
"""
        )
    fp.write("\n")
    for module in modules:
        if module.builtin_init:
            fp.write(
                f"""\
void FUNC({module.name}, {INIT_MODULE_NAME})(Thread*, const Module&, View<byte>);
"""
            )

    fp.write(
        f"""
}}  // namespace py
"""
    )


def main(argv):
    parser = argparse.ArgumentParser(prog=argv[0])
    parser.add_argument("dir", help="directory where output is written")
    parser.add_argument("files", nargs="+", help="list of files to freeze")
    args = parser.parse_args(argv[1:])
    os.makedirs(args.dir, exist_ok=True)
    builtins = []
    modules = [process_module(filename, builtins) for filename in args.files]
    modules.sort(key=lambda x: x.name)
    with open(pathlib.Path(args.dir, "frozen-modules.cpp"), "w") as f:
        write_source(f, modules)
    with open(pathlib.Path(args.dir, "frozen-modules.h"), "w") as f:
        write_header(f, modules)
    with open(pathlib.Path(args.dir, "builtins.cpp"), "w") as f:
        write_builtins_source(f, builtins)
    with open(pathlib.Path(args.dir, "builtins.h"), "w") as f:
        write_builtins_header(f, modules, builtins)


if __name__ == "__main__":
    main(sys.argv)
