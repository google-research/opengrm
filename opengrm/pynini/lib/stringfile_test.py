"""Tests for the automatic stringfile creation utility."""

import io

from absl import logging
from absl.testing import absltest
from opengrm.pynini import pynini
from opengrm.pynini.lib import stringfile


class StringfileEscapeTest(absltest.TestCase):

  def test_empty_string(self):
    self.assertEqual(stringfile.escape(""), "")

  def test_non_special_chars(self):
    self.assertEqual(stringfile.escape(r"abc"), r"abc")

  def test_brackets(self):
    self.assertEqual(stringfile.escape(r"[abc]"), r"\[abc\]")

  def test_backslash(self):
    self.assertEqual(stringfile.escape("a\\slash\\"), "a\\\\slash\\\\")

  def test_hashtag(self):
    self.assertEqual(stringfile.escape(r"#awesome"), r"\#awesome")

  def test_escaped_backslash_before_hashtag(self):
    self.assertEqual(stringfile.escape("\\\\#"), "\\\\\\\\\\#")

  def test_unescaped_backslash_before_hashtag(self):
    self.assertEqual(stringfile.escape("\\#"), "\\\\\\#")


class StringfileWriteTest(absltest.TestCase):

  def test_write_line(self):
    with io.StringIO() as f:
      stringfile.writeline(f, ["#awesome", "#lo mejor", "4"])
      f.seek(0)
      lines = f.readlines()
      self.assertLen(lines, 1)
      self.assertEqual(lines[0], "\\#awesome\t\\#lo mejor\t4\n")

  def test_too_few_fields(self):
    with io.StringIO() as f:
      with self.assertLogs(level=logging.ERROR):
        stringfile.writeline(f, [])
      f.seek(0)
      lines = f.readlines()
      self.assertLen(lines, 1)
      self.assertEqual(lines[0], "\n")

  def test_too_many_fields(self):
    with io.StringIO() as f:
      with self.assertLogs(level=logging.ERROR):
        stringfile.writeline(f, ["#awesome", "#lo mejor", "4", "too many"])
      f.seek(0)
      lines = f.readlines()
      self.assertLen(lines, 1)
      self.assertEqual(lines[0], "\\#awesome\t\\#lo mejor\t4\ttoo many\n")

  def test_write_two_column_line(self):
    with io.StringIO() as f:
      stringfile.writeline(f, ["#awesome", "#lo mejor"])
      f.seek(0)
      lines = f.readlines()
      self.assertLen(lines, 1)
      self.assertEqual(lines[0], "\\#awesome\t\\#lo mejor\n")

  def test_write_one_column_line(self):
    with io.StringIO() as f:
      stringfile.writeline(f, ["#awesome"])
      f.seek(0)
      lines = f.readlines()
      self.assertLen(lines, 1)
      self.assertEqual(lines[0], "\\#awesome\n")

  def test_write_lines(self):
    with io.StringIO() as f:
      stringfile.writelines(
          f,
          [
              ("#awesome", "#lo mejor", "4"),
              ("[bracketed]", "סוגריים מרובעים", "5"),
          ],
      )
      f.seek(0)
      lines = f.readlines()
      self.assertLen(lines, 2)
      self.assertEqual(lines[0], "\\#awesome\t\\#lo mejor\t4\n")
      self.assertEqual(lines[1], "\\[bracketed\\]\tסוגריים מרובעים\t5\n")

  def test_roundtrip(self):
    lines = [
        ("#awesome", "#lo mejor", "4"),
        ("[bracketed]", "סוגריים מרובעים", "5"),
        ("\\\\#", "words", "0"),
    ]
    temp = self.create_tempfile()
    with temp.open_text("wt") as f:
      stringfile.writelines(f, lines)

    fst = pynini.string_file(temp.full_path)
    self.assertEqual(
        lines,
        [
            (istring, ostring, str(weight))
            for istring, ostring, weight in fst.paths().items()
        ],
    )


if __name__ == "__main__":
  absltest.main()
