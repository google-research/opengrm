# Encoding: UTF-8
"""FAR compiler for Latin verbs."""

import re

from absl import app
from absl import flags
from opengrm.pynini import pynini
from opengrm.pynini.export import export
from opengrm.pynini.lib import byte
from opengrm.pynini.lib import features
from opengrm.pynini.lib import paradigms



BASE = "opengrm/pynini/examples/latin/"

# TODO:
#
# 1) Add in irregular perfect 1st conjugation verbs
#
# 2) Finish 2nd conjugation

_FIRST_CONJUGATION = flags.DEFINE_string(
    "first_conjugation",
    BASE + "first_conjugation.txt",
    "Location of first conjugation inflection",
)
_FIRST_VERB_STEMS = flags.DEFINE_string(
    "first_verb_stems",
    BASE + "first_verb_stems.txt",
    "Location of first conjugation verb stems",
)
_SECOND_CONJUGATION = flags.DEFINE_string(
    "second_conjugation",
    BASE + "second_conjugation.txt",
    "Location of second conjugation inflection",
)
_SECOND_VERB_STEMS = flags.DEFINE_string(
    "second_verb_stems",
    BASE + "second_verb_stems.txt",
    "Location of second conjugation verb stems",
)
_THIRD_CONJUGATION = flags.DEFINE_string(
    "third_conjugation",
    BASE + "third_conjugation.txt",
    "Location of third conjugation inflection",
)
_THIRD_VERB_STEMS = flags.DEFINE_string(
    "third_verb_stems",
    BASE + "third_verb_stems.txt",
    "Location of third conjugation verb stems",
)
_FAR = flags.DEFINE_string("far", BASE + "latin_verbs.far", "Output FAR file")

PERSON = features.Feature("per", "1st", "2nd", "3rd", "n/a")
NUMBER = features.Feature("num", "sg", "pl", "n/a")
TENSE = features.Feature("tense", "pres", "fut", "past", "n/a")
ASPECT = features.Feature("asp", "perf", "imperf", "n/a")
MOOD = features.Feature("mood", "ind", "subj", "imper", "n/a")
VOICE = features.Feature("voice", "act", "pass", "n/a")
CASE = features.Feature("case", "nom", "gen", "dat", "acc", "abl", "voc", "n/a")
GENDER = features.Feature("gen", "mas", "fem", "neu", "n/a")
VERB = features.Category(
    PERSON, NUMBER, TENSE, ASPECT, MOOD, VOICE, CASE, GENDER
)
LEMMA = features.FeatureVector(
    VERB,
    "per=1st",
    "num=sg",
    "tense=pres",
    "asp=imperf",
    "mood=ind",
    "voice=act",
    "case=n/a",
    "gen=n/a",
)
SIGMA_STAR = pynini.closure(byte.BYTE).optimize()
# "—" represents a missing stem
MISSING = (SIGMA_STAR - (SIGMA_STAR + "—" + SIGMA_STAR)).optimize()


class Verb:
  """Defines a verb class in terms of a paradigm and set of stems."""

  def __init__(self, name, verb_stems, paradigm, second_third_suffixes=None):
    self._final_o = re.compile("ō$")
    if second_third_suffixes:
      assert len(second_third_suffixes) == 2
      stem_triples = self._load_simple_verb_stems(
          verb_stems, second_third_suffixes[0], second_third_suffixes[1]
      )
    else:
      stem_triples = self._load_verb_stems(verb_stems)
    affixes = self._load_paradigm(paradigm)
    slots = self._set_up_stems(stem_triples, affixes)
    self._paradigm = paradigms.Paradigm(
        category=VERB,
        name=name,
        slots=slots,
        stems=[c[0] for c in stem_triples],
        lemma_feature_vector=LEMMA,
    )

  @property
  def paradigm(self):
    return self._paradigm

  def _load_simple_verb_stems(self, path, second_suffix, third_suffix):
    """Loads simple verb stems from path.

    Args:
      path: Path to stems
      second_suffix: second stem suffix
      third_suffix: third stem suffix

    Returns:
      A list of triples (first, second, third) stems for each verb.
    """
    stems = []
    with open(path, mode='rb') as stream:
      for line in stream:
        line = line.decode("utf8").rstrip()
        if line.startswith("#"):
          continue
        first = line.strip()
        first = self._final_o.sub("", first)
        second = first + second_suffix
        third = first + third_suffix
        stems.append((first, second, third))
    return stems

  # TODO: clean all this up & make the stems in the files actual stems.
  def _load_verb_stems(self, path):
    """Loads verb stems from path.

    Args:
      path: Path to stems

    Returns:
      A list of triples (first, second, third) stems for each verb.
    """
    stems = []
    with open(path, mode='rb') as stream:
      for line in stream:
        line = line.decode("utf8").rstrip()
        if line.startswith("#"):
          continue
        try:
          first, second, third = line.split()[:3]
        except ValueError:
          continue
        first = first.replace("eō,", "")  # 2nd conj
        first = first.replace("ō,", "")
        second = second.replace("ī,", "")
        third = third.replace("-", "")
        stems.append((first, second, third))
    return stems

  def _load_paradigm(self, path):
    """Loads paradigm from path.

    Args:
      path: Path to paradigm

    Returns:
      A list of triples (ending, stem, features) for each slot in paradigm.
    """
    affixes = []
    with open(path, mode='rb') as stream:
      for line in stream:
        line = line.decode("utf8").rstrip()
        if line.startswith("#"):
          continue
        try:
          ending, stem, *feats = line.split(",")
          affixes.append((ending, stem, feats))
        except ValueError:
          continue
    return affixes

  def _set_up_stems(self, stem_triples, affixes):
    """Sets up stems for verbs.

    Args:
      stem_triples: a list of stem triples from _load_stems
      affixes: a list of affix triples from _load_paradigm

    Returns:
      Stem forms for verbs as list of pairs of a stem base as an FST,
      and a FeatureVector.
    """
    stem_table = {}
    stem_table["first_stem"] = pynini.union(*[p[0] for p in stem_triples])
    second_stem = pynini.union(
        *[pynini.cross(p[0], p[1]) for p in stem_triples]
    )
    second_stem @= MISSING
    stem_table["second_stem"] = second_stem
    third_stem = pynini.union(*[pynini.cross(p[0], p[2]) for p in stem_triples])
    third_stem @= MISSING
    stem_table["third_stem"] = third_stem
    slots = []
    for suf, stem, feats in affixes:
      slots.append([
          paradigms.suffix(suf, stem_table[stem]),
          features.FeatureVector(VERB, *feats),
      ])
    return slots


def construct_conjugation(
    name, stems_file, paradigm_file, table, second_third_suffixes=None
):
  """Helper function to construct a named conjugation.

  Args:
    name: Name for this conjugation, such as "Third"
    stems_file: Path to file containing verb stems
    paradigm_file: Path to file containing paradigm
    table: Dictionary to store the four Paradigm FSTs
    second_third_suffixes: Optional second and third stem suffixes to pass to
      Verb constructor.
  """
  verb = Verb(
      name=f"{name} Conjugation",
      verb_stems=stems_file,
      paradigm=paradigm_file,
      second_third_suffixes=second_third_suffixes,
  )
  paradigm = verb.paradigm
  table[f"{name}ConjugationAnalyzer"] = paradigm.analyzer
  table[f"{name}ConjugationTagger"] = paradigm.tagger
  table[f"{name}ConjugationLemmatizer"] = paradigm.lemmatizer
  table[f"{name}ConjugationInflector"] = paradigm.inflector
  table[f"{name}FeatureLabelRewriter"] = paradigm.feature_label_rewriter
  table[f"{name}FeatureLabelEncoder"] = paradigm.feature_label_encoder


def main(unused_argv):
  exporter = export.Exporter(_FAR.value)
  construct_conjugation(
      "First",
      _FIRST_VERB_STEMS.value,
      _FIRST_CONJUGATION.value,
      exporter,
      second_third_suffixes=("āv", "āt"),
  )
  construct_conjugation(
      "Second", _SECOND_VERB_STEMS.value, _SECOND_CONJUGATION.value, exporter
  )
  construct_conjugation(
      "Third", _THIRD_VERB_STEMS.value, _THIRD_CONJUGATION.value, exporter
  )
  exporter.close()


if __name__ == "__main__":
  app.run(main)
