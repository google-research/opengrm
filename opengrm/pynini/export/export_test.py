import os

from absl import flags
from absl.testing import absltest
from opengrm.pynini import pynini
from opengrm.pynini.export import export

FLAGS = flags.FLAGS


def _read_fst_map(filename):
  with pynini.Far(filename) as far:
    stored_fsts = dict(far)
  return stored_fsts


class PyniniExporterTest(absltest.TestCase):
  _filename: str

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls._filename = os.path.join(FLAGS.test_tmpdir, 'test.far')

  def testEmptyExporter(self):
    """Export an empty grammar."""
    exporter = export.Exporter(self._filename)
    exporter.close()
    self.assertTrue(os.path.isfile(self._filename))

  def testFilledExporter(self):
    """Export two FSTs."""
    exporter = export.Exporter(self._filename)
    exporter['FST1'] = pynini.accep('1234')
    exporter['FST2'] = pynini.accep('4321')
    exporter.close()
    stored_fsts = _read_fst_map(self._filename)
    self.assertLen(stored_fsts, 2)
    self.assertTrue(stored_fsts['FST1'])
    self.assertTrue(stored_fsts['FST2'])

  def testFilledExporterWithFarTypes(self):
    """Export two FSTs different far types."""
    for far_type in ['default', 'sstable', 'sttable', 'stlist']:
      exporter = export.Exporter(self._filename, far_type=far_type)
      exporter['FSTA'] = pynini.accep('1234')
      exporter['FSTB'] = pynini.accep('4321')
      exporter.close()
      stored_fsts = _read_fst_map(self._filename)
      self.assertLen(stored_fsts, 2)
      self.assertTrue(stored_fsts['FSTA'])
      self.assertTrue(stored_fsts['FSTB'])


if __name__ == '__main__':
  absltest.main()
