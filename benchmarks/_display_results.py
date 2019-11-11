#!/usr/bin/env python3
import logging


logging.basicConfig(level=logging.INFO)
log = logging.getLogger(__name__)


MD_ROW_SEP = "-"
MD_COLUMN_SEP = "|"


def _format_value(value):
    if isinstance(value, float):
        return f"{round(value, 2):,.2f}"
    if isinstance(value, int):
        return f"{round(value, 2):,d}"
    return value


def build_perf_delta_table(benchmark_results, num_interpreters):
    # Deltas are done with every interpreter against the last interpreter
    # Formula: ((new / old) - 1) * 100
    results = []
    benchmark_results.sort(key=lambda x: x["benchmark"], reverse=True)
    last_interpreter = benchmark_results[num_interpreters - 1 :: num_interpreters]
    for idx in range(0, num_interpreters - 1):
        current_interpreter = benchmark_results[idx::num_interpreters]
        for old, new in zip(current_interpreter, last_interpreter):
            result = {
                "benchmark": old["benchmark"],
                "compare": f"{old['interpreter']} vs {new['interpreter']}",
            }
            for k, v in old.items():
                # Ignore any results that are not present in both interpreters
                if k not in new.keys():
                    log.error(f"Metric: {k} is not present in {new['interpreter']}")
                    continue
                # Only compare numerical values
                if not isinstance(v, int) and not isinstance(v, float):
                    continue
                percent = ((new[k] / old[k]) - 1) * 100
                if percent >= 0:
                    result[k] = f"**{round(percent, 2)}%**"
                else:
                    result[k] = f"{round(percent, 2)}%"
            results.append(result)
    return build_table(results)


def build_table(results):
    # Groups results by benchmark name to compare them more easily
    results.sort(key=lambda x: x["benchmark"], reverse=True)

    message = ""
    tbl = [list(results[0].keys())]
    data = [list(r.values()) for r in results]
    data = [list(map(_format_value, row)) for row in data]
    tbl.extend(data)
    cols = [list(x) for x in zip(*tbl)]
    lengths = [max(map(len, map(str, col))) for col in cols]
    row = (
        MD_COLUMN_SEP
        + MD_COLUMN_SEP.join(f"{{:{col_len}}}  " for col_len in lengths)
        + MD_COLUMN_SEP
    )
    header_split = (
        MD_COLUMN_SEP
        + MD_COLUMN_SEP.join(MD_ROW_SEP * (col_len + 2) for col_len in lengths)
        + MD_COLUMN_SEP
    )

    for idx in range(0, len(tbl)):
        row_data = tbl[idx]
        message += row.format(*row_data) + "\n"
        if idx == 0:
            message += header_split + "\n"
    return message
