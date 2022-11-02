#!/usr/bin/env python3

import json
import logging
import sys
from os import path


logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)

LEFT_ALIGN = " :----- "
RIGHT_ALIGN = " -----: "
METRICS_HEADER = (
    ("benchmark", LEFT_ALIGN),
    ("task-clock", RIGHT_ALIGN),
    ("instructions", RIGHT_ALIGN),
    ("branch-misses", RIGHT_ALIGN),
    ("L1-icache-load-misses", RIGHT_ALIGN),
    ("cg_instructions", RIGHT_ALIGN),
)
METRICS = [metric_name for metric_name, align in METRICS_HEADER]
SUMMARY_HEADER = (
    ("Metric", LEFT_ALIGN),
    ("Average", LEFT_ALIGN),
    ("Best", LEFT_ALIGN),
    ("Worst", LEFT_ALIGN),
    ("Notes", LEFT_ALIGN),
)
SUMMARY_METRIC_NOISE_MAP = {
    "cg_instructions": "typically < 0.2% noise",
    "task-clock": "typically 1-2% noise",
    "branch-misses": "typically >10% noise",
    "L1-icache-load-misses": "typically >10% noise",
    "instructions": "typically 3% noise",
}

# Some hard-coded strings
BENCHMARK = "benchmark"
INTERPRETER = "interpreter"

# variants
CPYTHON_VARIANT = "fbcode-cpython"
BASE_VARIANT = "python_base"
NEW_VARIANT = "python_new"
BASE_JIT_VARIANT = "python_base+jit"
NEW_JIT_VARIANT = "python_new+jit"
VARIANTS = (
    CPYTHON_VARIANT,
    BASE_VARIANT,
    NEW_VARIANT,
    BASE_JIT_VARIANT,
    NEW_JIT_VARIANT,
)


def read_filename_from_arg():
    """
    Read filename from argument, and ensure that the only argument provided is the filename.
    """
    if len(sys.argv) != 2:
        sys.exit("Invalid argument. Expect one argument (filename)")
    return sys.argv[1]


def ensure_file_exists(filename):
    """
    Ensures that the file provided in the arguments exist.
    """
    if not path.isfile(filename):
        sys.exit(f"The provided file does not exist: {filename}")


def read_json(filename):
    """
    Loads and ensure that the json file is valid.
    """
    with open(filename) as f:
        try:
            return json.load(f)
        except json.decoder.JSONDecodeError:
            sys.exit(f"The provided json file is corrupted: {filename}")


def split_json_by_variants(data):
    """
    From the json input, split the data into the different variants.
    """
    variants = {variant: [] for variant in VARIANTS}
    for row in data:
        interpreter = row[INTERPRETER]
        variants[interpreter].append(row)
    return variants


def generate_markdown_row_partition(data):
    """
    Convert a list into a string separated by `|`.

    eg.
    ['apple', 'boy', 'car'] -> "| apple | boy | car |".
    """
    return "|".join([""] + data + [""])


def generate_markdown_table_header(title, cols):
    """
    Generate the markdown table's header

    eg.
    title: Summary, cols: [('apple', RIGHT_ALIGN), ('boy', LEFT_ALIGN), ('car', LEFT_ALIGN)]
    -> ["# Summary", "| apple | boy | car |", "| :----- | -----: | -----: |"]
    """
    table = [f"# {title}", ""]
    column_header = [f" {col_name} " for col_name, _ in cols]
    table.append(generate_markdown_row_partition(column_header))
    table.append(generate_markdown_row_partition([col_align for _, col_align in cols]))
    return table


def generate_markdown_table(title, cols, rows, is_percentage=False):
    """
    Generate the markdown table from the data.
    """

    def format_value(value):
        if isinstance(value, (int, float)):
            if is_percentage:
                return " {:.1%} ".format(value)
            else:
                return f" {value:,} "
        else:
            return f" {value} "

    table = generate_markdown_table_header(title, cols)
    for row in rows:
        row_data = [
            format_value(row[col]) for col in [col_name for col_name, _ in cols]
        ]
        table.append(generate_markdown_row_partition(row_data))
    return "\n".join(table)


def compare_metric(old_data, new_data, title):
    """
    From the old and new data, compute percentage change for each benchmark.
    """
    benchmark_to_data = {}
    unmatched = []
    for row in old_data:
        benchmark = row[BENCHMARK]
        benchmark_to_data[benchmark] = [row]
    for row in new_data:
        benchmark = row[BENCHMARK]
        if benchmark not in benchmark_to_data:
            unmatched.append(benchmark)
            continue
        benchmark_to_data[benchmark].append(row)

    result = []
    for benchmark, data in benchmark_to_data.items():
        if len(data) != 2:
            unmatched.append(benchmark)
            continue
        old_metric_data, new_metric_data = data
        metric_data0 = [old_metric_data[metric] for metric in METRICS]
        metric_data1 = [new_metric_data[metric] for metric in METRICS]
        row = {BENCHMARK: benchmark}
        for index, metric in enumerate(METRICS):
            if metric == BENCHMARK:
                continue
            row[metric] = metric_data1[index] / metric_data0[index] - 1
        result.append(row)
    if unmatched:
        log.warn(
            f"The following benchmarks cannot be matched (so results are discarded) when comparing {title}: {unmatched}"
        )
        return []
    return result


def generate_summary(data):
    result = []
    for metric, notes in SUMMARY_METRIC_NOISE_MAP.items():
        avg = sum(d[metric] for d in data) / len(data) if data else 0
        best = min(data, key=lambda x: x[metric])  # smaller is better
        worst = max(data, key=lambda x: x[metric])  # larger is worse

        row = {
            "Metric": metric,
            "Average": f"**{avg:.1%}**",
            "Best": f"{best[BENCHMARK]} {best[metric]:.1%}",
            "Worst": f"{worst[BENCHMARK]} {worst[metric]:.1%}",
            "Notes": notes,
        }
        result.append(row)
    return result


if __name__ == "__main__":
    filename = read_filename_from_arg()
    ensure_file_exists(filename)
    data = read_json(filename)
    variants = split_json_by_variants(data)

    print(
        generate_markdown_table(
            "Summary",
            SUMMARY_HEADER,
            generate_summary(
                compare_metric(variants[BASE_VARIANT], variants[NEW_VARIANT], "summary")
            ),
        )
    )
    print("[See also](https://www.internalfb.com/intern/wiki/Pyro/PerformanceResults)")
    print()
    print(
        generate_markdown_table(
            "Base vs. New",
            METRICS_HEADER,
            compare_metric(
                variants[BASE_VARIANT], variants[NEW_VARIANT], "base vs new"
            ),
            is_percentage=True,
        )
    )
    print()
    print(
        generate_markdown_table(
            "Base (JIT) vs. New (JIT)",
            METRICS_HEADER,
            compare_metric(
                variants[BASE_JIT_VARIANT], variants[NEW_JIT_VARIANT], "base vs new"
            ),
            is_percentage=True,
        )
    )
    print()
    print(
        generate_markdown_table(
            "Non-JIT vs. JIT",
            METRICS_HEADER,
            compare_metric(
                variants[NEW_VARIANT], variants[NEW_JIT_VARIANT], "non-jit vs jit"
            ),
            is_percentage=True,
        )
    )
    print()
    print(
        generate_markdown_table(
            "CPython vs New",
            METRICS_HEADER,
            compare_metric(
                variants[CPYTHON_VARIANT], variants[NEW_VARIANT], "cpython vs new"
            ),
            is_percentage=True,
        )
    )
    print()
    print(generate_markdown_table("Base", METRICS_HEADER, variants[BASE_VARIANT]))
    print()
    print(
        generate_markdown_table(
            "Base (JIT)", METRICS_HEADER, variants[BASE_JIT_VARIANT]
        )
    )
    print()
    print(generate_markdown_table("New", METRICS_HEADER, variants[NEW_VARIANT]))
    print()
    print(
        generate_markdown_table("New (JIT)", METRICS_HEADER, variants[NEW_JIT_VARIANT])
    )
    print()
    print(generate_markdown_table("CPython", METRICS_HEADER, variants[CPYTHON_VARIANT]))
