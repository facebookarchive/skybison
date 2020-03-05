#!/usr/bin/env python3
"""Generate Unicode name, property, and type databases.

This script converts Unicode 3.2 and 11.0 database files into a Pyro-specific format.
Run the script using Python version >= 3.7, from this directory, without args.

The algorithm was taken from Fredrik Lundh's script for CPython.
"""

import os
import sys
import zipfile
from dataclasses import dataclass


SCRIPT = os.path.relpath(__file__)
PYRO_DIR = os.path.normpath(os.path.join(SCRIPT, "..", ".."))
RUNTIME_DIR = os.path.join(PYRO_DIR, "runtime")
HEADER = "unicode-db.h"
HEADER_PATH = os.path.join(RUNTIME_DIR, HEADER)
DB_PATH = os.path.join(RUNTIME_DIR, "unicode-db.cpp")
LINE_WIDTH = 80

NUM_CODE_POINTS = sys.maxunicode + 1
CODE_POINTS = range(NUM_CODE_POINTS)

UNIDATA_VERSION = "11.0.0"

# UCD files
CASE_FOLDING = "CaseFolding.txt"
COMPOSITION_EXCLUSIONS = "CompositionExclusions.txt"
DERIVED_CORE_PROPERTIES = "DerivedCoreProperties.txt"
DERIVEDNORMALIZATION_PROPS = "DerivedNormalizationProps.txt"
EASTASIAN_WIDTH = "EastAsianWidth.txt"
LINE_BREAK = "LineBreak.txt"
NAME_ALIASES = "NameAliases.txt"
NAMED_SEQUENCES = "NamedSequences.txt"
SPECIAL_CASING = "SpecialCasing.txt"
UNICODE_DATA = "UnicodeData.txt"
UNIHAN = "Unihan.zip"

# Private Use Areas -- in planes 1, 15, 16
PUA_1 = range(0xE000, 0xF900)
PUA_15 = range(0xF0000, 0xFFFFE)
PUA_16 = range(0x100000, 0x10FFFE)

# we use this ranges of PUA_15 to store name aliases and named sequences
NAME_ALIASES_START = 0xF0000
NAMED_SEQUENCES_START = 0xF0200

old_versions = ["3.2.0"]

CATEGORY_NAMES = [
    "Cn",
    "Lu",
    "Ll",
    "Lt",
    "Mn",
    "Mc",
    "Me",
    "Nd",
    "Nl",
    "No",
    "Zs",
    "Zl",
    "Zp",
    "Cc",
    "Cf",
    "Cs",
    "Co",
    "Cn",
    "Lm",
    "Lo",
    "Pc",
    "Pd",
    "Ps",
    "Pe",
    "Pi",
    "Pf",
    "Po",
    "Sm",
    "Sc",
    "Sk",
    "So",
]

BIDIRECTIONAL_NAMES = [
    "",
    "L",
    "LRE",
    "LRO",
    "R",
    "AL",
    "RLE",
    "RLO",
    "PDF",
    "EN",
    "ES",
    "ET",
    "AN",
    "CS",
    "NSM",
    "BN",
    "B",
    "S",
    "WS",
    "ON",
    "LRI",
    "RLI",
    "FSI",
    "PDI",
]

EASTASIANWIDTH_NAMES = ["F", "H", "W", "Na", "A", "N"]

MANDATORY_LINE_BREAKS = ["BK", "CR", "LF", "NL"]

ALPHA_MASK = 0x01
DECIMAL_MASK = 0x02
DIGIT_MASK = 0x04
LOWER_MASK = 0x08
LINEBREAK_MASK = 0x10
SPACE_MASK = 0x20
TITLE_MASK = 0x40
UPPER_MASK = 0x80
XID_START_MASK = 0x100
XID_CONTINUE_MASK = 0x200
PRINTABLE_MASK = 0x400
NUMERIC_MASK = 0x800
CASE_IGNORABLE_MASK = 0x1000
CASED_MASK = 0x2000
EXTENDED_CASE_MASK = 0x4000


cjk_ranges = [
    ("3400", "4DB5"),
    ("4E00", "9FEF"),
    ("20000", "2A6D6"),
    ("2A700", "2B734"),
    ("2B740", "2B81D"),
    ("2B820", "2CEA1"),
    ("2CEB0", "2EBE0"),
]


def open_data(filename, version):
    ucd_dir = os.path.join(PYRO_DIR, "third-party", f"ucd-{version}")
    if not os.path.exists(ucd_dir):
        os.mkdir(ucd_dir)

    path = os.path.join(ucd_dir, filename)
    if not os.path.exists(path):
        if version == "3.2.0":
            # irregular url structure
            root, path = os.path.splitext(filename)
            url = f"http://www.unicode.org/Public/3.2-Update/{root}-{version}{path}"
        else:
            url = f"http://www.unicode.org/Public/{version}/ucd/{filename}"
        raise OSError(
            f"""Cannot open {path}: file does not exist.
Find it online at {url}"""
        )

    if path.endswith(".txt"):
        return open(path, encoding="utf-8")
    else:
        # Unihan.zip
        return open(path, "rb")


########################################################################################
# the following support code is taken from the unidb utilities
# Copyright (c) 1999-2000 by Secret Labs AB

# load a unicode-data file from disk


class UnicodeData:
    """Loads a Unicode data file, checking disk first and then internet.

    Taken from the unidb utilities.
    Copyright (c) 1999-2000 by Secret Labs AB
    """

    # Record structure:
    # [ID, name, category, combining, bidi, decomp,  (6)
    #  decimal, digit, numeric, bidi-mirrored, Unicode-1-name, (11)
    #  ISO-comment, uppercase, lowercase, titlecase, ea-width, (16)
    #  derived-props] (17)

    def __init__(self, version, check_cjk=True):  # noqa: C901
        self.version = version

        self.aliases = []
        self.case_folding = {}
        self.changed = []
        self.cjk_ranges = []
        self.exclusions = set()
        self.named_sequences = []
        self.special_casing = {}
        self.table = [None] * NUM_CODE_POINTS

        with open_data(UNICODE_DATA, version) as file:
            while True:
                s = file.readline()
                if not s:
                    break
                s = s.strip().split(";")
                char = int(s[0], 16)
                self.table[char] = s

        # expand first-last ranges
        field = None
        for char in CODE_POINTS:
            s = self.table[char]
            if s is not None:
                if s[1][-6:] == "First>":
                    s[1] = ""
                    field = s
                elif s[1][-5:] == "Last>":
                    if check_cjk and s[1].startswith("<CJK Ideograph"):
                        self.cjk_ranges.append((field[0], s[0]))
                    s[1] = ""
                    field = None
            elif field:
                f2 = field[:]
                f2[0] = f"{char:X}"
                self.table[char] = f2

        # check for name aliases and named sequences, see #12753
        # aliases and named sequences are not in 3.2.0
        if version != "3.2.0":
            self.aliases = []
            # store aliases in the Private Use Area 15, in range U+F0000..U+F00FF,
            # in order to take advantage of the compression and lookup
            # algorithms used for the other characters
            pua_index = NAME_ALIASES_START
            with open_data(NAME_ALIASES, version) as file:
                for s in file:
                    s = s.strip()
                    if not s or s.startswith("#"):
                        continue
                    char, name, abbrev = s.split(";")
                    char = int(char, 16)
                    self.aliases.append((name, char))
                    # also store the name in the PUA 1
                    self.table[pua_index][1] = name
                    pua_index += 1
            assert pua_index - NAME_ALIASES_START == len(self.aliases)

            self.named_sequences = []
            # store named sequences in the PUA 1, in range U+F0100..,
            # in order to take advantage of the compression and lookup
            # algorithms used for the other characters.

            assert pua_index < NAMED_SEQUENCES_START
            pua_index = NAMED_SEQUENCES_START
            with open_data(NAMED_SEQUENCES, version) as file:
                for s in file:
                    s = s.strip()
                    if not s or s.startswith("#"):
                        continue
                    name, chars = s.split(";")
                    chars = tuple(int(char, 16) for char in chars.split())
                    # check that the structure defined in makeunicodename is OK
                    assert 2 <= len(chars) <= 4, "change the Py_UCS2 array size"
                    assert all(c <= 0xFFFF for c in chars), (
                        "use Py_UCS4 in "
                        "the NamedSequence struct and in unicodedata_lookup"
                    )
                    self.named_sequences.append((name, chars))
                    # also store these in the PUA 1
                    self.table[pua_index][1] = name
                    pua_index += 1
            assert pua_index - NAMED_SEQUENCES_START == len(self.named_sequences)

        self.exclusions = set()
        with open_data(COMPOSITION_EXCLUSIONS, version) as file:
            for s in file:
                s = s.strip()
                if s and not s.startswith("#"):
                    char = int(s.split()[0], 16)
                    self.exclusions.add(char)

        widths = [None] * NUM_CODE_POINTS
        with open_data(EASTASIAN_WIDTH, version) as file:
            for s in file:
                s = s.strip()
                if not s:
                    continue
                if s[0] == "#":
                    continue
                s = s.split()[0].split(";")
                if ".." in s[0]:
                    first, last = [int(c, 16) for c in s[0].split("..")]
                    chars = list(range(first, last + 1))
                else:
                    chars = [int(s[0], 16)]
                for char in chars:
                    widths[char] = s[1]

        for char in CODE_POINTS:
            if self.table[char] is not None:
                self.table[char].append(widths[char])
                self.table[char].append(set())

        with open_data(DERIVED_CORE_PROPERTIES, version) as file:
            for s in file:
                s = s.split("#", 1)[0].strip()
                if not s:
                    continue

                r, p = s.split(";")
                r = r.strip()
                p = p.strip()
                if ".." in r:
                    first, last = [int(c, 16) for c in r.split("..")]
                    chars = list(range(first, last + 1))
                else:
                    chars = [int(r, 16)]
                for char in chars:
                    if self.table[char]:
                        # Some properties (e.g. Default_Ignorable_Code_Point)
                        # apply to unassigned code points; ignore them
                        self.table[char][-1].add(p)

        with open_data(LINE_BREAK, version) as file:
            for s in file:
                s = s.partition("#")[0]
                s = [i.strip() for i in s.split(";")]
                if len(s) < 2 or s[1] not in MANDATORY_LINE_BREAKS:
                    continue
                if ".." not in s[0]:
                    first = last = int(s[0], 16)
                else:
                    first, last = [int(c, 16) for c in s[0].split("..")]
                for char in range(first, last + 1):
                    self.table[char][-1].add("Line_Break")

        # We only want the quickcheck properties
        # Format: NF?_QC; Y(es)/N(o)/M(aybe)
        # Yes is the default, hence only N and M occur
        # In 3.2.0, the format was different (NF?_NO)
        # The parsing will incorrectly determine these as
        # "yes", however, unicodedata.c will not perform quickchecks
        # for older versions, and no delta records will be created.
        quickchecks = [0] * NUM_CODE_POINTS
        qc_order = "NFD_QC NFKD_QC NFC_QC NFKC_QC".split()
        with open_data(DERIVEDNORMALIZATION_PROPS, version) as file:
            for s in file:
                if "#" in s:
                    s = s[: s.index("#")]
                s = [i.strip() for i in s.split(";")]
                if len(s) < 2 or s[1] not in qc_order:
                    continue
                quickcheck = "MN".index(s[2]) + 1  # Maybe or No
                quickcheck_shift = qc_order.index(s[1]) * 2
                quickcheck <<= quickcheck_shift
                if ".." not in s[0]:
                    first = last = int(s[0], 16)
                else:
                    first, last = [int(c, 16) for c in s[0].split("..")]
                for char in range(first, last + 1):
                    assert not (quickchecks[char] >> quickcheck_shift) & 3
                    quickchecks[char] |= quickcheck

        with open_data(UNIHAN, version) as file:
            zip = zipfile.ZipFile(file)
            if version == "3.2.0":
                data = zip.open("Unihan-3.2.0.txt").read()
            else:
                data = zip.open("Unihan_NumericValues.txt").read()
        for line in data.decode("utf-8").splitlines():
            if not line.startswith("U+"):
                continue
            code, tag, value = line.split(None, 3)[:3]
            if tag not in ("kAccountingNumeric", "kPrimaryNumeric", "kOtherNumeric"):
                continue
            value = value.strip().replace(",", "")
            i = int(code[2:], 16)
            # Patch the numeric field
            if self.table[i] is not None:
                self.table[i][8] = value
        sc = self.special_casing = {}
        with open_data(SPECIAL_CASING, version) as file:
            for s in file:
                s = s[:-1].split("#", 1)[0]
                if not s:
                    continue
                data = s.split("; ")
                if data[4]:
                    # We ignore all conditionals (since they depend on
                    # languages) except for one, which is hardcoded. See
                    # handle_capital_sigma in unicodeobject.c.
                    continue
                c = int(data[0], 16)
                lower = [int(char, 16) for char in data[1].split()]
                title = [int(char, 16) for char in data[2].split()]
                upper = [int(char, 16) for char in data[3].split()]
                sc[c] = (lower, title, upper)
        cf = self.case_folding = {}
        if version != "3.2.0":
            with open_data(CASE_FOLDING, version) as file:
                for s in file:
                    s = s[:-1].split("#", 1)[0]
                    if not s:
                        continue
                    data = s.split("; ")
                    if data[1] in "CF":
                        c = int(data[0], 16)
                        cf[c] = [int(char, 16) for char in data[2].split()]

        for char in CODE_POINTS:
            if self.table[char] is not None:
                self.table[char].append(quickchecks[char])

    def merge(self, old):  # noqa: C901
        # Changes to exclusion file not implemented yet
        if old.exclusions != self.exclusions:
            raise NotImplementedError("exclusions differ")

        # In these change records, 0xFF means "no change"
        bidir_changes = [0xFF] * NUM_CODE_POINTS
        category_changes = [0xFF] * NUM_CODE_POINTS
        decimal_changes = [0xFF] * NUM_CODE_POINTS
        mirrored_changes = [0xFF] * NUM_CODE_POINTS
        east_asian_width_changes = [0xFF] * NUM_CODE_POINTS
        # In numeric data, 0 means "no change",
        # -1 means "did not have a numeric value
        numeric_changes = [0] * NUM_CODE_POINTS
        # normalization_changes is a list of key-value pairs
        normalization_changes = []
        for char in range(NUM_CODE_POINTS):
            if self.table[char] is None:
                # Characters unassigned in the new version ought to
                # be unassigned in the old one
                assert old.table[char] is None
                continue
            # check characters unassigned in the old version
            if old.table[char] is None:
                # category 0 is "unassigned"
                category_changes[char] = 0
                continue
            # check characters that differ
            if old.table[char] != self.table[char]:
                for k in range(len(old.table[char])):
                    if old.table[char][k] != self.table[char][k]:
                        value = old.table[char][k]
                        if k == 1 and char in PUA_15:
                            # the name is not set in the old.table, but in the
                            # self.table we are using it for aliases and named seq
                            assert value == ""
                        elif k == 2:
                            category_changes[char] = CATEGORY_NAMES.index(value)
                        elif k == 4:
                            bidir_changes[char] = BIDIRECTIONAL_NAMES.index(value)
                        elif k == 5:
                            # We assume all normalization changes are in 1:1 mappings
                            assert " " not in value
                            normalization_changes.append((char, value))
                        elif k == 6:
                            # only support changes where the old value is a single digit
                            assert value in "0123456789"
                            decimal_changes[char] = int(value)
                        elif k == 8:
                            # Since 0 encodes "no change", the old value is better not 0
                            if not value:
                                numeric_changes[char] = -1
                            else:
                                numeric_changes[char] = float(value)
                                assert numeric_changes[char] not in (0, -1)
                        elif k == 9:
                            if value == "Y":
                                mirrored_changes[char] = "1"
                            else:
                                mirrored_changes[char] = "0"
                        elif k == 11:
                            # change to ISO comment, ignore
                            pass
                        elif k == 12:
                            # change to simple uppercase mapping; ignore
                            pass
                        elif k == 13:
                            # change to simple lowercase mapping; ignore
                            pass
                        elif k == 14:
                            # change to simple titlecase mapping; ignore
                            pass
                        elif k == 15:
                            # change to east asian width
                            east_asian_width_changes[char] = EASTASIANWIDTH_NAMES.index(
                                value
                            )
                        elif k == 16:
                            # derived property changes; not yet
                            pass
                        elif k == 17:
                            # normalization quickchecks are not performed
                            # for older versions
                            pass
                        else:

                            class Difference(Exception):
                                pass

                            raise Difference(
                                hex(char), k, old.table[char], self.table[char]
                            )
        self.changed.append(
            (
                old.version,
                list(
                    zip(
                        bidir_changes,
                        category_changes,
                        decimal_changes,
                        mirrored_changes,
                        east_asian_width_changes,
                        numeric_changes,
                    )
                ),
                normalization_changes,
            )
        )


########################################################################################
# Helper functions


def getsize(data):
    """Return the smallest possible integer size for the given array.
    """
    maxdata = max(data)
    if maxdata < (1 << 8):
        return 1
    if maxdata < (1 << 16):
        return 2
    return 4


def splitbins(t, trace=False):  # noqa: C901
    """t, trace=False -> (t1, t2, shift).  Split a table to save space.

    t is a sequence of ints.  This function can be useful to save space if
    many of the ints are the same.  t1 and t2 are lists of ints, and shift
    is an int, chosen to minimize the combined size of t1 and t2 (in C
    code), and where for each i in range(len(t)),
        t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    where mask is a bitmask isolating the last "shift" bits.

    If optional arg trace is non-zero (default zero), progress info
    is printed to sys.stderr.  The higher the value, the more info
    you'll get.
    """

    if trace:

        def dump(t1, t2, shift, bytes):
            sys.stderr.write(
                f"{len(t1)}+{len(t2)} bins at shift {shift}; {bytes} bytes\n"
            )

        sys.stderr.write(f"Size of original table: {len(t) * getsize(t)} bytes\n")

    n = len(t) - 1  # last valid index
    maxshift = 0  # the most we can shift n and still have something left
    if n > 0:
        while n >> 1:
            n >>= 1
            maxshift += 1
    del n
    bytes = sys.maxsize  # smallest total size so far
    t = tuple(t)  # so slices can be dict keys
    for shift in range(maxshift + 1):
        t1 = []
        t2 = []
        size = 2 ** shift
        bincache = {}
        for i in range(0, len(t), size):
            bin = t[i : i + size]
            index = bincache.get(bin)
            if index is None:
                index = len(t2)
                bincache[bin] = index
                t2.extend(bin)
            t1.append(index >> shift)
        # determine memory size
        b = len(t1) * getsize(t1) + len(t2) * getsize(t2)
        if trace:
            dump(t1, t2, shift, b)
        if b < bytes:
            best = t1, t2, shift
            bytes = b
    t1, t2, shift = best
    if trace:
        sys.stderr.write(f"Best: ")
        dump(t1, t2, shift, bytes)
    if __debug__:
        # exhaustively verify that the decomposition is correct
        mask = ~((~0) << shift)  # i.e., low-bit mask of shift bits
        for i in range(len(t)):
            assert t[i] == t2[(t1[i >> shift] << shift) + (i & mask)]
    return best


########################################################################################
# Dataclasses for database tables


@dataclass(frozen=True)
class TypeRecord:
    upper: int
    lower: int
    title: int
    decimal: int
    digit: int
    flags: int

    def __str__(self):
        return f"{{{self.upper}, {self.lower}, {self.title}, \
{self.decimal}, {self.digit}, {self.flags}}}"


class Array:
    """Store an array for writing to a generated C/C++ file.
    """

    def __init__(self, ctype, name, data, comment=None):
        self.type = ctype
        self.name = name
        self.data = data
        self.comment = comment

    def dump(self, file, trace=False):
        if self.comment is not None:
            file.write(f"\n// {self.comment}")
        file.write(f"\nstatic const {self.type} {self.name}[] = {{")

        if self.data:
            file.write("\n")
            for item in self.data:
                file.write(f"    {item},\n")

        file.write("};\n")


class UIntArray:
    """Unsigned integer arrays, printed as hex.
    """

    def __init__(self, name, data):
        self.name = name
        self.data = data
        self.size = getsize(data)

    def dump(self, file, trace=False):
        if trace:
            sys.stderr.write(f"{self.name}: {self.size * len(self.data)} bytes\n")

        file.write(f"\nstatic const uint{8 * self.size}_t {self.name}[] = {{")
        columns = (LINE_WIDTH - 3) // (2 * self.size + 4)
        for i in range(0, len(self.data), columns):
            line = "\n   "
            for item in self.data[i : i + columns]:
                if self.size == 1:
                    line = f"{line} {item:#04x},"
                elif self.size == 2:
                    line = f"{line} {item:#06x},"
                else:
                    line = f"{line} {item:#010x},"
            file.write(line)
        file.write("\n};\n")


########################################################################################
# Unicode database writers


def write_header(header):
    """Writes type declarations to the database header file.
    """

    header.write(
        f"""\
// Generated by {os.path.basename(SCRIPT)}
#pragma once

#include <cstdint>

namespace py {{

enum : int32_t {{
  kAlphaMask = {ALPHA_MASK:#x},
  kDecimalMask = {DECIMAL_MASK:#x},
  kDigitMask = {DIGIT_MASK:#x},
  kLowerMask = {LOWER_MASK:#x},
  kLinebreakMask = {LINEBREAK_MASK:#x},
  kSpaceMask = {SPACE_MASK:#x},
  kTitleMask = {TITLE_MASK:#x},
  kUpperMask = {UPPER_MASK:#x},
  kXidStartMask = {XID_START_MASK:#x},
  kXidContinueMask = {XID_CONTINUE_MASK:#x},
  kPrintableMask = {PRINTABLE_MASK:#x},
  kNumericMask = {NUMERIC_MASK:#x},
  kCaseIgnorableMask = {CASE_IGNORABLE_MASK:#x},
  kCasedMask = {CASED_MASK:#x},
  kExtendedCaseMask = {EXTENDED_CASE_MASK:#x},
}};

struct UnicodeTypeRecord {{
  // Deltas to the character or offsets in kExtendedCase
  const int32_t upper;
  const int32_t lower;
  const int32_t title;
  // Note: if more flag space is needed, decimal and digit could be unified
  const int8_t decimal;
  const int8_t digit;
  const int16_t flags;
}};

const UnicodeTypeRecord* typeRecord(int32_t code_point);

}}  // namespace py
"""
    )


def write_db_prelude(db):
    """Writes required include directives to the database file.
    """

    db.write(
        f"""\
// Generated by generate_unicode_database.py
#include "{HEADER}"

#include <cstdint>

#include "globals.h"

namespace py {{
"""
    )


def write_type_data(unicode, db, trace):  # noqa: C901
    """Writes Unicode character type tables to the database file.
    """

    # extract unicode types
    dummy = TypeRecord(0, 0, 0, 0, 0, 0)
    table = [dummy]
    cache = {0: dummy}
    index = [0] * NUM_CODE_POINTS
    numeric = {}
    spaces = []
    linebreaks = []
    extra_casing = []

    for char in CODE_POINTS:
        record = unicode.table[char]
        if record:
            # extract database properties
            category = record[2]
            bidirectional = record[4]
            properties = record[16]
            flags = 0
            # TODO(T55176519): delta = True
            if category in ("Lm", "Lt", "Lu", "Ll", "Lo"):
                flags |= ALPHA_MASK
            if "Lowercase" in properties:
                flags |= LOWER_MASK
            if "Line_Break" in properties or bidirectional == "B":
                flags |= LINEBREAK_MASK
                linebreaks.append(char)
            if category == "Zs" or bidirectional in ("WS", "B", "S"):
                flags |= SPACE_MASK
                spaces.append(char)
            if category == "Lt":
                flags |= TITLE_MASK
            if "Uppercase" in properties:
                flags |= UPPER_MASK
            if char == ord(" ") or category[0] not in ("C", "Z"):
                flags |= PRINTABLE_MASK
            if "XID_Start" in properties:
                flags |= XID_START_MASK
            if "XID_Continue" in properties:
                flags |= XID_CONTINUE_MASK
            if "Cased" in properties:
                flags |= CASED_MASK
            if "Case_Ignorable" in properties:
                flags |= CASE_IGNORABLE_MASK
            sc = unicode.special_casing.get(char)
            cf = unicode.case_folding.get(char, [char])
            if record[12]:
                upper = int(record[12], 16)
            else:
                upper = char
            if record[13]:
                lower = int(record[13], 16)
            else:
                lower = char
            if record[14]:
                title = int(record[14], 16)
            else:
                title = upper
            if sc is None and cf != [lower]:
                sc = ([lower], [title], [upper])
            if sc is None:
                if upper == lower == title:
                    upper = lower = title = 0
                else:
                    upper = upper - char
                    lower = lower - char
                    title = title - char
                    assert (
                        abs(upper) <= 2147483647
                        and abs(lower) <= 2147483647
                        and abs(title) <= 2147483647
                    )
            else:
                # This happens either when some character maps to more than one
                # character in uppercase, lowercase, or titlecase or the
                # casefolded version of the character is different from the
                # lowercase. The extra characters are stored in a different
                # array.
                flags |= EXTENDED_CASE_MASK
                lower = len(extra_casing) | (len(sc[0]) << 24)
                extra_casing.extend(sc[0])
                if cf != sc[0]:
                    lower |= len(cf) << 20
                    extra_casing.extend(cf)
                upper = len(extra_casing) | (len(sc[2]) << 24)
                extra_casing.extend(sc[2])
                # Title is probably equal to upper.
                if sc[1] == sc[2]:
                    title = upper
                else:
                    title = len(extra_casing) | (len(sc[1]) << 24)
                    extra_casing.extend(sc[1])
            # decimal digit, integer digit
            decimal = 0
            if record[6]:
                flags |= DECIMAL_MASK
                decimal = int(record[6])
            digit = 0
            if record[7]:
                flags |= DIGIT_MASK
                digit = int(record[7])
            if record[8]:
                flags |= NUMERIC_MASK
                numeric.setdefault(record[8], []).append(char)
            item = TypeRecord(upper, lower, title, decimal, digit, flags)
            # add entry to index and item tables
            i = cache.get(item)
            if i is None:
                cache[item] = i = len(table)
                table.append(item)
            index[char] = i

    print(len(table), "unique character type entries")
    print(sum(map(len, numeric.values())), "numeric code points")
    print(len(spaces), "whitespace code points")
    print(len(linebreaks), "linebreak code points")
    print(len(extra_casing), "extended case array")

    Array(
        "UnicodeTypeRecord",
        "kTypeRecords",
        table,
        "a list of unique character type descriptors",
    ).dump(db, trace)

    # split decomposition index table
    index1, index2, shift = splitbins(index, trace)
    db.write(
        f"""
// type indices
static const int kTypeIndexShift = {shift};
static const int32_t kTypeIndexMask = (int32_t{{1}} << kTypeIndexShift) - 1;
"""
    )
    UIntArray("kTypeIndex1", index1).dump(db, trace)
    UIntArray("kTypeIndex2", index2).dump(db, trace)

    db.write(
        """
const UnicodeTypeRecord* typeRecord(int32_t code_point) {
  if (code_point > kMaxUnicode) {
    return kTypeRecords;
  }

  int index = kTypeIndex1[code_point >> kTypeIndexShift];
  index <<= kTypeIndexShift;
  index = kTypeIndex2[index + (code_point & kTypeIndexMask)];
  return kTypeRecords + index;
}
"""
    )


def write_db_coda(db):
    db.write(
        """
}  // namespace py
"""
    )


def write_db(trace=False):
    print(f"--- Reading {UNICODE_DATA} v{UNIDATA_VERSION} ...")
    unicode = UnicodeData(UNIDATA_VERSION)

    print(len(list(filter(None, unicode.table))), "characters")

    for version in old_versions:
        print(f"--- Reading {UNICODE_DATA} v{version} ...")
        old_unicode = UnicodeData(version, check_cjk=False)
        print(len(list(filter(None, old_unicode.table))), "characters")
        unicode.merge(old_unicode)

    with open(HEADER_PATH, "w") as header:
        write_header(header)

    with open(DB_PATH, "w") as db:
        write_db_prelude(db)
        write_type_data(unicode, db, trace)
        write_db_coda(db)


if __name__ == "__main__":
    write_db(True)
