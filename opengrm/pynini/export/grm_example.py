"""Test pynini file using the grm."""

from opengrm.pynini import pynini
from opengrm.pynini.export import grm


def generator_main(exporter: grm.Exporter):
  exporter['FST1'] = pynini.accep('1234')
  exporter['FST2'] = pynini.accep('4321')
  exporter['FST3'] = pynini.accep('ABCD')


if __name__ == '__main__':
  grm.run(generator_main)
