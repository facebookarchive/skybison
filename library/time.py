#!/usr/bin/env python3
"""This module provides various functions to manipulate time values.

There are two standard representations of time.  One is the number
of seconds since the Epoch, in UTC (a.k.a. GMT).  It may be an integer
or a floating point number (to represent fractions of seconds).
The Epoch is system-defined; on Unix, it is generally January 1st, 1970.
The actual value can be retrieved by calling gmtime(0).

The other representation is a tuple of 9 integers giving local time.
The tuple items are:
  year (including century, e.g. 1998)
  month (1-12)
  day (1-31)
  hours (0-23)
  minutes (0-59)
  seconds (0-59)
  weekday (0-6, Monday is 0)
  Julian day (day in the year, 1-366)
  DST (Daylight Savings Time) flag (-1, 0 or 1)
If the DST flag is 0, the time is given in the regular time zone;
if it is 1, the time is given in the DST time zone;
if it is -1, mktime() should guess based on the date and time."""


# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_Unbound = _Unbound  # noqa: F821
_patch = _patch  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


def asctime(tuple=_Unbound):
    _unimplemented()


def clock():
    _unimplemented()


def clock_getres(clk_id):
    _unimplemented()


def clock_gettime(clk_id):
    _unimplemented()


def clock_settime(clk_id, time):
    _unimplemented()


def ctime(seconds):
    _unimplemented()


def get_clock_info(name):
    _unimplemented()


def gmtime(seconds=_Unbound):
    _unimplemented()


def localtime(seconds=_Unbound):
    _unimplemented()


def mktime(tuple):
    _unimplemented()


def monotonic():
    _unimplemented()


def perf_counter():
    _unimplemented()


def process_time():
    _unimplemented()


def sleep(seconds):
    _unimplemented()


def strftime(format, tuple=_Unbound):
    _unimplemented()


def strptime(string, format):
    _unimplemented()


class struct_time:
    def __init__(self, *args, **kwargs):
        _unimplemented()


@_patch
def time():
    pass


def tzset():
    _unimplemented()
