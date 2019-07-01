#!/usr/bin/env python3
"""
Utility script that provides default arguments for executing a command
with various performance measurement tools.
"""
import time


class PerformanceTool:
    # The main function to execute the specified performance tool.
    # Input: _benchmark_runner.Benchmark
    # Output: A dictionary with the values to be reported
    def run(self, benchmark):
        pass

    # Read any optional command line arguments to set the internal defaults
    # Input: A dictionary with command line arguments
    def __init__(self, args):
        pass

    # Called after all the benchmarks have been run, the post processing allows
    # the results to be modified to provide extra formatting or additional
    # information regarding the collective results
    # Input: A list of dicts with the return values from PerformanceTool.run
    @staticmethod
    def post_process(benchmark_results):
        return benchmark_results

    # Specify the name of the tool along with a description
    @staticmethod
    def add_tool():
        return ""

    # Add any optional command line arguments to tune the tool
    @staticmethod
    def add_optional_arguments(parser):
        return parser


class TimeTool(PerformanceTool):
    NAME = "time"

    # Minimum number of seconds we should run benchmark before
    # results are considered significant.
    MIN_TIME = 0.5

    # The number of runs of each benchmark. If greater than 1, the
    # mean and standard deviation of the runs will be reported
    REPETITIONS = 1

    # Upper bound on the number of iterations
    MAX_ITERATIONS = 1000000000

    def __init__(self, args):
        min_time = args.get("time_min_benchmark_time", TimeTool.MIN_TIME)
        self.min_time = float(min_time)
        reps = args.get("time_num_repetitions", TimeTool.REPETITIONS)
        self.reps = int(reps)
        self.max_iters = TimeTool.MAX_ITERATIONS

    @staticmethod
    def mean(numbers):
        total = 0.0
        for x in numbers:
            total += x
        return total / len(numbers)

    @staticmethod
    def stdev(numbers):
        m = TimeTool.mean(numbers)
        variance = 0.0
        for x in numbers:
            r = x - m
            variance += r ** 2
        variance /= len(numbers) - 1
        return variance ** 0.5

    def _should_report_results(self, iters, tm):
        if iters >= self.max_iters or tm >= self.min_time:
            return True
        return False

    def _do_n_iterations(self, iters):
        total_time = 0.0
        while iters:
            begin = time.time()
            self.module.run()
            end = time.time()
            total_time += end - begin
            iters -= 1
        return total_time

    def _predict_num_iters(self, iters, tm):
        # See how much iterations should be increased by.
        # Note: Avoid division by zero with max(seconds, 1ns).
        multiplier = self.min_time * 1.4 / max(tm, 1e-9)

        # If our last run was at least 10% of self.min_time then we
        # use the multiplier directly.
        # Otherwise we use at most 10 times expansion.
        # Note: When the last run was at least 10% of the min time the max
        # expansion should be 14x.
        is_significant = (tm / self.min_time) > 0.1
        multiplier = multiplier if is_significant else min(10.0, multiplier)
        if multiplier <= 1.0:
            multiplier = 2.0

        # So what seems to be the sufficiently-large iteration count? Round up.
        max_next_iters = 0.5 + max(multiplier * iters, iters + 1.0)

        # But we do have *some* sanity limits though..
        next_iters = min(max_next_iters, self.max_iters)

        return int(next_iters)

    def _do_one_repetition(self):
        iters = 1
        while True:
            tm = self._do_n_iterations(iters)
            if self._should_report_results(iters, tm):
                break
            iters = self._predict_num_iters(iters, tm)
        return (tm / iters, iters)

    def run(self, benchmark):
        self.module = benchmark.module
        tm_results = []
        total_iters = 0
        repetitions = self.reps
        while repetitions > 0:
            tm, iters = self._do_one_repetition()
            tm_results.append(tm)
            total_iters += iters
            repetitions -= 1
        if self.reps == 1:
            return {
                "time_sec": tm_results[0],
                "num_iters": total_iters,
                "num_repetitions": self.reps,
            }
        return {
            "time_sec_mean": TimeTool.mean(tm_results),
            "time_sec_stdev": TimeTool.stdev(tm_results),
            "num_iters": total_iters,
            "num_repetitions": self.reps,
        }

    @staticmethod
    def post_process(benchmark_results):
        num_reps = 1
        if "num_repetitions" in benchmark_results[0]:
            num_reps = benchmark_results[0]["num_repetitions"]

        for br in benchmark_results:
            # Round float values to 6 digits
            if num_reps == 1:
                br["time_sec"] = format(round(br["time_sec"], 6), "0.6f")
            else:
                br["time_sec_mean"] = format(round(br["time_sec_mean"], 6), "0.6f")
                br["time_sec_stdev"] = format(round(br["time_sec_stdev"], 6), "0.6f")

        return benchmark_results

    @staticmethod
    def add_tool():
        return f"""
'{TimeTool.NAME}': Use python's time library to measure the execution time of
a benchmark. This find a statistically stable time measurement by
dynamically determining the number of iterations a given benchmark should run.
"""

    @staticmethod
    def add_optional_arguments(parser):
        min_benchmark_time_help = f"""Optional argument to the 'time' tool.
The minimum number of seconds a benchmark should be run before the results are
considered significant.
Default: {TimeTool.MIN_TIME} sec

"""
        parser.add_argument(
            "--time_min_benchmark_time",
            type=float,
            metavar="MIN_TIME",
            default=TimeTool.MIN_TIME,
            help=min_benchmark_time_help,
        )

        num_repetitions_help = f"""Optional argument to the 'time' tool.
The number of runs of each benchmark. If greater than 1, the mean and
standard deviation of the runs will be reported
Default: {TimeTool.REPETITIONS} repetitions

"""
        parser.add_argument(
            "--time_num_repetitions",
            type=int,
            metavar="REPETITIONS",
            default=TimeTool.REPETITIONS,
            help=num_repetitions_help,
        )
        return parser


REGISTERED_TOOLS = {TimeTool.NAME: TimeTool}


def add_tools_arguments(parser):
    measure_tools_help = """The measurement tool to use.
Available Tools:
"""

    for tool in REGISTERED_TOOLS.values():
        measure_tools_help += tool.add_tool()

    measure_tools_help += """
Default: time

"""
    parser.add_argument(
        "--tool",
        "-t",
        metavar="TOOL",
        dest="tools",
        type=str,
        action="append",
        default=["time"],
        help=measure_tools_help,
    )

    for tool in REGISTERED_TOOLS.values():
        parser = tool.add_optional_arguments(parser)

    return parser


def post_process_results(benchmark_results, tool_names):
    for name, tool in REGISTERED_TOOLS.items():
        if name in tool_names:
            benchmark_results = tool.post_process(benchmark_results)
    return benchmark_results
