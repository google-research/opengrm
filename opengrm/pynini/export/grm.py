"""Interface for generating a FAR file from a Pynini file.

Given an output FAR file (FST archive) via --output, a Pynini file produces this
FAR file by exporting a number of FSTs into this FAR file. To this end, a Pynini
file creates a generator_main file with the signature:

  def generator_main(exporter: grm.Exporter):

which can contain lines of the form

  exporter[fst_name] = fst

using the exporter argument in generator_main to export a given FST under a
corresponding name. The generator_main function must be referenced with

  if __name__ == '__main__':
    grm.run(generator_main)

and should be used in a compile_grm_py BUILD rule to build the FAR.

For an example, see grm_example.py.
"""

from collections.abc import Callable

from absl import app
from absl import flags
from opengrm.pynini.export import export

_OUTPUT = flags.DEFINE_string('output', None, 'The output FAR file.')
FLAGS = flags.FLAGS

# Expose this definition as `grm.Exporter`.
Exporter = export.Exporter


def run(generator_main: Callable[[export.Exporter], None]) -> None:
  """Executes the grm FAR export program to export exactly one FAR.

  Args:
    generator_main: The FAR generator_main function to execute. It takes a
      single argument "exporter", which is an export.Exporter object with a
      __setitem__ method used to save an FST with a named designator into a FAR.
  """

  def main(unused_argv):
    try:
      unused_argv.pop(0)
      if unused_argv:
        raise app.UsageError(
            f'Unexpected command line arguments: {unused_argv}'
        )
      exporter = export.Exporter(_OUTPUT.value)
      generator_main(exporter)
      exporter.close()
    except:
      FLAGS.stderrthreshold = 'fatal'
      raise

  flags.mark_flag_as_required('output')
  FLAGS.stderrthreshold = 'warning'
  app.run(main)
