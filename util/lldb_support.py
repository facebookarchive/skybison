#!/usr/bin/env python2

# To enable, put something like this in ~/.lldbinit:
#
# command script import $PYRO_ROOT/util/lldb_support.py
# type summary add -F lldb_support.format_raw_object -p -x "python::Raw.*"

import lldb


def as_enum(target, name, value):
    """Given a name like LayoutId or ObjectFormat and an int, return the string
    name of the value.
    """
    type = target.FindFirstType("python::" + name)
    data = lldb.SBData.CreateDataFromInt(value, size=type.size)
    enum = target.CreateValueFromData(name, data, type)
    return str(enum).split(" = ")[1]


def format_heap_obj(val, raw):
    """Format a HeapObject's address and information from its header."""
    address = raw - 1
    header_type = val.target.FindFirstType("python::RawHeader")
    header = val.CreateValueFromAddress("header", address - 8, header_type)
    header_raw = header.GetChildMemberWithName("raw_").GetValueAsUnsigned()
    return "HeapObject @ 0x%x %s" % (address, format_header(val, header_raw))


def get_bits(raw, n):
    """Extract n bottom bits from raw, returning the value and raw with those
    bits shifted out.
    """
    return (raw & ((1 << n) - 1), raw >> n)


def format_header(val, raw):
    """Format all relevant information from a HeapObject's header."""
    tag, raw = get_bits(raw, 3)
    format, raw = get_bits(raw, 3)
    layout_id, raw = get_bits(raw, 20)
    hash, raw = get_bits(raw, 30)
    count, raw = get_bits(raw, 8)
    return "Header<%s, %s, hash=%d, count=%d>" % (
        as_enum(val.target, "ObjectFormat", format),
        as_enum(val.target, "LayoutId", layout_id),
        hash,
        count,
    )


def format_error_or_seq(raw):
    """Format an error or small sequence."""
    tag = (raw >> 3) & 0x3
    if tag == 2:
        return "Error"
    if tag == 3:
        return "<invalid>"
    raw >>= 5
    length = raw & 0x7
    raw >>= 3
    result = ""
    for _i in range(length):
        result += chr(raw & 0xFF)
        raw >>= 8
    return "%s('%s')" % ("SmallStr" if tag == 1 else "SmallBytes", result)


def format_imm(raw):
    """Format one of the singleton immediate values."""
    tag = (raw >> 3) & 0x3
    if tag == 0:
        return "True" if raw & 0x20 else "False"
    return ("NotImplemented", "Unbound", "None")[tag - 1]


def format_raw_object(raw_object, internal_dict):
    """Format a RawObject."""
    raw = raw_object.GetChildMemberWithName("raw_").GetValueAsUnsigned()
    # SmallInt
    if raw & 0x1 == 0:
        return str(raw_object.GetChildMemberWithName("raw_").GetValueAsSigned() >> 1)

    low_tag = (raw >> 1) & 0x3
    if low_tag == 0:
        return format_heap_obj(raw_object, raw)
    if low_tag == 1:
        return format_header(raw_object, raw)
    if low_tag == 2:
        return format_error_or_seq(raw)
    return format_imm(raw)
