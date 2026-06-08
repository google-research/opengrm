"""Test pynini file using the multi_grm."""

from opengrm.pynini import pynini
from opengrm.pynini.export import multi_grm


def generator_main(exporter_map: multi_grm.ExporterMapping):
  exporter_map['a']['FST1'] = pynini.accep('1234')
  exporter_map['a']['FST2'] = pynini.accep('4321')
  exporter_map['b']['FST3'] = pynini.accep('ABCD')


if __name__ == '__main__':
  multi_grm.run(generator_main)
