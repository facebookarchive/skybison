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
_bytes_check = _bytes_check  # noqa: F821
_float_check = _float_check  # noqa: F821
_index = _index  # noqa: F821
_int_check = _int_check  # noqa: F821
_object_type_hasattr = _object_type_hasattr  # noqa: F821
_object_type_getattr = _object_type_getattr  # noqa: F821
_os_write = _os_write  # noqa: F821
_patch = _patch  # noqa: F821
_str_check = _str_check  # noqa: F821
_type = _type  # noqa: F821
_type_name = _type_name  # noqa: F821
_unimplemented = _unimplemented  # noqa: F821


DEFAULT_BUFFER_SIZE = 8 * 1024  # bytes
SEEK_SET = 0
SEEK_CUR = 1
SEEK_END = 2


from builtins import BlockingIOError
from errno import EISDIR as errno_EISDIR

from _os import (
    close as _os_close,
    fstat_size as _os_fstat_size,
    ftruncate as _os_ftruncate,
    isatty as _os_isatty,
    isdir as _os_isdir,
    lseek as _os_lseek,
    open as _os_open,
    parse_mode as _os_parse_mode,
    read as _os_read,
    set_noinheritable as _os_set_noinheritable,
)


class BufferedRWPair:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class BufferedRandom:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class BufferedReader:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class BufferedWriter:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class IncrementalNewlineDecoder:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class StringIO:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class TextIOWrapper:
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


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
    """unimplemented"""

    def __init__(self, *args, **kwargs):
        _unimplemented()


class _RawIOBase(bootstrap=True):
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


class _BufferedIOBase(bootstrap=True):
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


class BytesIO(bootstrap=True):
    """Buffered I/O implementation using an in-memory bytes buffer."""

    def __init__(self, initial_bytes=None):
        self._closed = False
        buf = bytearray()
        if initial_bytes is not None:
            buf += initial_bytes
        self._buffer = buf
        self._pos = 0
        self.__dict__ = {}

    def __getstate__(self):
        if self.closed:
            raise ValueError("__getstate__ on closed file")
        return (self.getvalue(), self._pos, self.__dict__.copy())

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
        with memoryview(b) as view:
            num_written = view.nbytes  # Size of any bytes-like object
        if num_written == 0:
            return 0
        pos = self._pos
        if pos > len(self._buffer):
            # Inserts null bytes between the current end of the file
            # and the new write position.
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
        # TODO(emacs): Replace 0 with a SEEK_* constant.
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
                _os_lseek(fd, 0, SEEK_END)
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
            pos = _os_lseek(self._fd, 0, SEEK_CUR)
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

    def seek(self, pos, whence=SEEK_SET):
        if _float_check(pos):
            raise TypeError("an integer is required")
        self._checkClosed()
        return _os_lseek(self._fd, pos, whence)

    def tell(self):
        self._checkClosed()
        return _os_lseek(self._fd, 0, SEEK_CUR)

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


def open(
    file,
    mode="r",
    buffering=-1,
    encoding=None,
    errors=None,
    newline=None,
    closefd=True,
    opener=None,
):
    _unimplemented()


# TODO(T47813322): Kill this function once IncrementalNewlineDecoder is written.
@_patch
def _readbytes(bytes):
    pass


# TODO(T47813347): Kill this function once FileIO is written.
@_patch
def _readfile(path):
    pass
