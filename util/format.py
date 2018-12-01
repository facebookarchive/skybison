#!/usr/bin/env python3

import argparse
import os
import re
import shutil
import subprocess
import sys
from abc import ABC, abstractmethod
from pathlib import Path
from typing import List


class VCS(ABC):
    @abstractmethod
    def root(self) -> Path:
        """Return the root of the repo"""
        raise NotImplementedError("Root of the VCS not supported")

    @abstractmethod
    def has_changes(self) -> bool:
        """Return whether there are modified files with respect to the last commit"""
        raise NotImplementedError("Checking if there are changes is not supported")

    @abstractmethod
    def get_commit_title(self) -> str:
        """Return the title of the last commit"""
        raise NotImplementedError("Getting the commit title is not supported")

    @abstractmethod
    def get_changed_files(self, from_commit: bool = False) -> List[Path]:
        pass

    @abstractmethod
    def authored_previous(self) -> bool:
        pass

    @staticmethod
    def infer_vcs(dir: Path):
        """Return the VCS inferred from the given directory"""
        path = Path(os.getcwd())
        while True:
            if Path(path, ".git").exists():
                raise RuntimeError("git is not supported")
            if Path(path, ".hg").exists():
                return Mercurial()
            if str(path) == path.root:
                raise RuntimeError(f"Couldn't find vcs root from {os.getcwd()}")
            path = path.parent


class Mercurial(VCS):
    files_re = re.compile("(\.cpp|\.h|\.py)$")

    def __init__(self) -> None:
        self.exe = get_exe("hg")

    def root(self) -> Path:
        return Path(subprocess.check_output([str(self.exe), "root"]).decode().strip())

    def has_changes(self) -> bool:
        return (
            subprocess.check_output(
                # Ask for all files except untracked.
                [str(self.exe), "status", "-mard"]
            )
            .decode()
            .strip()
            != ""
        )

    def get_commit_title(self) -> str:
        return (
            subprocess.check_output([str(self.exe), "log", "-r.", "-T{desc|firstline}"])
            .decode()
            .strip()
        )

    def get_changed_files(self, from_commit: bool = False) -> List[Path]:
        files = (
            subprocess.check_output(
                [str(self.exe), "status", "--rev", ".^" if from_commit else ".", "-man"]
            )
            .decode()
            .strip()
            .split("\n")
        )
        return [Path(f) for f in files if (type(self).files_re.search(f) is not None)]

    def authored_previous(self) -> bool:
        author = (
            subprocess.check_output([str(self.exe), "log", "-r.", "-T{author}"])
            .decode()
            .strip()
        )
        return os.environ["USER"] in author


class Formatter(ABC):
    @abstractmethod
    def format(self, file: Path) -> str:
        raise NotImplementedError("Formatting not supported")

    @staticmethod
    def for_file(file: Path):
        if file.suffix == ".py":
            return BlackFormat()
        elif file.suffix in {".cpp", ".h"}:
            return ClangFormat()


class ClangFormat(Formatter):
    def __init__(self) -> None:
        self.exe = get_exe("clang-format")

    def format(self, file: Path) -> str:
        return subprocess.check_output(
            [str(self.exe), "-i", "-style=file", str(file)]
        ).decode()


class BlackFormat(Formatter):
    def __init__(self) -> None:
        self.exe = get_exe("black")

    def format(self, file: Path) -> str:
        return subprocess.check_output([str(self.exe), "--quiet", str(file)]).decode()


def main() -> int:
    args = parse_args()
    vcs = VCS.infer_vcs(Path("."))
    vcs_root = vcs.root()
    assert vcs_root.exists()
    files: List[Path] = []
    if args.all:
        print("Formatting all files...")
        files += list(vcs_root.glob("**/*.py"))
        files += list(vcs_root.glob("**/*.cpp"))
        files += list(vcs_root.glob("**/*.h"))
    elif vcs.has_changes():
        print("Formatting only modified files...")
        files = vcs.get_changed_files()
    elif vcs.authored_previous():
        print("There are no modified files, but you authored the last commit:")
        print(vcs.get_commit_title())
        reply = input("Format files in that commit? [Y/n] ")
        if not reply or reply.lower() == "y":
            files = vcs.get_changed_files(from_commit=True)
    else:
        print(
            "You have no modified files and you didn't recently commit on this "
            "branch."
        )
        print(f"To format all files: {sys.argv[0]} -a")
        return 1

    if not files:
        print("No files to format")
        return 0

    files = [f for f in sorted(files) if file_should_be_formatted(f)]
    print(
        "Formatting {num_files} {extensions} file{plural}".format(
            num_files=len(files),
            extensions=" and ".join(sorted({f.suffix for f in files})),
            plural="s" if len(files) != 1 else "",
        )
    )

    for f in files:
        Formatter.for_file(f).format(f)
        # Don't buffer stdout because otherwise the dots aren't written out
        # to show progress.
        sys.stdout.write(".")
        sys.stdout.flush()
    print()
    print("Done")
    return 0


def parse_args():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--all", "-a", action="store_true", help="Format all files in the repo"
    )
    return parser.parse_args()


def file_should_be_formatted(file: Path) -> bool:
    """Return true if the given file should be formatted"""
    file_str = str(file)
    return (
        "benchmarks" not in file_str
        and "third-party" not in file_str
        and "ext/config" not in file_str
        and "ext/Include" not in file_str
        and "library/importlib" not in file_str
    )


def get_exe(exe: str) -> Path:
    path = shutil.which(exe)
    if path is None:
        raise RuntimeError(f"Couldn't find {exe} in PATH")
    return Path(path)


if __name__ == "__main__":
    sys.exit(main())
