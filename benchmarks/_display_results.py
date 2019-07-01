#!/usr/bin/env python3


def build_table(results):
    # Groups results by benchmark name to compare them more easily
    results.sort(key=lambda x: x["benchmark"], reverse=True)

    message = ""
    tbl = [list(results[0].keys())]
    data = [list(r.values()) for r in results]
    tbl.extend(data)
    horizontal = "-"
    vertical = "|"
    cross = "+"
    cols = [list(x) for x in zip(*tbl)]
    lengths = [max(map(len, map(str, col))) for col in cols]
    f = vertical + " ".join(" {:>%d} " % l for l in lengths) + vertical
    s = cross + horizontal.join(horizontal * (l + 2) for l in lengths) + cross

    message += s + "\n"
    for idx in range(0, len(tbl)):
        row = tbl[idx]
        message += f.format(*row) + "\n"
        if idx == 0:
            message += s + "\n"
    message += s + "\n"
    return message
