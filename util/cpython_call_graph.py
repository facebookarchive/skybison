#!/usr/bin/env python3
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import pprint
import io
import sys
import re
import os
import pickle
import copy
from collections import deque


def print_list(l):
    elems = []
    for k, _v in l.items():
        elems.append(k)
    pprint.pprint(sorted(elems))


def callee_is_func_type(callee, leaf_type, cpython_project):
    if leaf_type == "all":
        return callee in cpython_project.functions \
            or callee in cpython_project.macros
    if leaf_type == "capi":
        return callee in cpython_project.functions \
            and callee in cpython_project.capis
    if leaf_type == "macros":
        return callee in cpython_project.macros
    if leaf_type == "private":
        return callee in cpython_project.functions \
            and callee not in cpython_project.capis


def result_to_file(file_name, result, properties):
    with io.open(file_name, 'w', encoding='utf-8') as dot_file:
        dot_file.write("digraph G {\n")
        for r in properties:
            dot_file.write("  " + r + "\n")
        for r in result:
            dot_file.write("  " + r + "\n")
        dot_file.write("}\n")


def file_call_graph(
        cpython_project,
        file_name,
        func_type,
        whitelist,
        blacklist,
        cut_object_access=True):
    file_functions = []
    for _function_name, function in cpython_project.functions.items():
        if function.file_name == file_name:
            file_functions.append(function)

    result = []
    func_seen = set()
    callees_seen = set()
    dq = deque()
    for function in file_functions:
        dq.append((function.name, []))

    while len(dq) > 0:
        func_tuple = dq.popleft()
        func_name = func_tuple[0]
        if func_name in func_seen:
            continue

        def add_callees(x):
            if x.name in whitelist:
                x.object_access = set()
            if x.name in blacklist:
                x.object_access = set('blacklist')
            if cut_object_access and len(x.object_access) > 0:
                return False
            return True

        function = None
        func_tuple[1].append(func_name)
        func_seen.add(func_name)
        if func_name in cpython_project.functions:
            function = cpython_project.functions[func_name]
        elif func_name in cpython_project.macros:
            function = cpython_project.macros[func_name]
        if function is None:
            continue

        for callee in function.callees:
            func_chain = copy.copy(func_tuple[1])
            if callee_is_func_type(callee, func_type, cpython_project):
                if callee not in callees_seen:
                    while len(func_chain) > 1:
                        func = func_chain.pop(0)
                        if func_chain[0] not in callees_seen:
                            dep = func + " -> " + func_chain[0]
                            result.append(dep)
                            callees_seen.add(func_chain[0])
                    if add_callees(function):
                        dep = func_name + " -> " + callee
                        result.append(dep)
                        callees_seen.add(callee)
                        dq.append((callee, []))
            else:
                if add_callees(function):
                    dq.append((callee, func_chain))

    result = list(filter(
        lambda x: True if x.split(" -> ")[0] != x.split(" -> ")[1] else False,
        result))
    return result


def file_table(cpython_project, top_file, results):
    func_calls = set()
    for r in results:
        func_calls.add(r.split(" -> ")[1])

    all_files = {}
    all_files[top_file] = ([], [], [])

    for func_name in func_calls:
        if callee_is_func_type(func_name, "capi", cpython_project):
            function = cpython_project.functions[func_name]
            if function.file_name not in all_files:
                all_files[function.file_name] = ([], [], [])
            if len(function.object_access) >= 0:
                all_files[function.file_name][0].append(func_name)
        elif callee_is_func_type(func_name, "private", cpython_project):
            function = cpython_project.functions[func_name]
            if function.file_name not in all_files:
                all_files[function.file_name] = ([], [], [])
            if len(function.object_access) >= 0:
                all_files[function.file_name][1].append(func_name)
        elif callee_is_func_type(func_name, "macros", cpython_project):
            function = cpython_project.macros[func_name]
            if function.file_name not in all_files:
                all_files[function.file_name] = ([], [], [])
            if len(function.object_access) >= 0:
                all_files[function.file_name][2].append(func_name)

    pprint.pprint(all_files)


def graph_properties(cpython_project, results):
    graph_props = set()
    for r in results:
        res = r.split(" -> ")
        graph_props.add(res[0])
        graph_props.add(res[1])

    properties = []
    for func_name in graph_props:
        function = ""
        if (callee_is_func_type(func_name, "capi", cpython_project) or
                callee_is_func_type(func_name, "private", cpython_project)):
            function = cpython_project.functions[func_name]
        else:
            function = cpython_project.macros[func_name]

        if len(function.object_access) > 0:
            properties.append(
                func_name + " [style=filled, fillcolor = firebrick3]")

    return properties


class CPythonFile:
    def __init__(self, file_path):
        self.file_path = file_path
        self.file_name = file_path.rsplit("/", 1)[1]
        self.content = []
        self.get_content()

    def get_content(self):
        with io.open(self.file_path, 'r', encoding='utf-8') as f:
            self.content = f.read().split('\n')

    def __str__(self):
        return self.file_name


class CPythonMacro:
    def __init__(self, cpython_file, line_num):
        self.file_name = cpython_file.file_name
        self.line_num = line_num
        self.name = self.get_name(cpython_file)
        self.content = self.get_content(cpython_file)
        self.callees = self.get_callees()
        self.object_access = self.get_object_access()

    def get_name(self, cpython_file):
        line = cpython_file.content[self.line_num]
        match = re.sub(r'^#\s*?define ', r'', line)
        match = re.sub(r'([0-9a-zA-Z_-]*).*', r'\1', match)
        match = match.rstrip()
        return match

    def get_content(self, cpython_file):
        content = []
        curr_num = self.line_num
        while not re.search(r'^\s*$', cpython_file.content[curr_num]):
            content.append(cpython_file.content[curr_num])
            curr_num += 1
        return content

    def get_callees(self):
        callees = set()
        content = ' '.join(self.content)
        match = re.findall(r'[0-9a-zA-Z_-]*?\(', content)
        for m in match:
            m = m[:-1]
            if m != "":
                callees.add(m)
        return callees

    def get_object_access(self):
        object_access = set()
        content = ' '.join(self.content)
        match = re.findall(r'->[0-9a-zA-Z_.]*', content)
        for m in match:
            object_access.add(m[2:])
        return object_access

    def __str__(self):
        return self.name


class CPythonFunction:
    def __init__(self, cpython_file, line_num):
        self.file_name = cpython_file.file_name
        self.line_num = line_num
        self.name = self.get_name(cpython_file)
        self.content = self.get_content(cpython_file)
        self.callees = self.get_callees()
        self.object_access = self.get_object_access()

    def get_name(self, cpython_file):
        curr_num = self.line_num
        while not re.search(r'.*\(', cpython_file.content[curr_num]):
            curr_num -= 1
        line = cpython_file.content[curr_num]
        match = re.sub(r'\(.*', r'', line)
        match = re.sub(r'.* ', r'', match)
        match = re.sub(r'\*', r'', match)
        return match

    def get_content(self, cpython_file):
        content = []
        curr_num = self.line_num + 1
        while not re.search(r'^}', cpython_file.content[curr_num]):
            content.append(cpython_file.content[curr_num])
            curr_num += 1
        return content

    def get_callees(self):
        callees = set()
        content = ' '.join(self.content)
        match = re.findall(r'[0-9a-zA-Z_-]*?\(', content)
        for m in match:
            m = m[:-1]
            if m != "":
                callees.add(m)
        return callees

    def get_object_access(self):
        object_access = set()
        content = ' '.join(self.content)
        match = re.findall(r'->[0-9a-zA-Z_.]*', content)
        for m in match:
            object_access.add(m[2:])
        return object_access

    def __str__(self):
        return self.name


class CPythonAPI:
    def __init__(self, cpython_file, line_num):
        self.file_name = cpython_file.file_name
        self.line_num = line_num
        self.name = self.get_name(cpython_file)
        self.public = False if self.name[0] == '_' else True

    def get_name(self, cpython_file):
        line = cpython_file.content[self.line_num]
        match = ""
        if re.search(r'^PyAPI_FUNC\(.*?\).*?\(', line):
            match = re.sub(r'^PyAPI_FUNC\(.*?\)(.*?)\(.*', r'\1', line)
        else:
            line = cpython_file.content[self.line_num + 1]
            match = re.sub(r'(.*?)\(.*', r'\1', line)
        match = match.lstrip()
        match = match.rstrip()
        return match

    def __str__(self):
        return self.name


class CPythonProject:
    def __init__(self, root):
        self.root = root
        self.filter_dirs = [
            "Doc", "Grammar", "PC", "Mac", "Tools", "Misc", "test", "_ctypes"]
        self.headers = []
        self.sources = []
        self.macros = {}
        self.functions = {}
        self.capis = {}
        self.find_files(self.root)
        self.find_macros()
        self.find_functions()
        self.find_capis()

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
                    if file_path.endswith(".c"):
                        self.sources.append(CPythonFile(file_path))
                if os.path.isdir(directory):
                    self.find_files(directory)
        # print_list(self.headers)

    def find_macros(self):
        for cpython_file in self.headers + self.sources:
            for line_num in range(0, len(cpython_file.content)):
                if re.search(r'^#\s*?define ', cpython_file.content[line_num]):
                    macro = CPythonMacro(cpython_file, line_num)
                    self.macros[macro.name] = macro
        # print_list(self.macros)

    def find_functions(self):
        for cpython_file in self.sources:
            for line_num in range(0, len(cpython_file.content)):
                if ((re.search(r'^{', cpython_file.content[line_num]) and
                        not re.search(r'^static',
                                      cpython_file.content[line_num - 1]))
                        or (re.search(r'{$', cpython_file.content[line_num]))
                        and re.search(r'^[a-zA-Z0-9_-]*?\(',
                                      cpython_file.content[line_num])):
                    function = CPythonFunction(cpython_file, line_num)
                    self.functions[function.name] = function
        # print_list(self.functions)

    def find_capis(self):
        for cpython_file in self.headers:
            for line_num in range(0, len(cpython_file.content)):
                if re.search(r'^PyAPI_FUNC', cpython_file.content[line_num]):
                    capi = CPythonAPI(cpython_file, line_num)
                    self.capis[capi.name] = capi
        # print_list(self.capi)


# [1] cpython home directory
# [2] name of c file
# [3] output graph names
# [4] optional: whitelist file (fall through function node)
# [5] optional: blacklist file (stop at function node)
if __name__ == "__main__":
    if not os.path.exists("cpython_project.pkl"):
        cpython_path = os.path.abspath(sys.argv[1])
        cpython_project = CPythonProject(cpython_path)
        with io.open("cpython_project.pkl", 'wb') as cpython_pkl_file:
            pickle.dump(cpython_project, cpython_pkl_file)
    else:
        with io.open("cpython_project.pkl", 'rb') as cpython_pkl_file:
            cpython_project = pickle.load(cpython_pkl_file)

    whitelist = []
    if len(sys.argv) > 4:
        with io.open(sys.argv[4], 'r') as wl:
            whitelist = wl.read().split("\n")
            whitelist.pop()

    blacklist = []
    if len(sys.argv) > 5:
        with io.open(sys.argv[5], 'r') as bl:
            blacklist = bl.read().split("\n")
            blacklist.pop()

    # Print call graphs
    all_result = file_call_graph(
        cpython_project, sys.argv[2], "all", whitelist, blacklist)
    all_props = graph_properties(cpython_project, all_result)
    result_to_file(sys.argv[3] + "_graph.dot", all_result, all_props)

    # Print file type table
    file_table(cpython_project, sys.argv[2], all_result)
