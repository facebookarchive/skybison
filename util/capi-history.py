#!/usr/bin/env python3


import argparse
import datetime as dt
import os
import subprocess
import sys
from collections import namedtuple
from pathlib import Path


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


# Run the given command, returning its stdout. Exit if the command fails.
def cmd_output(*args):
    if os.getenv("TRACE_COMMANDS"):
        print(">", *args, file=sys.stderr)
    proc = subprocess.run(
        args,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        encoding=sys.stdout.encoding,
    )
    if proc.returncode != 0:
        sys.exit(
            f"Command\n  {args}\nexited with status {proc.returncode}:\n\n"
            f"{proc.stderr}"
        )
    return proc.stdout.strip()


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
    print(f"{rev.date.strftime(DATE_FORMAT)},{parts[0]},{rev.type},{rev.hash[:10]}")


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
        help="The output of --func-list from a previous run of capi.py with "
        "full access to all external C code.",
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

    capi_args = ("--read-func-list", func_list)

    revs = {}
    git_dir = None
    if args.pyro_git:
        git_dir = args.pyro_git
        for rev in get_git_revs(git_dir, args.start_date):
            revs[rev.date] = rev

    fbs_dir = None
    if args.fbsource:
        fbs_dir = args.fbsource
        for rev in get_hg_revs(fbs_dir, args.start_date):
            revs[rev.date] = rev

    print("date,completed,vcs_type,rev")
    for date in sorted(revs.keys()):
        rev = revs[date]
        if rev.type == "hg":
            os.chdir(fbs_dir)
            cmd_output("hg", "update", rev.hash)
            print_result(
                rev, cmd_output(capi_script, "--pyro", "fbcode/pyro", *capi_args)
            )
        else:
            os.chdir(git_dir)
            cmd_output("git", "checkout", rev.hash)
            print_result(rev, cmd_output(capi_script, "--pyro", ".", *capi_args))


if __name__ == "__main__":
    sys.exit(main())
