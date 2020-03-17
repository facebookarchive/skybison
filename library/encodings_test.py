#!/usr/bin/env python3
import codecs
import encodings  # noqa: F401
import io
import unittest


class EncodingsModuleTest(unittest.TestCase):
    def test_raw_unicode_escape_getencoder_returns_encoder(self):
        inc = codecs.getencoder("raw-unicode-escape")
        self.assertEqual(inc, codecs.raw_unicode_escape_encode)

    def test_unicode_escape_getencoder_returns_encoder(self):
        inc = codecs.getencoder("unicode-escape")
        self.assertEqual(inc, codecs.unicode_escape_encode)

    def test_utf_16_be_getencoder_returns_encoder(self):
        inc = codecs.getencoder("utf-16-be")
        self.assertEqual(inc, codecs.utf_16_be_encode)

    def test_utf_16_getencoder_returns_encoder(self):
        inc = codecs.getencoder("utf-16")
        self.assertEqual(inc, codecs.utf_16_encode)

    def test_utf_16_le_getencoder_returns_encoder(self):
        inc = codecs.getencoder("utf-16-le")
        self.assertEqual(inc, codecs.utf_16_le_encode)

    def test_utf_8_incremental_decoder_returns_str(self):
        inc = codecs.getincrementaldecoder("utf-8")()
        self.assertEqual(inc.decode(b"test"), "test")

    def test_latin1_streamwriter_streamreader(self):
        stream = io.BytesIO()
        codec = codecs.lookup("latin1")
        writer = codec.streamwriter(stream)
        writer.write("foo")
        stream.seek(0)
        reader = codec.streamreader(stream)
        string = reader.readline()
        self.assertEqual(string, "foo")

    def test_utf8_streamwriter_streamreader(self):
        stream = io.BytesIO()
        codec = codecs.lookup("utf8")
        writer = codec.streamwriter(stream)
        writer.write("foo")
        stream.seek(0)
        reader = codec.streamreader(stream)
        string = reader.readline()
        self.assertEqual(string, "foo")

    def test_ascii_streamwriter_streamreader(self):
        stream = io.BytesIO()
        codec = codecs.lookup("ascii")
        writer = codec.streamwriter(stream)
        writer.write("foo")
        stream.seek(0)
        reader = codec.streamreader(stream)
        string = reader.readline()
        self.assertEqual(string, "foo")


if __name__ == "__main__":
    unittest.main()
