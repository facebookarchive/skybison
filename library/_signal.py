#!/usr/bin/env python3

from _builtins import _builtin, _unimplemented


def alarm(seconds):
    _unimplemented()


def default_int_handler(*args, **kwargs):
    _builtin()


def getitimer(which):
    _unimplemented()


def getsignal(signalnum):
    _builtin()


def pause():
    _unimplemented()


def pthread_kill(thread_id, signalnum):
    _unimplemented()


def set_wakeup_fd(fd):
    _unimplemented()


def setitimer(which, seconds, interval=0.0):
    _unimplemented()


def siginterrupt(signalnum, flag):
    _unimplemented()


def signal(signalnum, handler):
    _builtin()


def sigpending():
    _unimplemented()


def sigwait(sigset):
    _unimplemented()
