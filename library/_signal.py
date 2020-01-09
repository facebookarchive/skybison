#!/usr/bin/env python3

# These values are injected by our boot process. flake8 has no knowledge about
# their definitions and will complain without this circular helper here.
_unimplemented = _unimplemented  # noqa: F821


def alarm(seconds):
    _unimplemented()


def default_int_handler(*args, **kwargs):
    _unimplemented()


def getitimer(which):
    _unimplemented()


def getsignal(signalnum):
    _unimplemented()


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
    _unimplemented()


def sigpending():
    _unimplemented()


def sigwait(sigset):
    _unimplemented()
