#!/usr/bin/env python3
import os
import sys
import time


class TimeTool:
    # Minimum number of seconds we should run benchmark before
    # results are considered significant.
    MIN_TIME = 0.5

    # The number of runs of each benchmark. If greater than 1, the
    # mean and standard deviation of the runs will be reported
    REPETITIONS = 1

    # Upper bound on the number of iterations
    MAX_ITERATIONS = 1000000000

    def __init__(self):
        self.min_time = TimeTool.MIN_TIME
        self.reps = TimeTool.REPETITIONS
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

    def run(self, module):
        self.module = module
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


def main(argv):
    benchmark_path = os.path.realpath(sys.argv[1])
    directory, _, name_and_ext = benchmark_path.rpartition("/")
    name, _, _ = name_and_ext.rpartition(".")
    sys.path.append(directory)
    module = __import__(name)
    module_file = getattr(module, "__file__", "<builtin>")
    if os.path.realpath(module_file) != benchmark_path:
        print(
            f"Module {name} was imported from {module_file}, not "
            f"{benchmark_path} as expected. Does your benchmark name conflict "
            "with a builtin module?",
            file=sys.stderr,
        )
        sys.exit(1)
    sys.path.pop()
    time_tool = TimeTool()
    result = time_tool.run(module)
    for k, v in result.items():
        print(k, ",", v)


if __name__ == "__main__":
    main(sys.argv[1:])
