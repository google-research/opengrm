# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.
"""Tests for the chatspeak model."""

import os

from absl import flags
from absl.testing import absltest
from opengrm.pynini import pynini
from opengrm.pynini.examples import chatspeak
from opengrm.pynini.lib import rewrite

FLAGS = flags.FLAGS


class ChatspeakTest(absltest.TestCase):

  deduplicator: chatspeak.Deduplicator
  deabbreviator: chatspeak.Deabbreviator
  regexps: chatspeak.Regexps
  lexicon: chatspeak.Lexicon

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    lexicon = pynini.union(
        "the",
        "cool",
        "warthog",
        "escaped",
        "easily",
        "from",
        "baltimore",
        "zoo",
        "col",
    )
    cls.deduplicator = chatspeak.Deduplicator(lexicon)
    cls.deabbreviator = chatspeak.Deabbreviator(lexicon)
    cls.regexps = chatspeak.Regexps()
    # Set the directory to org_opengrm_pynini/tests/testdata" for Bazel testing.
    cls.lexicon = chatspeak.Lexicon(
        os.path.join(
            FLAGS.test_srcdir,
            "opengrm/pynini/examples/"
            "testdata/chatspeak_lexicon.tsv",
        )
    )

  def testDeduplicator(self):

    def expand_string(s: str) -> list[str]:
      return rewrite.lattice_to_strings(self.deduplicator.expand(s))

    self.assertSameElements(expand_string("cooooool"), ["cool", "col"])
    self.assertSameElements(
        expand_string("coooooooooooooooollllllllll"), ["cool", "col"]
    )
    self.assertSameElements(expand_string("chicken"), [])

  def testDeabbreviator(self):

    def expand_string(s: str) -> list[str]:
      return rewrite.lattice_to_strings(self.deabbreviator.expand(s))

    self.assertSameElements(expand_string("wrthg"), ["warthog"])
    self.assertSameElements(expand_string("wthg"), ["warthog"])
    self.assertSameElements(expand_string("z"), [])

  def testRegexps(self):

    def expand_string(s: str) -> list[str]:
      return rewrite.lattice_to_strings(self.regexps.expand(s))

    result = expand_string("delish")
    self.assertSameElements(result, ["delicious"])
    result = expand_string("kooooooooool")
    self.assertSameElements(result, ["cool"])
    result = expand_string("zomgggggggg")
    self.assertSameElements(result, ["oh my god"])

  def testLexicon(self):

    def expand_string(s: str) -> list[str]:
      return rewrite.lattice_to_strings(self.lexicon.expand(s))

    self.assertSameElements(expand_string("1nam"), ["one in a million"])


if __name__ == "__main__":
  absltest.main()
