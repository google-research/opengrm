# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.
"""Tests of (American) English number names."""

from absl.testing import absltest
from opengrm.pynini.examples import numbers


class EnNumbersTest(absltest.TestCase):

  def test_rewrites(self):
    pairs = [
        (324, "three hundred twenty four"),
        (314, "three hundred fourteen"),  # Forcing newline.
        (3014, "three thousand fourteen"),
        (30014, "thirty thousand fourteen"),
        (300014, "three hundred thousand fourteen"),
        (3000014, "three million fourteen"),
    ]
    for number, name in pairs:
      prediction = numbers.number(str(number))
      self.assertEqual(name, prediction, f"{name} != {prediction}")


if __name__ == "__main__":
  absltest.main()
