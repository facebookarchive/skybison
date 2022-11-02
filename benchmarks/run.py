#!/usr/bin/env python3
import argparse
import json
import logging
import os
import shlex
import subprocess
import sys
from _display_results import build_table
from _tools import add_tools_arguments, PARALLEL_TOOLS, SEQUENTIAL_TOOLS
from typing import Optional


log = logging.getLogger(__name__)


class Benchmark:
    def __init__(self, path, name, ext):
        self.name = name
        self.path = path
        assert ext == ".py"
        self.ext = ext
        self.bytecode = {}  # mapping of Interpreter to bytecode location

    def __lt__(self, other):
        return self.name.__lt__(other.name)

    def __ne__(self, other):
        return not self.__eq__(other)

    def __eq__(self, other):
        return type(other) is Benchmark and self.name.__eq__(other.name)

    def __repr__(self):
        return self.name

    def filepath(self):
        return f"{self.path}/{self.name}{self.ext}"


class BenchmarkRunner:
    def __init__(self, args, interpreters):
        self.interpreters = interpreters
        self.path = sys.argv[0].rsplit("/", 1)[0]
        self._register_measurement_tools(args["tools"], args)

    def _register_measurement_tools(self, tool_list, args):
        sys.path.append(self.path)
        sys.path.pop()
        self.tools = []
        for tool in SEQUENTIAL_TOOLS:
            if tool.NAME in tool_list:
                self.tools.append(tool(args))
        self.parallel_tools = []
        for tool in PARALLEL_TOOLS:
            if tool.NAME in tool_list:
                self.parallel_tools.append(tool(args))

    @staticmethod
    def merge_parallel_results(results, parallel_results):
        results = sorted(results, key=lambda x: (x["benchmark"], x["interpreter"]))
        parallel_results = sorted(
            parallel_results, key=lambda x: (x["benchmark"], x["interpreter"])
        )
        for seq_result, parallel_result in zip(results, parallel_results):
            seq_result.update(parallel_result)
        return results

    def run_benchmarks(self):
        results = []
        for interpreter in self.interpreters:
            log.info(f"Running interpreter: {interpreter.name}")
            for benchmark in interpreter.benchmarks_to_run:
                log.info(f"Running benchmark: {benchmark.name}")
                result = {"benchmark": benchmark.name, "interpreter": interpreter.name}
                for tool in self.tools:
                    tool_result = tool.execute(interpreter, benchmark)
                    result.update(tool_result)
                results.append(result)
        for tool in self.parallel_tools:
            parallel_results = tool.execute_parallel(
                self.interpreters, self.interpreters[0].benchmarks_to_run
            )
            results = self.merge_parallel_results(results, parallel_results)
        return results


class Interpreter:
    CPYTHON = "fbcode-python"
    CPYTHON_PATH = "/usr/bin/python3.8"

    def __init__(
        self,
        binary_path,
        benchmarks_path,
        interpreter_args: Optional[str] = None,
        interpreter_name: Optional[str] = None,
        benchmark_args: Optional[str] = None,
    ):
        if binary_path == Interpreter.CPYTHON:
            # Running locally (probably from a unit test)
            # This could stand to be refactored
            self.name = Interpreter.CPYTHON
            self.binary_path = Interpreter.CPYTHON_PATH
            self.benchmarks_path = benchmarks_path
        else:
            self.name = binary_path.rsplit("/", 2)[-3]
            self.binary_path = binary_path
            directory = f"{binary_path.rsplit('/', 1)[0]}"
            if not os.path.isabs(benchmarks_path):
                benchmarks_path = os.path.normpath(f"{directory}/../{benchmarks_path}")
            self.benchmarks_path = benchmarks_path

        if interpreter_name is not None:
            self.name = interpreter_name

        self.available_benchmarks = self.discover_benchmarks()
        self.interpreter_cmd = [self.binary_path]
        self.interpreter_args = []
        if interpreter_args is not None:
            self.interpreter_args.extend(shlex.split(interpreter_args))
            self.interpreter_cmd.extend(self.interpreter_args)
        if benchmark_args is not None:
            self.benchmark_args = shlex.split(benchmark_args)
        else:
            self.benchmark_args = []

    def __repr__(self):
        return f"<Interpreter {self.binary_path!r}>"

    def create_benchmark_from_file(self, benchmark_file):
        benchmark_path, _, name_and_ext = benchmark_file.rpartition("/")
        name, _, ext = name_and_ext.rpartition(".")
        benchmark = Benchmark(benchmark_path, name, f".{ext}")
        return benchmark

    def create_benchmark_from_dir(self, benchmark_dir):
        benchmark_path, _, name = benchmark_dir.rpartition("/")
        benchmark = Benchmark(benchmark_path, name, "")
        return benchmark

    def discover_benchmarks(self):
        discovered_benchmarks = []
        for f in os.listdir(self.benchmarks_path):
            path = f"{self.benchmarks_path}/{f}"
            if os.path.isfile(path) and path.endswith(".py"):
                b = self.create_benchmark_from_file(path)
                discovered_benchmarks.append(b)
            elif os.path.isdir(path) and (
                "__pycache__" not in path and "data" not in path
            ):
                b = self.create_benchmark_from_dir(path)
                discovered_benchmarks.append(b)
        return discovered_benchmarks


class PyroBenchmarkSuite:
    def arg_parser(self):
        parser = argparse.ArgumentParser(
            description="Pyro benchmark suite",
            formatter_class=argparse.RawTextHelpFormatter,
        )
        parser.add_argument("--verbose", "-v", action="store_true")
        parser.add_argument(
            "--json", action="store_true", help=f"Print the data in a json format"
        )
        interpreter_help = f"""
Specify interpreter(s) to use:

-i /path/to/python
    """
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
        interpreter_args_help = """
Specify command-line arguments to pass to an interpreter. Arguments must be
supplied as a single string. Arguments apply to the corresponding interpreter.
For example, if you supplied "-i foo -i bar -a '-X debug'" then the arguments
'-X debug' would apply to interpreter 'foo', while interpreter 'bar' would
have no additional arguments.
"""
        parser.add_argument(
            "--interpreter-args",
            "-a",
            metavar="INTERPRETER_ARGS",
            dest="interpreter_args",
            type=str,
            action="append",
            default=[],
            help=interpreter_args_help,
        )
        interpreter_names_help = """
Specify the name that should be used when reporting benchmark results for an
interpreter. Each name applies to the corresponding interpreter, in order.
The value for interpreter is used if no display name is provided. For example,
if you supplied "-i foo -i bar -n 'Foo interp'" then the results for interpreter
'foo' would be displayed as 'Foo interp', while the results for interpreter 'bar'
would be displayed as 'bar'. This is useful if you want to use the same interpreter
with different arguments.
"""
        parser.add_argument(
            "--interpreter-name",
            "-n",
            metavar="INTERPRETER_NAME",
            dest="interpreter_names",
            type=str,
            action="append",
            default=[],
            help=interpreter_names_help,
        )
        benchmark_args_help = """
Specify command-line arguments to pass to the benchmarks. Arguments must be
supplied as a single string. Arguments apply to all the benchmarks.
For example, if you supplied "-i lmao -i xyz -b foo -b bar --benchmark-args=
--benchmark-args='-X debug'" then the arguments '-X debug' would apply to all
the benchmarks run under interpreter 'xyz' and none for interpreter 'lmao'.
"""
        parser.add_argument(
            "--benchmark-args",
            metavar="BENCHMARK_ARGS",
            dest="benchmark_args",
            type=str,
            action="append",
            default=[],
            help=benchmark_args_help,
        )
        benchmarks_path_help = f"""
Specify benchmarks_path(s) to use. This must match with the interpreters used

-p /path/to/benchmarks

    """
        parser.add_argument(
            "--path",
            "-p",
            metavar="BENCHMARK_PATH",
            dest="benchmarks_path",
            type=str,
            action="append",
            default=[],
            help=benchmarks_path_help,
        )
        benchmark_help = f"""
The benchmark that you wish to run. Use repeatedly
to select more than one benchmark:

-b richards

Default: all

    """
        parser.add_argument(
            "--benchmark",
            "-b",
            metavar="BENCHMARK",
            dest="benchmarks",
            type=str,
            action="append",
            default=[],
            help=benchmark_help,
        )
        parser = add_tools_arguments(parser)
        return parser

    def start_benchmarks(self, args):
        log.info(f"Verifying benchmark arguments")

        # Check that at least one tool was selected
        if not args.tools:
            raise Exception("At least one `--tool` should be specified")

        # Check that at least one interpreter was selected
        if not args.interpreters:
            raise Exception("At least one `--interpreter` should be specified")

        # Check that benchmarks path matches the number of interpreters
        if len(args.benchmarks_path) != len(args.interpreters):
            raise Exception("The number of --interpreter and --path should match")

        if len(args.interpreter_args) > len(args.interpreters):
            raise Exception(
                "The number of interpreter arguments cannot exceed the number"
                " of interpreters"
            )

        assert len(args.interpreters) > 0
        interpreters = []
        for i, interp in enumerate(args.interpreters):
            interp_args = None
            if i < len(args.interpreter_args):
                interp_args = args.interpreter_args[i]
            interp_name = None
            if i < len(args.interpreter_names):
                interp_name = args.interpreter_names[i]
            benchmark_args = None
            if i < len(args.benchmark_args):
                benchmark_args = args.benchmark_args[i]
            interpreters.append(
                Interpreter(
                    interp,
                    args.benchmarks_path[i],
                    interp_args,
                    interp_name,
                    benchmark_args,
                )
            )

        # If no benchmark is defined, add all of them
        if not args.benchmarks:
            for interpreter in interpreters:
                to_run = [b for b in interpreter.available_benchmarks]
                interpreter.benchmarks_to_run = sorted(to_run)
        else:
            for interpreter in interpreters:
                interpreter.benchmarks_to_run = []
                for b in interpreter.available_benchmarks:
                    if b.name in args.benchmarks:
                        interpreter.benchmarks_to_run.append(b)
                interpreter.benchmarks_to_run = sorted(interpreter.benchmarks_to_run)

        # Try to run the benchmarks of the interpreter with the least benchmarks
        benchmarks_to_run = interpreters[0].benchmarks_to_run
        for interpreter in interpreters:
            if len(interpreter.benchmarks_to_run) < len(benchmarks_to_run):
                benchmarks_to_run = interpreter.benchmarks_to_run
        for interpreter in interpreters:
            temp_to_run = interpreter.benchmarks_to_run
            for benchmark in interpreter.benchmarks_to_run:
                if benchmark not in benchmarks_to_run:
                    log.info(f"Removing: {benchmark}")
                    temp_to_run.remove(benchmark)
            interpreter.benchmarks_to_run = temp_to_run

        # Only run if all interpreters have the same benchmarks to run
        assert len(benchmarks_to_run) > 0
        log.info(f"Will run the following benchmarks: {benchmarks_to_run}")
        for interpreter in interpreters:
            if benchmarks_to_run != interpreter.benchmarks_to_run:
                raise Exception(
                    "Can't run parallel tools. The interpreters "
                    "have different available benchmarks: "
                    f"{benchmarks_to_run} vs {interpreter.benchmarks_to_run}"
                )

        print_json = args.json

        log.info(f"Running benchmarks with args: {args}")
        runner = BenchmarkRunner(vars(args), interpreters)
        try:
            benchmark_results = runner.run_benchmarks()
        except subprocess.CalledProcessError as cpe:
            raise RuntimeError(
                f"{cpe}\n\nstdout:\n{cpe.stdout}\n\nstderr:\n{cpe.stderr}"
            )
        log.info(benchmark_results)

        if print_json:
            json_output = json.dumps(benchmark_results)
            if __name__ == "__main__":
                print(json_output)
            return json_output

        if not print_json:
            if __name__ == "__main__":
                print(build_table(benchmark_results))
            return benchmark_results


def main(argv):
    suite = PyroBenchmarkSuite()
    parser = suite.arg_parser()
    args = parser.parse_args(argv)
    logging.basicConfig(
        level=logging.DEBUG if args.verbose else logging.WARN, format="%(message)s"
    )
    return suite.start_benchmarks(args)


if __name__ == "__main__":
    main(sys.argv[1:])
