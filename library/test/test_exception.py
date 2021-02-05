import unittest


class TracebackTest(unittest.TestCase):
    def test_reraise_exception_appends_traceback(self):
        def raise_exc():
            raise ValueError("Hello")

        def reraise():
            try:
                raise_exc()
            except ValueError as e:
                raise e

        def catch():
            try:
                reraise()
            except Exception as e:
                tb = e.__traceback__
                frames = []
                while tb is not None:
                    frames.append(tb.tb_frame)
                    tb = tb.tb_next
            return frames

        frames = catch()
        names = [frame.f_code.co_name for frame in frames]
        self.assertIn("catch", names)
        self.assertIn("reraise", names)
        self.assertIn("raise_exc", names)


if __name__ == "__main__":
    unittest.main()
