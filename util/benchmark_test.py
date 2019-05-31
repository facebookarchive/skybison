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


if __name__ == "__main__":
    unittest.main()
