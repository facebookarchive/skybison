#!/usr/bin/env python3

import argparse
import os
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
        raise NotImplementedError(
            "Checking if there are changes is not supported"
        )

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
        # For now assume it is Git, later do detection of .git or .hg directory.
        return Git()


class Git(VCS):
    def __init__(self) -> None:
        self.exe = get_exe("git")

    def root(self) -> Path:
        return Path(
            subprocess.check_output(
                [str(self.exe), "rev-parse", "--show-toplevel"]
            ).decode()
            # Trim the trailing new line.
            .strip()
        )

    def has_changes(self) -> bool:
        return subprocess.run(
            [str(self.exe), "diff", "--exit-code", "--quiet"]
        ).returncode

    def get_commit_title(self) -> str:
        return subprocess.check_output(
            [str(self.exe), "log", '--format="%h %s"', "HEAD~1..HEAD"]
        ).decode()

    def get_changed_files(self, from_commit: bool = False) -> List[Path]:
        return [
            Path(f.decode())
            for f in subprocess.check_output(
                [str(self.exe), "diff", "--name-only"]
                + (["HEAD~1..HEAD"] if from_commit else [])
                + ["--", "*.cpp", "*.h", "*.py"]
            ).splitlines()
        ]

    def authored_previous(self) -> bool:
        return subprocess.run(
            [
                str(self.exe),
                "log",
                "--exit-code",
                "HEAD~1..HEAD",
                "--author",
                os.environ["USER"],
            ],
            stdout=subprocess.DEVNULL,
        ).returncode


class Mercurial(VCS):
    pass


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
        return subprocess.check_output(
            [str(self.exe), "--quiet", str(file)]
        ).decode()


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
        reply = input("Format files in that commit? [y/N] ")
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
    )


def get_exe(exe: str) -> Path:
    return Path(shutil.which(exe))


if __name__ == "__main__":
    sys.exit(main())
