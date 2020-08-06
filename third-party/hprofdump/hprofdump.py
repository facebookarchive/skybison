#!/usr/bin/env python3
# Copyright (C) 2018 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
# usage: python hprofdump.py FILE
#   Dumps a binary heap dump file to text, to facilitate debugging of heap
#   dumps and heap dump viewers.
import struct
import sys

from time import strftime, gmtime


def readu1(hprof):
    return struct.unpack("!B", hprof.read(1))[0]


def readu2(hprof):
    return struct.unpack("!H", hprof.read(2))[0]


def readu4(hprof):
    return struct.unpack("!I", hprof.read(4))[0]


def readu8(hprof):
    return struct.unpack("!Q", hprof.read(8))[0]


def readN(n, hprof):
    if n == 1:
        return readu1(hprof)
    if n == 2:
        return readu2(hprof)
    if n == 4:
        return readu4(hprof)
    if n == 8:
        return readu8(hprof)
    raise Exception("Unsupported size of readN: %d" % n)


TY_OBJECT = 2
TY_BOOLEAN = 4
TY_CHAR = 5
TY_FLOAT = 6
TY_DOUBLE = 7
TY_BYTE = 8
TY_SHORT = 9
TY_INT = 10
TY_LONG = 11


def showty(ty):
    if ty == TY_OBJECT:
        return "Object"
    if ty == TY_BOOLEAN:
        return "boolean"
    if ty == TY_CHAR:
        return "char"
    if ty == TY_FLOAT:
        return "float"
    if ty == TY_DOUBLE:
        return "double"
    if ty == TY_BYTE:
        return "byte"
    if ty == TY_SHORT:
        return "short"
    if ty == TY_INT:
        return "int"
    if ty == TY_LONG:
        return "long"
    raise Exception("Unsupported type %d" % ty)


strs = {}


def showstr(id):
    if id in strs:
        return strs[id]
    return "STR[@%x]" % id


loaded = {}


def showloaded(serial):
    if serial in loaded:
        return showstr(loaded[serial])
    return "SERIAL[@%x]" % serial


classobjs = {}


def showclassobj(id):
    if id in classobjs:
        return "%s @%x" % (showstr(classobjs[id]), id)
    return "@%x" % id


idsize = None


def readID(hprof):
    return readN(idsize, hprof)


def valsize(ty):
    if ty == TY_OBJECT:
        return idsize
    if ty == TY_BOOLEAN:
        return 1
    if ty == TY_CHAR:
        return 2
    if ty == TY_FLOAT:
        return 4
    if ty == TY_DOUBLE:
        return 8
    if ty == TY_BYTE:
        return 1
    if ty == TY_SHORT:
        return 2
    if ty == TY_INT:
        return 4
    if ty == TY_LONG:
        return 8
    raise Exception("Unsupported type %d" % ty)


def readval(ty, hprof):
    return readN(valsize(ty), hprof)


def main(filename):
    with open(filename, "rb") as hprof:
        # [u1]* An initial NULL terminate series of bytes representing the format name
        # and version.
        version = b""
        c = hprof.read(1)
        while c != b"\0":
            version += c
            c = hprof.read(1)
        print("Version: %s" % version)
        # [u4] size of identifiers.
        global idsize
        idsize = readu4(hprof)
        print("ID Size: %d bytes" % idsize)

        # [u4] high word of number of ms since 0:00 GMT, 1/1/70
        # [u4] low word of number of ms since 0:00 GMT, 1/1/70
        timestamp = (readu4(hprof) << 32) | readu4(hprof)
        s, ms = divmod(timestamp, 1000)
        print("Date: %s.%03d" % (strftime("%Y-%m-%d %H:%M:%S", gmtime(s)), ms))
        while hprof.read(1):
            hprof.seek(-1, 1)
            pos = hprof.tell()
            tag = readu1(hprof)
            time = readu4(hprof)
            length = readu4(hprof)
            if tag == 0x01:
                id = readID(hprof)
                string = hprof.read(length - idsize)
                print("%d: STRING %x %s" % (pos, id, repr(string)))
                strs[id] = string
            elif tag == 0x02:
                serial = readu4(hprof)
                classobj = readID(hprof)
                stack = readu4(hprof)
                classname = readID(hprof)
                loaded[serial] = classname
                classobjs[classobj] = classname
                print(
                    "LOAD CLASS #%d %s @%x stack=@%x"
                    % (serial, showstr(classname), classobj, stack)
                )
            elif tag == 0x04:
                id = readID(hprof)
                method = readID(hprof)
                sig = readID(hprof)
                file = readID(hprof)
                serial = readu4(hprof)
                line = readu4(hprof)
                print(
                    "STACK FRAME %d '%s' '%s' '%s' line=%d classserial=%d"
                    % (id, showstr(method), showstr(sig), showstr(file), line, serial)
                )
            elif tag == 0x05:
                serial = readu4(hprof)
                print("STACK TRACE %d" % serial)
                thread = readu4(hprof)
                frames = readu4(hprof)
                hprof.read(idsize * frames)
            elif tag == 0x06:
                print("ALLOC SITES")
                flags = readu2(hprof)
                cutoff_ratio = readu4(hprof)
                live_bytes = readu4(hprof)
                live_insts = readu4(hprof)
                alloc_bytes = readu8(hprof)
                alloc_insts = readu8(hprof)
                numsites = readu4(hprof)
                while numsites > 0:
                    indicator = readu1(hprof)
                    class_serial = readu4(hprof)
                    stack = readu4(hprof)
                    live_bytes = readu4(hprof)
                    live_insts = readu4(hprof)
                    alloc_bytes = readu4(hprof)
                    alloc_insts = readu4(hprof)
                    numsites -= 1
            elif tag == 0x0A:
                thread = readu4(hprof)
                object = readID(hprof)
                stack = readu4(hprof)
                name = readID(hprof)
                group_name = readID(hprof)
                pgroup_name = readID(hprof)
                print("START THREAD serial=%d" % thread)
            elif tag == 0x0B:
                thread = readu4(hprof)
                print("END THREAD")
            elif tag == 0x0C or tag == 0x1C:
                if tag == 0x0C:
                    print("HEAP DUMP")
                else:
                    print("HEAP DUMP SEGMENT")
                while length > 0:
                    subtag = readu1(hprof)
                    length -= 1
                    if subtag == 0xFF:
                        print(" ROOT UNKNOWN")
                        objid = readID(hprof)
                        length -= idsize
                    elif subtag == 0x01:
                        print(" ROOT JNI GLOBAL")
                        objid = readID(hprof)
                        length -= idsize
                        ref = readID(hprof)
                        length -= idsize
                    elif subtag == 0x02:
                        print(" ROOT JNI LOCAL")
                        objid = readID(hprof)
                        length -= idsize
                        thread = readu4(hprof)
                        length -= 4
                        frame = readu4(hprof)
                        length -= 4
                    elif subtag == 0x03:
                        print(" ROOT JAVA FRAME")
                        objid = readID(hprof)
                        length -= idsize
                        serial = readu4(hprof)
                        length -= 4
                        frame = readu4(hprof)
                        length -= 4
                    elif subtag == 0x04:
                        objid = readID(hprof)
                        length -= idsize
                        serial = readu4(hprof)
                        length -= 4
                        print(" ROOT NATIVE STACK serial=%d" % serial)
                    elif subtag == 0x05:
                        print(" ROOT STICKY CLASS")
                        objid = readID(hprof)
                        length -= idsize
                    elif subtag == 0x06:
                        print(" ROOT THREAD BLOCK")
                        objid = readID(hprof)
                        length -= idsize
                        thread = readu4(hprof)
                        length -= 4
                    elif subtag == 0x07:
                        print(" ROOT MONITOR USED")
                        objid = readID(hprof)
                        length -= idsize
                    elif subtag == 0x08:
                        threadid = readID(hprof)
                        length -= idsize
                        serial = readu4(hprof)
                        length -= 4
                        stack = readu4(hprof)
                        length -= 4
                        print(
                            " ROOT THREAD OBJECT threadid=@%x serial=%d"
                            % (threadid, serial)
                        )
                    elif subtag == 0x20:
                        print(" CLASS DUMP")
                        print(
                            "  class class object ID: %s" % showclassobj(readID(hprof))
                        )
                        length -= idsize
                        print("  stack trace serial number: #%d" % readu4(hprof))
                        length -= 4
                        print("  super class object ID: @%x" % readID(hprof))
                        length -= idsize
                        print("  class loader object ID: @%x" % readID(hprof))
                        length -= idsize
                        print("  signers object ID: @%x" % readID(hprof))
                        length -= idsize
                        print("  protection domain object ID: @%x" % readID(hprof))
                        length -= idsize
                        print("  reserved: @%x" % readID(hprof))
                        length -= idsize
                        print("  reserved: @%x" % readID(hprof))
                        length -= idsize
                        print("  instance size (in bytes): %d" % readu4(hprof))
                        length -= 4
                        print("  constant pool:")
                        poolsize = readu2(hprof)
                        length -= 2
                        while poolsize > 0:
                            poolsize -= 1
                            idx = readu2(hprof)
                            length -= 2
                            ty = readu1(hprof)
                            length -= 1
                            val = readval(ty, hprof)
                            length -= valsize(ty)
                            print("   %d %s 0x%x" % (idx, showty(ty), val))
                        numstatic = readu2(hprof)
                        length -= 2
                        print("  static fields:")
                        while numstatic > 0:
                            numstatic -= 1
                            nameid = readID(hprof)
                            length -= idsize
                            ty = readu1(hprof)
                            length -= 1
                            val = readval(ty, hprof)
                            length -= valsize(ty)
                            print("   %s %s 0x%x" % (showstr(nameid), showty(ty), val))
                        numinst = readu2(hprof)
                        length -= 2
                        print("  instance fields:")
                        while numinst > 0:
                            numinst -= 1
                            nameid = readID(hprof)
                            length -= idsize
                            ty = readu1(hprof)
                            length -= 1
                            print("   %s %s" % (showstr(nameid), showty(ty)))
                    elif subtag == 0x21:
                        print(" INSTANCE DUMP:")
                        print("  object ID: @%x" % readID(hprof))
                        length -= idsize
                        stack = readu4(hprof)
                        length -= 4
                        print("  stack: %s" % stack)
                        print("  class object ID: %s" % showclassobj(readID(hprof)))
                        length -= idsize
                        datalen = readu4(hprof)
                        length -= 4
                        print("  %d bytes of instance data" % datalen)
                        data = hprof.read(datalen)
                        length -= datalen
                    elif subtag == 0x22:
                        print(" OBJECT ARRAY DUMP:")
                        print("  array object ID: @%x" % readID(hprof))
                        length -= idsize
                        stack = readu4(hprof)
                        length -= 4
                        print("  stack: %s" % stack)
                        count = readu4(hprof)
                        length -= 4
                        print(
                            "  array class object ID: %s" % showclassobj(readID(hprof))
                        )
                        length -= idsize
                        hprof.read(idsize * count)
                        length -= idsize * count
                    elif subtag == 0x23:
                        print(" PRIMITIVE ARRAY DUMP:")
                        print("  array object ID: @%x" % readID(hprof))
                        length -= idsize
                        stack = readu4(hprof)
                        length -= 4
                        count = readu4(hprof)
                        length -= 4
                        ty = readu1(hprof)
                        length -= 1
                        hprof.read(valsize(ty) * count)
                        length -= valsize(ty) * count
                    elif subtag == 0x89:
                        print(" HPROF_ROOT_INTERNED_STRING")
                        objid = readID(hprof)
                        length -= idsize
                    elif subtag == 0x8B:
                        objid = readID(hprof)
                        length -= idsize
                        print(
                            " HPROF ROOT DEBUGGER @%x (at offset %d)"
                            % (objid, hprof.tell() - (idsize + 1))
                        )
                    elif subtag == 0x8D:
                        objid = readID(hprof)
                        length -= idsize
                        print(" HPROF ROOT VM INTERNAL @%x" % objid)
                    elif subtag == 0xFE:
                        hty = readu4(hprof)
                        length -= 4
                        hnameid = readID(hprof)
                        length -= idsize
                        print(" HPROF_HEAP_DUMP_INFO %s" % showstr(hnameid))
                    else:
                        raise Exception("TODO: subtag %x" % subtag)
            elif tag == 0x0E:
                flags = readu4(hprof)
                depth = readu2(hprof)
                print("CONTROL SETTINGS %x %d" % (flags, depth))
            elif tag == 0x2C:
                print("HEAP DUMP END")
            else:
                raise Exception("TODO: TAG %x" % tag)


if __name__ == "__main__":
    main(sys.argv[1])
