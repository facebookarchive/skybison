#!/usr/bin/env python3
import json
import unittest

import benchmark


class TestBenchmark(unittest.TestCase):
    def test_benchmark_json(self):
        arguments = ["-i", "fbcode-python", "--json"]
        json_output = json.loads(benchmark.main(arguments))
        # There's at least 4 default benchmarks
        self.assertGreater(len(json_output), 3)
        single_result = json_output[0]

        # Test the basic json output results
        self.assertIn("benchmark", single_result)
        self.assertIn("interpreter", single_result)
        self.assertIn("num_iters", single_result)
        self.assertIn("num_repetitions", single_result)
        self.assertIn("time_sec", single_result)
        self.assertIn("version", single_result)

    def test_benchmark_table(self):
        arguments = ["-i", "fbcode-python"]
        list_output = benchmark.main(arguments)
        # There's at least 4 default benchmarks
        self.assertGreater(len(list_output), 3)
        single_result = list_output[0]

        # Test the basic json output results
        self.assertIn("benchmark", single_result)
        self.assertIn("interpreter", single_result)
        self.assertNotIn("num_iters", single_result)
        self.assertNotIn("num_repetitions", single_result)
        self.assertIn("time_sec", single_result)
        self.assertIn("version", single_result)

    def test_choose_single_benchmark(self):
        arguments = ["-i", "fbcode-python", "-b", "richards", "--json"]
        json_output = json.loads(benchmark.main(arguments))
        self.assertEqual(len(json_output), 1)
        single_result = json_output[0]
        self.assertEqual(single_result["benchmark"], "richards")

    def test_choose_multiple_benchmarks(self):
        arguments = [
            "-i",
            "fbcode-python",
            "-b",
            "richards",
            "-b",
            "deltablue",
            "--json",
        ]
        json_output = json.loads(benchmark.main(arguments))
        self.assertEqual(len(json_output), 2)
        expected_results = ["deltablue", "richards"]
        result1 = json_output[0]
        result2 = json_output[1]
        results = sorted([result1["benchmark"], result2["benchmark"]])
        self.assertListEqual(expected_results, results)


if __name__ == "__main__":
    unittest.main()
