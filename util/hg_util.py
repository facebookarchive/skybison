#!/usr/bin/env python3
import logging
import os
import subprocess
import sys


def run(cmd, cwd, stdout, **kwargs):
    logging.info(f"$ cd {cwd}; {' '.join(cmd)}")
    return subprocess.run(cmd, cwd=cwd, encoding="UTF-8", stdout=stdout, **kwargs)


def repo_root():
    dirname = os.path.dirname(__file__)
    completed_process = run(["hg", "root"], cwd=dirname, stdout=subprocess.PIPE)
    if completed_process.returncode == 0:
        return completed_process.stdout.strip()

    print("Unknown source control", file=sys.stderr)
    sys.exit(1)


def current_rev():
    completed_process = run(["hg", "id", "-i"], cwd=repo_root(), stdout=subprocess.PIPE)
    if completed_process.returncode != 0:
        print("Hg couldn't get the current hash")
        sys.exit(1)
    return completed_process.stdout.strip()
