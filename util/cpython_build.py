#!/usr/bin/env python3
from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals
import io
import multiprocessing
import os
import re
import subprocess
import sys
import platform


# [1]: CPython build directory
# [2]: CPython path
# [3]: Pyro's CPython Objects path
if __name__ == "__main__":
    cpython_build_path = sys.argv[1]
    cpython_path = sys.argv[2]
    pyro_objects_dir = sys.argv[3]

    if not os.path.exists(cpython_build_path):
        os.mkdir(cpython_build_path)
        os.chdir(cpython_build_path)
        subprocess.check_output(cpython_path + "/configure", shell=True)
        subprocess.check_output(
            "make -j " + str(multiprocessing.cpu_count()), shell=True)
    else:
        os.chdir(cpython_build_path)
    for f in os.listdir(cpython_build_path):
        if "libpython" in f:
            libpython = cpython_build_path + '/' + f

    try:
        subprocess.check_output("ar -xv " + libpython, shell=True)
    except:
        print(libpython)
        sys.exit()

    object_files = []
    for f in os.listdir(pyro_objects_dir):
        file_path = pyro_objects_dir + "/" + f
        if file_path.endswith(".cpp"):
            func_names = []
            read_file = []
            with io.open(file_path, 'r', encoding='utf-8') as cpp_file:
                read_file = cpp_file.read().split("\n")

            obj = cpython_build_path + "/" + f.split(".")[0] + ".o"
            object_files.append(obj)
            for l in read_file:
                if not re.search(r'^[a-zA-Z]', l):
                    continue
                match = re.findall(r'[a-zA-Z0-9_]*?\(', l)
                for m in match:
                    # redefine cross compilation symbols
                    opt = "--redefine-sym " + m[:-1] + "=__" + m[:-1]
                    cmd = "objcopy {opt} {obj} {obj}".format(opt=opt,obj=obj)
                    if platform.system() == "Darwin":
                        cmd = "g" + cmd
                    subprocess.check_output(cmd, shell=True)

    subprocess.check_output(
        "ar -rcs " + libpython + " " + " ".join(object_files), shell=True)

    for f in os.listdir(cpython_build_path):
        file_path = cpython_build_path + "/" + f
        if file_path.endswith(".o"):
            subprocess.check_output("rm " + file_path, shell=True)

    print(libpython)
