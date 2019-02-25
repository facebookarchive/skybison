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


def compile_pyro(pyro_build_dir):
    os.makedirs(pyro_build_dir, exist_ok=True)
    cmake_flags = ["-DCMAKE_BUILD_TYPE=Release"]
    if shutil.which("gcc.par"):
        cmake_flags += ["-DCMAKE_C_COMPILER=gcc.par"]
        cmake_flags += ["-DCMAKE_CXX_COMPILER=g++.par"]
    cmake_flags += [PYRO_PATH]
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


def run_benchmark(build_dir, python_bin, benchmark_pyc):
    # We execute measure_command.py instead of importing it, so users see it
    # in the log and can replicate the command.
    command = [f"{PYRO_PATH}/util/measure_command.py", "--time"]
    if logging.root.level <= logging.DEBUG:
        command += ["-v"]
    command += ["--", python_bin, benchmark_pyc]
    completed_process = run(command, cwd=build_dir, stdout=subprocess.PIPE)
    completed_process.check_returncode()
    return json.loads(completed_process.stdout)


def describe_revision():
    completed_process = run(["hg", "id"], cwd=PYRO_PATH, stdout=subprocess.PIPE)
    completed_process.check_returncode()
    return completed_process.stdout.strip()


def make_sample(benchmark, interpreter, revision, results):
    sample = dict(results)
    sample.update(
        {
            "benchmark_name": os.path.basename(benchmark),
            "interpreter_name": interpreter,
            "revision": revision,
        }
    )
    return sample


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
    parser.add_argument("--pyro-build", help="Use an already-built Pyro.")
    args = parser.parse_args(argv)

    if args.verbose:
        logging.root.setLevel(logging.DEBUG)

    if args.benchmarks == []:
        args.benchmarks = [f"{PYRO_PATH}/benchmarks/richards.py"]

    build_root = tempfile.mkdtemp()

    try:
        if args.pyro_build:
            pyro_build = os.path.realpath(args.pyro_build)
            do_build = False
            if not os.path.isfile(os.path.join(pyro_build, "python")):
                raise RuntimeError(
                    f"Given build dir of {pyro_build} doesn't exist or doesn't"
                    " have python in it"
                )
        else:
            pyro_build = os.path.join(build_root, "pyro")
            do_build = True

        samples = []
        revision = describe_revision()
        if do_build:
            compile_pyro(pyro_build)
        for benchmark in args.benchmarks:
            benchmark_pyc = compile_benchmark(build_root, benchmark)
            results = run_benchmark(
                build_root, os.path.join(pyro_build, "python"), benchmark_pyc
            )
            samples.append(make_sample(benchmark, "Pyro", revision, results))
            results = run_benchmark(build_root, CPYTHON, benchmark_pyc)
            samples.append(make_sample(benchmark, "CPython", revision, results))
        for sample in samples:
            print(json.dumps(sample, indent=2, sort_keys=True))
            # TODO(matthiasb): Log to scuba?
    finally:
        if not args.keep:
            shutil.rmtree(build_root)


if __name__ == "__main__":
    main(sys.argv[1:])
