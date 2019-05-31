#!/usr/bin/env python3
import argparse
import ast
import json
import logging
import os
import shutil
import subprocess
import sys

from _benchmark_tools import add_tools_arguments, post_process_results
from hg_util import current_rev, repo_root


def run(cmd, stdout, **kwargs):
    assert "env" not in kwargs
    env = dict(os.environ)
    env["PYTHONHASHSEED"] = "0"
    env["PYRO_ENABLE_CACHE"] = "1"
    logging.info(f"${' '.join(cmd)}")
    return subprocess.run(cmd, encoding="UTF-8", stdout=stdout, env=env, **kwargs)


def print_table(results):
    # Groups results by benchmark name to compare them more easily
    results.sort(key=lambda x: x["benchmark"], reverse=True)

    tbl = [list(results[0].keys())]
    data = [list(r.values()) for r in results]
    tbl.extend(data)
    horizontal = "-"
    vertical = "|"
    cross = "+"
    cols = [list(x) for x in zip(*tbl)]
    lengths = [max(map(len, map(str, col))) for col in cols]
    f = vertical + " ".join(" {:>%d} " % l for l in lengths) + vertical
    s = cross + horizontal.join(horizontal * (l + 2) for l in lengths) + cross

    print(s)
    for idx in range(0, len(tbl)):
        row = tbl[idx]
        print(f.format(*row))
        if idx == 0:
            print(s)
    print(s)


# Given that pyro doesn't support argparse, set the argument list
# in an easily parsable way.
def create_benchmark_runner_command(binary, arguments):
    runner_path = os.path.dirname(os.path.abspath(__file__))
    command = [
        binary,
        f"{runner_path}/_benchmark_runner.py",
        "benchmarks_path",
        arguments["benchmarks_path"],
    ]
    tools = "tool " + " tool".join(arguments["tools"])
    command.extend(tools.split(" "))
    command.extend(
        [
            "time_min_benchmark_time",
            str(arguments["time_min_benchmark_time"]),
            "time_num_repetitions",
            str(arguments["time_num_repetitions"]),
        ]
    )
    return command


class Interpreter:
    # This is the python currently hardcoded in pyro/runtime/runtime.cpp
    CPYTHON = "/usr/local/fbcode/gcc-5-glibc-2.23/bin/python3.6"

    def __init__(self, name):
        self.name = name
        self.binary = self._interpreter_binary()
        self.version = self._interpreter_version()

    def _interpreter_binary(self):
        if self.name == "fbcode-python":
            return Interpreter.CPYTHON
        return self.name

    def _cpython_version(self):
        completed_process = run([self.binary, "--version"], stdout=subprocess.PIPE)
        if completed_process.returncode != 0:
            print(f"Couldn't get the version of the python binary: {self.binary}")
            sys.exit(1)
        version = completed_process.stdout.strip()
        version = version.replace("Python ", "")
        return version

    def _interpreter_version(self):
        if self.name == "fbcode-python":
            return self._cpython_version()
        return current_rev()

    def run_benchmark_runner(self, arguments):
        command = create_benchmark_runner_command(self.binary, arguments)
        completed_process = run(command, stdout=subprocess.PIPE)
        if completed_process.returncode != 0:
            print(f"Couldn't run the benchmark runner")
            sys.exit(1)
        benchmark_runner_stdout = completed_process.stdout.strip()
        benchmark_results = ast.literal_eval(benchmark_runner_stdout)

        # Append interpreter metadata
        for b in benchmark_results:
            b["interpreter"] = self.name
            b["version"] = self.version

        return benchmark_results


def main(argv):
    parser = argparse.ArgumentParser(
        description="Pyro benchmark suite",
        formatter_class=argparse.RawTextHelpFormatter,
    )
    parser.add_argument("--verbose", "-v", action="store_true")
    interpreter_help = """Specify interpreter(s) to use:

Available Interpreters:
'fbcode-python': {Interpreter.CPYTHON}
'/path/to/bin/python': Any python 3.6 binary

Default: fbcode-python ./build/python (if existing)

"""
    parser.add_argument(
        "--json", action="store_true", help=f"Print the data in a json format"
    )
    parser.add_argument(
        "--interpreter",
        "-i",
        metavar="INTERPRETER",
        dest="interpreters",
        type=str,
        action="append",
        default=[],
        help=interpreter_help,
    )
    benchmark_help = """Path to a directory with Python benchmarks
Default: pyro/benchmarks

"""
    parser.add_argument(
        "--benchmarks_path",
        "-b",
        metavar="BENCHMARKS_PATH",
        default="",
        type=str,
        action="store",
        help=benchmark_help,
    )
    parser = add_tools_arguments(parser)
    args = parser.parse_args(argv)

    if args.verbose:
        logging.root.setLevel(logging.DEBUG)

    print_json = args.json

    # Set the default pyro/benchmarks path
    if not args.benchmarks_path:
        args.benchmarks_path = f"{repo_root()}/fbcode/pyro/benchmarks"

    # Specify a default if no interpreters were set
    if not args.interpreters:
        # Add fbcode-python if it exists
        if os.path.isfile(Interpreter.CPYTHON):
            args.interpreters.append("fbcode-python")
        # Add ./build/python if it exists
        if os.path.isfile("./build/python"):
            args.interpreters.append("./build/python")
    assert len(args.interpreters) > 0
    interpreters = [Interpreter(i) for i in args.interpreters]

    # Remove arguments that are not needed for _benchmark_runner.py
    benchmark_runner_args = vars(args)
    del benchmark_runner_args["interpreters"]
    del benchmark_runner_args["json"]
    del benchmark_runner_args["verbose"]

    benchmark_results = []
    for interpreter in interpreters:
        logging.info(f"$Running Interpreter: {interpreter.name}")
        runner_result = interpreter.run_benchmark_runner(benchmark_runner_args)
        # TODO(T43453160): Pyro does not return a sorted dict
        benchmark_results.extend([dict(sorted(rr.items())) for rr in runner_result])

    if print_json:
        json_output = json.dumps(benchmark_results)
        if __name__ == "__main__":
            print(json_output)
        return json_output

    if not print_json:
        benchmark_results = post_process_results(benchmark_results, args.tools)
        if __name__ == "__main__":
            print_table(benchmark_results)
        return benchmark_results


if __name__ == "__main__":
    main(sys.argv[1:])
