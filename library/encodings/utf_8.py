#!/usr/bin/env python3
# WARNING: This is a temporary copy of code from the cpython library to
# facilitate bringup. Please file a task for anything you change!
# flake8: noqa
# fmt: off
""" Python 'utf-8' Codec


Written by Marc-Andre Lemburg (mal@lemburg.com).

(c) Copyright CNRI, All Rights Reserved. NO WARRANTY.

"""
import codecs


### Codec APIs

encode = codecs.utf_8_encode

def decode(input, errors='strict'):
    return codecs.utf_8_decode(input, errors, True)

class IncrementalEncoder(codecs.IncrementalEncoder):
    def encode(self, input, final=False):
        return codecs.utf_8_encode(input, self.errors)[0]

class IncrementalDecoder(codecs.BufferedIncrementalDecoder):
    # TODO(T54587721): Revert change once we can bind builtins as static methods
    # _buffer_decode = codecs.utf_8_decode
    _buffer_decode = staticmethod(codecs.utf_8_decode)

class StreamWriter(codecs.StreamWriter):
    # TODO(T54587721): Revert change once we can bind builtins as static methods
    # encode = codecs.utf_8_encode
    encode = staticmethod(codecs.utf_8_encode)

class StreamReader(codecs.StreamReader):
    # TODO(T54587721): Revert change once we can bind builtins as static methods
    # decode = codecs.utf_8_decode
    decode = staticmethod(codecs.utf_8_decode)

### encodings module API

def getregentry():
    return codecs.CodecInfo(
        name='utf-8',
        encode=encode,
        decode=decode,
        incrementalencoder=IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=StreamWriter,
    )
