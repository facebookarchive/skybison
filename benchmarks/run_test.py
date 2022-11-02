#!/usr/bin/env python3
import distutils.spawn
import json
import os
import sys
import unittest

import _tools
import run


def tool_exists(name):
    return distutils.spawn.find_executable(name) is not None


BENCHMARKS_PATH = f"{os.path.dirname(os.path.realpath(__file__))}/benchmarks"


class TestBenchmark(unittest.TestCase):
    def test_benchmark_pins_to_single_cpu_from_list(self):
        available_cpus = "1,2,3,4"
        result = _tools.create_taskset_command(available_cpus)
        self.assertEqual(result, ["taskset", "--cpu-list", "1"])

    def test_benchmark_pins_to_single_cpu_from_range(self):
        available_cpus = "7-13"
        result = _tools.create_taskset_command(available_cpus)
        self.assertEqual(result, ["taskset", "--cpu-list", "7"])

    def test_benchmark_json(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        # There's at least 4 default benchmarks
        self.assertGreater(len(json_output), 3)
        single_result = json_output[0]

        # Test the basic json output results
        self.assertIn("benchmark", single_result)
        self.assertIn("interpreter", single_result)
        self.assertIn("time_sec", single_result)

    def test_benchmark_table(self):
        arguments = ["-i", "fbcode-python", "-p", BENCHMARKS_PATH, "-t", "time"]
        list_output = run.main(arguments)
        # There's at least 4 default benchmarks
        self.assertGreater(len(list_output), 3)
        single_result = list_output[0]

        # Test the basic json output results
        self.assertIn("benchmark", single_result)
        self.assertIn("interpreter", single_result)
        self.assertIn("time_sec", single_result)

    def test_choose_single_benchmark(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "richards",
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        self.assertEqual(len(json_output), 1)
        single_result = json_output[0]
        self.assertEqual(single_result["benchmark"], "richards")

    def test_choose_go_benchmark(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "go",
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        self.assertEqual(len(json_output), 1)
        single_result = json_output[0]
        self.assertEqual(single_result["benchmark"], "go")

    def test_choose_nqueens_benchmark(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "nqueens",
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        self.assertEqual(len(json_output), 1)
        single_result = json_output[0]
        self.assertEqual(single_result["benchmark"], "nqueens")

    def test_choose_nbody_benchmark(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "nbody",
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        self.assertEqual(len(json_output), 1)
        single_result = json_output[0]
        self.assertEqual(single_result["benchmark"], "nbody")

    def test_choose_base64_benchmark(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "bench_base64",
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        self.assertEqual(len(json_output), 1)
        single_result = json_output[0]
        self.assertEqual(single_result["benchmark"], "bench_base64")

    def test_choose_pyflate_benchmark(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "pyflate",
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        self.assertEqual(len(json_output), 1)
        single_result = json_output[0]
        self.assertEqual(single_result["benchmark"], "pyflate")

    def test_choose_2to3_benchmark(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "2to3",
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        self.assertEqual(len(json_output), 1)
        single_result = json_output[0]
        self.assertEqual(single_result["benchmark"], "2to3")

    def test_choose_bench_pickle_benchmark(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "bench_pickle",
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        self.assertEqual(len(json_output), 1)
        single_result = json_output[0]
        self.assertEqual(single_result["benchmark"], "bench_pickle")

    def test_choose_multiple_benchmarks(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "richards",
            "-b",
            "deltablue",
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        self.assertEqual(len(json_output), 2)
        expected_results = ["deltablue", "richards"]
        result1 = json_output[0]
        result2 = json_output[1]
        results = sorted([result1["benchmark"], result2["benchmark"]])
        self.assertListEqual(expected_results, results)

    def test_callgrind(self):
        # valgrind is not available on all machines
        if not tool_exists("valgrind"):
            return

        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "deltablue",
            "-b",
            "richards",
            "-t",
            "callgrind",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        single_result = json_output[0]

        self.assertIn("benchmark", single_result)
        self.assertIn("interpreter", single_result)
        self.assertIn("cg_instructions", single_result)

    def test_perfstat(self):
        # perf stat can't run on MacOS
        if not tool_exists("perf"):
            return

        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "deltablue",
            "-t",
            "perfstat",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        single_result = json_output[0]

        self.assertIn("benchmark", single_result)
        self.assertIn("interpreter", single_result)
        self.assertIn("task-clock", single_result)
        self.assertIn("instructions", single_result)

    def test_perfstat_custom_events(self):
        # perf stat can't run on MacOS
        if not tool_exists("perf"):
            return

        arguments = [
            "--interpreter",
            "fbcode-python",
            "--path",
            BENCHMARKS_PATH,
            "--benchmark",
            "deltablue",
            "--tool",
            "perfstat",
            "--event",
            "cycles",
            "--event",
            "branches",
            "--event",
            "branch-misses",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        single_result = json_output[0]
        self.assertIn("benchmark", single_result)
        self.assertIn("interpreter", single_result)
        self.assertIn("cycles", single_result)
        self.assertIn("branches", single_result)
        self.assertIn("branch-misses", single_result)

    def test_multiple_tools(self):
        # perf stat can't run on MacOS
        if not tool_exists("perf"):
            return
        if not tool_exists("valgrind"):
            return

        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "-b",
            "deltablue",
            "-b",
            "richards",
            "-t",
            "time",
            "-t",
            "perfstat",
            "-t",
            "callgrind",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        single_result = json_output[0]

        # Both time_sec from time and task-clock from perfstat are present
        self.assertIn("time_sec", single_result)
        self.assertIn("task-clock", single_result)
        self.assertIn("cg_instructions", single_result)

    def test_benchmark_args(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-p",
            BENCHMARKS_PATH,
            "--benchmark-args=--jit",
            "-b",
            "richards",
            "-t",
            "time",
            "--json",
        ]
        json_output = json.loads(run.main(arguments))
        self.assertEqual(len(json_output), 1)
        single_result = json_output[0]
        self.assertEqual(single_result["benchmark"], "richards")

    def test_benchmark_runner_merge_parallel_results(self):
        seq_data = [
            {
                "benchmark": "richards",
                "interpreter": "python_base",
                "task-clock": 228.8,
                "instructions": 1478343960,
            },
            {
                "benchmark": "deltablue",
                "interpreter": "python_base",
                "task-clock": 1414.33,
                "instructions": 9713750376,
            },
            {
                "benchmark": "richards",
                "interpreter": "fbcode-python",
                "task-clock": 867.86,
                "instructions": 5875444144,
            },
            {
                "benchmark": "deltablue",
                "interpreter": "fbcode-python",
                "task-clock": 813.83,
                "instructions": 5776832979,
            },
        ]
        parallel_data = [
            {
                "benchmark": "deltablue",
                "interpreter": "python_base",
                "cg_instructions": 9976758111,
            },
            {
                "benchmark": "deltablue",
                "interpreter": "fbcode-python",
                "cg_instructions": 5779752171,
            },
            {
                "benchmark": "richards",
                "interpreter": "fbcode-python",
                "cg_instructions": 5792328343,
            },
            {
                "benchmark": "richards",
                "interpreter": "python_base",
                "cg_instructions": 1501239987,
            },
        ]
        expected_results = [
            {
                "benchmark": "deltablue",
                "interpreter": "fbcode-python",
                "task-clock": 813.83,
                "instructions": 5776832979,
                "cg_instructions": 5779752171,
            },
            {
                "benchmark": "deltablue",
                "interpreter": "python_base",
                "task-clock": 1414.33,
                "instructions": 9713750376,
                "cg_instructions": 9976758111,
            },
            {
                "benchmark": "richards",
                "interpreter": "fbcode-python",
                "task-clock": 867.86,
                "instructions": 5875444144,
                "cg_instructions": 5792328343,
            },
            {
                "benchmark": "richards",
                "interpreter": "python_base",
                "task-clock": 228.8,
                "instructions": 1478343960,
                "cg_instructions": 1501239987,
            },
        ]
        results = run.BenchmarkRunner.merge_parallel_results(seq_data, parallel_data)
        self.assertEqual(results, expected_results)


if __name__ == "__main__":
    unittest.main()
