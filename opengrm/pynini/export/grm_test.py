import os

from absl import flags
from absl.testing import absltest
from absl.testing import flagsaver
from opengrm.pynini import pynini
from opengrm.pynini.export import grm

FLAGS = flags.FLAGS


def generator_method(exporter: grm.Exporter):
  exporter['FST1'] = pynini.accep('1234')
  exporter['FST2'] = pynini.accep('4321')


class PyniniGrmTest(absltest.TestCase):

  @flagsaver.flagsaver()
  def testFilledExporter(self):
    """Export two FSTs and check that they are stored in the file."""
    filename = os.path.join(FLAGS.test_tmpdir, 'test.far')
    FLAGS.output = filename
    with self.assertRaises(SystemExit):
      grm.run(generator_method)
    with pynini.Far(filename, 'r') as far:
      stored_fsts = dict(far)
    self.assertLen(stored_fsts, 2)
    self.assertTrue(stored_fsts['FST1'])
    self.assertTrue(stored_fsts['FST2'])


if __name__ == '__main__':
  absltest.main()
