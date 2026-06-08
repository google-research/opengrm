# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.
"""Tests of Spanish g2p rules."""

from absl.testing import absltest
from opengrm.pynini.examples import g2p


class SpanishG2PTest(absltest.TestCase):

  def assertPron(self, grapheme: str, phoneme: str):
    self.assertEqual(g2p.g2p(grapheme), phoneme)

  def testG2P(self):
    self.assertPron("anarquista", "anarkista")
    self.assertPron("cantar", "kantar")
    self.assertPron("gañir", "gaɲir")
    self.assertPron("hacer", "aser")
    self.assertPron("hijo", "ixo")
    self.assertPron("llave", "ʝabe")
    self.assertPron("pero", "peɾo")
    self.assertPron("perro", "pero")
    self.assertPron("vivir", "bibir")


if __name__ == "__main__":
  absltest.main()
