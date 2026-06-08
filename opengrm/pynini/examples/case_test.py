# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.
"""Tests of Finnish case suffixes."""

from absl.testing import absltest
from opengrm.pynini.examples import case


class FinnishCaseTest(absltest.TestCase):

  def assertCases(
      self,
      nominative: str,
      abessive: str,
      ablative: str,
      adessive: str,
      allative: str,
      elative: str,
      essive: str,
      inessive: str,
  ):
    self.assertEqual(case.abessive(nominative), abessive)
    self.assertEqual(case.ablative(nominative), ablative)
    self.assertEqual(case.adessive(nominative), adessive)
    self.assertEqual(case.allative(nominative), allative)
    self.assertEqual(case.elative(nominative), elative)
    self.assertEqual(case.essive(nominative), essive)
    self.assertEqual(case.inessive(nominative), inessive)

  def testBackHarmonicStems(self):
    # Back-back: 'house'; https://en.wiktionary.org/wiki/talo#Finnish
    self.assertCases(
        "talo",
        "talotta",
        "talolta",
        "talolla",
        "talolle",
        "talosta",
        "talona",
        "talossa",
    )
    # Front-back: 'cyst'; https://en.wiktionary.org/wiki/kysta#Finnish
    self.assertCases(
        "kysta",
        "kystatta",
        "kystalta",
        "kystalla",
        "kystalle",
        "kystasta",
        "kystana",
        "kystassa",
    )
    # Back-neutral: 'sailor'; https://en.wiktionary.org/wiki/gasti#Finnish
    self.assertCases(
        "gasti",
        "gastitta",
        "gastilta",
        "gastilla",
        "gastille",
        "gastista",
        "gastina",
        "gastissa",
    )
    # Neutral-back: 'tax': https://en.wiktionary.org/wiki/vero#Finnish
    self.assertCases(
        "vero",
        "verotta",
        "verolta",
        "verolla",
        "verolle",
        "verosta",
        "verona",
        "verossa",
    )

  def testFrontHarmonicStems(self):
    # Back-front: 'fondue'; https://en.wiktionary.org/wiki/fondyy#Finnish
    self.assertCases(
        "fondyy",
        "fondyyttä",
        "fondyyltä",
        "fondyyllä",
        "fondyylle",
        "fondyystä",
        "fondyynä",
        "fondyyssä",
    )
    # Front-front: 'spit'; https://en.wiktionary.org/wiki/sylky#Finnish
    self.assertCases(
        "sylky",
        "sylkyttä",
        "sylkyltä",
        "sylkyllä",
        "sylkylle",
        "sylkystä",
        "sylkynä",
        "sylkyssä",
    )
    # Neutral-neutral: 'pleat'; https://en.wiktionary.org/wiki/vekki#Finnish
    self.assertCases(
        "vekki",
        "vekkittä",
        "vekkiltä",
        "vekkillä",
        "vekkille",
        "vekkistä",
        "vekkinä",
        "vekkissä",
    )


if __name__ == "__main__":
  absltest.main()
