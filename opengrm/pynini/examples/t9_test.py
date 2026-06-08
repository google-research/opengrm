# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.
"""Tests for the T9 model."""

from absl.testing import absltest
from opengrm.pynini.examples import t9


class T9Test(absltest.TestCase):

  t9: t9.T9

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    lexicon = [
        "the",
        "cool",
        "warthog",
        "escaped",
        "easily",
        "from",
        "baltimore",
        "zoo",
        "col",
    ]
    cls.t9 = t9.T9(lexicon)

  def testExample(self):
    example = "the cool warthog escaped easily from baltimore zoo"
    encoded = self.t9.encode(example)
    self.assertTrue(example in self.t9.decode(encoded).paths().ostrings())


if __name__ == "__main__":
  absltest.main()
