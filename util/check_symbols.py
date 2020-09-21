#!/usr/bin/env python3.7
"""
This script checks pyro/runtime/symbols.h for unused symbols.
"""
import enum
import glob
import json
import os
import re
import sys
import typing


class LintSeverity(str, enum.Enum):
    ERROR = "error"
    WARNING = "warning"
    ADVICE = "advice"
    DISABLED = "disabled"


class LintMessage(typing.NamedTuple):
    path: str
    code: str
    name: str
    severity: LintSeverity = LintSeverity.WARNING
    line: typing.Optional[int] = None
    char: typing.Optional[int] = None
    original: typing.Optional[str] = None
    replacement: typing.Optional[str] = None
    description: typing.Optional[str] = None
    bypassChangedLineFiltering: typing.Optional[bool] = None

    def __json__(self):
        return json.dumps(self._asdict())


symbols_file = "fbcode/pyro/runtime/symbols.h"
cleaned_file = "/tmp/symbols.h.cleaned"
if not os.path.exists(os.path.relpath(symbols_file)):
    sys.stderr.write(f"{symbols_file} not found\n")
    sys.stderr.write("You should run this script in the PyRo directory\n")
    sys.exit(1)

used = set()
use_re = re.compile(r"\bID\((?P<sym>[^\)]+)\)")
for path in glob.iglob("fbcode/pyro/**", recursive=True):
    if path.endswith(".h") or path.endswith(".cpp") or path.endswith(".cc"):
        with open(path) as fp:
            for line in fp:
                for m in use_re.finditer(line):
                    used.add(m.group("sym"))
sym_re = re.compile(r"\s*V\((?P<sym>[^\)]+)\)")

with open(symbols_file) as fp:
    for linenumber, line in enumerate(fp):
        m = sym_re.match(line)
        if not m:
            continue
        sym = m.group("sym")
        if sym not in used:
            msg = LintMessage(
                path=symbols_file,
                code="PYROSYMBOLCHECK",
                name="unused-symbol",
                line=linenumber
                + 1,  # if just linenumber is used linter will delete line before unused symbol
                description=f"unused symbol {sym}",
                char=1,
                original=line,
                severity=LintSeverity.WARNING,
                replacement="",
                bypassChangedLineFiltering=True,
            )
            print(msg.__json__())
