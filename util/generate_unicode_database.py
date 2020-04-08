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
# CJK ranges


@dataclass(frozen=True)
class CJKRange:
    start: int
    end: int


CJK_RANGES = (
    CJKRange(0x3400, 0x4DB5),
    CJKRange(0x4E00, 0x9FEF),
    CJKRange(0x20000, 0x2A6D6),
    CJKRange(0x2A700, 0x2B734),
    CJKRange(0x2B740, 0x2B81D),
    CJKRange(0x2B820, 0x2CEA1),
    CJKRange(0x2CEB0, 0x2EBE0),
)


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
        cjk_ranges = []
        for char in CODE_POINTS:
            s = self.table[char]
            if s is not None:
                if s[1][-6:] == "First>":
                    s[1] = ""
                    field = s
                elif s[1][-5:] == "Last>":
                    if check_cjk and s[1].startswith("<CJK Ideograph"):
                        start = int(field[0], 16)
                        end = int(s[0], 16)
                        cjk_ranges.append(CJKRange(start, end))
                    s[1] = ""
                    field = None
            elif field:
                f2 = field[:]
                f2[0] = f"{char:X}"
                self.table[char] = f2

        if check_cjk and set(cjk_ranges) != set(CJK_RANGES):
            raise ValueError(f"CJK ranges deviate: found {cjk_ranges!r}")

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
        for char in CODE_POINTS:
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
# Helper functions & classes


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


@dataclass
class WordData:
    word: str
    index: int
    frequency: int


class Words:
    def __init__(self, names, trace=False):
        self.indices = {}
        self.frequencies = {}

        count = num_chars = num_words = 0
        for char in CODE_POINTS:
            name = names[char]
            if name is None:
                continue

            words = name.split()
            num_chars += len(name)
            num_words += len(words)
            for word in words:
                if word in self.indices:
                    self.frequencies[word] += 1
                else:
                    self.frequencies[word] = 1
                    self.indices[word] = count
                    count += 1

        if trace:
            print(f"{num_words} words in text; {num_chars} code points")

    def tolist(self):
        return sorted(
            (
                WordData(word, index, self.frequencies[word])
                for word, index in self.indices.items()
            ),
            key=lambda data: (-data.frequency, data.word),
        )


########################################################################################
# Classes for database tables


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


class CodePointArray:
    """Arrays of code points, printed as hex.
    """

    def __init__(self, name, data):
        self.name = name
        self.data = data

    def dump(self, file, trace=False):
        if trace:
            sys.stderr.write(f"{self.name}: {4 * len(self.data)} bytes\n")

        file.write(f"\nstatic const int32_t {self.name}[] = {{")
        columns = (LINE_WIDTH - 3) // 12
        for i in range(0, len(self.data), columns):
            line = "\n   "
            for item in self.data[i : i + columns]:
                line = f"{line} {item:#010x},"
            file.write(line)
        file.write("\n};\n")


class StructArray:
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
# Reimplementation of dictionaries using static structures and a custom string hash


# Number chosen to minimize the number of collisions. CPython uses 47.
MAGIC = 43


def myhash(s):
    h = 0
    for c in map(ord, s.upper()):
        h = (h * MAGIC) + c
        if h > 0x00FFFFFF:
            h = (h & 0x00FFFFFF) ^ ((h & 0xFF000000) >> 24)
    return h


SIZES = [
    (4, 3),
    (8, 3),
    (16, 3),
    (32, 5),
    (64, 3),
    (128, 3),
    (256, 29),
    (512, 17),
    (1024, 9),
    (2048, 5),
    (4096, 83),
    (8192, 27),
    (16384, 43),
    (32768, 3),
    (65536, 45),
    (131072, 9),
    (262144, 39),
    (524288, 39),
    (1048576, 9),
    (2097152, 5),
    (4194304, 3),
    (8388608, 33),
    (16777216, 27),
]


class Hash:
    def __init__(self, data):
        """Turn a (key, value) list into a static hash table structure.
        """

        # determine table size
        for size, poly in SIZES:
            if size > len(data):
                self.poly = poly + size
                self.size = size
                break
        else:
            raise AssertionError("ran out of polynomials")

        print(self.size, "slots in hash table")
        table = [None] * self.size
        mask = self.size - 1

        # initialize hash table
        collisions = 0
        for key, value in data:
            h = myhash(key)
            i = (~h) & mask
            if table[i] is None:
                table[i] = value
                continue
            incr = (h ^ (h >> 3)) & mask
            if not incr:
                incr = mask
            while True:
                collisions += 1
                i = (i + incr) & mask
                if table[i] is None:
                    table[i] = value
                    break
                incr = incr << 1
                if incr > mask:
                    incr = incr ^ self.poly
        print(collisions, "collisions")

        for i in range(len(table)):
            if table[i] is None:
                table[i] = 0

        self.data = CodePointArray("kCodeHash", table)

    def dump(self, file, trace):
        self.data.dump(file, trace)
        file.write(
            f"""
static const int32_t kCodeMagic = {MAGIC};
static const int32_t kCodeMask = {self.size - 1};
static const int32_t kCodePoly = {self.poly};
"""
        )


########################################################################################
# Unicode database writers


def write_header(unicode, header):
    """Writes type declarations to the database header file.
    """

    header.write("// @")
    header.write(
        f"""\
generated by {os.path.basename(SCRIPT)}
#pragma once

#include <cstdint>

#include "globals.h"
#include "unicode.h"

namespace py {{

static const int kMaxNameLength = 256;

static_assert(Unicode::kAliasStart == {NAME_ALIASES_START:#x},
              "Unicode aliases start at unexpected code point");
static_assert(Unicode::kAliasCount == {len(unicode.aliases)},
              "Unexpected number of Unicode aliases");
static_assert(Unicode::kNamedSequenceStart == {NAMED_SEQUENCES_START:#x},
              "Unicode named sequences start at unexpected code point");
static_assert(Unicode::kNamedSequenceCount == {len(unicode.named_sequences)},
              "Unexpected number of Unicode named sequences");

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

// Get a code point from its Unicode name.
// Returns the code point if the lookup succeeds, -1 if it fails.
int32_t codePointFromName(const byte* name, word size);
int32_t codePointFromNameOrNamedSequence(const byte* name, word size);

// Returns the case mapping for code points where offset is insufficient
int32_t extendedCaseMapping(int32_t index);

// Write the Unicode name for the given code point into the buffer.
// Returns true if the name was written successfully, false otherwise.
bool nameFromCodePoint(int32_t code_point, byte* buffer, word size);

const UnicodeTypeRecord* typeRecord(int32_t code_point);

}}  // namespace py
"""
    )


def write_db_prelude(db):
    """Writes required include directives to the database file.
    """

    db.write("// @")
    db.write(
        f"""\
generated by {os.path.basename(SCRIPT)}
#include "{HEADER}"

#include <cinttypes>
#include <cstdio>
#include <cstring>

#include "asserts.h"
#include "globals.h"
#include "unicode.h"

namespace py {{
"""
    )


def write_name_data(unicode, db, trace):  # noqa: C901
    # Collect names
    names = [None] * NUM_CODE_POINTS
    for char in CODE_POINTS:
        record = unicode.table[char]
        if record:
            name = record[1].strip()
            if name and name[0] != "<":
                names[char] = f"{name}\0"

    if trace:
        print(len([n for n in names if n is not None]), "distinct names")

    # Collect unique words from names. Note that we differ between
    # words ending a sentence, which include the trailing null byte,
    # and words inside a sentence, which do not.
    words = Words(names, trace)
    wordlist = words.tolist()

    # Figure out how many phrasebook escapes we need
    short = (65536 - len(wordlist)) // 256
    assert short > 0

    if trace:
        print(short, "short indexes in lexicon")
        n = sum(wordlist[i].frequency for i in range(short))
        print(n, "short indexes in phrasebook")

    # Pick the most commonly used words, and sort the rest by falling
    # length to maximize overlap.
    tail = wordlist[short:]
    tail.sort(key=lambda data: len(data.word), reverse=True)
    wordlist[short:] = tail

    # Build a lexicon string from words
    current_offset = 0
    lexicon_offset = [0]
    lexicon_string = ""
    word_offsets = {}
    for data in wordlist:
        # Encoding: bit 7 indicates last character in word
        # chr(128) indicates the last character in an entire string)
        last = ord(data.word[-1])
        encoded = f"{data.word[:-1]}{chr(last + 128)}"

        # reuse string tails, when possible
        offset = lexicon_string.find(encoded)
        if offset < 0:
            offset = current_offset
            lexicon_string = lexicon_string + encoded
            current_offset += len(encoded)

        word_offsets[data.word] = len(lexicon_offset)
        lexicon_offset.append(offset)

    lexicon = [ord(ch) for ch in lexicon_string]

    # generate phrasebook from names and lexicon
    phrasebook = [0]
    phrasebook_offset = [0] * NUM_CODE_POINTS
    for char in CODE_POINTS:
        name = names[char]
        if name is not None:
            words = name.split()
            phrasebook_offset[char] = len(phrasebook)
            for word in words:
                offset = word_offsets[word]
                if offset < short:
                    phrasebook.append(offset)
                else:
                    # store as two bytes
                    phrasebook.append((offset >> 8) + short)
                    phrasebook.append(offset & 255)

    assert getsize(phrasebook) == 1

    db.write(f"\n// lexicon")
    UIntArray("kLexicon", lexicon).dump(db, trace)
    UIntArray("kLexiconOffset", lexicon_offset).dump(db, trace)

    # split decomposition index table
    offset1, offset2, shift = splitbins(phrasebook_offset, trace)

    db.write(
        f"""
// code => name phrasebook
static const int kPhrasebookShift = {shift};
static const int kPhrasebookShort = {short};
static const int kPhrasebookMask = (1 << kPhrasebookShift) - 1;
"""
    )
    UIntArray("kPhrasebook", phrasebook).dump(db, trace)
    UIntArray("kPhrasebookOffset1", offset1).dump(db, trace)
    UIntArray("kPhrasebookOffset2", offset2).dump(db, trace)

    # Extract names for name hash table
    hash_data = []
    for char in CODE_POINTS:
        record = unicode.table[char]
        if record:
            name = record[1].strip()
            if name and name[0] != "<":
                hash_data.append((name, char))

    db.write("\n// name => code dictionary")
    Hash(hash_data).dump(db, trace)

    aliases = [codepoint for _, codepoint in unicode.aliases]
    UIntArray("kNameAliases", aliases).dump(db, trace)


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
    extended_cases = []

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
                lower = len(extended_cases) | (len(sc[0]) << 24)
                extended_cases.extend(sc[0])
                if cf != sc[0]:
                    lower |= len(cf) << 20
                    extended_cases.extend(cf)
                upper = len(extended_cases) | (len(sc[2]) << 24)
                extended_cases.extend(sc[2])
                # Title is probably equal to upper.
                if sc[1] == sc[2]:
                    title = upper
                else:
                    title = len(extended_cases) | (len(sc[1]) << 24)
                    extended_cases.extend(sc[1])
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
    print(len(extended_cases), "extended case array")

    StructArray(
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

    # extended case mappings
    CodePointArray("kExtendedCase", extended_cases).dump(db, trace)


def write_db_coda(db):
    db.write(
        """
struct HangulJamo {
  const int length;
  const byte name[3];
};

static const HangulJamo kHangulLeads[] = {
    {1, {'G'}},      {2, {'G', 'G'}}, {1, {'N'}},      {1, {'D'}},
    {2, {'D', 'D'}}, {1, {'R'}},      {1, {'M'}},      {1, {'B'}},
    {2, {'B', 'B'}}, {1, {'S'}},      {2, {'S', 'S'}}, {0},
    {1, {'J'}},      {2, {'J', 'J'}}, {1, {'C'}},      {1, {'K'}},
    {1, {'T'}},      {1, {'P'}},      {1, {'H'}},
};

static const HangulJamo kHangulVowels[] = {
    {1, {'A'}},           {2, {'A', 'E'}},      {2, {'Y', 'A'}},
    {3, {'Y', 'A', 'E'}}, {2, {'E', 'O'}},      {1, {'E'}},
    {3, {'Y', 'E', 'O'}}, {2, {'Y', 'E'}},      {1, {'O'}},
    {2, {'W', 'A'}},      {3, {'W', 'A', 'E'}}, {2, {'O', 'E'}},
    {2, {'Y', 'O'}},      {1, {'U'}},           {3, {'W', 'E', 'O'}},
    {2, {'W', 'E'}},      {2, {'W', 'I'}},      {2, {'Y', 'U'}},
    {2, {'E', 'U'}},      {2, {'Y', 'I'}},      {1, {'I'}},
};

static const HangulJamo kHangulTrails[] = {
    {0},
    {1, {'G'}},
    {2, {'G', 'G'}},
    {2, {'G', 'S'}},
    {1, {'N'}},
    {2, {'N', 'J'}},
    {2, {'N', 'H'}},
    {1, {'D'}},
    {1, {'L'}},
    {2, {'L', 'G'}},
    {2, {'L', 'M'}},
    {2, {'L', 'B'}},
    {2, {'L', 'S'}},
    {2, {'L', 'T'}},
    {2, {'L', 'P'}},
    {2, {'L', 'H'}},
    {1, {'M'}},
    {1, {'B'}},
    {2, {'B', 'S'}},
    {1, {'S'}},
    {2, {'S', 'S'}},
    {2, {'N', 'G'}},
    {1, {'J'}},
    {1, {'C'}},
    {1, {'K'}},
    {1, {'T'}},
    {1, {'P'}},
    {1, {'H'}},
};

static_assert(ARRAYSIZE(kHangulLeads) == Unicode::kHangulLeadCount,
              "mismatch in number of Hangul lead jamo");
static_assert(ARRAYSIZE(kHangulVowels) == Unicode::kHangulVowelCount,
              "mismatch in number of Hangul vowel jamo");
static_assert(ARRAYSIZE(kHangulTrails) == Unicode::kHangulTrailCount,
              "mismatch in number of Hangul trail jamo");

static int32_t findHangulJamo(const byte* str, word size,
                              const HangulJamo array[], word count) {
  word result = -1;
  int length = -1;
  for (word i = 0; i < count; i++) {
    HangulJamo jamo = array[i];
    if (length < jamo.length && jamo.length <= size &&
        (std::memcmp(str, jamo.name, jamo.length) == 0)) {
      length = jamo.length;
      result = i;
    }
  }
  return result;
}

static int32_t findHangulSyllable(const byte* name, word size) {
  word pos = 16;
  word lead = findHangulJamo(name + pos, size - pos, kHangulLeads,
                             Unicode::kHangulLeadCount);
  if (lead == -1) {
    return -1;
  }

  pos += kHangulLeads[lead].length;
  word vowel = findHangulJamo(name + pos, size - pos, kHangulVowels,
                              Unicode::kHangulVowelCount);
  if (vowel == -1) {
    return -1;
  }

  pos += kHangulVowels[vowel].length;
  word trail = findHangulJamo(name + pos, size - pos, kHangulTrails,
                              Unicode::kHangulTrailCount);
  if (trail == -1 || pos + kHangulTrails[trail].length != size) {
    return -1;
  }

  return Unicode::kHangulSyllableStart +
         (lead * Unicode::kHangulVowelCount + vowel) *
             Unicode::kHangulTrailCount +
         trail;
}

static bool isUnifiedIdeograph(int32_t code_point) {
  return """
    )

    for i, cjk in enumerate(CJK_RANGES):
        if i > 0:
            db.write(" ||\n         ")
        db.write(f"({cjk.start:#010x} <= code_point && code_point <= {cjk.end:#010x})")

    db.write(
        """;
}

static uint32_t myHash(const byte* name, word size) {
  uint32_t hash = 0;
  for (word i = 0; i < size; i++) {
    hash = (hash * kCodeMagic) + ASCII::toUpper(name[i]);
    if (hash > 0x00ffffff) {
      hash = (hash & 0x00ffffff) ^ ((hash & 0xff000000) >> 24);
    }
  }
  return hash;
}

static bool nameMatchesCodePoint(int32_t code_point, const byte* name,
                                 word size) {
  byte buffer[kMaxNameLength + 1];
  if (!nameFromCodePoint(code_point, buffer, kMaxNameLength)) {
    return false;
  }
  for (word i = 0; i < size; i++) {
    if (ASCII::toUpper(name[i]) != buffer[i]) {
      return false;
    }
  }
  return buffer[size] == '\\0';
}

static int32_t getCodePoint(const byte* name, word size,
                            int32_t (*check)(int32_t)) {
  if (size >= 16 && std::memcmp(name, "HANGUL SYLLABLE ", 16) == 0) {
    return findHangulSyllable(name, size);
  }

  if (size >= 22 && std::memcmp(name, "CJK UNIFIED IDEOGRAPH-", 22) == 0) {
    // Four or five uppercase hexdigits must follow
    name += 22;
    size -= 22;
    if (size != 4 && size != 5) {
      return -1;
    }

    int32_t result = 0;
    while (size--) {
      result *= 16;
      if ('0' <= *name && *name <= '9') {
        result += *name - '0';
      } else if ('A' <= *name && *name <= 'F') {
        result += *name - 'A' + 10;
      } else {
        return -1;
      }
      name++;
    }
    if (!isUnifiedIdeograph(result)) {
      return -1;
    }
    return result;
  }

  uint32_t hash = myHash(name, size);
  uint32_t step = (hash ^ (hash >> 3)) & kCodeMask;
  if (step == 0) {
    step = kCodeMask;
  }

  for (uint32_t idx = (~hash) & kCodeMask;; idx = (idx + step) & kCodeMask) {
    int32_t result = kCodeHash[idx];
    if (result == 0) {
      return -1;
    }
    if (nameMatchesCodePoint(result, name, size)) {
      return check(result);
    }
    step = step << 1;
    if (step > kCodeMask) {
      step = step ^ kCodePoly;
    }
  }
}

static int32_t checkAliasAndNamedSequence(int32_t code_point) {
  if (Unicode::isNamedSequence(code_point)) {
    return -1;
  }
  if (Unicode::isAlias(code_point)) {
    return kNameAliases[code_point - Unicode::kAliasStart];
  }
  return code_point;
}

int32_t codePointFromName(const byte* name, word size) {
  return getCodePoint(name, size, checkAliasAndNamedSequence);
}

static int32_t checkAlias(int32_t code_point) {
  if (Unicode::isAlias(code_point)) {
    return kNameAliases[code_point - Unicode::kAliasStart];
  }
  return code_point;
}

int32_t codePointFromNameOrNamedSequence(const byte* name, word size) {
  return getCodePoint(name, size, checkAlias);
}

int32_t extendedCaseMapping(int32_t index) {
  return kExtendedCase[index];
}

bool nameFromCodePoint(int32_t code_point, byte* buffer, word size) {
  DCHECK_BOUND(code_point, kMaxUnicode);

  if (Unicode::isHangulSyllable(code_point)) {
    if (size < 27) {
      return false;  // worst case: HANGUL SYLLABLE <10 chars>
    }

    int32_t offset = code_point - Unicode::kHangulSyllableStart;
    HangulJamo lead = kHangulLeads[offset / Unicode::kHangulCodaCount];
    HangulJamo vowel = kHangulVowels[(offset % Unicode::kHangulCodaCount) /
                                     Unicode::kHangulTrailCount];
    HangulJamo trail = kHangulTrails[offset % Unicode::kHangulTrailCount];

    std::memcpy(buffer, "HANGUL SYLLABLE ", 16);
    buffer += 16;
    std::memcpy(buffer, lead.name, lead.length);
    buffer += lead.length;
    std::memcpy(buffer, vowel.name, vowel.length);
    buffer += vowel.length;
    std::memcpy(buffer, trail.name, trail.length);
    buffer[trail.length] = '\\0';
    return true;
  }

  if (isUnifiedIdeograph(code_point)) {
    if (size < 28) {
      return false;  // worst case: CJK UNIFIED IDEOGRAPH-*****
    }
    std::snprintf(reinterpret_cast<char*>(buffer), size,
                  "CJK UNIFIED IDEOGRAPH-%" PRIX32, code_point);
    return true;
  }

  uint32_t offset = kPhrasebookOffset1[code_point >> kPhrasebookShift];
  offset = kPhrasebookOffset2[(offset << kPhrasebookShift) +
                              (code_point & kPhrasebookMask)];

  if (offset == 0) {
    return false;
  }

  for (word i = 0;;) {
    if (i > 0) {
      if (i >= size) {
        return false;  // buffer overflow
      }
      buffer[i++] = ' ';
    }

    // get word index
    word word_idx = kPhrasebook[offset] - kPhrasebookShort;
    if (word_idx >= 0) {
      word_idx = (word_idx << 8) + kPhrasebook[offset + 1];
      offset += 2;
    } else {
      word_idx = kPhrasebook[offset++];
    }

    // copy word string from lexicon
    const byte* word_ptr = kLexicon + kLexiconOffset[word_idx];
    while (*word_ptr < 128) {
      if (i >= size) {
        return false;  // buffer overflow
      }
      buffer[i++] = *word_ptr++;
    }

    if (i >= size) {
      return false;  // buffer overflow
    }
    buffer[i++] = *word_ptr & 0x7f;
    if (*word_ptr == 0x80) {
      return true;
    }
  }
}

const UnicodeTypeRecord* typeRecord(int32_t code_point) {
  if (code_point > kMaxUnicode) {
    return kTypeRecords;
  }

  int index = kTypeIndex1[code_point >> kTypeIndexShift];
  index <<= kTypeIndexShift;
  index = kTypeIndex2[index + (code_point & kTypeIndexMask)];
  return kTypeRecords + index;
}

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
        write_header(unicode, header)

    with open(DB_PATH, "w") as db:
        write_db_prelude(db)
        write_name_data(unicode, db, trace)
        write_type_data(unicode, db, trace)
        write_db_coda(db)


if __name__ == "__main__":
    write_db(True)
