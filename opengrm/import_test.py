from absl.testing import absltest
from opengrm import pynini


class ImportTest(absltest.TestCase):

  def test_import(self):
    self.assertIsNotNone(pynini.SymbolTable)


if __name__ == "__main__":
  absltest.main()
