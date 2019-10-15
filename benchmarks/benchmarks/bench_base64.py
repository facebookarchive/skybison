#!/usr/bin/env python3

# ***** BEGIN LICENSE BLOCK *****
# Version: MPL 1.1/GPL 2.0/LGPL 2.1
#
# The contents of this file are subject to the Mozilla Public License Version
# 1.1 (the "License"); you may not use this file except in compliance with
# the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS" basis,
# WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
# for the specific language governing rights and limitations under the
# License.
#
# The Original Code is Mozilla XML-RPC Client component.
#
# The Initial Developer of the Original Code is
# Digital Creations 2, Inc.
# Portions created by the Initial Developer are Copyright (C) 2000
# the Initial Developer. All Rights Reserved.
#
# Contributor(s):
#   Martijn Pieters <mj@digicool.com> (original author)
#   Samuel Sieb <samuel@sieb.net>
#   Facebook, Inc. and its affiliates (port to Python from JavaScript)
#
# Alternatively, the contents of this file may be used under the terms of
# either the GNU General Public License Version 2 or later (the "GPL"), or
# the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
# in which case the provisions of the GPL or the LGPL are applicable instead
# of those above. If you wish to allow use of your version of this file only
# under the terms of either the GPL or the LGPL, and not to allow others to
# use your version of this file under the terms of the MPL, indicate your
# decision by deleting the provisions above and replace them with the notice
# and other provisions required by the GPL or the LGPL. If you do not delete
# the provisions above, a recipient may use your version of this file under
# the terms of any one of the MPL, the GPL or the LGPL.
#
# ***** END LICENSE BLOCK *****

import random
import sys


# Convert data (an array of integers) to a Base64 string.
TO_BASE64_TABLE = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"

BASE64_PAD = ord("=")

# Convert Base64 data to a string
TO_BINARY_TABLE = (
    b"\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    b"\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff"
    b"\xff\xff\xff\xff\xff\xff\xff\xff\xff\xff>\xff\xff\xff?456789:;<=\xff\xff\xff"
    b"\x00\xff\xff\xff\x00\x01\x02\x03\x04\x05\x06\x07\x08\t\n\x0b\x0c\r\x0e\x0f"
    b"\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\xff\xff\xff\xff\xff\xff\x1a\x1b\x1c"
    b"\x1d\x1e\x1f !\"#$%&'()*+,-./0123\xff\xff\xff\xff\xff"
)

DEFAULT_ITERATIONS = 20


def to_base64(data):
    # TODO(bsimmers): When we have bytearray.append, use a bytearray here.
    result = []
    length = len(data)
    # Convert every three bytes to 4 ascii characters.
    for i in range(0, length - 2, 3):
        result.append(TO_BASE64_TABLE[data[i] >> 2])
        result.append(TO_BASE64_TABLE[((data[i] & 0x03) << 4) + (data[i + 1] >> 4)])
        result.append(TO_BASE64_TABLE[((data[i + 1] & 0x0F) << 2) + (data[i + 2] >> 6)])
        result.append(TO_BASE64_TABLE[data[i + 2] & 0x3F])

    # Convert the remaining 1 or 2 bytes, pad out to 4 characters.
    if length % 3:
        i = length - (length % 3)
        result.append(TO_BASE64_TABLE[data[i] >> 2])
        if (length % 3) == 2:
            result.append(TO_BASE64_TABLE[((data[i] & 0x03) << 4) + (data[i + 1] >> 4)])
            result.append(TO_BASE64_TABLE[(data[i + 1] & 0x0F) << 2])
            result.append(BASE64_PAD)
        else:
            result.append(TO_BASE64_TABLE[(data[i] & 0x03) << 4])
            result.append(BASE64_PAD)
            result.append(BASE64_PAD)

    return bytes(result)


def base64_to_bytes(data):
    # TODO(bsimmers): When we have bytearray.append, use a bytearray here.
    result = []
    leftbits = 0  # number of bits decoded, but yet to be appended
    leftdata = 0  # bits decoded, but yet to be appended

    # Convert one by one.
    for byte in data:
        c = TO_BINARY_TABLE[byte & 0x7F]
        padding = byte == BASE64_PAD
        # Skip illegal characters and whitespace
        if c == 255:
            continue

        # Collect data into leftdata, update bitcount
        leftdata = (leftdata << 6) | c
        leftbits += 6

        # If we have 8 or more bits, append 8 bits to the result
        if leftbits >= 8:
            leftbits -= 8
            # Append if not padding.
            if not padding:
                result.append((leftdata >> leftbits) & 0xFF)
            leftdata &= (1 << leftbits) - 1

    # If there are any bits left, the base64 string was corrupted
    if leftbits:
        raise RuntimeError("Corrupted base64 string")

    return bytes(result)


def run_one():
    # TODO(bsimmers): When we have bytearray.append, use a bytearray here.
    data = []
    for _ in range(8192):
        data.append(random.randint(97, 122))
    data = bytes(data)

    for _ in range(2):
        base64 = to_base64(data)
        encoded = base64_to_bytes(base64)
        if encoded != data:
            raise RuntimeError(
                f"ERROR: bad result: expected\n{data}\n\n  "
                f"but got\n\n{encoded}\n\n  with base64\n\n{base64}"
            )

        # Double the data
        data += data


def bench_base64(num_iterations):
    random.seed(0xDEADBEEF)
    for _ in range(num_iterations):
        run_one()


def run():
    bench_base64(DEFAULT_ITERATIONS)


if __name__ == "__main__":
    num_iterations = DEFAULT_ITERATIONS
    if len(sys.argv) > 1:
        num_iterations = int(sys.argv[1])
    bench_base64(num_iterations)
