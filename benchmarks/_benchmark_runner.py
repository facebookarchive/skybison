#!/usr/bin/env python3
import sys


class Benchmark:
    def __init__(self, path, name):
        self.name = name
        self.path = path
        sys.path.append(path)
        self.module = __import__(name)
        sys.path.pop()


class BenchmarkRunner:
    def __init__(self, path):
        self.path = path
        self.benchmarks = []
        self.tools = []

    def register_benchmarks(self, benchmarks_path):
        for b_path in benchmarks_path:
            benchmark_path, _, name_and_ext = b_path.rpartition("/")
            name, _, _ = name_and_ext.rpartition(".")
            benchmark = Benchmark(benchmark_path, name)
            self.benchmarks.append(benchmark)

    def register_measurement_tools(self, args):
        sys.path.append(self.path)
        registered_tools = __import__("_tools").REGISTERED_TOOLS
        sys.path.pop()
        for k, v in registered_tools.items():
            if k in args["tools"]:
                self.tools.append(v(args))

    def run_benchmarks(self):
        results = []
        for benchmark in self.benchmarks:
            result = {"benchmark": benchmark.name}
            for tool in self.tools:
                tool_result = tool.run(benchmark)
                result.update(tool_result)
            results.append(result)
        return results


def main():
    # TODO(eelizondo): Add argparse support
    args = {"tools": [], "benchmarks": []}
    i = 1
    while i < len(sys.argv):
        if sys.argv[i] == "tool":
            args["tools"].append(sys.argv[i + 1])
        elif sys.argv[i] == "benchmark":
            args["benchmarks"].append(sys.argv[i + 1])
        else:
            args[sys.argv[i]] = sys.argv[i + 1]
        i += 2

    path = sys.argv[0].rsplit("/", 1)[0]
    runner = BenchmarkRunner(path)
    runner.register_benchmarks(args["benchmarks"])
    runner.register_measurement_tools(args)
    results = runner.run_benchmarks()

    # Print results for benchmark.py to parse and report it
    print(results)


if __name__ == "__main__":
    main()
