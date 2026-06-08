# Encoding: UTF-8
"""Tests for Latin 3rd Conjugation."""

import collections
import os

from absl import app
from absl import flags
from absl.testing import absltest
from opengrm.pynini import pynini

FLAGS = flags.FLAGS
BASE = "opengrm/pynini/examples/latin/"

TestParadigm = collections.namedtuple(
    "Paradigm",
    (
        "analyzer",
        "inflector",
        "lemmatizer",
        "tagger",
        "feature_label_rewriter",
        "feature_label_encoder",
    ),
)


class InflectionTester(absltest.TestCase):

  @classmethod
  def setUpClass(cls):
    super().setUpClass()

  # Helper function for inflection tests
  def inflect(self, inp):
    return (
        pynini.escape(inp)
        @ self.paradigm.feature_label_encoder
        @ self.paradigm.inflector
        @ self.paradigm.feature_label_rewriter
    )


class TestFirstConjugation(InflectionTester):

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    far = pynini.Far(os.path.join(FLAGS.test_srcdir, BASE + "latin_verbs.far"))
    cls.paradigm = TestParadigm(
        analyzer=far["FirstConjugationAnalyzer"],
        inflector=far["FirstConjugationInflector"],
        lemmatizer=far["FirstConjugationLemmatizer"],
        tagger=far["FirstConjugationTagger"],
        feature_label_rewriter=far["FirstFeatureLabelRewriter"],
        feature_label_encoder=far["FirstFeatureLabelEncoder"],
    )

  def testAnalyzer(self):
    form = (
        "laudō" @ self.paradigm.analyzer @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "laud+ō[asp=imperf][case=n/a][gen=n/a][mood=ind]"
            "[num=sg][per=1st][tense=pres][voice=act]"
        ],
    )
    form = (
        "laudāvērunt"
        @ self.paradigm.analyzer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "laudāv+ērunt[asp=perf][case=n/a][gen=n/a][mood=ind]"
            "[num=pl][per=3rd][tense=pres][voice=act]"
        ],
    )

  def testInflector(self):
    form = self.inflect(
        "laudō[asp=perf][case=nom][gen=neu][mood=n/a]"
        "[num=sg][per=n/a][tense=pres][voice=pass]"
    )
    self.assertSameElements(form.paths().ostrings(), ["laudātum"])
    form = self.inflect(
        "laudō[asp=perf][case=gen][gen=neu][mood=n/a]"
        "[num=pl][per=n/a][tense=pres][voice=pass]"
    )
    self.assertSameElements(form.paths().ostrings(), ["laudātōrum"])


class TestSecondConjugation(InflectionTester):

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    far = pynini.Far(os.path.join(FLAGS.test_srcdir, BASE + "latin_verbs.far"))
    cls.paradigm = TestParadigm(
        analyzer=far["SecondConjugationAnalyzer"],
        inflector=far["SecondConjugationInflector"],
        lemmatizer=far["SecondConjugationLemmatizer"],
        tagger=far["SecondConjugationTagger"],
        feature_label_rewriter=far["SecondFeatureLabelRewriter"],
        feature_label_encoder=far["FirstFeatureLabelEncoder"],
    )

  def testAnalyzer(self):
    form = (
        "moneō" @ self.paradigm.analyzer @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "mon+eō[asp=imperf][case=n/a][gen=n/a][mood=ind]"
            "[num=sg][per=1st][tense=pres][voice=act]"
        ],
    )
    form = (
        "monuērunt"
        @ self.paradigm.analyzer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "monu+ērunt[asp=perf][case=n/a][gen=n/a][mood=ind]"
            "[num=pl][per=3rd][tense=pres][voice=act]"
        ],
    )
    form = (
        "monitum"
        @ self.paradigm.analyzer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            (
                "monit+um[asp=perf][case=acc][gen=mas][mood=n/a][num=sg]"
                "[per=n/a][tense=pres][voice=pass]"
            ),
            (
                "monit+um[asp=perf][case=acc][gen=neu][mood=n/a][num=sg]"
                "[per=n/a][tense=pres][voice=pass]"
            ),
            (
                "monit+um[asp=perf][case=voc][gen=neu][mood=n/a][num=sg]"
                "[per=n/a][tense=pres][voice=pass]"
            ),
            (
                "monit+um[asp=perf][case=nom][gen=neu][mood=n/a][num=sg]"
                "[per=n/a][tense=pres][voice=pass]"
            ),
        ],
    )

  def testLemmatizer(self):
    form = (
        "moneō"
        @ self.paradigm.lemmatizer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "moneō[asp=imperf][case=n/a][gen=n/a][mood=ind]"
            "[num=sg][per=1st][tense=pres][voice=act]"
        ],
    )
    form = (
        "monuērunt"
        @ self.paradigm.lemmatizer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "moneō[asp=perf][case=n/a][gen=n/a][mood=ind]"
            "[num=pl][per=3rd][tense=pres][voice=act]"
        ],
    )
    form = (
        "monitum"
        @ self.paradigm.lemmatizer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            (
                "moneō[asp=perf][case=acc][gen=mas][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
            (
                "moneō[asp=perf][case=acc][gen=neu][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
            (
                "moneō[asp=perf][case=nom][gen=neu][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
            (
                "moneō[asp=perf][case=voc][gen=neu][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
        ],
    )

  def testInflector(self):
    form = self.inflect(
        "correspondeō[asp=perf][case=nom][gen=neu][mood=n/a]"
        "[num=sg][per=n/a][tense=pres][voice=pass]"
    )
    self.assertSameElements(form.paths().ostrings(), ["correspōnsum"])


class TestThirdConjugation(InflectionTester):

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    far = pynini.Far(os.path.join(FLAGS.test_srcdir, BASE + "latin_verbs.far"))
    cls.paradigm = TestParadigm(
        analyzer=far["ThirdConjugationAnalyzer"],
        inflector=far["ThirdConjugationInflector"],
        lemmatizer=far["ThirdConjugationLemmatizer"],
        tagger=far["ThirdConjugationTagger"],
        feature_label_rewriter=far["ThirdFeatureLabelRewriter"],
        feature_label_encoder=far["ThirdFeatureLabelEncoder"],
    )

  def testAnalyzer(self):
    form = (
        "ferō" @ self.paradigm.analyzer @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "fer+ō[asp=imperf][case=n/a][gen=n/a][mood=ind]"
            "[num=sg][per=1st][tense=pres][voice=act]"
        ],
    )
    form = (
        "tulērunt"
        @ self.paradigm.analyzer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "tul+ērunt[asp=perf][case=n/a][gen=n/a][mood=ind]"
            "[num=pl][per=3rd][tense=pres][voice=act]"
        ],
    )
    form = (
        "lātum" @ self.paradigm.analyzer @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            (
                "lāt+um[asp=perf][case=acc][gen=mas][mood=n/a][num=sg]"
                "[per=n/a][tense=pres][voice=pass]"
            ),
            (
                "lāt+um[asp=perf][case=acc][gen=neu][mood=n/a][num=sg]"
                "[per=n/a][tense=pres][voice=pass]"
            ),
            (
                "lāt+um[asp=perf][case=voc][gen=neu][mood=n/a][num=sg]"
                "[per=n/a][tense=pres][voice=pass]"
            ),
            (
                "lāt+um[asp=perf][case=nom][gen=neu][mood=n/a][num=sg]"
                "[per=n/a][tense=pres][voice=pass]"
            ),
        ],
    )
    form = (
        "pepulī" @ self.paradigm.analyzer @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "pepul+ī[asp=perf][case=n/a][gen=n/a][mood=ind]"
            "[num=sg][per=1st][tense=pres][voice=act]"
        ],
    )

  def testLemmatizer(self):
    form = (
        "ferō" @ self.paradigm.lemmatizer @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "ferō[asp=imperf][case=n/a][gen=n/a][mood=ind]"
            "[num=sg][per=1st][tense=pres][voice=act]"
        ],
    )
    form = (
        "tulērunt"
        @ self.paradigm.lemmatizer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "ferō[asp=perf][case=n/a][gen=n/a][mood=ind]"
            "[num=pl][per=3rd][tense=pres][voice=act]"
        ],
    )
    form = (
        "lātum"
        @ self.paradigm.lemmatizer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            (
                "ferō[asp=perf][case=acc][gen=mas][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
            (
                "ferō[asp=perf][case=acc][gen=neu][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
            (
                "ferō[asp=perf][case=nom][gen=neu][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
            (
                "ferō[asp=perf][case=voc][gen=neu][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
        ],
    )
    form = (
        "pepulī"
        @ self.paradigm.lemmatizer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            "pellō[asp=perf][case=n/a][gen=n/a][mood=ind]"
            "[num=sg][per=1st][tense=pres][voice=act]"
        ],
    )
    form = (
        "quiētum"
        @ self.paradigm.lemmatizer
        @ self.paradigm.feature_label_rewriter
    )
    self.assertSameElements(
        form.paths().ostrings(),
        [
            (
                "quiēscō[asp=perf][case=acc][gen=mas][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
            (
                "quiēscō[asp=perf][case=acc][gen=neu][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
            (
                "quiēscō[asp=perf][case=nom][gen=neu][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
            (
                "quiēscō[asp=perf][case=voc][gen=neu][mood=n/a][num=sg][per=n/a]"
                "[tense=pres][voice=pass]"
            ),
        ],
    )

  def testInflector(self):
    form = self.inflect(
        "quiēscō[asp=perf][case=nom][gen=neu][mood=n/a]"
        "[num=sg][per=n/a][tense=pres][voice=pass]"
    )
    self.assertSameElements(form.paths().ostrings(), ["quiētum"])
    form = self.inflect(
        "vomō[asp=perf][case=nom][gen=neu][mood=n/a]"
        "[num=sg][per=n/a][tense=n/a][voice=act]"
    )
    self.assertEmpty(list(form.paths().ostrings()))
    form = self.inflect(
        "vomō[asp=perf][case=n/a][gen=n/a][mood=ind]"
        "[num=sg][per=1st][tense=past][voice=act]"
    )
    self.assertSameElements(form.paths().ostrings(), ["vomueram"])
    form = self.inflect(
        "ferō[asp=perf][case=gen][gen=neu][mood=n/a]"
        "[num=pl][per=n/a][tense=pres][voice=pass]"
    )
    self.assertSameElements(form.paths().ostrings(), ["lātōrum"])


def main(unused_argv):
  absltest.main()


if __name__ == "__main__":
  app.run(main)
