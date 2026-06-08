# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.
"""Tests of English plurals."""

from absl.testing import absltest
from opengrm.pynini.examples import plurals


class EnglishPluralsTest(absltest.TestCase):

  def assertPlural(self, singular: str, plural: str):
    result = plurals.plural(singular)
    self.assertEqual(result, plural)

  def testPlurals(self):
    self.assertPlural("analysis", "analyses")
    self.assertPlural("boy", "boys")
    self.assertPlural("deer", "deer")
    self.assertPlural("hamlet", "hamlets")
    self.assertPlural("house", "houses")
    self.assertPlural("lunch", "lunches")
    self.assertPlural("mouse", "mice")
    self.assertPlural("photo", "photos")
    self.assertPlural("puppy", "puppies")
    self.assertPlural("wife", "wives")


if __name__ == "__main__":
  absltest.main()
