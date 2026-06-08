# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.
"""Combination of Chatspeak model with LM."""

from opengrm.pynini import pynini
from opengrm.pynini.examples import chatspeak
from opengrm.pynini.lib import pynutil
from opengrm.pynini.lib import rewrite


# TODO: The chatspeak lexicon is ill matched to the tokenizations used in
# the Earnest data, since things like "don't" appear in the latter as "don
# t". Fixing this is left as an exercise to the reader.
class ChatspeakModel:
  """Combination of the Chatspeak model with an LM."""

  def __init__(self, chat_lexicon_path: str, lm_path: str) -> None:
    self._lm = pynini.Fst.read(lm_path)
    assert self._lm.output_symbols(), "No LM output symbol table found"
    self._lm_syms = self._lm.output_symbols()
    lexicon = [w for (l, w) in self._lm_syms if l > 0]
    lexicon_fsa = pynini.string_map(lexicon).optimize()
    self._deduplicator = chatspeak.Deduplicator(lexicon_fsa)
    self._deabbreviator = chatspeak.Deabbreviator(lexicon_fsa)
    self._regexps = chatspeak.Regexps()
    self._lexicon = chatspeak.Lexicon(chat_lexicon_path)
    lm_mapper = pynini.string_map(
        lexicon, input_token_type="byte", output_token_type=self._lm_syms
    )
    self._bytes_to_lm_mapper = pynutil.join(lm_mapper, " ").optimize()
    self._lm_to_bytes_mapper = pynini.invert(self._bytes_to_lm_mapper)

  def token_lattice(self, token: str) -> pynini.Fst:
    """Constructs a "link" of the lattice for a given token.

    Args:
      token: An input token.

    Returns:
      An FST "link".
    """
    return pynini.union(
        self._deduplicator.expand(token),
        self._deabbreviator.expand(token),
        self._regexps.expand(token),
        self._lexicon.expand(token),
    ).optimize()

  def decode(self, sentence: str) -> str:
    """Decodes sentence with the Chatspeak model + LM.

    Args:
      sentence: an input sentence.

    Returns:
      String representing the normalized sentence.
    """
    it = iter(sentence.split())
    token = next(it)
    lattice = self.token_lattice(token)
    for token in it:
      lattice.concat(" ")
      lattice.concat(self.token_lattice(token))
    lattice.optimize()
    # Scores with LM.
    lattice = rewrite.rewrite_lattice(lattice, self._bytes_to_lm_mapper)
    lattice = rewrite.rewrite_lattice(lattice, self._lm)
    lattice = rewrite.rewrite_lattice(lattice, self._lm_to_bytes_mapper)
    return rewrite.lattice_to_top_string(lattice)
