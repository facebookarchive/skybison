#!/usr/bin/env python3


import argparse
import datetime as dt
import os
import subprocess
import sys
import tempfile
from collections import namedtuple
from pathlib import Path

from capi_util import cmd_output


# The commit that introduced Pyro to fbsource.
PYRO_FBS_MERGE = "5d8ac8a762c2efe2d9e5dc622238ce6e7f41dfd8"

# The first commit in pyro-git with any C-API implementations.
PYRO_GIT_START = "b79bb8ce360e4f7a16f29c4a47859c9930ea0260"

# Format string to use when parsing and printing dates.
DATE_FORMAT = "%Y-%m-%d"


# A git or hg revision with its date.
Rev = namedtuple("Rev", ("date", "hash", "type"))


# Parse a date object using DATE_FORMAT.
def parse_date(date):
    return dt.datetime.strptime(date, DATE_FORMAT).date()


# Return a list of Rev objects for all interesting fbsource commits.
def get_hg_revs(path, start_date):
    rev_parts = [
        f"descendants({PYRO_FBS_MERGE})",
        "file('fbcode/pyro/ext/**')",
        "ancestors(master)",
    ]
    if start_date:
        rev_parts.append(f"date('>{start_date}')")

    os.chdir(path)
    output = cmd_output(
        "hg", "log", "-r", " & ".join(rev_parts), "-T", "{date|shortdate},{node}\n"
    )
    if output == "":
        return []

    result = []
    for line in output.split("\n"):
        date, rev = line.split(",")
        result.append(Rev(date=parse_date(date), hash=rev, type="hg"))
    return result


# Return a list of Rev objects for all interesting git commits.
def get_git_revs(path, start_date):
    date_args = []
    if start_date:
        date_args = ["--since", start_date]

    os.chdir(path)
    output = cmd_output(
        "git",
        "log",
        "--reverse",
        "--format=%cd,%H",
        f"--date=format:{DATE_FORMAT}",
        f"{PYRO_GIT_START}^..origin/master",
        *date_args,
        "--",
        "ext/",
    )
    if output == "":
        return []

    result = []
    for line in output.split("\n"):
        date, rev = line.split(",")
        result.append(Rev(date=parse_date(date), hash=rev, type="git"))
    return result


# Print one line of CSV output.
def print_result(rev, result):
    parts = result.split()
    print(
        f"{rev.date.strftime(DATE_FORMAT)},{parts[0]},{rev.type},{rev.hash[:10]}",
        flush=True,
    )


def parse_args():
    parser = argparse.ArgumentParser(
        description="Run capi.py on historical commits, printing csv data."
    )

    req = parser.add_argument_group("Required arguments")
    req.add_argument(
        "--fbsource",
        required=True,
        help="Path to fbsource checkout. This should either be a different "
        "checkout than the one containing this script, or you should copy "
        "this script and capi.py outside of the tree before running it, "
        "because various different revisions will be checked out.",
    )
    req.add_argument(
        "--func-list",
        required=True,
        help="A list of all required C-API functions (usually used_funcs.txt "
        "from a previous run of capi.py with --reports).",
    )

    parser.add_argument("--pyro-git", help="Path to Pyro git repository.")
    parser.add_argument(
        "--start-date", help="Ignore commits before this date (YYYY-MM-DD)"
    )

    return parser.parse_args()


def main():
    args = parse_args()

    func_list = os.path.realpath(args.func_list)
    if not Path(func_list).is_file():
        sys.exit(f"Function list {func_list} does not exist")

    capi_script = os.path.join(os.path.dirname(os.path.realpath(__file__)), "capi.py")
    if not Path(capi_script).is_file():
        sys.exit(f"Couldn't find capi.py at {capi_script}")

    revs = {}
    git_dir = None
    if args.pyro_git:
        git_dir = os.path.realpath(args.pyro_git)
        for rev in get_git_revs(git_dir, args.start_date):
            revs[rev.date] = rev

    fbs_dir = None
    if args.fbsource:
        fbs_dir = os.path.realpath(args.fbsource)
        for rev in get_hg_revs(fbs_dir, args.start_date):
            revs[rev.date] = rev

    print("date,completed,vcs_type,rev")
    for date in sorted(revs.keys()):
        rev = revs[date]
        if rev.type == "hg":
            os.chdir(fbs_dir)
            cmd_output("hg", "update", rev.hash)
            pyro_path = "fbcode/pyro"
        else:
            os.chdir(git_dir)
            cmd_output("git", "checkout", rev.hash)
            pyro_path = "."

        pyro_path = os.path.realpath(pyro_path)

        with tempfile.TemporaryDirectory() as build_dir:
            os.chdir(build_dir)
            try:
                cmd_output(
                    "cmake",
                    "-DCMAKE_BUILD_TYPE=Debug",
                    "-DCMAKE_C_COMPILER=clang.par",
                    "-DCMAKE_CXX_COMPILER=clang++.par",
                    pyro_path,
                )
                cmd_output("make", "-j", str(os.cpu_count()), "python")
            except subprocess.CalledProcessError:
                # Just skip this rev if the build fails.
                print(
                    f"Skipping rev {rev.hash} due to failed build",
                    file=sys.stderr,
                    flush=True,
                )
                continue

            os.chdir(pyro_path)
            print_result(
                rev,
                cmd_output(
                    capi_script,
                    "--pyro",
                    ".",
                    "--read-func-list",
                    func_list,
                    "--pyro-build",
                    build_dir,
                ),
            )


if __name__ == "__main__":
    sys.exit(main())
