#!/usr/bin/env python3
"""The io module provides the Python interfaces to stream handling. The
builtin open function is defined in this module.

At the top of the I/O hierarchy is the abstract base class IOBase. It
defines the basic interface to a stream. Note, however, that there is no
separation between reading and writing to streams; implementations are
allowed to raise an IOError if they do not support a given operation.

Extending IOBase is RawIOBase which deals simply with the reading and
writing of raw bytes to a stream. FileIO subclasses RawIOBase to provide
an interface to OS files.

BufferedIOBase deals with buffering on a raw byte stream (RawIOBase). Its
subclasses, BufferedWriter, BufferedReader, and BufferedRWPair buffer
streams that are readable, writable, and both respectively.
BufferedRandom provides a buffered interface to random access
streams. BytesIO is a simple stream of in-memory bytes.

Another IOBase subclass, TextIOBase, deals with the encoding and decoding
of streams into text. TextIOWrapper, which extends it, is a buffered text
interface to a buffered raw stream (`BufferedIOBase`). Finally, StringIO
is an in-memory stream for text.

Argument names are not part of the specification, and only the arguments
of open() are intended to be used as keyword arguments."""

# This is the patch decorator, injected by our boot process. flake8 has no
# knowledge about its definition and will complain without this gross circular
# helper here.
_Unbound = _Unbound  # noqa: F821
_address = _address  # noqa: F821
_bytearray_len = _bytearray_len  # noqa: F821
_bytes_check = _bytes_check  # noqa: F821
_byteslike_guard = _byteslike_guard  # noqa: F821
_float_check = _float_check  # noqa: F821
_index = _index  # noqa: F821
_int_check = _int_check  # noqa: F821
_object_type_hasattr = _object_type_hasattr  # noqa: F821
_object_type_getattr = _object_type_getattr  # noqa: F821
_os_write = _os_write  # noqa: F821
_patch = _patch  # noqa: F821
_str_check = _str_check  # noqa: F821
_str_len = _str_len  # noqa: F821
_type = _type  # noqa: F821
_type_name = _type_name  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


DEFAULT_BUFFER_SIZE = 8 * 1024  # bytes


import builtins
from errno import EAGAIN as errno_EAGAIN, EISDIR as errno_EISDIR

from _os import (
    close as _os_close,
    fstat_size as _os_fstat_size,
    ftruncate as _os_ftruncate,
    isatty as _os_isatty,
    isdir as _os_isdir,
    linesep as _os_linesep,
    lseek as _os_lseek,
    open as _os_open,
    parse_mode as _os_parse_mode,
    read as _os_read,
    set_noinheritable as _os_set_noinheritable,
)
from _thread import Lock as _thread_Lock


def _whence_guard(whence):
    if whence == 0 or whence == 1 or whence == 2:
        return
    raise ValueError("invalid whence value")


BlockingIOError = builtins.BlockingIOError


class BufferedRandom:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class IncrementalNewlineDecoder(bootstrap=True):
    def __init__(self, decoder, translate, errors="strict"):
        if not _int_check(translate):
            raise TypeError(
                f"an integer is required (got type {_type(translate).__name__})"
            )
        self._errors = errors
        self._translate = translate
        self._decoder = decoder
        self._seennl = 0
        self._pendingcr = False

    def decode(self, input, final=False):
        if not _int_check(final):
            raise TypeError(
                f"an integer is required (got type {_type(final).__name__})"
            )
        # decode input (with the eventual \r from a previous pass)
        if self._decoder is None:
            output = input
        else:
            output = self._decoder.decode(input, final=bool(final))
        if self._pendingcr and (output or final):
            output = "\r" + output
            self._pendingcr = False

        # retain last \r even when not translating data:
        # then readline() is sure to get \r\n in one pass
        if output.endswith("\r") and not final:
            output = output[:-1]
            self._pendingcr = True

        # Record which newlines are read
        crlf = output.count("\r\n")
        cr = output.count("\r") - crlf
        lf = output.count("\n") - crlf
        self._seennl |= (lf and self._LF) | (cr and self._CR) | (crlf and self._CRLF)

        if self._translate:
            if crlf:
                output = output.replace("\r\n", "\n")
            if cr:
                output = output.replace("\r", "\n")

        return output

    def getstate(self):
        if self._decoder is None:
            buf = b""
            flag = 0
        else:
            buf, flag = self._decoder.getstate()
        flag <<= 1
        if self._pendingcr:
            flag |= 1
        return buf, flag

    def setstate(self, state):
        buf, flag = state
        self._pendingcr = bool(flag & 1)
        if self._decoder is not None:
            self._decoder.setstate((buf, flag >> 1))

    def reset(self):
        self._seennl = 0
        self._pendingcr = False
        if self._decoder is not None:
            self._decoder.reset()

    _LF = 1
    _CR = 2
    _CRLF = 4

    @property
    def newlines(self):
        return (
            None,
            "\n",
            "\r",
            ("\r", "\n"),
            "\r\n",
            ("\n", "\r\n"),
            ("\r", "\r\n"),
            ("\r", "\n", "\r\n"),
        )[self._seennl]


class UnsupportedOperation(OSError, ValueError):
    pass


class _IOBase(bootstrap=True):
    """The abstract base class for all I/O classes, acting on streams of
    bytes. There is no public constructor.

    This class provides default method implementations that derived classes can
    override selectively; the default implementations represent a file that
    cannot be read, written or seeked.

    The basic type used for binary data read from or written to a file is
    bytes. Other bytes-like objects are accepted as method arguments too. In
    some cases (such as readinto), a writable object is required.

    Note that calling any method (even inquiries) on a closed stream is
    undefined. Implementations may raise OSError in this case.
    """

    ### Internal ###

    def _unsupported(self, name):
        """Internal: raise an OSError exception for unsupported operations."""
        raise UnsupportedOperation(f"{self.__class__.__name__}.{name}() not supported")

    ### Positioning ###

    def seek(self, pos, whence=0):
        """Change stream position.

        Change the stream position to byte offset pos. Argument pos is
        interpreted relative to the position indicated by whence.  Values
        for whence are ints:

        * SEEK_SET=0 -- start of stream (the default); offset should be zero or
                        positive
        * SEEK_CUR=1 -- current stream position; offset may be negative
        * SEEK_END=2 -- end of stream; offset is usually negative
        Some operating systems / file systems could provide additional values.

        Return an int indicating the new absolute position.
        """
        self._unsupported("seek")

    def tell(self):
        """Return an int indicating the current stream position."""
        return self.seek(0, 1)

    def truncate(self, pos=None):
        """Truncate file to size bytes.

        Size defaults to the current IO position as reported by tell().  Return
        the new size.
        """
        self._unsupported("truncate")

    ### Flush and close ###

    def flush(self):
        """Flush write buffers, if applicable.

        This is not implemented for read-only and non-blocking streams.
        """
        self._checkClosed()

    def close(self):
        """Flush and close the IO object.

        This method has no effect if the file is already closed.

        Note that calling any method (even inquiries) on a closed stream is
        undefined. Implementations may raise OSError in this case.
        """
        if not self._closed:
            try:
                self.flush()
            finally:
                self._closed = True

    ### Inquiries ###

    def seekable(self):
        """Return a bool indicating whether object supports random access.

        If False, seek(), tell() and truncate() will raise OSError.
        This method may need to do a test seek().
        """
        return False

    def _checkSeekable(self, msg=None):
        """Internal: raise UnsupportedOperation if file is not seekable
        """
        if not self.seekable():
            raise UnsupportedOperation(
                "File or stream is not seekable." if msg is None else msg
            )

    def readable(self):
        """Return a bool indicating whether object was opened for reading.

        If False, read() will raise OSError.
        """
        return False

    def _checkReadable(self, msg=None):
        """Internal: raise UnsupportedOperation if file is not readable
        """
        if not self.readable():
            raise UnsupportedOperation(
                "File or stream is not readable." if msg is None else msg
            )

    def writable(self):
        """Return a bool indicating whether object was opened for writing.

        If False, write() and truncate() will raise OSError.
        """
        return False

    def _checkWritable(self, msg=None):
        """Internal: raise UnsupportedOperation if file is not writable
        """
        if not self.writable():
            raise UnsupportedOperation(
                "File or stream is not writable." if msg is None else msg
            )

    @property
    def closed(self):
        """closed: bool.  True iff the file has been closed.

        For backwards compatibility, this is a property, not a predicate.
        """
        return self._closed

    def _checkClosed(self, msg=None):
        """Internal: raise a ValueError if file is closed
        """
        if self.closed:
            raise ValueError("I/O operation on closed file." if msg is None else msg)

    ### Context manager ###

    def __enter__(self):  # That's a forward reference
        """Context management protocol.  Returns self (an instance of IOBase).

        IOBase supports the :keyword:`with` statement. In this example, fp
        is closed after the suite of the with statement is complete:

        with open('spam.txt', 'r') as fp:
            fp.write('Spam and eggs!')
        """
        self._checkClosed()
        return self

    def __exit__(self, *args):
        """Context management protocol.  Calls close()"""
        self.close()

    ### Lower-level APIs ###

    def fileno(self):
        """Returns underlying file descriptor (an int) if one exists.

        An OSError is raised if the IO object does not use a file descriptor.
        """
        self._unsupported("fileno")

    def isatty(self):
        """Return a bool indicating whether this is an 'interactive' stream.

        Return False if it can't be determined.
        """
        self._checkClosed()
        return False

    ### Readline[s] and writelines ###

    def _peek_readahead(self, size):
        readahead = self.peek(1)
        if not readahead:
            return 1
        n = (readahead.find(b"\n") + 1) or len(readahead)
        if size >= 0:
            # TODO(T47866758): Use less generic code to do this computation
            # since all of the types are known ahead of time.
            n = min(n, size)
        return n

    def _const_readahead(self, size):
        return 1

    def readline(self, size=-1):
        r"""Read and return a line of bytes from the stream.

        If size is specified, at most size bytes will be read.
        Size should be an int.

        The line terminator is always b'\n' for binary files; for text
        files, the newlines argument to open can be used to select the line
        terminator(s) recognized.
        """
        if hasattr(self, "peek"):
            nreadahead = self._peek_readahead
        else:
            nreadahead = self._const_readahead

        if size is None:
            size = -1
        elif not isinstance(size, int):
            raise TypeError("size must be an integer")
        res = bytearray()
        while size < 0 or len(res) < size:
            b = self.read(nreadahead(size))
            if not b:
                break
            res += b
            # TODO(T47144276): Implement bytearray.endswith so we can do
            # res.endswith(b"\n"):
            if res[-1] == ord("\n"):
                break
        return bytes(res)

    def __iter__(self):
        """IOBase (and its subclasses) support the iterator protocol, meaning
        that an IOBase object can be iterated over yielding the lines in a
        stream.
        """
        self._checkClosed()
        return self

    def __next__(self):
        line = self.readline()
        if not line:
            raise StopIteration
        return line

    def readlines(self, hint=None):
        """Return a list of lines from the stream.

        hint can be specified to control the number of lines read: no more
        lines will be read if the total size (in bytes/characters) of all
        lines so far exceeds hint.
        """
        if hint is None or hint <= 0:
            return list(self)
        n = 0
        lines = []
        for line in self:
            lines.append(line)
            n += len(line)
            if n >= hint:
                break
        return lines

    def writelines(self, lines):
        self._checkClosed()
        for line in lines:
            self.write(line)


class _TextIOBase(_IOBase):
    def read(self, size=-1):
        self._unsupported("read")

    def write(self, s):
        self._unsupported("write")

    def readline(self):
        self._unsupported("readline")

    def detach(self):
        self._unsupported("detach")

    @property
    def encoding(self):
        return None

    @property
    def newlines(self):
        return None

    @property
    def errors(self):
        return None


class _RawIOBase(_IOBase, bootstrap=True):
    """Base class for raw binary I/O."""

    def read(self, size=-1):
        """Read and return up to size bytes, where size is an int.

        Returns an empty bytes object on EOF, or None if the object is
        set not to block and has no data to read.
        """
        if size < 0:
            return self.readall()
        # TODO(T47866758): This should use a mutablebytes or some other data
        # structure to avoid so much copying and so many round-trips. Consider:
        # 1. We create a bytearray
        # 2. We pass that to readinto
        # 3. readinto calls native code
        # 4. Native code allocates some native memory to write into
        # 5. Native code copies that native stuff out into the byte array
        # 6. The byte array is copied out into bytes here
        # Very slow.
        b = bytearray(size.__index__())
        n = self.readinto(b)
        if n is None:
            return None
        # TODO(T47144276): Implement bytearray.__delitem__ so we can use
        # del b[n:]
        b = b[:n]
        return bytes(b)

    def readall(self):
        """Read until EOF, using multiple read() call."""
        res = bytearray()
        while True:
            data = self.read(DEFAULT_BUFFER_SIZE)
            # data could be b'' or None
            if not data:
                break
            # TODO(T47866758): This is a really sub-par readall that could
            # stress the GC with large I/O operations. We really want a rope or
            # similar data structure here.
            res += data
        if res:
            return bytes(res)
        # b'' or None
        return data

    def readinto(self, b):
        """Read bytes into a pre-allocated bytes-like object b.

        Returns an int representing the number of bytes read (0 for EOF), or
        None if the object is set not to block and has no data to read.
        """
        raise NotImplementedError("readinto")

    def write(self, b):
        """Write the given buffer to the IO stream.

        Returns the number of bytes written, which may be less than the
        length of b in bytes.
        """
        raise NotImplementedError("write")


class _BufferedIOBase(_IOBase, bootstrap=True):
    """Base class for buffered IO objects.

    The main difference with RawIOBase is that the read() method
    supports omitting the size argument, and does not have a default
    implementation that defers to readinto().

    In addition, read(), readinto() and write() may raise
    BlockingIOError if the underlying raw stream is in non-blocking
    mode and not ready; unlike their raw counterparts, they will never
    return None.

    A typical implementation should not inherit from a RawIOBase
    implementation, but wrap one.
    """

    def read(self, size=None):
        """Read and return up to size bytes, where size is an int.

        If the argument is omitted, None, or negative, reads and
        returns all data until EOF.

        If the argument is positive, and the underlying raw stream is
        not 'interactive', multiple raw reads may be issued to satisfy
        the byte count (unless EOF is reached first).  But for
        interactive raw streams, at most one raw read will be issued, and a
        short result does not imply that EOF is imminent.

        Returns an empty bytes array on EOF.

        Raises BlockingIOError if the underlying raw stream has no
        data at the moment.
        """
        self._unsupported("read")

    def read1(self, size=None):
        """Read up to size bytes with at most one read() system call,
        where size is an int.
        """
        self._unsupported("read1")

    def readinto(self, b):
        """Read bytes into a pre-allocated bytes-like object b.

        Like read(), this may issue multiple reads to the underlying raw
        stream, unless the latter is 'interactive'.

        Returns an int representing the number of bytes read (0 for EOF).

        Raises BlockingIOError if the underlying raw stream has no
        data at the moment.
        """

        return self._readinto(b, read1=False)

    def readinto1(self, b):
        """Read bytes into buffer *b*, using at most one system call

        Returns an int representing the number of bytes read (0 for EOF).

        Raises BlockingIOError if the underlying raw stream has no
        data at the moment.
        """

        return self._readinto(b, read1=True)

    def _readinto(self, b, read1):
        if not isinstance(b, memoryview):
            b = memoryview(b)
        # TODO(emacs): Here and throughout this file, come up with a better
        # buffer / byteslike name than "b"
        b = b.cast("B")

        if read1:
            data = self.read1(len(b))
        else:
            data = self.read(len(b))
        n = len(data)

        # TODO(T47880928): Implement memoryview.__setitem__ and
        # _memoryview_setslice so we can use that here
        b[:n] = data

        return n

    def write(self, b):
        """Write the given bytes buffer to the IO stream.

        Return the number of bytes written, which is always the length of b
        in bytes.

        Raises BlockingIOError if the buffer is full and the
        underlying raw stream cannot accept more data at the moment.
        """
        self._unsupported("write")

    def detach(self):
        """
        Separate the underlying raw stream from the buffer and return it.

        After the raw stream has been detached, the buffer is in an unusable
        state.
        """
        self._unsupported("detach")


class _BufferedIOMixin(_BufferedIOBase, bootstrap=True):
    def __getstate__(self):
        raise TypeError(f"can not serialize a '{self.__class__.__name__}' object")

    def __init__(self, raw):
        self._raw = raw

    def __repr__(self):
        try:
            name = self.name
        except Exception:
            return f"<{_type_name(self.__class__)}>"
        else:
            return f"<{_type_name(self.__class__)} name={name!r}>"

    ### Positioning ###

    def seek(self, pos, whence=0):
        _whence_guard(whence)
        new_position = self.raw.seek(pos, whence)
        if new_position < 0:
            raise OSError("seek() returned an invalid position")
        return new_position

    def tell(self):
        pos = self.raw.tell()
        if pos < 0:
            raise OSError("tell() returned an invalid position")
        return pos

    def truncate(self, pos=None):
        # Flush the stream.  We're mixing buffered I/O with lower-level I/O,
        # and a flush may be necessary to synch both views of the current
        # file state.
        self.flush()

        if pos is None:
            pos = self.tell()
        return self.raw.truncate(pos)

    ### Flush and close ###

    def flush(self):
        if self.closed:
            raise ValueError("flush of closed file")
        self.raw.flush()

    def close(self):
        if self.raw is not None and not self.closed:
            try:
                # may raise BlockingIOError or BrokenPipeError etc
                self.flush()
            finally:
                self.raw.close()

    def detach(self):
        if self.raw is None:
            raise ValueError("raw stream already detached")
        self.flush()
        raw = self._raw
        self._raw = None
        return raw

    ### Inquiries ###

    def seekable(self):
        return self.raw.seekable()

    @property
    def raw(self):
        return self._raw

    @property
    def closed(self):
        return self.raw.closed

    @property
    def name(self):
        return self.raw.name

    @property
    def mode(self):
        return self.raw.mode

    ### Lower-level APIs ###

    def fileno(self):
        return self.raw.fileno()

    def isatty(self):
        return self.raw.isatty()


class BufferedRWPair(_BufferedIOBase):
    def __init__(self, reader, writer, buffer_size=DEFAULT_BUFFER_SIZE):
        if not reader.readable():
            raise UnsupportedOperation('"reader" argument must be readable.')

        if not writer.writable():
            raise UnsupportedOperation('"writer" argument must be writable.')

        self.reader = BufferedReader(reader, buffer_size)
        self.writer = BufferedWriter(writer, buffer_size)

    def close(self):
        try:
            self.writer.close()
        finally:
            self.reader.close()

    @property
    def closed(self):
        return self.writer.closed

    def flush(self):
        return self.writer.flush()

    def isatty(self):
        return self.writer.isatty() or self.reader.isatty()

    def peek(self, size=0):
        return self.reader.peek(size)

    def read(self, size=None):
        if size is None:
            size = -1
        return self.reader.read(size)

    def read1(self, size):
        return self.reader.read1(size)

    def readable(self):
        return self.reader.readable()

    def readinto(self, b):
        return self.reader.readinto(b)

    def readinto1(self, b):
        return self.reader.readinto1(b)

    def writable(self):
        return self.writer.writable()

    def write(self, b):
        return self.writer.write(b)


class BufferedReader(_BufferedIOMixin, bootstrap=True):
    def __init__(self, raw, buffer_size=DEFAULT_BUFFER_SIZE):
        if not raw.readable():
            raise OSError("File or stream is not readable.")

        _BufferedIOMixin.__init__(self, raw)
        if buffer_size <= 0:
            raise ValueError("invalid buffer size")
        self.buffer_size = buffer_size
        self._reset_read_buf()
        self._read_lock = _thread_Lock()

    def readable(self):
        return self.raw.readable()

    def _reset_read_buf(self):
        self._read_buf = b""
        self._read_pos = 0

    def read(self, size=None):
        if size is not None and size < -1:
            raise ValueError("read length must be positive or -1")
        with self._read_lock:
            return self._read_unlocked(size)

    def _read_unlocked(self, n=None):
        if self.raw is None:
            raise ValueError("raw stream has been detached")

        if self.closed:
            raise ValueError("read of closed file")

        nodata_val = b""
        empty_values = (b"", None)
        buf = self._read_buf
        pos = self._read_pos

        # Special case for when the number of bytes to read is unspecified.
        if n is None or n == -1:
            self._reset_read_buf()
            if hasattr(self.raw, "readall"):
                chunk = self.raw.readall()
                if chunk is None:
                    return buf[pos:] or None
                else:
                    return buf[pos:] + chunk
            chunks = [buf[pos:]]  # Strip the consumed bytes.
            current_size = 0
            while True:
                # Read until EOF or until read() would block.
                chunk = self.raw.read()
                if chunk in empty_values:
                    nodata_val = chunk
                    break
                current_size += len(chunk)
                chunks.append(chunk)
            return b"".join(chunks) or nodata_val

        # The number of bytes to read is specified, return at most n bytes.
        avail = len(buf) - pos  # Length of the available buffered data.
        if n <= avail:
            # Fast path: the data to read is fully buffered.
            self._read_pos += n
            return buf[pos : pos + n]
        # Slow path: read from the stream until enough bytes are read, or until
        # an EOF occurs or until read() would block.
        chunks = [buf[pos:]]
        wanted = max(self.buffer_size, n)
        while avail < n:
            chunk = self.raw.read(wanted)
            if chunk in empty_values:
                nodata_val = chunk
                break
            avail += len(chunk)
            chunks.append(chunk)
        # n is more than avail only when an EOF occurred or when
        # read() would have blocked.
        n = min(n, avail)
        out = b"".join(chunks)
        self._read_buf = out[n:]  # Save the extra data in the buffer.
        self._read_pos = 0
        return out[:n] if out else nodata_val

    def peek(self, size=0):
        with self._read_lock:
            return self._peek_unlocked(size)

    def _peek_unlocked(self, n=0):
        want = min(n, self.buffer_size)
        have = len(self._read_buf) - self._read_pos
        if have < want or have <= 0:
            to_read = self.buffer_size - have
            current = self.raw.read(to_read)
            if current:
                self._read_buf = self._read_buf[self._read_pos :] + current
                self._read_pos = 0
        return self._read_buf[self._read_pos :]

    def read1(self, size):
        # Returns up to size bytes. If at least one byte is buffered, we only
        # return buffered bytes. Otherwise, we do one raw read.
        if size < 0:
            raise ValueError("number of bytes to read must be positive")
        if size == 0:
            return b""
        with self._read_lock:
            self._peek_unlocked(1)
            return self._read_unlocked(size)

    def _readinto(self, buf, read1):
        # Need to create a memoryview object of type 'b', otherwise we may not
        # be able to assign bytes to it, and slicing it would create a new
        # object.
        if not isinstance(buf, memoryview):
            buf = memoryview(buf)
        if buf.nbytes == 0:
            return 0
        buf = buf.cast("B")

        written = 0
        with self._read_lock:
            while written < len(buf):

                # First try to read from internal buffer
                avail = min(len(self._read_buf) - self._read_pos, len(buf))
                if avail:
                    buf[written : written + avail] = self._read_buf[
                        self._read_pos : self._read_pos + avail
                    ]
                    self._read_pos += avail
                    written += avail
                    if written == len(buf):
                        break

                # If remaining space in callers buffer is larger than internal
                # buffer, read directly into callers buffer
                if len(buf) - written > self.buffer_size:
                    n = self.raw.readinto(buf[written:])
                    if not n:
                        break  # EOF
                    written += n

                # Otherwise refill internal buffer - unless we're in read1 mode
                # and already got some data
                elif not (read1 and written):
                    if not self._peek_unlocked(1):
                        break  # EOF

                # In readinto1 mode, return as soon as we have some data
                if read1 and written:
                    break

        return written

    def tell(self):
        return _BufferedIOMixin.tell(self) - len(self._read_buf) + self._read_pos

    def seek(self, pos, whence=0):
        _whence_guard(whence)
        with self._read_lock:
            if whence == 1:
                pos -= len(self._read_buf) - self._read_pos
            pos = _BufferedIOMixin.seek(self, pos, whence)
            self._reset_read_buf()
            return pos


class BufferedWriter(_BufferedIOMixin, bootstrap=True):
    def __init__(self, raw, buffer_size=DEFAULT_BUFFER_SIZE):
        if not raw.writable():
            raise UnsupportedOperation("File or stream is not writable.")

        _BufferedIOMixin.__init__(self, raw)
        if buffer_size <= 0:
            raise ValueError("buffer size must be strictly positive")
        self.buffer_size = buffer_size
        self._write_buf = bytearray()  # TODO(T47880928): use a memoryview
        self._write_lock = _thread_Lock()

    def _flush_unlocked(self):
        if self.closed:
            raise ValueError("flush of closed file")
        while self._write_buf:
            try:
                n = self.raw.write(self._write_buf)
            except BlockingIOError:
                raise RuntimeError(
                    "self.raw should implement RawIOBase: "
                    "it should not raise BlockingIOError"
                )
            if n is None:
                raise BlockingIOError(
                    errno_EAGAIN, "write could not complete without blocking", 0
                )
            if n < 0 or n > _bytearray_len(self._write_buf):
                raise IOError(
                    f"raw write() returned invalid length {n} (should have "
                    f"been between 0 and {_bytearray_len(self._write_buf)})"
                )
            del self._write_buf[:n]

    def flush(self):
        with self._write_lock:
            self._flush_unlocked()

    def seek(self, pos, whence=0):
        _whence_guard(whence)
        with self._write_lock:
            self._flush_unlocked()
            return _BufferedIOMixin.seek(self, pos, whence)

    def tell(self):
        return _BufferedIOMixin.tell(self) + _bytearray_len(self._write_buf)

    def truncate(self, pos=None):
        with self._write_lock:
            self._flush_unlocked()
            if pos is None:
                pos = self.raw.tell()
            return self.raw.truncate(pos)

    def writable(self):
        return self.raw.writable()

    def write(self, b):
        if self.closed:
            raise ValueError("write to closed file")
        if _str_check(b):
            raise TypeError("can't write str to binary stream")
        with self._write_lock:
            if _bytearray_len(self._write_buf) > self.buffer_size:
                # We're full, so let's pre-flush the buffer.  (This may raise
                # BlockingIOError with characters_written == 0.)
                self._flush_unlocked()
            before = _bytearray_len(self._write_buf)
            self._write_buf.extend(b)
            written = _bytearray_len(self._write_buf) - before
            if _bytearray_len(self._write_buf) > self.buffer_size:
                try:
                    self._flush_unlocked()
                except BlockingIOError as e:
                    if _bytearray_len(self._write_buf) > self.buffer_size:
                        # We've hit the buffer_size. We have to accept a partial
                        # write and cut back our buffer.
                        overage = _bytearray_len(self._write_buf) - self.buffer_size
                        written -= overage
                        self._write_buf = self._write_buf[: self.buffer_size]
                        raise BlockingIOError(e.errno, e.strerror, written)
            return written


class BytesIO(bootstrap=True):
    """Buffered I/O implementation using an in-memory bytes buffer."""

    def __init__(self, initial_bytes=None):
        if initial_bytes is None:
            self._buffer = bytearray()
        else:
            _byteslike_guard(initial_bytes)
            self._buffer = bytearray(initial_bytes)
        self._closed = False
        self._pos = 0

    def __getstate__(self):
        _unimplemented("BytesIO.__getstate__")
        # if self.closed:
        #     raise ValueError("__getstate__ on closed file")
        # return (self.getvalue(), self._pos, self.__dict__.copy())

    def getvalue(self):
        """Return the bytes value (contents) of the buffer
        """
        if self.closed:
            raise ValueError("getvalue on closed file")
        return bytes(self._buffer)

    def getbuffer(self):
        """Return a readable and writable view of the buffer.
        """
        if self.closed:
            raise ValueError("getbuffer on closed file")
        return memoryview(self._buffer)

    def close(self):
        self._buffer.clear()
        _BufferedIOBase.close(self)

    def read(self, size=None):
        if self.closed:
            raise ValueError("read from closed file")
        if size is None:
            size = -1
        if size < 0:
            size = len(self._buffer)
        if len(self._buffer) <= self._pos:
            return b""
        # TODO(T47866758): Use less generic code to do this computation and
        # slicing since all of the types are known ahead of time.
        newpos = min(len(self._buffer), self._pos + size)
        # TODO(T47866758): This should slice directly into the bytearray and
        # produce a bytes object, no intermediate bytearray.
        # TODO(T41628357): Implement bytearray.__setitem__ and
        # _bytearray_setslice so we can use that here
        b = self._buffer[self._pos : newpos]
        self._pos = newpos
        return bytes(b)

    def read1(self, size):
        """This is the same as read.
        """
        return self.read(size)

    def write(self, b):
        if self.closed:
            raise ValueError("write to closed file")
        if isinstance(b, str):
            raise TypeError("can't write str to binary stream")
        # TODO(T53866215): memoryview.__enter__ and __exit__
        num_written = memoryview(b).nbytes  # Size of any bytes-like object
        if num_written == 0:
            return 0
        pos = self._pos
        if pos > len(self._buffer):
            # Inserts null bytes between the current end of the file and the
            # new write position.
            # TODO(T47866758): Use less generic code to pad a bytearray buffer
            # with NUL bytes.
            padding = b"\x00" * (pos - len(self._buffer))
            self._buffer += padding
        # TODO(T41628357): Implement bytearray.__setitem__ and
        # _bytearray_setslice so we can use that here
        self._buffer[pos : pos + num_written] = b
        self._pos += num_written
        return num_written

    def seek(self, pos, whence=0):
        if self.closed:
            raise ValueError("seek on closed file")
        # TODO(emacs):
        try:
            pos = _index(pos)
        except AttributeError as err:
            raise TypeError("an integer is required") from err
        if whence == 0:
            if pos < 0:
                raise ValueError(f"negative seek position {pos!r}")
            self._pos = pos
        elif whence == 1:
            # TODO(T47866758): Use less generic code to do this computation
            # since all of the types are known ahead of time.
            self._pos = max(0, self._pos + pos)
        elif whence == 2:
            self._pos = max(0, len(self._buffer) + pos)
        else:
            raise ValueError("unsupported whence value")
        return self._pos

    def tell(self):
        if self.closed:
            raise ValueError("tell on closed file")
        return self._pos

    def truncate(self, pos=None):
        if self.closed:
            raise ValueError("truncate on closed file")
        if pos is None:
            pos = self._pos
        elif not _int_check(pos):
            raise TypeError(f"integer argument expected, got '{_type(pos).__name__}'")
        if pos < 0:
            raise ValueError(f"negative truncate position {pos!r}")
        # TODO(T47144276): Implement bytearray.__delitem__ so we can use
        # del self._buffer[pos:]
        self._buffer = self._buffer[:pos]
        return pos

    def readable(self):
        if self.closed:
            raise ValueError("I/O operation on closed file.")
        return True

    def writable(self):
        if self.closed:
            raise ValueError("I/O operation on closed file.")
        return True

    def seekable(self):
        if self.closed:
            raise ValueError("I/O operation on closed file.")
        return True


class FileIO(_RawIOBase, bootstrap=True):
    def __init__(self, file, mode="r", closefd=True, opener=None):  # noqa: C901
        if _float_check(file):
            raise TypeError("integer argument expected, got float")
        fd = -1
        if _int_check(file):
            if file < 0:
                raise ValueError("negative file descriptor")
            fd = file

        if not _str_check(mode):
            raise TypeError(f"invalid mode for FileIO: {mode!s}")
        mode_set = frozenset(mode)
        if not mode_set <= frozenset("xrwab+"):
            raise ValueError(f"invalid mode: {mode!s}")
        # Is mode non empty, with exactly one of r, w, a, or x, and maybe a +
        # i.e. it should match [rwax]\+?
        if sum(c in "rwax" for c in mode) != 1 or mode.count("+") > 1:
            raise ValueError(
                "Must have exactly one of create/read/write/append "
                "mode and at most one plus"
            )

        appending = False
        created = False
        readable = False
        seekable = None
        writable = False

        if "x" in mode:
            created = True
            writable = True
        elif "r" in mode:
            readable = True
        elif "w" in mode:
            writable = True
        elif "a" in mode:
            writable = True
            appending = True

        if "+" in mode:
            readable = True
            writable = True

        flags = _os_parse_mode(mode)
        self.name = file
        if fd < 0:
            # file was not an int, so we have to open it
            if not closefd:
                raise ValueError("Cannot use closefd=False with file name")
            if opener is None:
                fd = _os_open(file, flags, 0o666)
            else:
                fd = opener(file, flags)
                if not _int_check(fd):
                    raise TypeError("expected integer from opener")
                if fd < 0:
                    raise ValueError(f"opener returned {fd}")

        try:
            if opener:
                _os_set_noinheritable(fd)

            if _os_isdir(fd):
                raise IsADirectoryError(errno_EISDIR, "Is a directory")

            # TODO(T52792779): Don't translate newlines if _setmode is non-None
            # by setting O_BINARY

            if appending:
                # For consistent behavior, we explicitly seek to the end of
                # file (otherwise, it might be done only on the first write()).
                _os_lseek(fd, 0, 2)
        except Exception:
            _os_close(fd)
            raise

        self._fd = fd
        self._closefd = closefd
        self._appending = appending
        self._created = created
        self._readable = readable
        self._seekable = seekable
        self._writable = writable

    def __del__(self):
        if not self.closed and self._closefd:
            import warnings

            warnings.warn(
                f"unclosed file {self!r}", ResourceWarning, stacklevel=2, source=self
            )
            self.close()

    def __getstate__(self):
        raise TypeError(f"cannot serialize '{_type_name(self.__class__)}' object")

    def __repr__(self):
        class_name = f"_io.{self.__class__.__qualname__}"
        if self.closed:
            return f"<{class_name} [closed]>"
        try:
            name = self.name
        except AttributeError:
            return (
                f"<{class_name} name={self._fd} "
                f"mode={self.mode!r} closefd={self._closefd!r}>"
            )
        else:
            return (
                f"<{class_name} name={name!r} "
                f"mode={self.mode!r} closefd={self._closefd!r}>"
            )

    def _checkReadable(self):
        if not self._readable:
            raise UnsupportedOperation("File not open for reading")

    def _checkWritable(self, msg=None):
        if not self._writable:
            raise UnsupportedOperation("File not open for writing")

    def read(self, size=None):
        self._checkClosed()
        self._checkReadable()
        if size is None or size < 0:
            return FileIO.readall(self)
        try:
            return _os_read(self._fd, size)
        except BlockingIOError:
            return None

    def readall(self):
        self._checkClosed()
        if not self._readable:
            return b""
        bufsize = DEFAULT_BUFFER_SIZE
        try:
            pos = _os_lseek(self._fd, 0, 1)
            end = _os_fstat_size(self._fd)
            if end >= pos:
                bufsize = end - pos + 1
        except OSError:
            pass

        result = bytearray()
        result_len = 0
        while True:
            if result_len >= bufsize:
                bufsize = result_len
                bufsize += max(bufsize, DEFAULT_BUFFER_SIZE)
            num_bytes = bufsize - result_len
            try:
                chunk = _os_read(self._fd, num_bytes)
            except BlockingIOError:
                if result:
                    break
                return None
            if not chunk:  # reached the end of the file
                break
            result += chunk
            result_len = len(result)

        return bytes(result)

    def readinto(self, byteslike):
        # TODO(T47880928): Support memoryview.__setitem__
        view = memoryview(byteslike).cast("B")
        data = self.read(len(view))
        num_bytes = len(data)
        view[:num_bytes] = data
        return num_bytes

    def write(self, byteslike):
        self._checkClosed()
        self._checkWritable()
        buf = byteslike
        if not _bytes_check(byteslike):
            if not _object_type_hasattr(byteslike, "__buffer__"):
                raise TypeError(
                    "a bytes-like object is required, not "
                    f"'{_type(byteslike).__name__}'"
                )
            try:
                buf = byteslike.__buffer__()
            except Exception:
                raise TypeError(
                    "a bytes-like object is required, not "
                    f"'{_type(byteslike).__name__}'"
                )
            if not _bytes_check(buf):
                raise TypeError(
                    "a bytes-like object is required, not "
                    f"'{_type(byteslike).__name__}'"
                )
        try:
            return _os_write(self._fd, buf)
        except BlockingIOError:
            return None

    def seek(self, pos, whence=0):
        _whence_guard(whence)
        if _float_check(pos):
            raise TypeError("an integer is required")
        self._checkClosed()
        return _os_lseek(self._fd, pos, whence)

    def tell(self):
        self._checkClosed()
        return _os_lseek(self._fd, 0, 1)

    def truncate(self, size=None):
        self._checkClosed()
        self._checkWritable()
        if size is None:
            size = self.tell()
        _os_ftruncate(self._fd, size)
        return size

    def close(self):
        if not self.closed:
            try:
                if self._closefd:
                    _os_close(self._fd)
            finally:
                _RawIOBase.close(self)
                self._fd = -1

    def seekable(self):
        self._checkClosed()
        if self._seekable is None:
            try:
                FileIO.tell(self)
                self._seekable = True
            except OSError:
                self._seekable = False
        return self._seekable

    def readable(self):
        self._checkClosed()
        return self._readable

    def writable(self):
        self._checkClosed()
        return self._writable

    def fileno(self):
        self._checkClosed()
        return self._fd

    def isatty(self):
        self._checkClosed()
        return _os_isatty(self._fd)

    @property
    def closefd(self):
        return self._closefd

    @property
    def mode(self):
        if self._created:
            if self._readable:
                return "xb+"
            else:
                return "xb"
        elif self._appending:
            if self._readable:
                return "ab+"
            else:
                return "ab"
        elif self._readable:
            if self._writable:
                return "rb+"
            else:
                return "rb"
        else:
            return "wb"


class TextIOWrapper(_TextIOBase, bootstrap=True):
    _CHUNK_SIZE = 2048

    # The write_through argument has no effect here since this
    # implementation always writes through.  The argument is present only
    # so that the signature can match the signature of the C version.
    def __init__(  # noqa: C901
        self,
        buffer,
        encoding=None,
        errors=None,
        newline=None,
        line_buffering=False,
        write_through=False,
    ):
        # Argument parsing happens first in CPython's _io module
        if encoding is None:
            # TODO(T53865493): Get correct device/locale encoding using
            # os.device_encoding and locale.getpreferredencoding
            encoding = "ascii"
        elif not _str_check(encoding):
            raise TypeError(
                "TextIOWrapper() argument 2 must be str or None, not "
                f"{_type(encoding).__name__}"
            )
        if errors is None:
            errors = "strict"
        elif not _str_check(errors):
            raise TypeError(
                "TextIOWrapper() argument 3 must be str or None, not "
                f"{_type(errors).__name__}"
            )
        if newline is not None and not _str_check(newline):
            raise TypeError(
                "TextIOWrapper() argument 4 must be str or None, not "
                f"{_type(newline).__name__}"
            )
        if line_buffering is None:
            line_buffering = False
        elif not _int_check(line_buffering):
            raise TypeError(
                f"an integer is required (got type {_type(line_buffering).__name__})"
            )
        if write_through is None:
            write_through = False
        elif not _int_check(write_through):
            raise TypeError(
                f"an integer is required (got type {_type(write_through).__name__})"
            )

        if newline not in (None, "", "\n", "\r", "\r\n"):
            raise ValueError(f"illegal newline value: {newline}")

        self._buffer = buffer
        self._line_buffering = bool(line_buffering)
        self._encoding = encoding
        self._errors = errors
        self._readuniversal = not newline
        self._readtranslate = newline is None
        self._readnl = newline
        self._writetranslate = newline != ""
        self._writenl = newline or _os_linesep
        self._decoder = self._get_decoder() if buffer.readable() else None
        self._encoder = self._get_encoder() if buffer.writable() else None
        self._decoded_chars = ""  # buffer for text returned from decoder
        self._decoded_chars_used = 0  # offset into _decoded_chars for read()
        self._snapshot = None  # info for reconstructing decoder state
        self._seekable = self._telling = buffer.seekable()
        self._has_read1 = hasattr(buffer, "read1")
        self._b2cratio = 0.0

        if self._seekable and self._encoder:
            position = self.buffer.tell()
            if position != 0:
                self._encoder.setstate(0)

    def __next__(self):
        self._checkAttached()
        self._telling = False
        line = self.readline()
        if not _str_check(line):
            raise IOError(
                "readline() should have returned a str object, not "
                f"'{_type(line).__name__}'"
            )
        if not line:
            self._snapshot = None
            self._telling = self._seekable
            raise StopIteration
        return line

    # self._snapshot is either None, or a tuple (dec_flags, next_input)
    # where dec_flags is the second (integer) item of the decoder state
    # and next_input is the chunk of input bytes that comes next after the
    # snapshot point.  We use this to reconstruct decoder states in tell().

    # Naming convention:
    #   - "bytes_..." for integer variables that count input bytes
    #   - "chars_..." for integer variables that count decoded characters

    def __repr__(self):
        try:
            name_component = f" name={self.name!r}"
        except Exception:
            name_component = ""
        try:
            mode_component = "" if self.mode is None else f" mode={self.mode!r}"
        except Exception:
            mode_component = ""
        return (
            f"<_io.TextIOWrapper{name_component}"
            f"{mode_component} encoding={self._encoding!r}>"
        )

    def _checkAttached(self, msg=None):
        if self._buffer is None:
            raise ValueError(
                "underlying buffer has been detached" if msg is None else msg
            )

    def _get_decoded_chars(self, n=None):
        offset = self._decoded_chars_used
        if n is None:
            chars = self._decoded_chars[offset:]
        else:
            chars = self._decoded_chars[offset : offset + n]
        self._decoded_chars_used += len(chars)
        return chars

    def _get_decoder(self):
        import codecs
        import encodings  # noqa: F401

        make_decoder = codecs.getincrementaldecoder(self._encoding)
        decoder = make_decoder(self._errors)
        if self._readuniversal:
            return IncrementalNewlineDecoder(decoder, self._readtranslate)
        return decoder

    def _get_encoder(self):
        import codecs
        import encodings  # noqa: F401

        make_encoder = codecs.getincrementalencoder(self._encoding)
        return make_encoder(self._errors)

    def _pack_cookie(
        self, position, dec_flags=0, bytes_to_feed=0, need_eof=0, chars_to_skip=0
    ):
        # The meaning of a tell() cookie is: seek to position, set the
        # decoder flags to dec_flags, read bytes_to_feed bytes, feed them
        # into the decoder with need_eof as the EOF flag, then skip
        # chars_to_skip characters of the decoded result.  For most simple
        # decoders, tell() will often just give a byte offset in the file.
        return (
            position
            | (dec_flags << 64)
            | (bytes_to_feed << 128)
            | (chars_to_skip << 192)
            | bool(need_eof) << 256
        )

    def _read_chunk(self):
        # The return value is True unless EOF was reached.  The decoded
        # string is placed in self._decoded_chars (replacing its previous
        # value).  The entire input chunk is sent to the decoder, though
        # some of it may remain buffered in the decoder, yet to be
        # converted.

        if self._decoder is None:
            raise UnsupportedOperation("not readable")

        if self._telling:
            # To prepare for tell(), we need to snapshot a point in the
            # file where the decoder's input buffer is empty.

            dec_buffer, dec_flags = self._decoder.getstate()
            # Given this, we know there was a valid snapshot point
            # len(dec_buffer) bytes ago with decoder state (b'', dec_flags).
            if not _bytes_check(dec_buffer):
                raise TypeError(
                    "illegal decoder state: the first item should be a bytes "
                    f"object, not '{_type(dec_buffer).__name__}'"
                )

        # Read a chunk, decode it, and put the result in self._decoded_chars.
        if self._has_read1:
            input_chunk = self.buffer.read1(self._CHUNK_SIZE)
        else:
            input_chunk = self.buffer.read(self._CHUNK_SIZE)
        eof = not input_chunk
        decoded_chars = self._decoder.decode(input_chunk, eof)
        self._set_decoded_chars(decoded_chars)
        if decoded_chars:
            self._b2cratio = len(input_chunk) / len(self._decoded_chars)
        else:
            self._b2cratio = 0.0

        if self._telling:
            # At the snapshot point, len(dec_buffer) bytes before the read,
            # the next input to be decoded is dec_buffer + input_chunk.
            self._snapshot = (dec_flags, dec_buffer + input_chunk)

        return not eof

    def _rewind_decoded_chars(self, n):
        if self._decoded_chars_used < n:
            raise AssertionError("rewind decoded_chars out of bounds")
        self._decoded_chars_used -= n

    # The following three methods implement an ADT for _decoded_chars.
    # Text returned from the decoder is buffered here until the client
    # requests it by calling our read() or readline() method.
    def _set_decoded_chars(self, chars):
        self._decoded_chars = chars
        self._decoded_chars_used = 0

    def _unpack_cookie(self, bigint):
        rest, position = divmod(bigint, 1 << 64)
        rest, dec_flags = divmod(rest, 1 << 64)
        rest, bytes_to_feed = divmod(rest, 1 << 64)
        need_eof, chars_to_skip = divmod(rest, 1 << 64)
        return position, dec_flags, bytes_to_feed, need_eof, chars_to_skip

    @property
    def buffer(self):
        return self._buffer

    def close(self):
        self._checkAttached()
        if not self.closed:
            try:
                self.flush()
            finally:
                self._buffer.close()

    @property
    def closed(self):
        self._checkAttached()
        return self._buffer.closed

    def detach(self):
        self._checkAttached()
        self.flush()
        buffer = self._buffer
        self._buffer = None
        return buffer

    @property
    def encoding(self):
        return self._encoding

    @property
    def errors(self):
        return self._errors

    def fileno(self):
        self._checkAttached()
        return self._buffer.fileno()

    def flush(self):
        self._checkAttached()
        self._checkClosed()
        self.buffer.flush()
        self._telling = self._seekable

    def isatty(self):
        self._checkAttached()
        return self.buffer.isatty()

    @property
    def line_buffering(self):
        return self._line_buffering

    @property
    def name(self):
        self._checkAttached()
        return self._buffer.name

    @property
    def newlines(self):
        self._checkAttached()
        if self._decoder is None:
            return None
        try:
            return self._decoder.newlines
        except AttributeError:
            return None

    def read(self, size=None):
        if size is None:
            size = -1
        elif not _int_check(size):
            raise TypeError(f"integer argument expected, got '{_type(size).__name__}'")
        self._checkAttached()
        self._checkClosed()
        self._checkReadable("not readable")
        decoder = self._decoder
        try:
            size.__index__
        except AttributeError as err:
            raise TypeError("an integer is required") from err
        if size < 0:
            # Read everything.
            result = self._get_decoded_chars() + decoder.decode(
                self._buffer.read(), final=True
            )
            self._set_decoded_chars("")
            self._snapshot = None
            return result
        else:
            # Keep reading chunks until we have size characters to return.
            eof = False
            result = self._get_decoded_chars(size)
            while len(result) < size and not eof:
                eof = not self._read_chunk()
                result += self._get_decoded_chars(size - len(result))
            return result

    def readable(self):
        self._checkAttached()
        return self._buffer.readable()

    def readline(self, size=None):  # noqa: C901
        if size is None:
            size = -1
        elif not _int_check(size):
            size = _index(size)

        self._checkAttached()
        self._checkClosed()

        # Grab all the decoded text (we will rewind any extra bits later).
        line = self._get_decoded_chars()

        start = 0

        pos = endpos = None
        while True:
            if self._readtranslate:
                # Newlines are already translated, only search for \n
                pos = line.find("\n", start)
                if pos >= 0:
                    endpos = pos + 1
                    break
                else:
                    start = len(line)

            elif self._readuniversal:
                # Universal newline search. Find any of \r, \r\n, \n
                # The decoder ensures that \r\n are not split in two pieces
                nlpos = line.find("\n", start)
                crpos = line.find("\r", start)
                if crpos == -1:
                    if nlpos == -1:
                        # Nothing found
                        start = len(line)
                    else:
                        # Found \n
                        endpos = nlpos + 1
                        break
                elif nlpos == -1:
                    # Found lone \r
                    endpos = crpos + 1
                    break
                elif nlpos < crpos:
                    # Found \n
                    endpos = nlpos + 1
                    break
                elif nlpos == crpos + 1:
                    # Found \r\n
                    endpos = crpos + 2
                    break
                else:
                    # Found \r
                    endpos = crpos + 1
                    break
            else:
                # non-universal
                pos = line.find(self._readnl)
                if pos >= 0:
                    endpos = pos + len(self._readnl)
                    break

            if size >= 0 and len(line) >= size:
                endpos = size  # reached length size
                break

            # No line ending seen yet - get more data'
            while self._read_chunk():
                if self._decoded_chars:
                    break
            if self._decoded_chars:
                line += self._get_decoded_chars()
            else:
                # end of file
                self._set_decoded_chars("")
                self._snapshot = None
                return line

        if size >= 0 and endpos > size:
            endpos = size  # don't exceed size

        # Rewind _decoded_chars to just after the line ending we found.
        self._rewind_decoded_chars(len(line) - endpos)
        return line[:endpos]

    def seek(self, cookie, whence=0):  # noqa: C901
        if not _int_check(whence):
            raise TypeError(
                f"an integer is required (got type {_type(whence).__name__})"
            )
        self._checkAttached()
        self._checkClosed()
        self._checkSeekable("underlying stream is not seekable")

        def _reset_encoder(position):
            if self._encoder:
                if position != 0:
                    self._encoder.setstate(0)
                else:
                    self._encoder.reset()

        if whence == 1:  # seek relative to current position
            if cookie != 0:
                raise UnsupportedOperation("can't do nonzero cur-relative seeks")
            # Seeking to the current position should attempt to
            # sync the underlying buffer with the current position.
            whence = 0
            cookie = self.tell()
        elif whence == 2:  # seek relative to end of file
            if cookie != 0:
                raise UnsupportedOperation("can't do nonzero end-relative seeks")
            self.flush()
            position = self.buffer.seek(0, 2)
            self._set_decoded_chars("")
            self._snapshot = None
            if self._decoder:
                self._decoder.reset()
            _reset_encoder(position)
            return position
        elif whence != 0:
            raise ValueError(f"invalid whence ({whence}, should be 0, 1 or 2)")
        if cookie < 0:
            raise ValueError(f"negative seek position {cookie!r}")
        self.flush()

        # The strategy of seek() is to go back to the safe start point
        # and replay the effect of read(chars_to_skip) from there.
        unpacked = self._unpack_cookie(cookie)
        start_pos, dec_flags, bytes_to_feed, need_eof, chars_to_skip = unpacked
        # Seek back to the safe start point.
        self.buffer.seek(start_pos)
        self._set_decoded_chars("")
        self._snapshot = None

        # Restore the decoder to its state from the safe start point.
        if cookie == 0 and self._decoder:
            self._decoder.reset()
        elif self._decoder or dec_flags or chars_to_skip:
            self._decoder = self._decoder
            self._decoder.setstate((b"", dec_flags))
            self._snapshot = (dec_flags, b"")

        if chars_to_skip:
            # Just like _read_chunk, feed the decoder and save a snapshot.
            input_chunk = self.buffer.read(bytes_to_feed)
            self._set_decoded_chars(self._decoder.decode(input_chunk, need_eof))
            self._snapshot = (dec_flags, input_chunk)

            # Skip chars_to_skip of the decoded characters.
            if len(self._decoded_chars) < chars_to_skip:
                raise OSError("can't restore logical file position")
            self._decoded_chars_used = chars_to_skip

        _reset_encoder(cookie)
        return cookie

    def seekable(self):
        self._checkAttached()
        return self._buffer.seekable()

    def tell(self):  # noqa: C901
        self._checkAttached()
        self._checkClosed()
        self._checkSeekable("underlying stream is not seekable")
        if not self._telling:
            raise OSError("telling position disabled by next() call")
        self.flush()
        position = self.buffer.tell()
        decoder = self._decoder
        if decoder is None or self._snapshot is None:
            if self._decoded_chars:
                # This should never happen.
                raise AssertionError("pending decoded text")
            return position

        # Skip backward to the snapshot point (see _read_chunk).
        dec_flags, next_input = self._snapshot
        position -= len(next_input)

        # How many decoded characters have been used up since the snapshot?
        chars_to_skip = self._decoded_chars_used
        if chars_to_skip == 0:
            # We haven't moved from the snapshot point.
            return self._pack_cookie(position, dec_flags)

        # Starting from the snapshot position, we will walk the decoder
        # forward until it gives us enough decoded characters.
        saved_state = decoder.getstate()
        try:
            # Fast search for an acceptable start point, close to our
            # current pos.
            # Rationale: calling decoder.decode() has a large overhead
            # regardless of chunk size; we want the number of such calls to
            # be O(1) in most situations (common decoders, non-crazy input).
            # Actually, it will be exactly 1 for fixed-size codecs (all
            # 8-bit codecs, also UTF-16 and UTF-32).
            skip_bytes = int(self._b2cratio * chars_to_skip)
            skip_back = 1
            assert skip_bytes <= len(next_input)
            while skip_bytes > 0:
                decoder.setstate((b"", dec_flags))
                # Decode up to temptative start point
                n = len(decoder.decode(next_input[:skip_bytes]))
                if n <= chars_to_skip:
                    b, d = decoder.getstate()
                    if not b:
                        # Before pos and no bytes buffered in decoder => OK
                        dec_flags = d
                        chars_to_skip -= n
                        break
                    # Skip back by buffered amount and reset heuristic
                    skip_bytes -= len(b)
                    skip_back = 1
                else:
                    # We're too far ahead, skip back a bit
                    skip_bytes -= skip_back
                    skip_back = skip_back * 2
            else:
                skip_bytes = 0
                decoder.setstate((b"", dec_flags))

            # Note our initial start point.
            start_pos = position + skip_bytes
            start_flags = dec_flags
            if chars_to_skip == 0:
                # We haven't moved from the start point.
                return self._pack_cookie(start_pos, start_flags)

            # Feed the decoder one byte at a time.  As we go, note the
            # nearest "safe start point" before the current location
            # (a point where the decoder has nothing buffered, so seek()
            # can safely start from there and advance to this location).
            bytes_fed = 0
            need_eof = 0
            # Chars decoded since `start_pos`
            chars_decoded = 0
            for i in range(skip_bytes, len(next_input)):
                bytes_fed += 1
                chars_decoded += len(decoder.decode(next_input[i : i + 1]))
                dec_buffer, dec_flags = decoder.getstate()
                if not dec_buffer and chars_decoded <= chars_to_skip:
                    # Decoder buffer is empty, so this is a safe start point.
                    start_pos += bytes_fed
                    chars_to_skip -= chars_decoded
                    start_flags, bytes_fed, chars_decoded = dec_flags, 0, 0
                if chars_decoded >= chars_to_skip:
                    break
            else:
                # We didn't get enough decoded data; signal EOF to get more.
                chars_decoded += len(decoder.decode(b"", final=True))
                need_eof = 1
                if chars_decoded < chars_to_skip:
                    raise OSError("can't reconstruct logical file position")

            # The returned cookie corresponds to the last safe start point.
            return self._pack_cookie(
                start_pos, start_flags, bytes_fed, need_eof, chars_to_skip
            )
        finally:
            decoder.setstate(saved_state)

    def truncate(self, pos=None):
        self._checkAttached()
        self.flush()
        return self.buffer.truncate(pos)

    def writable(self):
        self._checkAttached()
        return self.buffer.writable()

    def write(self, text):
        if not _str_check(text):
            raise TypeError(f"write() argument must be str, not {_type(text).__name__}")
        self._checkAttached()
        self._checkClosed()
        length = _str_len(text)
        haslf = (self._writetranslate or self._line_buffering) and "\n" in text
        if haslf and self._writetranslate and self._writenl != "\n":
            text = text.replace("\n", self._writenl)
        encoder = self._encoder
        b = encoder.encode(text)
        self.buffer.write(b)
        if self._line_buffering and (haslf or "\r" in text):
            self.flush()
        self._set_decoded_chars("")
        self._snapshot = None
        if self._decoder:
            self._decoder.reset()
        return length


class StringIO(TextIOWrapper):
    def __init__(self, initial_value="", newline="\n"):
        # TODO(T53865493): Set encoding to UTF-8 instead of ASCII
        super(StringIO, self).__init__(
            BytesIO(), encoding="ascii", errors="surrogatepass", newline=newline
        )
        if newline is None:
            self._writetranslate = False
        if initial_value is not None:
            if not _str_check(initial_value):
                raise TypeError(
                    "initial_value must be str or None, not "
                    f"{_type(initial_value).__name__}"
                )
            self.write(initial_value)
            self.seek(0)

    def __repr__(self):
        return f"<_io.StringIO object at {_address(self):#x}>"

    def detach(self):
        self._unsupported("detach")

    @property
    def encoding(self):
        return None

    @property
    def errors(self):
        return None

    def getvalue(self):
        self.flush()
        decoder = self._decoder or self._get_decoder()
        old_state = decoder.getstate()
        decoder.reset()
        try:
            return decoder.decode(self.buffer.getvalue(), final=True)
        finally:
            decoder.setstate(old_state)

    def readable(self):
        self._checkClosed()
        return True

    def seekable(self):
        self._checkClosed()
        return True

    def writable(self):
        self._checkClosed()
        return True


def _fspath(obj):
    if _str_check(obj) or _bytes_check(obj):
        return obj
    dunder_fspath = _object_type_getattr(obj, "__fspath__")
    if dunder_fspath is _Unbound:
        raise TypeError("expected str, bytes, or os.PathLike object")
    result = dunder_fspath()
    if _str_check(result) or _bytes_check(result):
        return result
    raise TypeError("expected __fspath__ to return str or bytes")


def open(  # noqa: C901
    file,
    mode="r",
    buffering=-1,
    encoding=None,
    errors=None,
    newline=None,
    closefd=True,
    opener=None,
):
    if not _int_check(file):
        file = _fspath(file)
    if not isinstance(file, (str, bytes, int)):
        # TODO(emacs): Is this check necessary? os.fspath guarantees str/bytes,
        # above check guarantees int or str or bytes
        raise TypeError("invalid file: %r" % file)
    if not isinstance(mode, str):
        raise TypeError(f"open() argument 2 must be str, not {_type(mode).__name__}")
    if not isinstance(buffering, int):
        raise TypeError(
            f"an integer is required (got type {_type(buffering).__name__})"
        )
    if encoding is not None and not isinstance(encoding, str):
        raise TypeError(
            f"open() argument 4 must be str or None, not {_type(encoding).__name__}"
        )
    if errors is not None and not isinstance(errors, str):
        raise TypeError(
            f"open() argument 5 must be str or None, not {_type(errors).__name__}"
        )
    modes = set(mode)
    if modes - set("axrwb+tU") or len(mode) > len(modes):
        raise ValueError("invalid mode: %r" % mode)
    creating = "x" in modes
    reading = "r" in modes
    writing = "w" in modes
    appending = "a" in modes
    updating = "+" in modes
    text = "t" in modes
    binary = "b" in modes
    if "U" in modes:
        if creating or writing or appending or updating:
            raise ValueError("mode U cannot be combined with 'x', 'w', 'a', or '+'")
        import warnings

        warnings.warn("'U' mode is deprecated", DeprecationWarning, 2)
        reading = True
    if text and binary:
        raise ValueError("can't have text and binary mode at once")
    if creating + reading + writing + appending > 1:
        raise ValueError("must have exactly one of create/read/write/append mode")
    if not (creating or reading or writing or appending):
        raise ValueError(
            "Must have exactly one of create/read/write/append mode and at "
            "most one plus"
        )
    if binary and encoding is not None:
        raise ValueError("binary mode doesn't take an encoding argument")
    if binary and errors is not None:
        raise ValueError("binary mode doesn't take an errors argument")
    if binary and newline is not None:
        raise ValueError("binary mode doesn't take a newline argument")
    raw = FileIO(
        file,
        (creating and "x" or "")
        + (reading and "r" or "")
        + (writing and "w" or "")
        + (appending and "a" or "")
        + (updating and "+" or ""),
        closefd,
        opener=opener,
    )
    result = raw
    try:
        line_buffering = False
        if buffering == 1 or buffering < 0 and raw.isatty():
            buffering = -1
            line_buffering = True
        if buffering < 0:
            buffering = DEFAULT_BUFFER_SIZE
        if buffering == 0:
            if binary:
                return result
            raise ValueError("can't have unbuffered text I/O")
        if updating:
            buffer = BufferedRandom(raw, buffering)
        elif creating or writing or appending:
            buffer = BufferedWriter(raw, buffering)
        elif reading:
            buffer = BufferedReader(raw, buffering)
        else:
            raise ValueError("unknown mode: %r" % mode)
        result = buffer
        if binary:
            return result
        text = TextIOWrapper(buffer, encoding, errors, newline, line_buffering)
        result = text
        text.mode = mode
        return result
    except Exception:
        result.close()
        raise


builtins.open = open


# TODO(T47813322): Kill this function once IncrementalNewlineDecoder is written.
@_patch
def _readbytes(bytes):
    pass


# TODO(T47813347): Kill this function once FileIO is written.
@_patch
def _readfile(path):
    pass
