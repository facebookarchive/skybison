#!/usr/bin/env python3
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import argparse
import os
import pickle
import re
import sys
from collections import deque


class ModuleCallGraph(object):
    def __init__(self, cpython_project, ext_project, cpython_module):
        self.cpython = cpython_project
        self.ext = ext_project
        self.module = cpython_module
        self.to_implement = []
        self.copyable = []
        # A list of blacklisted functions that won't be implemented for reasons
        self.blacklist = {"PyUnicode_DecodeUTF8Stateful"}
        self.graph = self.create()
        self.properties = self.properties()

    def create(self):
        result = []
        dq = deque()
        for function_name, function in self.cpython.functions.items():
            if function.file_name == self.module:
                dq.append(function_name)
                result.append(function_name + " -> " + function_name)

        func_seen = set()
        callees_seen = set()
        while len(dq) > 0:
            func_name = dq.popleft()
            if func_name in func_seen:
                continue

            func_seen.add(func_name)

            # Function is already implemented in extensions, skip
            if func_name in self.ext.functions:
                continue

            function = None
            if func_name in self.cpython.functions:
                function = self.cpython.functions[func_name]
            elif func_name in self.cpython.macros:
                function = self.cpython.macros[func_name]
            else:
                # Function is a native C function
                continue

            # Function has object access, skip
            if len(function.object_access) > 0:
                continue

            # Function has a blacklisted callee, skip
            if self.blacklist & set(function.callees):
                continue

            for callee in function.callees:
                if callee not in callees_seen:
                    result.append(func_name + " -> " + callee)
                    dq.append(callee)
                    callees_seen.add(callee)

        return result

    def properties(self):
        graph_props = set()
        for r in self.graph:
            res = r.split(" -> ")
            graph_props.add(res[0])
            graph_props.add(res[1])

        properties = []
        for func_name in graph_props:
            color = ""
            # Function is already implemented in extensions
            if func_name in self.ext.functions:
                color = "chartreuse3"
            # Function is a macro
            elif func_name in cpython_project.macros:
                color = "dodgerblue3"
            elif func_name in cpython_project.functions:
                function = self.cpython.functions[func_name]
                # Function is part of the module
                if self.module in function.file_name:
                    color = "gold2"
                # Function has object access
                elif len(function.object_access) > 0:
                    color = "firebrick3"
                    self.to_implement.append(func_name)
                # Function has a blacklisted callee
                elif self.blacklist & set(function.callees):
                    color = "firebrick3"
                    self.to_implement.append(func_name)
                else:
                    color = "invis"
                    self.copyable.append(func_name)
            # Function is a native c function
            else:
                color = "invis"

            properties.append(
                func_name + " [style=filled, fillcolor = " + color + "]"
            )

        return properties

    def generate_dot(self, file_name):
        with open(file_name, "w", encoding="utf-8") as dot_file:
            dot_file.write("digraph G {\n")
            for r in self.properties:
                dot_file.write("  " + r + "\n")
            for r in self.graph:
                dot_file.write("  " + r + "\n")
            dot_file.write("}\n")
        print("Generated: " + file_name)

    def generate_todo(self):
        print("Module: " + self.module, "still requires the following:")
        for func_name in self.to_implement:
            print(func_name)

        print(
            "\nThe following functions can be automatically copied "
            + "once there are no missing functions to implement:"
        )
        for func_name in self.copyable:
            print(func_name)

    def generate_missing_files(self):
        current_sources = {s.file_path for s in self.ext.sources}

        needed_sources = set()
        for func_name in self.copyable:
            function = self.cpython.functions[func_name]
            function_file = function.cfile
            relative_path = function_file.file_path.split("cpython/", 1)[1]
            needed_sources.add("ext/" + relative_path + "pp")

        # TODO(eelizondo): Change print for write
        for s in needed_sources.difference(current_sources):
            print("Generating: " + s)
            print("// " + s.rsplit("/", 1)[1][:-2] + " implementation")
            print('#include "Python.h"')

    def generate_functions(self):
        if len(self.to_implement) > 0:
            print(
                "Can't autogenerate functions, please implement the missing"
                "functions. For a list of missing functions, use '--todo'"
            )
            return

        self.generate_missing_files()

        for func_name in self.copyable:
            function = self.cpython.functions[func_name]
            function_file = function.cfile
            relative_path = function_file.file_path.split("cpython/", 1)[1]
            # TODO(eelizondo): Change print for write
            print("Writing in: " + "ext/" + relative_path + "pp")
            print(
                "// Start of automatically generated code, from "
                + relative_path
                + "\n"
            )
            print("\n".join(function.header))
            print("\n".join(function.content))
            print("}")
            print("// End of automatically generated code")


class CPythonFile:
    def __init__(self, file_path):
        self.file_path = file_path
        self.file_name = file_path.rsplit("/", 1)[1]
        self.content = []
        self.get_content()

    def get_content(self):
        with open(self.file_path, "r", encoding="utf-8") as f:
            self.content = f.read().split("\n")

    def __str__(self):
        return self.file_name


class CPythonMacro:
    def __init__(self, cpython_file, line_num):
        self.file_name = cpython_file.file_name
        self.line_num = line_num
        self.name = self.get_name(cpython_file)
        self.content = self.get_content(cpython_file)
        self.callees = self.get_callees()
        self.allowed_access = {"ob_refcnt", "ob_type", "tp_name"}
        self.object_access = self.get_object_access()

    def get_name(self, cpython_file):
        line = cpython_file.content[self.line_num]
        match = re.sub(r"^#\s*?define ", r"", line)
        match = re.sub(r"([0-9a-zA-Z_-]*).*", r"\1", match)
        match = match.rstrip()
        return match

    def get_content(self, cpython_file):
        content = []
        curr_num = self.line_num
        while not re.search(r"^\s*$", cpython_file.content[curr_num]):
            content.append(cpython_file.content[curr_num])
            curr_num += 1
            if re.search(r"^#\s*?define", cpython_file.content[curr_num]):
                break
        return content

    def get_callees(self):
        callees = set()
        content = " ".join(self.content)
        match = re.findall(r"[0-9a-zA-Z_][0-9a-zA-Z_-]*?\(", content)
        for m in match:
            m = m[:-1]
            if m != "":
                callees.add(m)
        return callees

    def get_object_access(self):
        object_access = set()
        content = " ".join(self.content)
        match = re.findall(r"->[0-9a-zA-Z_.]*", content)
        for m in match:
            if m[2:] not in self.allowed_access:
                object_access.add(m[2:])
        return object_access

    def __str__(self):
        return self.name


class CPythonFunction:
    def __init__(self, cpython_file, line_num):
        self.file_name = cpython_file.file_name
        self.cfile = cpython_file
        self.line_num = line_num
        self.name = self.get_name(cpython_file)
        self.header = self.get_header(cpython_file)
        self.content = self.get_content(cpython_file)
        self.callees = self.get_callees()
        self.allowed_access = {"ob_refcnt", "ob_type", "tp_name"}
        self.object_access = self.get_object_access()

    def get_name(self, cpython_file):
        curr_num = self.line_num
        while not re.search(r".*\(", cpython_file.content[curr_num]):
            curr_num -= 1
        line = cpython_file.content[curr_num]
        match = re.sub(r"\(.*", r"", line)
        match = re.sub(r".* ", r"", match)
        match = re.sub(r"\*", r"", match)
        return match

    def get_header(self, cpython_file):
        header = []
        curr_num = self.line_num
        header.append(cpython_file.content[curr_num])
        while not re.search(
            r".*" + self.name + r".*", cpython_file.content[curr_num]
        ):
            curr_num -= 1
            header.append(cpython_file.content[curr_num])
        if re.search(r"^" + self.name, cpython_file.content[curr_num + 1]):
            header.append(cpython_file.content[curr_num])
        return list(reversed(header))

    def get_content(self, cpython_file):
        content = []
        curr_num = self.line_num + 1
        while not re.search(r"^}", cpython_file.content[curr_num]):
            content.append(cpython_file.content[curr_num])
            curr_num += 1
        return content

    def get_callees(self):
        callees = set()
        content = " ".join(self.content)
        match = re.findall(r"[0-9a-zA-Z_][0-9a-zA-Z_-]*?\(", content)
        for m in match:
            m = m[:-1]
            if m != "":
                callees.add(m)
        return callees

    def get_object_access(self):
        object_access = set()
        content = " ".join(self.content)
        match = re.findall(r"->[0-9a-zA-Z_.]*", content)
        for m in match:
            if m[2:] not in self.allowed_access:
                object_access.add(m[2:])

        # All Error handling functions need to be replaced
        match = re.findall(r".*Err.*", self.name)
        for m in match:
            object_access.add(m)

        return object_access

    def __str__(self):
        return self.name


class CPythonProject:
    def __init__(self, root):
        self.root = root
        self.filter_dirs = [
            "Doc",
            "Grammar",
            "PC",
            "Mac",
            "Tools",
            "Misc",
            "test",
            "_ctypes",
        ]
        self.headers = []
        self.sources = []
        self.macros = {}
        self.functions = {}
        self.find_files(self.root)
        self.find_macros()
        self.find_functions()

    def find_files(self, root):
        for f in self.filter_dirs:
            if f in root:
                return

        for directory in os.listdir(root):
            directory = root + "/" + directory
            if os.path.isdir(directory):
                for filename in os.listdir(directory):
                    file_path = directory + "/" + filename
                    if file_path.endswith(".h"):
                        self.headers.append(CPythonFile(file_path))
                    if not file_path.endswith("-test.cpp") and (
                        file_path.endswith(".c") or file_path.endswith(".cpp")
                    ):
                        self.sources.append(CPythonFile(file_path))
                if os.path.isdir(directory):
                    self.find_files(directory)

    def find_macros(self):
        for cpython_file in self.headers + self.sources:
            for line_num in range(0, len(cpython_file.content)):
                if re.search(r"^#\s*?define ", cpython_file.content[line_num]):
                    macro = CPythonMacro(cpython_file, line_num)
                    self.macros[macro.name] = macro

    def find_functions(self):
        for cpython_file in self.sources:
            if ".cpp" in cpython_file.file_path:
                for line_num in range(0, len(cpython_file.content)):
                    if re.search(
                        r"^[0-9a-zA-Z_]", cpython_file.content[line_num]
                    ) and re.search(
                        r"[0-9a-zA-Z_][0-9a-zA-Z_-]*?\(",
                        cpython_file.content[line_num],
                    ):
                        function = CPythonFunction(cpython_file, line_num)
                        self.functions[function.name] = function
                continue
            for line_num in range(0, len(cpython_file.content)):
                if (
                    (
                        re.search(r"^{", cpython_file.content[line_num])
                        and not re.search(
                            r"^static", cpython_file.content[line_num - 1]
                        )
                    )
                    or (re.search(r"{$", cpython_file.content[line_num]))
                    and re.search(
                        r"^[a-zA-Z0-9_-]*?\(", cpython_file.content[line_num]
                    )
                ):
                    function = CPythonFunction(cpython_file, line_num)
                    self.functions[function.name] = function


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Create a Call Graph of all the top level "
        "functions from the given CPython file."
    )
    parser.add_argument(
        "module",
        metavar="module.c",
        type=str,
        help="The file name of any CPython file",
    )
    parser.add_argument(
        "--todo",
        action="store_true",
        help="Output a list of all the missing functions to enable this module",
    )
    parser.add_argument(
        "--dot",
        action="store_true",
        help="Generate a .dot file of the Call Graph",
    )
    parser.add_argument(
        "--autogen",
        action="store_true",
        help="Import into the runtime the CPython "
        "functions that need no modification",
    )
    args = parser.parse_args()

    cpython_home = "third-party/cpython/"
    custom_extension_path = "ext"
    cpython_module = sys.argv[1]

    if not os.path.exists("cpython_project.pkl"):
        print("Creating a CPython directory index. Please wait...")
        cpython_path = os.path.abspath(cpython_home)
        cpython_project = CPythonProject(cpython_path)
        with open("cpython_project.pkl", "wb") as cpython_pkl_file:
            pickle.dump(cpython_project, cpython_pkl_file)
    else:
        with open("cpython_project.pkl", "rb") as cpython_pkl_file:
            cpython_project = pickle.load(cpython_pkl_file)

    # This is bound to constantly change, so generate every time
    ext_project = CPythonProject(custom_extension_path)

    # Create the call graph
    call_graph = ModuleCallGraph(cpython_project, ext_project, args.module)
    if args.todo:
        call_graph.generate_todo()
    if args.dot:
        call_graph.generate_dot("graph.dot")
    if args.autogen:
        call_graph.generate_functions()
