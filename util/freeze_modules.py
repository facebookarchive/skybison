#!/usr/bin/env python3

import __future__

# Add library directory so we can import the compiler.
import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
sys.path.append(os.path.join(SCRIPT_DIR, "..", "library"))

import argparse
import marshal
import re
import tempfile
from collections import namedtuple
from compiler import compile as compiler_compile
from pathlib import Path
from types import CodeType

from _compiler import PyroCodeGenerator


INIT_MODULE_NAME = "__init_module__"


# Pyro specific flag to mark code objects for builtin functions.
# (see also `runtime/objects.h`)
CO_BUILTIN = 0x200000


def builtin_function_template():
    _builtin()  # noqa: F821


def builtin_function_template_with_docu():
    """template with docu comment"""
    _builtin()  # noqa: F821


def marked_with_intrinsic(code):
    consts = code.co_consts
    return (
        consts
        and isinstance(consts, tuple)
        and isinstance(consts[0], str)
        and consts[0].startswith("$intrinsic$")
    )


def builtin_cpp_id(module_name, fqname, suffix=""):
    if len(fqname) == 1:
        return f"FUNC({module_name}, {fqname[0]}{suffix})"
    if len(fqname) == 2:
        return f"METH({fqname[0]}, {fqname[1]}{suffix})"
    raise Exception(f"Don't know how to create C++ name for {fqname}")


def builtin_intrinsic_id(module_name, fqname):
    if len(fqname) == 1:
        return f"FUNC({module_name}, {fqname[0]}_intrinsic)"
    if len(fqname) == 2:
        return f"METH({fqname[0]}, {fqname[1]}_intrinsic)"
    raise Exception(f"Don't know how to create C++ name for {fqname}")


def mark_native_functions(code, builtins, intrinsics, module_name, fqname=()):
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

        builtin_id = builtin_cpp_id(module_name, fqname)
        builtin_num = len(builtins)
        builtins.append(builtin_id)

        stacksize = builtin_num
        if marked_with_intrinsic(code):
            intrinsic_id = builtin_intrinsic_id(module_name, fqname)
            # The intrinsic IDs are biased by 1 so that 0 means no intrinsic
            stacksize |= (len(intrinsics) + 1) << 16
            intrinsics.append(intrinsic_id)

        # We abuse the stacksize field to store the index of the builtin
        # function and its intrinsic. This way we do not need to invent a new
        # marshaling type just for this.
        #
        # stacksize is 32 bits in the marshaled code. We use the upper 16 bits
        # for an intrinsic id and the lower 16 bits for the builtin id.
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

    stacksize = code.co_stacksize
    if stacksize > 0xFFFF:
        raise Exception(
            "stacksize greater than expected and overflows into intrinsic space"
        )
    if marked_with_intrinsic(code):
        intrinsic_id = builtin_intrinsic_id(module_name, fqname)
        # The intrinsic IDs are biased by 1 so that 0 means no intrinsic
        stacksize |= (len(intrinsics) + 1) << 16
        intrinsics.append(intrinsic_id)

    # Recursively transform code objects in the constants tuple.
    new_consts = []
    changed = False
    for c in code.co_consts:
        if isinstance(c, CodeType):
            assert c.co_name != "<module>"
            sub_name = fqname + (c.co_name,)
            new_c = mark_native_functions(
                c, builtins, intrinsics, module_name, sub_name
            )
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
        stacksize,
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


ModuleData = namedtuple(
    "ModuleData", ("name", "initializer", "builtin_init", "is_package")
)


def get_module_name(filename):
    path = Path(filename)
    module_name = path.name
    assert module_name.endswith(".py")
    module_name = module_name[:-3]
    for parent in path.parents:
        if not os.path.exists(Path(parent, "__init__.py")):
            break
        module_name = f"{parent.name}.{module_name}"
    return module_name


def process_module(filename, builtins, intrinsics):
    fullname = get_module_name(filename)
    if fullname.endswith(".__init__"):
        fullname = fullname[:-9]
        is_package = True
    else:
        is_package = False
    with open(filename) as fp:
        source = fp.read()
    builtin_init = "$builtin-init-module$" in source
    flags = __future__.CO_FUTURE_ANNOTATIONS
    module_code = compiler_compile(
        source,
        filename,
        "exec",
        flags,
        dont_inherit=None,
        optimize=0,
        compiler=PyroCodeGenerator,
    )
    marked_code = mark_native_functions(module_code, builtins, intrinsics, fullname)
    # We don't write pyc headers because it would make bootstrapping tricky.
    bytecode = marshal.dumps(marked_code)
    initializer = to_char_array(bytecode)
    return ModuleData(fullname, initializer, builtin_init, is_package)


def gen_frozen_modules_cpp(modules):
    result = f"""\
#include "frozen-modules.h"

#include "builtins.h"
#include "globals.h"
#include "modules.h"

namespace py {{
"""

    for module in modules:
        module_ident = module.name.replace(".", "_")
        result += f"""
static const byte kData_{module_ident}[] = {module.initializer};
"""

    result += f"""
const FrozenModule kFrozenModules[] = {{
"""

    for module in modules:
        module_ident = module.name.replace(".", "_")
        is_package = "true" if module.is_package else "false"
        init = (
            f"FUNC({module.name}, {INIT_MODULE_NAME})"
            if module.builtin_init
            else "nullptr"
        )
        result += f"""\
    {{"{module.name}", ARRAYSIZE(kData_{module_ident}), kData_{module_ident}, {init}, {is_package}}},
"""

    result += f"""\
}};
const word kNumFrozenModules = ARRAYSIZE(kFrozenModules);

}}  // namespace py
"""
    return result


def gen_frozen_modules_h():
    return f"""\
#include "modules.h"

namespace py {{

extern const FrozenModule kFrozenModules[];
extern const word kNumFrozenModules;

}}  // namespace py
"""


def gen_builtins_cpp(builtins, intrinsics):
    result = f"""\
#include "builtins.h"

namespace py {{

const BuiltinFunction kBuiltinFunctions[] = {{
"""

    for builtin in builtins:
        result += f"""\
    {builtin},
"""

    result += """\
};

const IntrinsicFunction kIntrinsicFunctions[] = {
"""
    for intrinsic in intrinsics:
        result += f"""\
    {intrinsic},
"""

    result += """\
};
const word kNumBuiltinFunctions = ARRAYSIZE(kBuiltinFunctions);
const word kNumIntrinsicFunctions = ARRAYSIZE(kIntrinsicFunctions);
}  // namespace py
"""
    return result


def gen_builtins_h(modules, builtins, intrinsics):
    result = f"""\
#include "globals.h"
#include "handles-decl.h"
#include "modules.h"
#include "view.h"

namespace py {{

class Thread;

extern const BuiltinFunction kBuiltinFunctions[];
extern const IntrinsicFunction kIntrinsicFunctions[];
extern const word kNumBuiltinFunctions;
extern const word kNumIntrinsicFunctions;

"""

    for builtin in builtins:
        result += f"""\
RawObject {builtin}(Thread* thread, Arguments args) ALIGN_16;
"""
    result += "\n"
    for intrinsic in intrinsics:
        result += f"""\
bool {intrinsic}(Thread* thread) ALIGN_16;
"""
    result += "\n"
    for module in modules:
        if module.builtin_init:
            result += f"""\
void FUNC({module.name}, {INIT_MODULE_NAME})(Thread*, const Module&, View<byte>);
"""

    result += f"""
}}  // namespace py
"""
    return result


def write_if_different(path, string):
    try:
        if path.read_text() == string:
            return
    except OSError:
        pass
    with tempfile.NamedTemporaryFile(dir=path.parent, delete=False, mode="w+t") as fp:
        fp.write(string)
    Path(fp.name).replace(path)


def main(argv):
    parser = argparse.ArgumentParser(prog=argv[0])
    parser.add_argument("dir", help="directory where output is written")
    parser.add_argument("files", nargs="+", help="list of files to freeze")
    args = parser.parse_args(argv[1:])
    os.makedirs(args.dir, exist_ok=True)
    builtins = []
    intrinsics = []
    modules = [
        process_module(filename, builtins, intrinsics) for filename in args.files
    ]
    modules.sort(key=lambda x: x.name)
    write_if_different(
        Path(args.dir, "builtins.cpp"), gen_builtins_cpp(builtins, intrinsics)
    )
    write_if_different(
        Path(args.dir, "builtins.h"), gen_builtins_h(modules, builtins, intrinsics)
    )
    write_if_different(
        Path(args.dir, "frozen-modules.cpp"), gen_frozen_modules_cpp(modules)
    )
    write_if_different(Path(args.dir, "frozen-modules.h"), gen_frozen_modules_h())


if __name__ == "__main__":
    main(sys.argv)
