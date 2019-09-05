#!/usr/bin/env python3
import os
import unittest

import _io
from test_support import pyro_only


def _getfd():
    r, w = os.pipe()
    os.close(w)  # So that read() is harmless
    return r


class _IOBaseTests(unittest.TestCase):
    def test_closed_with_closed_instance_returns_true(self):
        f = _io._IOBase()
        self.assertFalse(f.closed)
        f.close()
        self.assertTrue(f.closed)

    def test_fileno_raises_unsupported_operation(self):
        f = _io._IOBase()
        self.assertRaises(_io.UnsupportedOperation, f.fileno)

    def test_flush_with_closed_instance_raises_value_error(self):
        f = _io._IOBase()
        f.close()
        with self.assertRaises(ValueError):
            f.flush()

    def test_isatty_with_closed_instance_raises_value_error(self):
        f = _io._IOBase()
        f.close()
        with self.assertRaises(ValueError):
            f.isatty()

    def test_isatty_always_returns_false(self):
        f = _io._IOBase()
        self.assertEqual(f.isatty(), False)

    def test_iter_with_closed_instance_raises_value_error(self):
        f = _io._IOBase()
        f.close()
        with self.assertRaises(ValueError):
            f.__iter__()

    def test_next_reads_line(self):
        class C(_io._IOBase):
            def readline(self):
                return "foo"

        f = C()
        self.assertEqual(f.__next__(), "foo")

    def test_readline_calls_read(self):
        class C(_io._IOBase):
            def read(self, size):
                raise UserWarning("foo")

        f = C()
        with self.assertRaises(UserWarning):
            f.readline()

    def test_readlines_calls_read(self):
        class C(_io._IOBase):
            def read(self, size):
                raise UserWarning("foo")

        f = C()
        with self.assertRaises(UserWarning):
            f.readlines()

    def test_readlines_calls_readline(self):
        class C(_io._IOBase):
            def readline(self):
                raise UserWarning("foo")

        f = C()
        with self.assertRaises(UserWarning):
            f.readlines()

    def test_readlines_returns_list(self):
        class C(_io._IOBase):
            def __iter__(self):
                return ["x", "y", "z"].__iter__()

        f = C()
        self.assertEqual(f.readlines(), ["x", "y", "z"])

    def test_readable_always_returns_false(self):
        f = _io._IOBase()
        self.assertFalse(f.readable())

    def test_seek_raises_unsupported_operation(self):
        f = _io._IOBase()
        self.assertRaises(_io.UnsupportedOperation, f.seek, 0)

    def test_seekable_always_returns_false(self):
        f = _io._IOBase()
        self.assertFalse(f.seekable())

    def test_tell_raises_unsupported_operation(self):
        f = _io._IOBase()
        self.assertRaises(_io.UnsupportedOperation, f.tell)

    def test_truncate_raises_unsupported_operation(self):
        f = _io._IOBase()
        self.assertRaises(_io.UnsupportedOperation, f.truncate)

    def test_writable_always_returns_false(self):
        f = _io._IOBase()
        self.assertFalse(f.writable())

    def test_with_with_closed_instance_raises_value_error(self):
        f = _io._IOBase()
        f.close()
        with self.assertRaises(ValueError):
            with f:
                pass

    def test_writelines_calls_write(self):
        class C(_io._IOBase):
            def write(self, line):
                raise UserWarning("foo")

        f = C()
        with self.assertRaises(UserWarning):
            f.writelines(["a"])


class _RawIOBaseTests(unittest.TestCase):
    def test_read_with_readinto_returning_none_returns_none(self):
        class C(_io._RawIOBase):
            def readinto(self, b):
                return None

        f = C()
        self.assertEqual(f.read(), None)

    def test_read_with_negative_size_calls_readall(self):
        class C(_io._RawIOBase):
            def readall(self):
                raise UserWarning("foo")

        f = C()
        with self.assertRaises(UserWarning):
            f.read(-5)

    def test_read_with_nonnegative_size_calls_readinto(self):
        class C(_io._RawIOBase):
            def readinto(self, b):
                for i in range(len(b)):
                    b[i] = ord("x")
                return len(b)

        f = C()
        self.assertEqual(f.read(5), b"xxxxx")

    def test_readall_calls_read(self):
        class C(_io._RawIOBase):
            def read(self, size):
                raise UserWarning("foo")

        f = C()
        with self.assertRaises(UserWarning):
            f.readall()

    def test_readinto_raises_not_implemented_error(self):
        f = _io._RawIOBase()
        b = bytearray(b"")
        self.assertRaises(NotImplementedError, f.readinto, b)

    def test_write_raises_not_implemented_error(self):
        f = _io._RawIOBase()
        self.assertRaises(NotImplementedError, f.write, b"")


class _BufferedIOBaseTests(unittest.TestCase):
    def test_detach_raises_unsupported_operation(self):
        f = _io._BufferedIOBase()
        self.assertIn("detach", _io._BufferedIOBase.__dict__)
        self.assertRaises(_io.UnsupportedOperation, f.detach)

    def test_read_raises_unsupported_operation(self):
        f = _io._BufferedIOBase()
        self.assertIn("read", _io._BufferedIOBase.__dict__)
        self.assertRaises(_io.UnsupportedOperation, f.read)

    def test_read1_raises_unsupported_operation(self):
        f = _io._BufferedIOBase()
        self.assertIn("read1", _io._BufferedIOBase.__dict__)
        self.assertRaises(_io.UnsupportedOperation, f.read1)

    def test_readinto1_calls_read1(self):
        class C(_io._BufferedIOBase):
            def read1(self, n):
                raise UserWarning("foo")

        f = C()
        with self.assertRaises(UserWarning):
            f.readinto1(bytearray())

    def test_readinto_calls_read(self):
        class C(_io._BufferedIOBase):
            def read(self, n):
                raise UserWarning("foo")

        f = C()
        with self.assertRaises(UserWarning):
            f.readinto(bytearray())

    def test_write_raises_unsupported_operation(self):
        f = _io._BufferedIOBase()
        self.assertIn("write", _io._BufferedIOBase.__dict__)
        self.assertRaises(_io.UnsupportedOperation, f.write, b"")


class BytesIOTests(unittest.TestCase):
    def test_init_returns_bytesio_instance(self):
        f = _io.BytesIO(b"foo")
        self.assertIsInstance(f, _io.BytesIO)
        self.assertEqual(f.getvalue(), b"foo")
        f.close()

    def test_close_clears_buffer(self):
        f = _io.BytesIO(b"foo")
        f.close()
        with self.assertRaises(ValueError):
            f.getvalue()

    def test_close_with_closed_instance_does_not_raise_value_error(self):
        f = _io.BytesIO(b"foo")
        f.close()
        f.close()

    def test_dunder_getstate_returns_tuple(self):
        f = _io.BytesIO(b"foo")
        result = f.__getstate__()
        self.assertIsInstance(result, tuple)
        self.assertEqual(result[0], b"foo")
        self.assertEqual(result[1], 0)
        self.assertIsInstance(result[2], dict)

    def test_getvalue_returns_bytes_of_buffer(self):
        f = _io.BytesIO(b"foo")
        result = f.getvalue()
        self.assertIsInstance(result, bytes)
        self.assertEqual(result, b"foo")

    def test_getbuffer_returns_bytes_of_buffer(self):
        f = _io.BytesIO(b"foo")
        result = f.getbuffer()
        self.assertIsInstance(result, memoryview)
        # TODO(T47881505): Write memoryview.__eq__ so we can check the buffer
        # with self.assertEqual(result, b"foo")

    def test_read_one_byte_reads_bytes(self):
        f = _io.BytesIO(b"foo")
        self.assertEqual(f.read(1), b"f")
        self.assertEqual(f.read(1), b"o")
        self.assertEqual(f.read(1), b"o")
        self.assertEqual(f.read(1), b"")
        f.close()

    def test_read_reads_bytes(self):
        f = _io.BytesIO(b"foo")
        self.assertEqual(f.read(), b"foo")
        self.assertEqual(f.read(), b"")
        f.close()

    def test_read1_reads_bytes(self):
        f = _io.BytesIO(b"foo")
        self.assertEqual(f.read1(3), b"foo")
        self.assertEqual(f.read1(3), b"")
        f.close()

    # TODO(T47880928): Test BytesIO.readinto
    # TODO(T47880928): Test BytesIO.readinto1

    def test_readline_reads_one_line(self):
        f = _io.BytesIO(b"hello\nworld\nfoo\nbar")
        self.assertEqual(f.readline(), b"hello\n")
        self.assertEqual(f.readline(), b"world\n")
        self.assertEqual(f.readline(), b"foo\n")
        self.assertEqual(f.readline(), b"bar")
        self.assertEqual(f.readline(), b"")
        f.close()

    def test_readlines_reads_all_lines(self):
        f = _io.BytesIO(b"hello\nworld\nfoo\nbar")
        self.assertEqual(f.readlines(), [b"hello\n", b"world\n", b"foo\n", b"bar"])
        self.assertEqual(f.read(), b"")
        f.close()

    def test_seek_with_int_subclass_does_not_call_dunder_index(self):
        class C(int):
            def __index__(self):
                raise UserWarning("foo")

        f = _io.BytesIO(b"")
        pos = C()
        f.seek(pos)

    def test_seek_calls_dunder_index(self):
        class C:
            def __index__(self):
                raise UserWarning("foo")

        f = _io.BytesIO(b"")
        pos = C()
        with self.assertRaises(UserWarning):
            f.seek(pos)

    def test_seek_with_raising_dunder_int_raises_type_error(self):
        class C:
            def __int__(self):
                raise UserWarning("foo")

        f = _io.BytesIO(b"")
        pos = C()
        with self.assertRaises(TypeError):
            f.seek(pos)

    def test_seek_with_float_raises_type_error(self):
        f = _io.BytesIO(b"")
        pos = 3.4
        with self.assertRaises(TypeError):
            f.seek(pos)

    def test_tell_with_closed_file_raises_value_error(self):
        f = _io.BytesIO(b"")
        f.close()
        with self.assertRaises(ValueError):
            f.tell()

    def test_tell_returns_position(self):
        f = _io.BytesIO(b"foo")
        self.assertEqual(f.tell(), 0)
        f.read(1)
        self.assertEqual(f.tell(), 1)

    def test_truncate_with_int_subclass_does_not_call_dunder_int(self):
        class C(int):
            def __int__(self):
                raise UserWarning("foo")

        f = _io.BytesIO(b"")
        pos = C()
        f.truncate(pos)

    def test_truncate_with_non_int_raises_type_error(self):
        dunder_int_called = False

        class C:
            def __int__(self):
                nonlocal dunder_int_called
                dunder_int_called = True
                raise UserWarning("foo")

        f = _io.BytesIO(b"")
        pos = C()
        with self.assertRaises(TypeError):
            f.truncate(pos)
        self.assertFalse(dunder_int_called)

    def test_truncate_with_raising_dunder_int_raises_type_error(self):
        class C:
            def __int__(self):
                raise UserWarning("foo")

        f = _io.BytesIO(b"")
        pos = C()
        with self.assertRaises(TypeError):
            f.truncate(pos)

    def test_truncate_with_float_raises_type_error(self):
        f = _io.BytesIO(b"")
        pos = 3.4
        with self.assertRaises(TypeError):
            f.truncate(pos)


class FileIOTests(unittest.TestCase):
    def test_close_closes_file(self):
        f = _io.FileIO(_getfd(), closefd=True)
        self.assertFalse(f.closed)
        f.close()
        self.assertTrue(f.closed)

    def test_close_twice_does_not_raise(self):
        f = _io.FileIO(_getfd(), closefd=True)
        f.close()
        f.close()

    def test_closefd_with_closefd_set_returns_true(self):
        with _io.FileIO(_getfd(), closefd=True) as f:
            self.assertTrue(f.closefd)

    def test_closefd_with_closefd_not_set_returns_true(self):
        with _io.FileIO(_getfd(), closefd=False) as f:
            self.assertFalse(f.closefd)

    def test_dunder_getstate_always_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            with _io.FileIO(_getfd(), mode="r") as f:
                f.__getstate__()

        self.assertEqual(str(context.exception), "cannot serialize '_io.FileIO' object")

    def test_dunder_init_with_float_file_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            _io.FileIO(3.14)

        self.assertEqual(str(context.exception), "integer argument expected, got float")

    def test_dunder_init_with_negative_file_descriptor_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            _io.FileIO(-5)

        self.assertEqual(str(context.exception), "negative file descriptor")

    def test_dunder_init_with_non_str_mode_raises_type_error(self):
        with self.assertRaises(TypeError):
            _io.FileIO(_getfd(), mode=3.14)

    def test_dunder_init_with_no_mode_makes_object_readable(self):
        with _io.FileIO(_getfd()) as f:
            self.assertTrue(f.readable())
            self.assertFalse(f.writable())
            self.assertFalse(f.seekable())

    def test_dunder_init_with_r_in_mode_makes_object_readable(self):
        with _io.FileIO(_getfd(), mode="r") as f:
            self.assertTrue(f.readable())
            self.assertFalse(f.writable())

    def test_dunder_init_with_w_in_mode_makes_object_writable(self):
        with _io.FileIO(_getfd(), mode="w") as f:
            self.assertFalse(f.readable())
            self.assertTrue(f.writable())

    def test_dunder_init_with_r_plus_in_mode_makes_object_readable_and_writable(self):
        with _io.FileIO(_getfd(), mode="r+") as f:
            self.assertTrue(f.readable())
            self.assertTrue(f.writable())

    def test_dunder_init_with_w_plus_in_mode_makes_object_readable_and_writable(self):
        with _io.FileIO(_getfd(), mode="w+") as f:
            self.assertTrue(f.readable())
            self.assertTrue(f.writable())

    def test_dunder_init_with_only_plus_in_mode_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            _io.FileIO(_getfd(), mode="+")

        self.assertEqual(
            str(context.exception),
            "Must have exactly one of create/read/write/append mode and at "
            "most one plus",
        )

    def test_dunder_init_with_more_than_one_plus_in_mode_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            _io.FileIO(_getfd(), mode="r++")

        self.assertEqual(
            str(context.exception),
            "Must have exactly one of create/read/write/append mode and at "
            "most one plus",
        )

    def test_dunder_init_with_rw_in_mode_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            _io.FileIO(_getfd(), mode="rw")

        self.assertEqual(
            str(context.exception),
            "Must have exactly one of create/read/write/append mode and at "
            "most one plus",
        )

    def test_dunder_init_with_filename_and_closefd_equals_false_raises_value_error(
        self
    ):
        with self.assertRaises(ValueError) as context:
            _io.FileIO("foobar", closefd=False)

        self.assertEqual(
            str(context.exception), "Cannot use closefd=False with file name"
        )

    def test_dunder_init_with_filename_and_opener_calls_opener(self):
        def opener(fn, flags):
            raise UserWarning("foo")

        with self.assertRaises(UserWarning):
            _io.FileIO("foobar", opener=opener)

    def test_dunder_init_with_opener_returning_non_int_raises_type_error(self):
        def opener(fn, flags):
            return "not_an_int"

        with self.assertRaises(TypeError) as context:
            _io.FileIO("foobar", opener=opener)

        self.assertEqual(str(context.exception), "expected integer from opener")

    def test_dunder_init_with_opener_returning_negative_int_raises_value_error(self):
        def opener(fn, flags):
            return -1

        with self.assertRaises(ValueError) as context:
            _io.FileIO("foobar", opener=opener)

        self.assertEqual(str(context.exception), "opener returned -1")

    # TODO(emacs): Enable side-by-side against CPython when issue38031 is fixed
    @pyro_only
    def test_dunder_init_with_opener_returning_bad_fd_raises_os_error(self):
        with self.assertRaises(OSError):
            _io.FileIO("foobar", opener=lambda name, flags: 1000000)

    def test_dunder_init_with_directory_raises_is_a_directory_error(self):
        with self.assertRaises(IsADirectoryError) as context:
            _io.FileIO("/")

        self.assertEqual(context.exception.args, (21, "Is a directory"))

    def test_dunder_repr_with_closed_file_returns_closed_in_result(self):
        f = _io.FileIO(_getfd(), mode="r")
        f.close()
        self.assertEqual(f.__repr__(), "<_io.FileIO [closed]>")

    def test_dunder_repr_with_closefd_and_fd_returns_fd_in_result(self):
        fd = _getfd()
        with _io.FileIO(fd, mode="r", closefd=True) as f:
            self.assertEqual(
                f.__repr__(), f"<_io.FileIO name={fd} mode='rb' closefd=True>"
            )

    def test_dunder_repr_without_closefd_and_fd_returns_fd_in_result(self):
        fd = _getfd()
        with _io.FileIO(fd, mode="r", closefd=False) as f:
            self.assertEqual(
                f.__repr__(), f"<_io.FileIO name={fd} mode='rb' closefd=False>"
            )

    def test_dunder_repr_with_closefd_and_name_returns_name_in_result(self):
        def opener(fn, flags):
            return 1

        with _io.FileIO("foo", mode="r", opener=opener) as f:
            self.assertEqual(
                f.__repr__(), f"<_io.FileIO name='foo' mode='rb' closefd=True>"
            )

    def test_fileno_with_closed_file_raises_value_error(self):
        f = _io.FileIO(_getfd(), mode="r")
        f.close()
        self.assertRaises(ValueError, f.fileno)

    def test_fileno_returns_int(self):
        with _io.FileIO(_getfd(), mode="r") as f:
            self.assertIsInstance(f.fileno(), int)

    def test_isatty_with_closed_file_raises_value_error(self):
        f = _io.FileIO(_getfd(), mode="r")
        f.close()
        self.assertRaises(ValueError, f.isatty)

    def test_isatty_with_non_tty_returns_false(self):
        with _io.FileIO(_getfd(), mode="r") as f:
            self.assertFalse(f.isatty())

    def test_mode_with_created_and_readable_returns_xb_plus(self):
        with _io.FileIO(_getfd(), mode="x+") as f:
            self.assertEqual(f.mode, "xb+")

    def test_mode_with_created_and_non_readable_returns_xb(self):
        with _io.FileIO(_getfd(), mode="x") as f:
            self.assertEqual(f.mode, "xb")

    # Testing appendable/readable is really tricky.

    def test_mode_with_readable_and_writable_returns_rb_plus(self):
        with _io.FileIO(_getfd(), mode="r+") as f:
            self.assertEqual(f.mode, "rb+")

    def test_mode_with_readable_and_non_writable_returns_rb(self):
        with _io.FileIO(_getfd(), mode="r") as f:
            self.assertEqual(f.mode, "rb")

    def test_mode_with_writable_returns_wb(self):
        with _io.FileIO(_getfd(), mode="w") as f:
            self.assertEqual(f.mode, "wb")

    def test_read_on_closed_file_raises_value_error(self):
        f = _io.FileIO(_getfd(), mode="r")
        f.close()
        self.assertRaises(ValueError, f.read)

    def test_read_on_non_readable_file_raises_unsupported_operation(self):
        with _io.FileIO(_getfd(), mode="w") as f:
            self.assertRaises(_io.UnsupportedOperation, f.read)

    def test_read_returns_bytes(self):
        with _io.FileIO(_getfd(), mode="r") as f:
            self.assertIsInstance(f.read(), bytes)

    def test_readable_with_closed_file_raises_value_error(self):
        f = _io.FileIO(_getfd(), mode="r")
        f.close()
        self.assertRaises(ValueError, f.readable)

    def test_readable_with_readable_file_returns_true(self):
        with _io.FileIO(_getfd(), mode="r") as f:
            self.assertTrue(f.readable())

    def test_readable_with_non_readable_file_returns_false(self):
        with _io.FileIO(_getfd(), mode="w") as f:
            self.assertFalse(f.readable())

    def test_readall_on_closed_file_raises_value_error(self):
        f = _io.FileIO(_getfd(), mode="r")
        f.close()
        self.assertRaises(ValueError, f.readall)

    def test_readall_on_non_readable_file_returns_empty_bytes(self):
        with _io.FileIO(_getfd(), mode="w") as f:
            self.assertEqual(f.readall(), b"")

    def test_readall_returns_bytes(self):
        with _io.FileIO(_getfd(), mode="r") as f:
            self.assertIsInstance(f.readall(), bytes)

    # TODO(T53182806): Test FileIO.readinto once memoryview.__setitem__ is done

    def test_seek_with_float_raises_type_error(self):
        with _io.FileIO(_getfd(), mode="r") as f:
            self.assertRaises(TypeError, f.seek, 3.4)

    def test_seek_with_closed_file_raises_value_error(self):
        f = _io.FileIO(_getfd(), mode="r")
        f.close()
        self.assertRaises(ValueError, f.seek, 3)

    def test_seek_with_non_readable_file_raises_os_error(self):
        with _io.FileIO(_getfd(), mode="w") as f:
            self.assertRaises(OSError, f.seek, 3)

    def test_seekable_with_closed_file_raises_value_error(self):
        f = _io.FileIO(_getfd(), mode="r")
        f.close()
        self.assertRaises(ValueError, f.seekable)

    def test_seekable_does_not_call_subclass_tell(self):
        class C(_io.FileIO):
            def tell(self):
                raise UserWarning("foo")

        with C(_getfd(), mode="w") as f:
            self.assertFalse(f.seekable())

    def test_seekable_with_non_seekable_file_returns_false(self):
        with _io.FileIO(_getfd(), mode="w") as f:
            self.assertFalse(f.seekable())

    def test_tell_with_closed_file_raises_value_error(self):
        f = _io.FileIO(_getfd(), mode="r")
        f.close()
        self.assertRaises(ValueError, f.tell)

    def test_tell_with_non_readable_file_raises_os_error(self):
        with _io.FileIO(_getfd(), mode="w") as f:
            self.assertRaises(OSError, f.tell)

    def test_truncate_with_closed_file_raises_value_error(self):
        f = _io.FileIO(_getfd(), mode="w")
        f.close()
        self.assertRaises(ValueError, f.truncate)

    def test_truncate_with_non_writable_file_raises_unsupported_operation(self):
        with _io.FileIO(_getfd(), mode="r") as f:
            self.assertRaises(_io.UnsupportedOperation, f.truncate)

    def test_writable_with_closed_file_raises_value_error(self):
        f = _io.FileIO(_getfd(), mode="w")
        f.close()
        self.assertRaises(ValueError, f.writable)

    def test_writable_with_writable_file_returns_true(self):
        with _io.FileIO(_getfd(), mode="w") as f:
            self.assertTrue(f.writable())

    def test_writable_with_non_writable_file_returns_false(self):
        with _io.FileIO(_getfd(), mode="r") as f:
            self.assertFalse(f.writable())

    def test_write_with_non_buffer_raises_type_error(self):
        class C:
            pass

        c = C()
        f = _io.FileIO(_getfd(), mode="w")
        with self.assertRaises(TypeError) as context:
            f.write(c)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'C'"
        )
        f.close()

    def test_write_with_raising_dunder_buffer_raises_type_error(self):
        class C:
            def __buffer__(self):
                raise UserWarning("foo")

        f = _io.FileIO(_getfd(), mode="w")
        c = C()
        with self.assertRaises(TypeError) as context:
            f.write(c)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'C'"
        )
        f.close()

    def test_write_with_bad_dunder_buffer_raises_type_error(self):
        class C:
            def __buffer__(self):
                return "foo"

        f = _io.FileIO(_getfd(), mode="w")
        c = C()
        with self.assertRaises(TypeError) as context:
            f.write(c)
        self.assertEqual(
            str(context.exception), "a bytes-like object is required, not 'C'"
        )
        f.close()


if __name__ == "__main__":
    unittest.main()
