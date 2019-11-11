#!/usr/bin/env python3

import argparse
import collections
import os
import re
import subprocess
import sys


Stat = collections.namedtuple("Stat", ["count", "stddev", "min", "max"])


def measure_binary(binary, repetitions, events, common_args):
    events_args = []
    for event in events:
        events_args += ["-e", event]
    env = dict(os.environ)
    env["PYTHONHASHSEED"] = "0"

    return subprocess.run(
        [
            "perf",
            "stat",
            "--field-separator",
            ";",
            "--repeat",
            repetitions,
            *events_args,
            binary,
            *common_args,
        ],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.PIPE,
        encoding=sys.stderr.encoding,
        env=env,
        check=True,
    ).stderr


class RetryMeasurement(Exception):
    pass


IGNORE_LINES = {
    "Using perf wrapper that supports hot-text. Try perf.real if you encounter any issues.",
    "",
}


def parse_perf_stat_output(output):
    stats = {}
    for line in output.split("\n"):
        line = line.strip()
        if line in IGNORE_LINES:
            continue
        parts = line.split(";")
        if len(parts) != 8 and len(parts) != 10:
            print(f"Unexpected line: {line}", file=sys.stderr)
            sys.exit(1)

        run_ratio = float(parts[5].replace("%", ""))
        if run_ratio != 100.0:
            raise RetryMeasurement()

        count = int(parts[0])
        counter = parts[2]
        stddev = float(parts[3].replace("%", "")) / 100 * count
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


def collect_events(binary, repeat, events, common_args):
    tries = 10
    while True:
        output = measure_binary(binary, repeat, events, common_args)
        if "nmi_watchdog" in output:
            # Results reported when the nmi_watchdog interfered are useless.
            sys.stderr.write("\n\nError: perf stat complained about nmi_watchdog\n")
            sys.stderr.write(output)
            sys.stderr.write("\n\nAborting\n")
            sys.exit(1)
        try:
            return parse_perf_stat_output(output)
        except RetryMeasurement:
            if tries == 1:
                print(f"Failed to measure {events} for {binary}", file=sys.stderr)
                sys.exit(1)
            tries -= 1
            print(
                f"Re-measuring {events} for {binary}, {tries} attempts left",
                file=sys.stderr,
            )


def main():
    args = parse_args()
    if not args.event:
        args.event = ["cycles", "instructions", "branches", "branch-misses"]
    if not args.common_arg:
        args.common_arg = ["benchmarks/benchmarks/richards.py"]

    group_size = 2
    event_groups = [
        args.event[i : i + group_size] for i in range(0, len(args.event), group_size)
    ]
    results = []
    for binary in args.binary:
        print(f"Measuring {' '.join([binary] + args.common_arg)}", file=sys.stderr)
        stats = {}
        for events in event_groups:
            stats.update(collect_events(binary, args.repeat, events, args.common_arg))
        results.append(stats)

    diff_perf_stats(args.binary, results)


if __name__ == "__main__":
    sys.exit(main())
