#!/usr/bin/env python3

import argparse
import collections
import re
import subprocess
import sys


STAT_REGEX = re.compile(r"([\d,]+)\s+([-\w]+)\s+(?:#.+)?\s+\( \+-\s*([\d.]+)% \)")


Stat = collections.namedtuple("Stat", ["count", "stddev", "min", "max"])


def measure_binary(binary, repetitions, events, common_args):
    events_args = []
    for event in events:
        events_args += ["-e", event]
    print(f"Running {' '.join([binary] + common_args)}", file=sys.stderr)
    return subprocess.run(
        ["perf", "stat", "-r", repetitions, *events_args, binary, *common_args],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        encoding=sys.stderr.encoding,
        env={"PYRO_ENABLE_CACHE": "1", "PYTHONHASHSEED": "0"},
        check=True,
    ).stderr


def parse_perf_stat_output(output):
    stats = {}

    for line in output.split("\n"):
        match = STAT_REGEX.search(line)
        if match:
            count = int(match[1].replace(",", ""))
            counter = match[2]
            stddev = float(match[3]) * count / 100
            stats[counter] = Stat(count, stddev, count - stddev, count + stddev)

    return stats


def diff_perf_stats(binaries, stats):
    max_stat_len = 0
    for stats_dict in stats:
        for counter in stats_dict.keys():
            max_stat_len = max(max_stat_len, len(counter))
    stat_format = (
        f"  {{:{max_stat_len}}}" + " : {:+5.1f}% to {:+5.1f}%, mean {:+5.1f}%{}"
    )

    extra_line = ""
    for i in range(len(binaries)):
        for j in range(i + 1, len(binaries)):
            print(extra_line, end="")
            extra_line = "\n"

            prog1, stats1 = binaries[i], stats[i]
            prog2, stats2 = binaries[j], stats[j]

            print(f"{prog1} -> {prog2}")
            for counter, stat1 in stats1.items():
                stat2 = stats2[counter]

                min_change = (stat2.min / stat1.max - 1) * 100
                max_change = (stat2.max / stat1.min - 1) * 100
                change = (stat2.count / stat1.count - 1) * 100

                # Indicate results that have a very small delta or include 0 in
                # their range.
                tail = ""
                if (
                    abs(min_change) < 0.1
                    or abs(max_change) < 0.1
                    or (min_change < 0 and max_change > 0)
                ):
                    tail = " ✗"
                print(stat_format.format(counter, min_change, max_change, change, tail))


def parse_args():
    parser = argparse.ArgumentParser(
        description="""
Run the given binaries under `perf stat` and print pairwise comparisons of the
results. Delta ranges are printed using one standard deviation from the mean in
either direction. When either side of the interval is within 0.1% of 0, an '✗'
is used to indicate a probably-insignificant result.
"""
    )

    parser.add_argument("binary", nargs="+")
    parser.add_argument(
        "-r", "--repeat", default="12", help="Number of repetitions for each binary."
    )
    parser.add_argument(
        "--common-arg",
        action="append",
        help="Argument(s) to pass to each binary (may be given multiple times).",
    )
    parser.add_argument(
        "-e",
        "--event",
        action="append",
        help="Events to pass to `perf stat`. Defaults to cycles, instructions, "
        "branches, and branch-misses if not given.",
    )

    return parser.parse_args()


def main():
    args = parse_args()
    if not args.event:
        args.event = ["cycles", "instructions", "branches", "branch-misses"]
    if not args.common_arg:
        args.common_arg = ["benchmarks/richards.py"]

    results = []
    for binary in args.binary:
        output = measure_binary(binary, args.repeat, args.event, args.common_arg)
        results.append(parse_perf_stat_output(output))
    diff_perf_stats(args.binary, results)


if __name__ == "__main__":
    sys.exit(main())
