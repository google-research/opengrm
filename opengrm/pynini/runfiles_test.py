import os

from absl.testing import absltest
from opengrm.pynini import runfiles


class RunfilesTest(absltest.TestCase):

  def test_smoke(self):
    fst_file = runfiles.test_src_path(
        "openfst/test/testdata/determinize", "d1.fst"
    )
    self.assertTrue(os.path.exists(fst_file))
    grm_file = runfiles.test_src_path(
        "opengrm/string/testdata/str.map"
    )
    self.assertTrue(os.path.exists(grm_file))


if __name__ == "__main__":
  absltest.main()
