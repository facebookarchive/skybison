#!/usr/bin/env python3
import argparse
import json
import logging
import os
import shutil
import subprocess
import sys
import tempfile


def run(cmd, cwd, stdout, **kwargs):
    logging.info(f"$ cd {cwd}; {' '.join(cmd)}")
    return subprocess.run(cmd, cwd=cwd, encoding="UTF-8", stdout=stdout, **kwargs)


def get_repo_root(dirname):
    completed_process = run(["hg", "root"], cwd=dirname, stdout=subprocess.PIPE)
    if completed_process.returncode == 0:
        return completed_process.stdout.strip()

    print("Unknown source control", file=sys.stderr)
    sys.exit(1)


REPO_ROOT = get_repo_root(os.path.dirname(__file__))
PYRO_PATH = f"{REPO_ROOT}/fbcode/pyro"
# This is the python currently hardcoded in pyro/runtime/runtime.cpp
CPYTHON = "/usr/local/fbcode/gcc-5-glibc-2.23/bin/python3.6"


def compile_pyro(pyro_source, pyro_build_dir):
    os.makedirs(pyro_build_dir, exist_ok=True)
    cmake_flags = ["-DCMAKE_BUILD_TYPE=Release"]
    if shutil.which("gcc.par"):
        cmake_flags += ["-DCMAKE_C_COMPILER=gcc.par"]
        cmake_flags += ["-DCMAKE_CXX_COMPILER=g++.par"]
    cmake_flags += [pyro_source]
    run(
        ["cmake"] + cmake_flags,
        cwd=pyro_build_dir,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
    ).check_returncode()
    jobs = os.cpu_count() + 2  # (+2 so some compilers can wait for I/O)
    proc = run(["make", f"-j{jobs}"], cwd=pyro_build_dir, stdout=subprocess.PIPE)
    if proc.returncode != 0:
        print("Build failed:", file=sys.stderr)
        print(proc.stdout)
        sys.exit(1)
    return os.path.join(pyro_build_dir, "python")


def compile_benchmark(build_dir, benchmark):
    benchmarks_dir = f"{build_dir}/benchmarks"
    os.makedirs(benchmarks_dir, exist_ok=True)
    run(
        ["cp", benchmark, benchmarks_dir], cwd=os.getcwd(), stdout=subprocess.DEVNULL
    ).check_returncode()
    benchmark_basename = os.path.basename(benchmark)
    run(
        [CPYTHON, "-m", "compileall", "-q", "-b", benchmark_basename],
        cwd=benchmarks_dir,
        stdout=subprocess.DEVNULL,
    ).check_returncode()
    benchmark_pyc = f"{benchmarks_dir}/{benchmark_basename}c"
    return benchmark_pyc


def run_benchmark(python_bin, benchmark_pyc, measure_flags):
    env = dict(os.environ)
    env["PYTHONHASHSEED"] = "0"

    # We execute measure_command.py instead of importing it, so users see it
    # in the log and can replicate the command.
    command = [f"{PYRO_PATH}/util/measure_command.py"] + measure_flags
    if logging.root.level <= logging.DEBUG:
        command += ["-v"]
    command += ["--", python_bin, benchmark_pyc]
    completed_process = run(
        command, cwd=os.path.dirname(benchmark_pyc), stdout=subprocess.PIPE, env=env
    )
    completed_process.check_returncode()
    return json.loads(completed_process.stdout)


def describe_revision(pyro_source):
    try:
        completed_process = run(
            ["hg", "id"],
            cwd=pyro_source,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
        )
        completed_process.check_returncode()
        return completed_process.stdout.strip()
    except subprocess.CalledProcessError:
        return "n/a"


class Interpreter:
    pass


def prepare_interpreters(interpreter_args, tempdir, pyro_source):
    interpreters = []
    for name in interpreter_args:
        interpreter = Interpreter()
        interpreters.append(interpreter)
        interpreter.name = name
        interpreter.version = "n/a"
        if name == "pyro":
            pyro_build_dir = os.path.join(tempdir, "pyro")
            interpreter.executable = compile_pyro(pyro_source, pyro_build_dir)
            interpreter.version = describe_revision(pyro_source)
        elif name == "cpython":
            interpreter.executable = CPYTHON
        else:
            interpreter.executable = os.path.abspath(name)

        if interpreter.version == "n/a":
            try:
                output = subprocess.check_output([interpreter.executable, "--version"])
                output = output.strip()
                output = output.decode("utf-8", errors="replace")
                interpreter.version = output
            except subprocess.CalledProcessError:
                pass
    return interpreters


def main(argv):
    parser = argparse.ArgumentParser(
        description="Run a Pyro benchmark and log to Scuba"
    )
    parser.add_argument(
        "--benchmark", "-b", dest="benchmarks", action="append", default=[]
    )
    parser.add_argument("--verbose", "-v", action="store_true")
    parser.add_argument(
        "--keep", "-k", help="Do not remove build directory", action="store_true"
    )
    parser.add_argument(
        "--pyro-source", default=PYRO_PATH, help="Pyro source directory"
    )
    parser.add_argument(
        "--interpreter",
        "-i",
        dest="interpreters",
        default=[],
        help="Specify interpreter(s) to use (pyro, cpython, <executable>)",
        action="append",
    )
    parser.add_argument(
        "--time",
        help="Measure time",
        dest="measure_flags",
        default=[],
        action="append_const",
        const="--time",
    )
    parser.add_argument(
        "--callgrind",
        help="Measure number of instructions executed",
        dest="measure_flags",
        action="append_const",
        const="--callgrind",
    )
    args = parser.parse_args(argv)

    if args.verbose:
        logging.root.setLevel(logging.DEBUG)

    if args.benchmarks == []:
        args.benchmarks = [f"{PYRO_PATH}/benchmarks/richards.py"]
    if args.benchmarks == []:
        args.interpreters = ["pyro", "cpython"]

    build_root = tempfile.mkdtemp()
    try:
        interpreters = prepare_interpreters(
            args.interpreters, build_root, args.pyro_source
        )

        samples = []
        for benchmark in args.benchmarks:
            benchmark_pyc = compile_benchmark(build_root, benchmark)
            for interpreter in interpreters:
                results = run_benchmark(
                    interpreter.executable, benchmark_pyc, args.measure_flags
                )
                results.update(
                    {
                        "benchmark_name": os.path.basename(benchmark),
                        "interpreter_name": interpreter.name,
                        "version": interpreter.version,
                    }
                )
                samples.append(results)
        for sample in samples:
            print(json.dumps(sample, indent=2, sort_keys=True))
    finally:
        if not args.keep:
            shutil.rmtree(build_root)


if __name__ == "__main__":
    main(sys.argv[1:])
