#!/usr/bin/env python3
import unittest

from _display_results import build_perf_delta_table, build_table


class TestDisplayResults(unittest.TestCase):
    maxDiff = None

    def test_build_table(self):
        data = [
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
                "interpreter": "python_new",
                "task-clock": 227.53,
                "instructions": 1478320822,
            },
            {
                "benchmark": "deltablue",
                "interpreter": "python_new",
                "task-clock": 1500.82,
                "instructions": 10139403790,
            },
        ]
        result = "\n" + build_table(data)
        expected_result = """
|benchmark  |interpreter    |task-clock  |instructions    |
|-----------|---------------|------------|----------------|
|richards   |fbcode-python  |867.86      |5,875,444,144   |
|richards   |python_base    |228.80      |1,478,343,960   |
|richards   |python_new     |227.53      |1,478,320,822   |
|deltablue  |fbcode-python  |813.83      |5,776,832,979   |
|deltablue  |python_base    |1,414.33    |9,713,750,376   |
|deltablue  |python_new     |1,500.82    |10,139,403,790  |
"""
        self.assertEqual(result, expected_result)

    def test_build_perf_delta_table(self):
        data = [
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
                "interpreter": "python_new",
                "task-clock": 227.53,
                "instructions": 1478320822,
            },
            {
                "benchmark": "deltablue",
                "interpreter": "python_new",
                "task-clock": 1500.82,
                "instructions": 10139403790,
            },
        ]
        num_interpreters = 3
        result = "\n" + build_perf_delta_table(data, num_interpreters)
        expected_result = """
|benchmark  |compare                      |task-clock  |instructions  |
|-----------|-----------------------------|------------|--------------|
|richards   |fbcode-python vs python_new  |-73.78%     |-74.84%       |
|richards   |python_base vs python_new    |-0.56%      |-0.0%         |
|deltablue  |fbcode-python vs python_new  |**84.41%**  |**75.52%**    |
|deltablue  |python_base vs python_new    |**6.12%**   |**4.38%**     |
"""
        self.assertEqual(result, expected_result)


if __name__ == "__main__":
    unittest.main()
