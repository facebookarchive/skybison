#!/usr/bin/env python3
"""
Utility script that provides default arguments for executing a command
with various performance measurement tools.
"""
import argparse
import json
import logging
import re
import subprocess
import sys
import tempfile
import time


def _run(command):
    logging.info(f"$ {' '.join(command)}")
    proc = subprocess.run(command, encoding="UTF-8", stdout=subprocess.DEVNULL)
    if proc.returncode != 0:
        logging.fatal("Benchmark did not run successfully")
        sys.exit(proc.returncode)
    return {}


def _extract_callgrind_results(filename):
    instructions = None
    with open(filename) as cgfile:
        r = re.compile(r"summary:\s*(.*)")
        for line in cgfile:
            m = r.match(line)
            if m:
                instructions = int(m.group(1))
    return {"instructions": instructions}


def _callgrind(command):
    with tempfile.NamedTemporaryFile(prefix="callgrind_") as cgfile:
        cg_command = [
            "valgrind",
            "--quiet",
            "--tool=callgrind",
            f"--callgrind-out-file={cgfile.name}",
        ] + command
        _run(cg_command)
        return _extract_callgrind_results(cgfile.name)


def _time(command):
    begin = time.perf_counter()
    _run(command)
    end = time.perf_counter()
    return {"run_time_secs": end - begin}


def _main():
    modes = {"callgrind": _callgrind, "run": _run, "time": _time}

    parser = argparse.ArgumentParser()
    parser.add_argument("command", metavar="COMMAND", nargs="*")
    parser.add_argument("--verbose", "-v", action="store_true")
    for name, func in modes.items():
        parser.add_argument(
            f"--{name}", action="append_const", dest="run_funcs", const=func, default=[]
        )
    args = parser.parse_args()

    if args.verbose:
        logging.root.setLevel(logging.DEBUG)

    if args.command == []:
        parser.print_help()
        sys.exit(0)

    if args.run_funcs == []:
        args.run_funcs = [_time]

    results = {}
    for func in args.run_funcs:
        result = func(args.command)
        results.update(result)
    json.dump(results, sys.stdout, indent=2, sort_keys=True)
    sys.stdout.write("\n")


if __name__ == "__main__":
    _main()
