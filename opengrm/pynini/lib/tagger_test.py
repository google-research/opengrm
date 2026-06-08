# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.
"""Tests of the tagger automaton class."""

import string

from absl.testing import absltest
from opengrm.pynini import pynini
from opengrm.pynini.lib import rewrite
from opengrm.pynini.lib import tagger


class TaggerTest(absltest.TestCase):

  tagger: tagger.Tagger

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cheese = pynini.string_map([
        "tilsit",
        "caerphilly",
        "stilton",
        "gruyere",
        "emmental",
        "liptauer",
        "lancashire",
        "cheshire",
        "brie",
        "roquefort",
        "savoyard",
        "boursin",
        "camembert",
        "gouda",
        "edam",
        "caithness",
        "wensleydale",
        "gorgonzola",
        "parmesan",
        "mozzarella",
        "fynbo",
        "cheddar",
        "ilchester",
        "limburger",
    ]).optimize()
    sigma_star = pynini.union(*string.ascii_lowercase + "<>/ ").closure()
    cls.tagger = tagger.Tagger("cheese", cheese, sigma_star)

  def testMatch(self):
    request = "well how about cheddar"
    self.assertEqual(
        self.tagger.tag(request), "well how about <cheese>cheddar</cheese>"
    )

  def testMatches(self):
    request = "do you have tilsit caerphilly gruyere emmental or edam"
    self.assertEqual(
        self.tagger.tag(request),
        "do you have <cheese>tilsit</cheese> "
        "<cheese>caerphilly</cheese> "
        "<cheese>gruyere</cheese> "
        "<cheese>emmental</cheese> or "
        "<cheese>edam</cheese>",
    )

  def testOutofAlphabetQueryRaisesException(self):
    request = "Gruyère"
    with self.assertRaises(rewrite.Error):
      self.tagger.tag(request)


if __name__ == "__main__":
  absltest.main()
