# Copyright 2026 The OpenGrm Authors.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.
"""Tests for the chatspeak model in combination with the LM."""

from absl.testing import absltest
from opengrm.pynini import runfiles
from opengrm.pynini.examples import chatspeak_model


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
    chat_lexicon_path = runfiles.test_src_path(
        "opengrm/pynini/examples/testdata",
        "chatspeak_lexicon.tsv",
    )
    lm_path = runfiles.test_src_path(
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
