#!/usr/bin/env python3


MD_ROW_SEP = "-"
MD_COLUMN_SEP = "|"


def _format_value(value):
    if isinstance(value, float):
        return f"{round(value, 2):,.2f}"
    if isinstance(value, int):
        return f"{round(value, 2):,d}"
    return value


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
