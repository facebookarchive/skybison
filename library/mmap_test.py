#!/usr/bin/env python3
import mmap
import os
import tempfile
import unittest


class MmapTest(unittest.TestCase):
    def test_new_with_wrong_class_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(list, -1, 1)
        self.assertIn("is not a sub", str(context.exception))

    def test_new_with_non_int_fileno_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, "not-int", 1)
        self.assertIn("str", str(context.exception))

    def test_new_with_non_int_length_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, "not-int")
        self.assertIn("str", str(context.exception))

    def test_new_with_non_int_flags_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, flags="not-int")
        self.assertIn("str", str(context.exception))

    def test_new_with_non_int_prot_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, prot="not-int")
        self.assertIn("str", str(context.exception))

    def test_new_with_non_int_access_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, access="not-int")
        self.assertIn("str", str(context.exception))

    def test_new_with_non_int_offset_raises_type_error(self):
        with self.assertRaises(TypeError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, offset="not-int")
        self.assertIn("str", str(context.exception))

    def test_new_with_negative_len_raises_overflow_error(self):
        with self.assertRaises(OverflowError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, -1)
        self.assertEqual(
            "memory mapped length must be positive", str(context.exception)
        )

    def test_new_with_negative_offset_raises_overflow_error(self):
        with self.assertRaises(OverflowError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, offset=-1)
        self.assertEqual(
            "memory mapped offset must be positive", str(context.exception)
        )

    def test_new_that_sets_both_access_and_flags_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, flags=-1, access=1)
        self.assertEqual(
            "mmap can't specify both access and flags, prot.", str(context.exception)
        )

    def test_new_that_sets_both_access_and_prot_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, prot=-1, access=1)
        self.assertEqual(
            "mmap can't specify both access and flags, prot.", str(context.exception)
        )

    def test_new_that_sets_invalid_access_raises_value_error(self):
        with self.assertRaises(ValueError) as context:
            mmap.mmap.__new__(mmap.mmap, -1, 1, access=-1)
        self.assertEqual("mmap invalid access parameter.", str(context.exception))

    def test_anonymous_mmap_can_be_closed(self):
        m = mmap.mmap(-1, 1)
        m.close()

    def test_mmap_of_empty_file_raises_value_error(self):
        with tempfile.TemporaryDirectory() as dir_path:
            fd, path = tempfile.mkstemp(dir=dir_path)
            with self.assertRaises(ValueError) as context:
                mmap.mmap(fd, 0)
            self.assertEqual("cannot mmap an empty file", str(context.exception))
            os.close(fd)

    def test_mmap_of_file_with_bigger_offset_raises_value_error(self):
        with tempfile.TemporaryDirectory() as dir_path:
            fd, path = tempfile.mkstemp(dir=dir_path)
            os.write(fd, b"Hello")
            with self.assertRaises(ValueError) as context:
                mmap.mmap(fd, 0, offset=10)
            self.assertEqual(
                "mmap offset is greater than file size", str(context.exception)
            )
            os.close(fd)

    def test_mmap_of_file_with_bigger_length_raises_value_error(self):
        with tempfile.TemporaryDirectory() as dir_path:
            fd, path = tempfile.mkstemp(dir=dir_path)
            os.write(fd, b"Hello")
            with self.assertRaises(ValueError) as context:
                mmap.mmap(fd, 10)
            self.assertEqual(
                "mmap length is greater than file size", str(context.exception)
            )
            os.close(fd)

    def test_mmap_of_file_with_nonexistant_file_raises_os_error(self):
        with tempfile.TemporaryDirectory() as dir_path:
            fd, path = tempfile.mkstemp(dir=dir_path)
            os.close(fd)
            os.remove(path)
            with self.assertRaises(OSError) as context:
                mmap.mmap(fd, 0)
            self.assertEqual("[Errno 9] Bad file descriptor", str(context.exception))

    def test_mmap_of_file_with_directory(self):
        with tempfile.TemporaryDirectory() as dir_path:
            fd = os.open(dir_path, os.O_RDONLY)
            with self.assertRaises(OSError):
                mmap.mmap(fd, 0)
            os.close(fd)

    def test_mmap_of_file_with_zero_length_gets_file_size(self):
        with tempfile.TemporaryDirectory() as dir_path:
            fd, path = tempfile.mkstemp(dir=dir_path)
            os.write(fd, b"Hello")
            m = mmap.mmap(fd, 0)
            view = memoryview(m)
            self.assertEqual(view.nbytes, 5)
            os.close(fd)

    def test_mmap_of_file_can_write_to_file(self):
        with tempfile.TemporaryDirectory() as dir_path:
            fd, path = tempfile.mkstemp(dir=dir_path)
            os.write(fd, b"Hello")
            m = mmap.mmap(fd, 3)
            view = memoryview(m)
            self.assertEqual(view.nbytes, 3)
            view[:3] = b"foo"
            os.close(fd)
            with open(path) as f:
                result = f.read()
            self.assertEqual(result, "foolo")

    def test_mmap_of_file_with_readonly_prot_is_readonly(self):
        with tempfile.TemporaryDirectory() as dir_path:
            fd, path = tempfile.mkstemp(dir=dir_path)
            os.write(fd, b"Hello")
            m = mmap.mmap(fd, 3, prot=mmap.PROT_READ)
            view = memoryview(m)
            self.assertEqual(view.nbytes, 3)
            with self.assertRaises(TypeError) as context:
                view[:3] = b"foo"
            self.assertEqual("cannot modify read-only memory", str(context.exception))
            os.close(fd)

    def test_mmap_of_file_with_private_memory_doesnt_map_changes_to_file(self):
        with tempfile.TemporaryDirectory() as dir_path:
            fd, path = tempfile.mkstemp(dir=dir_path)
            os.write(fd, b"Hello")
            m = mmap.mmap(fd, 3, flags=mmap.MAP_PRIVATE)
            view = memoryview(m)
            self.assertEqual(view.nbytes, 3)
            view[:3] = b"foo"
            os.close(fd)
            with open(path) as f:
                result = f.read()
            self.assertEqual(result, "Hello")

    def test_prot_constants_are_all_different(self):
        self.assertNotEqual(mmap.PROT_EXEC, mmap.PROT_READ)
        self.assertNotEqual(mmap.PROT_READ, mmap.PROT_WRITE)
        self.assertNotEqual(mmap.PROT_WRITE, mmap.PROT_EXEC)


if __name__ == "__main__":
    unittest.main()
