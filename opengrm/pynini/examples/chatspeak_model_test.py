# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.
"""Tests for the chatspeak model in combination with the LM."""

import os

from absl import flags
from absl.testing import absltest
from opengrm.pynini.examples import chatspeak_model

FLAGS = flags.FLAGS


class ChatspeakModelTest(absltest.TestCase):

  examples = [
      (
          "well i can t eat mufffffins in an agitated mannnnner",
          "well i can t eat muffins in an agitated manner",
      ),
      ("1432 earnst", "i love you too earnest"),
      ("it s abt tiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiime", "it s about time"),
      ("the appt is in lndn", "the appointment is in london"),
      (
          "orly ily u silly rmntc foooooooooooooooooooooolllllllls",
          "oh really i love you you silly romantic fools",
      ),
  ]

  chatspeak_model: chatspeak_model.ChatspeakModel

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    chat_lexicon_path = os.path.join(
        FLAGS.test_srcdir,
        "opengrm/pynini/examples/"
        "testdata/chatspeak_lexicon.tsv",
    )
    lm_path = os.path.join(
        FLAGS.test_srcdir,
        "opengrm/pynini/examples/testdata/earnest.fst",
    )
    cls.chatspeak_model = chatspeak_model.ChatspeakModel(
        chat_lexicon_path, lm_path
    )

  def testExpansions(self):
    for i, o in self.examples:
      self.assertEqual(self.chatspeak_model.decode(i), o)


if __name__ == "__main__":
  absltest.main()
