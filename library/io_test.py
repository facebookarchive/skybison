#!/usr/bin/env python3
import unittest

import _io


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


if __name__ == "__main__":
    unittest.main()
