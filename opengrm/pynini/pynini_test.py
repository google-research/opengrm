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

"""Tests for the Pynini grammar compilation module."""

from collections.abc import Iterable
import functools
import itertools
import math
import os
import pathlib
import pickle
import string

from absl.testing import absltest
from absl.testing import parameterized
from openfst import pywrapfst
from opengrm.pynini import pynini
from opengrm.pynini import runfiles


class ArcIteratorTest(absltest.TestCase):
  testdir: pathlib.Path

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = pathlib.Path(
        runfiles.test_src_path(
            "openfst/test/testdata/compile"
        )
    )

  def testArcIteratorAfterFstDeletion(self):
    f = pynini.Fst.read(self.testdir / "fst.compiled")
    size = f.num_arcs(f.start())
    aiter = f.arcs(f.start())
    del f  # Should be garbage-collected immediately.
    self.assertLen(list(aiter), size)


class ArcmapTest(absltest.TestCase):
  testdir: str
  m1: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/arc-map"
    )
    cls.m1 = pynini.Fst.read(os.path.join(cls.testdir, "m1.fst"))

  def testIdentityArcmap(self):
    self.assertEqual(pynini.arcmap(self.m1, map_type="identity"), self.m1)

  def testRmweightArcmap(self):
    m6 = pynini.Fst.read(os.path.join(self.testdir, "m6.fst"))
    self.assertEqual(pynini.arcmap(self.m1, map_type="rmweight"), m6)

  def testInvertArcmap(self):
    m7 = pynini.Fst.read(os.path.join(self.testdir, "m7.fst"))
    self.assertEqual(pynini.arcmap(self.m1, map_type="invert"), m7)

  def testQuantizeArcmap(self):
    m8 = pynini.Fst.read(os.path.join(self.testdir, "m8.fst"))
    self.assertEqual(pynini.arcmap(self.m1, map_type="quantize", delta=2), m8)

  def testPlusArcmap(self):
    m9 = pynini.Fst.read(os.path.join(self.testdir, "m9.fst"))
    self.assertEqual(pynini.arcmap(self.m1, map_type="plus", weight=2), m9)

  def testPowerArcmap(self):
    m15 = pynini.Fst.read(os.path.join(self.testdir, "m15.fst"))
    self.assertEqual(pynini.arcmap(self.m1, map_type="power", power=0.5), m15)
    m16 = pynini.Fst.read(os.path.join(self.testdir, "m16.fst"))
    self.assertEqual(pynini.arcmap(self.m1, map_type="power", power=2), m16)

  def testSuperfinalArcmap(self):
    m4 = pynini.Fst.read(os.path.join(self.testdir, "m4.fst"))
    self.assertEqual(pynini.arcmap(self.m1, map_type="superfinal"), m4)

  def testTimesArcmap(self):
    m10 = pynini.Fst.read(os.path.join(self.testdir, "m10.fst"))
    self.assertEqual(pynini.arcmap(self.m1, map_type="times", weight=2), m10)

  def testToLogArcmap(self):
    m11 = pynini.Fst.read(os.path.join(self.testdir, "m11.fst"))
    self.assertEqual(pynini.arcmap(self.m1, map_type="to_log"), m11)

  def testToLog64Arcmap(self):
    m12 = pynini.Fst.read(os.path.join(self.testdir, "m12.fst"))
    self.assertEqual(pynini.arcmap(self.m1, map_type="to_log64"), m12)

  def testGarbageMapTypeRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.arcmap(self.m1, map_type="nonexistent")  # pytype: disable=wrong-arg-types


class ArcsortTest(absltest.TestCase):
  a1: pynini.Fst
  a2: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/arcsort"
    )
    cls.a1 = pynini.Fst.read(os.path.join(testdir, "a1.fst"))
    cls.a2 = pynini.Fst.read(os.path.join(testdir, "a2.fst"))

  def testILabelArcSort(self):
    self.assertEqual(pynini.arcsort(self.a2, "ilabel"), self.a1)

  def testOLabelArcSort(self):
    self.assertEqual(pynini.arcsort(self.a1, "olabel"), self.a2)


class CDRewriteTest(absltest.TestCase):
  sigstar: pynini.Fst
  coronal: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.sigstar = pynini.union(*string.ascii_letters)
    cls.sigstar.closure()
    cls.sigstar.optimize()
    cls.coronal = pynini.union("L", "N", "R", "T", "D")

  # Non-static helper.
  def TestRule(self, rule, istring, ostring):
    self.assertEqual((istring @ rule).string(), ostring)

  # A -> B / C __ D.
  def testAGoesToBInTheContextOfCAndD(self):
    a_to_b = pynini.cdrewrite(pynini.cross("A", "B"), "C", "D", self.sigstar)
    self.TestRule(a_to_b, "CADCAD", "CBDCBD")

  # A -> B / C __ #.
  def testAGoesToBInTheContextOfCAndHash(self):
    a_to_b = pynini.cdrewrite(
        pynini.cross("A", "B"), "C", "[EOS]", self.sigstar
    )
    self.TestRule(a_to_b, "CA", "CB")
    self.TestRule(a_to_b, "CAB", "CAB")

  # Pre-Latin rhotacism:
  # s > r / V __ V.
  def testRhotacism(self):
    vowel = pynini.union("A", "E", "I", "O", "V")
    rhotacism = pynini.cdrewrite(
        pynini.cross("S", "R"), vowel, vowel, self.sigstar
    )
    self.TestRule(rhotacism, "LASES", "LARES")

  # Classical-Latin "Pre-s deletion":
  # [+cor] -> 0 / __ [+str] (condition: LTR)
  def testPreSDeletion(self):
    pre_s_deletion = pynini.cdrewrite(
        pynini.cross(self.coronal, ""), "", "S[EOS]", self.sigstar
    )
    pre_s_deletion.optimize()
    self.TestRule(pre_s_deletion, "CONCORDS", "CONCORS")
    self.TestRule(pre_s_deletion, "PVLTS", "PVLS")  # cf. gen.sg. PVLTIS
    self.TestRule(pre_s_deletion, "HONORS", "HONOS")  # cf. gen.sg. HONORIS
    # cf. gen.sg. SANGVINIS
    self.TestRule(pre_s_deletion, "SANGVINS", "SANGVIS")

  # The same, but incorrectly applied RTL.
  def testPreSDeletionRTL(self):
    pre_s_deletion_wrong = pynini.cdrewrite(
        pynini.cross(self.coronal, ""),
        "",
        "S[EOS]",
        self.sigstar,
        direction="rtl",
    )
    # Should be CONCORS.
    self.TestRule(pre_s_deletion_wrong, "CONCORDS", "CONCOS")

  # Prothesis in loanwords in Hindi (informally):
  # 0 -> i / # __ [+str] [-cor, +con]
  def testProthesis(self):
    non_coronal_consonant = pynini.union("M", "P", "B", "K", "G")
    prothesis = pynini.cdrewrite(
        pynini.cross("", "I"),
        "[BOS]",
        "S" + non_coronal_consonant,
        self.sigstar,
    )
    self.TestRule(prothesis, "SKUUL", "ISKUUL")  # "school"

  # TD-deletion in English:
  # [+cor, +obst, -cont] -> 0 / [+cons] __ # (conditions: LTR, optional)
  def testTDDeletion(self):
    consonant = pynini.union(
        "M", "P", "B", "F", "V", "N", "S", "Z", "T", "D", "L", "K", "G"
    )  # etc.
    td_deletion = pynini.cdrewrite(
        pynini.cross(pynini.union("T", "D"), ""),
        consonant,
        "[EOS]",
        self.sigstar,
        direction="ltr",
        mode="opt",
    )
    # Asserts that both are possible.
    self.assertEqual(
        pynini.optimize(pynini.project("FIST" @ td_deletion, "output")),
        pynini.optimize(pynini.union("FIS", "FIST")),
    )

  # Polish yer-lowering, after Gussman (1980:30):
  # E -> e / __ C_0 E
  #   -> e / __ C_0 #
  #   -> 0 / elsewhere  (condition: simultaneous).
  def testYerLowering(self):
    consonant = pynini.union(
        "b",
        "c",
        "ć",
        "d",
        "f",
        "g",
        "h",
        "j",
        "k",
        "l",
        "ł",
        "m",
        "n",
        "ń",
        "p",
        "r",
        "s",
        "ś",
        "t",
        "w",
        "z",
        "ź",
        "ż",
    )
    vowel = pynini.union("a", "ą", "e", "ę", "i", "o", "u", "y", "E")
    sigma_star = pynini.union(consonant, vowel).closure().optimize()
    yer_lowering = (
        pynini.cdrewrite(
            pynini.cross("E", "e"),
            "",
            pynini.closure(consonant) + pynini.union("E", "[EOS]"),
            sigma_star,
            direction="sim",
        )
        @ pynini.cdrewrite(pynini.cross("E", ""), "", "", sigma_star)
    ).optimize()
    # 'chair', dim.nom.sg.
    self.TestRule(yer_lowering, "krzesEłEko", "krzesełko")
    # 'chair', dim.gen.pl. Arguably the gen.pl. is itself -E (like in Russian)
    # and the word-final condition is superfluous, but that's not how Gussman
    # says it here.
    self.TestRule(yer_lowering, "krzesEłEk", "krzesełek")
    # 'bucket', dim.dim.nom.sg. It's fun to think what a twice-diminuitive
    # bucket might mean.
    self.TestRule(yer_lowering, "wiadErEczEko", "wiadereczko")
    # 'pail', dim.dim.gen.pl.
    self.TestRule(yer_lowering, "wiadErEczEk", "wiadereczek")

  def testLambdaTransducerRaisesFstOpError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.cdrewrite(
          pynini.cross("A", "B"), pynini.cross("C", "D"), "E", self.sigstar
      )

  def testRhoTransducerRaisesFstOpError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.cdrewrite(
          pynini.cross("A", "B"), "C", pynini.cross("D", "E"), self.sigstar
      )

  def testWeightedLambdaRaisesFstOpError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.cdrewrite(
          pynini.cross("A", "B"), pynini.accep("C", weight=2), "D", self.sigstar
      )

  def testWeightedRhoRaisesFstOpError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.cdrewrite(
          pynini.cross("A", "B"), "C", pynini.accep("D", weight=2), self.sigstar
      )


class ClosureTest(absltest.TestCase):

  def testRangeClosure(self):
    m = 3
    n = 7
    cheese = "Red Windsor"
    f = pynini.accep(cheese)
    f.closure(m, n)  # Doesn't accept <3 copies.
    for i in range(m):
      self.assertEqual(pynini.compose(f, cheese * i).num_states(), 0)
    # Accepts between 3-7 copies.
    for i in range(m, n + 1):
      self.assertNotEqual(pynini.compose(f, cheese * i).num_states(), 0)
    # Doesn't accept more than 7 copies.
    self.assertEqual(pynini.compose(f, cheese * (n + 1)).num_states(), 0)

  def testInvalidRangeRaisesFstOpError(self):
    f = pynini.accep("Red Windsor")
    with self.assertRaises(pynini.FstOpError):
      f.closure(5, 2)
    with self.assertRaises(pynini.FstOpError):
      f.closure(-2, 5)
    with self.assertRaises(pynini.FstOpError):
      f.closure(5, -2)

  def testQuesOnEmptyFstIsEpsilonMachine(self):
    f = pynini.Fst()
    self.assertEqual(pynini.accep(""), f.ques)

  def Matches(self, ifst: pynini.FstLike, ofst: pynini.FstLike) -> bool:
    intersection = pynini.intersect(ifst, ofst)
    return intersection.start() != pynini.NO_STATE_ID

  def testRangeClosureOperator(self):
    wordstr = "word"
    wordfst = pynini.accep(wordstr)
    self.assertEqual(wordstr * 3, (wordfst**3).string())
    self.assertEqual(wordstr * 2, (wordfst**2).string())
    abcd4 = pynini.union("a", "b", "c", "d") ** 4
    self.assertTrue(self.Matches("aaaa", abcd4))
    self.assertTrue(self.Matches("bbbb", abcd4))
    self.assertTrue(self.Matches("abbb", abcd4))
    self.assertTrue(self.Matches("dcba", abcd4))
    self.assertFalse(self.Matches("a", abcd4))
    self.assertFalse(self.Matches("aaaaa", abcd4))
    self.assertFalse(self.Matches("aadaa", abcd4))
    with self.assertRaises(pynini.FstOpError):
      unused_f = pynini.union("a", "b", "c", "d") ** -4

  def testRangeClosureOperatorWithTupleArgFiniteBounds(self):
    wordstr = "word"
    wordfst = pynini.accep(wordstr)
    wordfst3to5 = wordfst ** (3, 5)
    self.assertFalse(self.Matches(wordstr * 2, wordfst3to5))
    self.assertTrue(self.Matches(wordstr * 3, wordfst3to5))
    self.assertTrue(self.Matches(wordstr * 4, wordfst3to5))
    self.assertTrue(self.Matches(wordstr * 5, wordfst3to5))
    self.assertFalse(self.Matches(wordstr * 6, wordfst3to5))
    self.assertFalse(self.Matches(wordstr * 7, wordfst3to5))

  def testRangeClosureOperatorWithTupleArgFiniteUpperBound(self):
    wordstr = "word"
    wordfst = pynini.accep(wordstr)
    wordfstupto5 = wordfst ** (0, 5)
    self.assertTrue(self.Matches(wordstr * 0, wordfstupto5))
    self.assertTrue(self.Matches(wordstr * 1, wordfstupto5))
    self.assertTrue(self.Matches(wordstr * 2, wordfstupto5))
    self.assertTrue(self.Matches(wordstr * 3, wordfstupto5))
    self.assertTrue(self.Matches(wordstr * 4, wordfstupto5))
    self.assertTrue(self.Matches(wordstr * 5, wordfstupto5))
    self.assertFalse(self.Matches(wordstr * 6, wordfstupto5))
    self.assertFalse(self.Matches(wordstr * 7, wordfstupto5))

  def testRangeClosureOperatorWithTupleArgFiniteLowerBound(self):
    wordstr = "word"
    wordfst = pynini.accep(wordstr)
    wordfstatleast5 = wordfst ** (5, ...)
    self.assertFalse(self.Matches(wordstr * 0, wordfstatleast5))
    self.assertFalse(self.Matches(wordstr * 1, wordfstatleast5))
    self.assertFalse(self.Matches(wordstr * 2, wordfstatleast5))
    self.assertFalse(self.Matches(wordstr * 3, wordfstatleast5))
    self.assertFalse(self.Matches(wordstr * 4, wordfstatleast5))
    self.assertTrue(self.Matches(wordstr * 5, wordfstatleast5))
    self.assertTrue(self.Matches(wordstr * 6, wordfstatleast5))
    self.assertTrue(self.Matches(wordstr * 7, wordfstatleast5))

  def testRangeClosureOperatorWithTupleArgEllipsisLowerBound(self):
    wordfst = pynini.accep("word")
    with self.assertRaisesRegex(
        TypeError, r"The lower bound must be an integer"
    ):
      unused_f = wordfst ** (..., 5)  # pytype: disable=unsupported-operands

  def testRangeClosureOperatorWithTupleArgNoFiniteBounds(self):
    wordfst = pynini.accep("word")
    with self.assertRaisesRegex(
        TypeError, r"The lower bound must be an integer"
    ):
      unused_f = wordfst ** (..., ...)  # pytype: disable=unsupported-operands

  def testRangeClosureOperatorWithTupleArgWrongSizeZero(self):
    wordfst = pynini.accep("word")
    with self.assertRaisesRegex(ValueError, r"Expected tuple of length two"):
      unused_f = wordfst ** ()  # pytype: disable=unsupported-operands

  def testRangeClosureOperatorWithTupleArgWrongSizeOne(self):
    wordfst = pynini.accep("word")
    with self.assertRaisesRegex(ValueError, r"Expected tuple of length two"):
      unused_f = wordfst ** (2,)  # pytype: disable=unsupported-operands

  def testRangeClosureOperatorWithTupleArgWrongSizeThree(self):
    wordfst = pynini.accep("word")
    with self.assertRaisesRegex(ValueError, r"Expected tuple of length two"):
      unused_f = wordfst ** (0, 1, 2)  # pytype: disable=unsupported-operands

  def testRangeClosureOperatorWithListArg(self):
    wordfst = pynini.accep("word")
    with self.assertRaisesRegex(
        TypeError,
        r"unsupported operand type\(s\) for \*\* or pow\(\): '.*Fst' and"
        r" 'list'",
    ):
      unused_f = wordfst ** [2, 3]  # pytype: disable=unsupported-operands


class ComposeTest(absltest.TestCase):
  c1: pynini.Fst
  c2: pynini.Fst
  c3: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/compose"
    )
    cls.c1 = pynini.Fst.read(os.path.join(testdir, "c1.fst"))
    cls.c2 = pynini.Fst.read(os.path.join(testdir, "c2.fst"))
    cls.c3 = pynini.Fst.read(os.path.join(testdir, "c3.fst"))

  def testCompose(self):
    self.assertEqual(pynini.compose(self.c1, self.c2), self.c3)

  def testComposeOperator(self):
    self.assertEqual(self.c1 @ self.c2, self.c3)


class ConnectTest(absltest.TestCase):
  c1: pynini.Fst
  c2: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/connect"
    )
    cls.c1 = pynini.Fst.read(os.path.join(testdir, "c1.fst"))
    cls.c2 = pynini.Fst.read(os.path.join(testdir, "c2.fst"))

  def testConnect(self):
    self.assertEqual(pynini.connect(self.c1), self.c2)


class ConcatTest(absltest.TestCase):
  c1: pynini.Fst
  c2: pynini.Fst
  c3: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/concat"
    )
    cls.c1 = pynini.Fst.read(os.path.join(testdir, "c1.fst"))
    cls.c2 = pynini.Fst.read(os.path.join(testdir, "c2.fst"))
    cls.c3 = pynini.Fst.read(os.path.join(testdir, "c3.fst"))

  def testConcat(self):
    self.assertEqual(pynini.concat(self.c1, self.c2), self.c3)

  def testConcatOperator(self):
    self.assertEqual(self.c1 + self.c2, self.c3)


class CrossTest(absltest.TestCase):
  upper: pynini.Fst
  lower: pynini.Fst
  weight: pynini.Weight
  xprod: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "opengrm/operators/testdata"
    )
    cls.upper = pynini.Fst.read(os.path.join(testdir, "upper.fst"))
    cls.lower = pynini.Fst.read(os.path.join(testdir, "lower.fst"))
    cls.xprod = pynini.Fst.read(os.path.join(testdir, "xprod.fst"))

  def testCross(self):
    result = pynini.cross(self.upper, self.lower)
    self.assertEqual(self.xprod, result)

  def testPrecompiledLogCrossProduct(self):
    upper = pynini.accep("Smoked Austrian", arc_type="log")
    lower = pynini.accep("No", arc_type="log")
    tr = pynini.cross(upper, lower)
    self.assertEqual(tr.arc_type(), "log")

  def testImplicitLeftLogCrossProducts(self):
    tr = pynini.cross("Smoked Austrian", pynini.accep("No", arc_type="log"))
    self.assertEqual(tr.arc_type(), "log")

  def testImplicitRightLogCrossProducts(self):
    tr = pynini.cross(pynini.accep("Smoked Austrian", arc_type="log"), "No")
    self.assertEqual(tr.arc_type(), "log")


class DeterminizeTest(absltest.TestCase):
  testdir: str
  d1: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/determinize",
    )
    cls.d1 = pynini.Fst.read(os.path.join(cls.testdir, "d1.fst"))

  def testAcceptorDeterminize(self):
    d2 = pynini.Fst.read(os.path.join(self.testdir, "d2.fst"))
    self.assertEqual(pynini.determinize(self.d1), d2)

  def testTransducerDeterminize(self):
    d3 = pynini.Fst.read(os.path.join(self.testdir, "d3.fst"))
    d4 = pynini.Fst.read(os.path.join(self.testdir, "d4.fst"))
    self.assertEqual(pynini.determinize(d3), d4)

  def testPrunedDeterminize(self):
    d5 = pynini.Fst.read(os.path.join(self.testdir, "d5.fst"))
    self.assertEqual(pynini.determinize(self.d1, weight=0.5, nstate=10), d5)

  def testGarbageDetTypeRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.determinize(self.d1, det_type="nonexistent")  # pytype: disable=wrong-arg-types


class DifferenceTest(absltest.TestCase):
  d1: pynini.Fst
  d2: pynini.Fst
  d3: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/difference",
    )
    cls.d1 = pynini.Fst.read(os.path.join(testdir, "d1.fst"))
    cls.d2 = pynini.Fst.read(os.path.join(testdir, "d2.fst"))
    cls.d3 = pynini.Fst.read(os.path.join(testdir, "d3.fst"))

  def testDifference(self):
    self.assertEqual(
        pynini.difference(self.d1, self.d2, connect=False), self.d3
    )

  def testDifferenceOperator(self):
    self.assertEqual(self.d1 - self.d2, pynini.connect(self.d3))

  def testDifferenceWithUnion(self):
    ab = pynini.union("a", "b")
    abc = pynini.union(ab, "c")
    self.assertEqual(pynini.difference(abc, ab).optimize(), "c")


class DisambiguateTest(absltest.TestCase):
  d1: pynini.Fst
  d2: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/disambiguate",
    )
    cls.d1 = pynini.Fst.read(os.path.join(testdir, "d1.fst"))
    cls.d2 = pynini.Fst.read(os.path.join(testdir, "d2.fst"))

  def testDisambiguate(self):
    self.assertEqual(pynini.disambiguate(self.d1), self.d2)


class DowncastTest(absltest.TestCase):
  f: pywrapfst.VectorFst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.f = pywrapfst.VectorFst()
    # Epsilon machine.
    s = cls.f.add_state()
    cls.f.set_start(s)
    cls.f.set_final(s)

  def testDowncastTypesAreCorrect(self):
    self.assertEqual(type(self.f), pywrapfst.VectorFst)
    f_downcast = pynini.Fst.from_pywrapfst(self.f)
    self.assertEqual(type(f_downcast), pynini.Fst)

  def testDowncastedMutationTriggersDeepCopy(self):
    f_downcast = pynini.Fst.from_pywrapfst(self.f)
    f_downcast.delete_states()
    self.assertEqual(f_downcast.num_states(), 0)
    self.assertNotEqual(self.f.num_states(), 0)


class EpsnormalizeTest(absltest.TestCase):
  testdir: str

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/epsnormalize",
    )

  def testInputEpsnormalize(self):
    e1 = pynini.Fst.read(os.path.join(self.testdir, "e1.fst"))
    e2 = pynini.Fst.read(os.path.join(self.testdir, "e2.fst"))
    self.assertEqual(pynini.epsnormalize(e1), e2)

  def testOutputEpsnormalize(self):
    e3 = pynini.Fst.read(os.path.join(self.testdir, "e3.fst"))
    e4 = pynini.Fst.read(os.path.join(self.testdir, "e4.fst"))
    self.assertEqual(pynini.epsnormalize(e3, "output"), e4)


class EncodeMapperTest(absltest.TestCase):
  e1: pynini.Fst
  e1_cd: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/encode"
    )
    cls.e1 = pynini.Fst.read(os.path.join(testdir, "e1.fst"))
    cls.e1_cd = pynini.Fst.read(os.path.join(testdir, "e1_cd.fst"))

  def testLabelRoundtripEncode(self):
    e1_res = self.e1.copy()
    encoder = pynini.EncodeMapper(e1_res.arc_type(), encode_labels=True)
    e1_res.encode(encoder)
    e1_res.decode(encoder)
    self.assertEqual(e1_res, self.e1_cd)

  def testWeightRoundtripEncode(self):
    e1_res = self.e1.copy()
    encoder = pynini.EncodeMapper(e1_res.arc_type(), encode_weights=True)
    e1_res.encode(encoder)
    e1_res.decode(encoder)
    self.assertEqual(e1_res, self.e1_cd)

  def testLabelAndWeightRoundtripEncode(self):
    e1_res = self.e1.copy()
    encoder = pynini.EncodeMapper(
        e1_res.arc_type(), encode_labels=True, encode_weights=True
    )
    e1_res.encode(encoder)
    e1_res.decode(encoder)
    self.assertEqual(e1_res, self.e1_cd)

  def testEncodeMapperSymbolTableValidity(self):
    e1_res = self.e1.copy()
    encoder = pynini.EncodeMapper(e1_res.arc_type(), encode_labels=True)
    syms = pynini.SymbolTable()
    syms.add_symbol("a")
    syms.add_symbol("b")
    encoder.set_output_symbols(syms)
    del syms
    self.assertEqual(list(encoder.output_symbols()), [(0, "a"), (1, "b")])
    attached_syms = encoder.output_symbols()
    attached_syms_owned = attached_syms.copy()
    encoder.set_output_symbols(None)
    with self.assertRaises(pynini.FstOpError):
      list(attached_syms)
    self.assertEqual(list(attached_syms_owned), [(0, "a"), (1, "b")])
    del encoder
    self.assertEqual(list(attached_syms_owned), [(0, "a"), (1, "b")])


class EqualTest(absltest.TestCase):
  f: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.f = pynini.accep("Danish Blue")

  def testEqual(self):
    self.assertTrue(pynini.equal(self.f, self.f.copy()))

  def testEqualOperator(self):
    self.assertTrue(self.f == self.f.copy())  # pylint: disable=g-generic-assert

  def testNotEqualOperator(self):
    self.assertFalse(self.f != self.f.copy())  # pylint: disable=g-generic-assert


class EquivalentTest(absltest.TestCase):
  e1: pynini.Fst
  e2: pynini.Fst
  e3: pynini.Fst
  e4: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/equivalent",
    )
    cls.e1 = pynini.Fst.read(os.path.join(testdir, "e1.fst"))
    cls.e2 = pynini.Fst.read(os.path.join(testdir, "e2.fst"))
    cls.e3 = pynini.Fst.read(os.path.join(testdir, "e3.fst"))
    cls.e4 = pynini.Fst.read(os.path.join(testdir, "e4.fst"))

  def testEquivalent(self):
    for a, b in itertools.combinations([self.e1, self.e2, self.e3], 2):
      self.assertTrue(pynini.equivalent(a, b))

  def testNotEquivalent(self):
    for a in (self.e1, self.e2, self.e3):
      self.assertFalse(pynini.equivalent(a, self.e4))


class ExceptionsTest(absltest.TestCase):
  exchange: pynini.Fst
  f: pynini.Fst
  s: pynini.SymbolTable
  map_file: pathlib.Path

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.exchange = pynini.cross("Liptauer", "No")
    cls.f = pynini.Fst()
    cls.s = pynini.SymbolTable()
    cls.map_file = pathlib.Path(
        runfiles.test_src_path(
            "opengrm/string/testdata/str.map",
        )
    )

  def testBadDestinationIndexAddArcDoesNotRaiseFstIndexError(self):
    f = self.f.copy()
    s = f.add_state()
    f.set_start(s)
    f.set_final(s)
    f.add_arc(s, pynini.Arc(0, 0, 0, -1))
    self.assertFalse(f.verify())

  def testBadIndexFinalRaisesFstIndexError(self):
    with self.assertRaises(pynini.FstIndexError):
      self.f.final(-1)

  def testBadIndexNumArcsRaisesFstIndexError(self):
    with self.assertRaises(pynini.FstIndexError):
      self.f.num_arcs(-1)

  def testBadIndexNumInputEpsilonsRaisesFstIndexError(self):
    with self.assertRaises(pynini.FstIndexError):
      self.f.num_input_epsilons(-1)

  def testBadIndexNumOutputEpsilonsRaisesFstIndexError(self):
    with self.assertRaises(pynini.FstIndexError):
      self.f.num_output_epsilons(-1)

  def testBadIndexDeleteArcsRaisesFstIndexError(self):
    f = self.f.copy()
    with self.assertRaises(pynini.FstIndexError):
      f.delete_arcs(-1)

  def testBadIndicesDeleteStatesRaisesFstIndexError(self):
    f = self.f.copy()
    with self.assertRaises(pynini.FstIndexError):
      f.delete_states((-1, -2))

  def testBadSourceIndexAddArcRaisesFstIndexError(self):
    f = self.f.copy()
    with self.assertRaises(pynini.FstIndexError):
      f.add_arc(-1, pynini.Arc(0, 0, 0, 0))

  def testGarbageComposeFilterComposeRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.compose(self.f, self.f, compose_filter="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageComposeFilterDifferenceRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.difference(self.f, self.f, compose_filter="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageQueueTypeRmepsilonRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.rmepsilon(self.f, queue_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageQueueTypeShortestDistanceRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.shortestdistance(self.f, queue_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageQueueTypeShortestPathRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.shortestpath(self.f, queue_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageSelectTypeRandgenRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.randgen(self.f, select="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageCallArcLabelingReplaceRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.replace([(1, self.f)], call_arc_labeling="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageReturnArcLabelingReplaceRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.replace([(1, self.f)], return_arc_labeling="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageInputTokenTypeStringFileRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.string_file(self.map_file, input_token_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageOutputTokenTypeStringFileRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.string_file(self.map_file, output_token_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testNonexistentStringFileRaisesFstIOError(self):
    with self.assertRaises(pynini.FstIOError):
      unused_f = pynini.string_file("nonexistent")

  def testGarbageInputTokenTypeStringMapRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.string_map([], input_token_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageOutputTokenTypeStringMapRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.string_map([], output_token_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageInputTokenTypeStringPathIteratorRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      self.f.paths(input_token_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testGarbageOutputTokenTypeStringPathIteratorRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      self.f.paths(output_token_type="nonexistent")  # pytype: disable=wrong-arg-types

  def testTransducerDifferenceRaisesFstArgError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.difference(self.exchange, self.exchange)

  def testWrongArcTypeEncodeRaisesFstOpError(self):
    with self.assertRaises(pynini.FstOpError):
      f = self.f.copy()
      mapper = pynini.EncodeMapper(arc_type="log")
      f.encode(mapper)

  def testWrongWeightTypeAddArcRaisesFstOpError(self):
    f = self.f.copy()
    s = f.add_state()
    f.set_start(s)
    f.set_final(s)
    with self.assertRaises(pynini.FstOpError):
      f.add_arc(s, pynini.Arc(0, 0, pynini.Weight.one("log"), 0))

  def testWrongWeightTypeDeterminizeRaisesFstOpError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.determinize(self.f, weight=pynini.Weight.one("log"))

  def testWrongWeightTypeDisambiguateRaisesFstOpError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.disambiguate(self.f, weight=pynini.Weight.one("log"))

  def testWrongWeightTypePruneRaisesFstOpError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.prune(self.f, weight=pynini.Weight.one("log"))

  def testWrongWeightTypeRmepsilonRaisesFstOpError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.rmepsilon(self.f, weight=pynini.Weight.one("log"))

  def testWrongWeightTypeSetFinalRaisesFstOpError(self):
    f = self.f.copy()
    s = f.add_state()
    f.set_start(s)
    with self.assertRaises(pynini.FstOpError):
      f.set_final(s, pynini.Weight.one("log"))

  def testGarbageWeightTypeRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.Weight("nonexistent", 1)


class FarTest(absltest.TestCase):
  farfile: str
  pairs: dict[str, pynini.Fst]
  arc_type: str

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/extensions/far/testdata"
    )
    cls.farfile = os.path.join(absltest.get_default_test_tmpdir(), "test.far")
    f1 = pynini.Fst.read(os.path.join(testdir, "test1-01.fst"))
    f2 = pynini.Fst.read(os.path.join(testdir, "test1-02.fst"))
    f3 = pynini.Fst.read(os.path.join(testdir, "test1-03.fst"))
    cls.pairs = {"1": f1, "2": f2, "3": f3}
    cls.arc_type = f1.arc_type()

  def testSTTableFar(self):
    with pynini.Far(
        self.farfile, "w", arc_type=self.arc_type, far_type="sttable"
    ) as sink:
      for k, f in sorted(self.pairs.items()):
        sink[k] = f
    self.assertTrue(sink.closed())
    with self.assertRaises(ValueError):
      sink.far_type()
    with pynini.Far(self.farfile, "r") as source:
      self.assertEqual(source.far_type(), "sttable")
      for k, f in source:
        self.assertEqual(self.pairs[k], f)
    self.assertTrue(source.closed())
    with self.assertRaises(ValueError):
      source.far_type()

  def testSTListFar(self):
    with pynini.Far(
        self.farfile, "w", arc_type=self.arc_type, far_type="stlist"
    ) as sink:
      for k, f in sorted(self.pairs.items()):
        sink[k] = f
    self.assertTrue(sink.closed())
    with self.assertRaises(ValueError):
      sink.far_type()
    with pynini.Far(self.farfile, "r") as source:
      self.assertEqual(source.far_type(), "stlist")
      for k, f in source:
        self.assertEqual(self.pairs[k], f)
    self.assertTrue(source.closed())
    with self.assertRaises(ValueError):
      source.far_type()


class GeneratedSymbolsTest(absltest.TestCase):

  def testBosIndex(self):
    bos_index = 0xF8FE  # Defined in stringcompile.h.
    f = pynini.accep("[BOS]")
    aiter = f.arcs(f.start())
    self.assertFalse(aiter.done())
    arc = aiter.value()
    self.assertEqual(bos_index, arc.ilabel)
    self.assertEqual(bos_index, arc.ilabel)
    aiter.next()
    self.assertTrue(aiter.done())

  def testEosIndex(self):
    eos_index = 0xF8FF  # Defined in stringcompile.h.
    f = pynini.accep("[EOS]")
    aiter = f.arcs(f.start())
    arc = aiter.value()
    self.assertEqual(eos_index, arc.ilabel)
    self.assertEqual(eos_index, arc.olabel)
    aiter.next()
    self.assertTrue(aiter.done())

  def testGeneratedSymbols(self):
    cheese = "Parmesan"
    unused_f = pynini.accep(f"[{cheese}]")
    syms = pynini.generated_symbols()
    self.assertTrue(syms.member(cheese))


class IntersectTest(absltest.TestCase):
  i1: pynini.Fst
  i2: pynini.Fst
  i3: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/intersect"
    )
    cls.i1 = pynini.Fst.read(os.path.join(testdir, "i1.fst"))
    cls.i2 = pynini.Fst.read(os.path.join(testdir, "i2.fst"))
    cls.i3 = pynini.Fst.read(os.path.join(testdir, "i3.fst"))

  def testIntersect(self):
    self.assertEqual(pynini.intersect(self.i1, self.i2), self.i3)


class InvertTest(absltest.TestCase):
  i1: pynini.Fst
  i2: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/invert"
    )
    cls.i1 = pynini.Fst.read(os.path.join(testdir, "i1.fst"))
    cls.i2 = pynini.Fst.read(os.path.join(testdir, "i2.fst"))

  def testInversion(self):
    self.assertEqual(pynini.invert(self.i1), self.i2)
    self.assertEqual(pynini.invert(self.i2), self.i1)


class IOTest(absltest.TestCase):
  testdir: str

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/compile"
    )

  # Non-static helper.
  def EqualityAndTypeEquality(self, f, g):
    self.assertEqual(f, g)
    self.assertEqual(type(f), type(g))

  def testFileReadAndWrite(self):
    f = pynini.Fst.read(os.path.join(self.testdir, "fst.compiled"))
    sink = os.path.join(absltest.get_default_test_tmpdir(), "copy.fst")
    try:
      f.write(sink)
      g = pynini.Fst.read(sink)
      self.EqualityAndTypeEquality(f, g)
    finally:
      os.remove(sink)

  def testStringReadAndWrite(self):
    with open(os.path.join(self.testdir, "fst.compiled"), "rb") as source:
      f = pynini.Fst.read_from_string(source.read())
    g = pynini.Fst.read_from_string(f.write_to_string())
    self.EqualityAndTypeEquality(f, g)

  def testPickleReadAndWrite(self):
    f = pynini.Fst.read(os.path.join(self.testdir, "fst.compiled"))
    sink = pickle.dumps(f)
    g = pickle.loads(sink)
    self.EqualityAndTypeEquality(f, g)

  def testGarbageFstReadRaisesFstIOError(self):
    with self.assertRaises(pynini.FstIOError):
      pynini.Fst.read("nonexistent")

  def testGarbageStringFileReadRaisesFstIOError(self):
    with self.assertRaises(pynini.FstIOError):
      pynini.string_file("nonexistent")


class LenientlyComposeTest(absltest.TestCase):
  sigstar: pynini.Fst
  cheese_geography: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.sigstar = pynini.union(*string.ascii_letters + " ").closure().optimize()
    cls.cheese_geography = pynini.string_map((
        ("Red Leicester", "England"),
        ("Tilsit", "Russia"),
        ("Caerphilly", "Wales"),
        ("Bel Paese", "Italy"),
        ("Red Windsor", "England"),
        ("Stilton", "England"),
        ("Emmental", "Switzerland"),
        ("Norwegian Jarlsberg", "Norway"),
        ("Liptauer", "Germany"),
        ("Lancashire", "England"),
        ("White Stilton", "England"),
        ("Danish Blue", "Denmark"),
        ("Double Gloucester", "England"),
        ("Cheshire", "England"),
        ("Dorset Blue Vinney", "England"),
        ("Brie", "France"),
        ("Roquefort", "France"),
        ("Port Salut", "France"),
    ))

  def testLenientCompositionOfOutOfDomainStringWithTransducerIsIdentity(self):
    cheese = "Wisconsin Cheddar"
    self.assertEqual(
        pynini.leniently_compose(cheese, self.cheese_geography, self.sigstar),
        cheese,
    )

  def testLenientCompositionOfInDomainStringWithTransducerIsTransduced(self):
    cheese = "Lancashire"
    self.assertEqual(
        pynini.leniently_compose(cheese, self.cheese_geography, self.sigstar)
        .project("output")
        .optimize(),
        "England",
    )


class MinimizeTest(absltest.TestCase):
  testdir: pathlib.Path

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = pathlib.Path(
        runfiles.test_src_path(
            "openfst/test/testdata/minimize",
        )
    )

  def testUnweightedAcyclicAcceptorMinimize(self):
    m1 = pynini.Fst.read(self.testdir / "m1.fst")
    acyclic_min = pynini.Fst.read(self.testdir / "acyclic_min.fst")
    self.assertEqual(pynini.minimize(m1), acyclic_min)

  def testUnweightedCyclicAcceptorMinimize(self):
    m2 = pynini.Fst.read(self.testdir / "m2.fst")
    cyclic_min = pynini.Fst.read(self.testdir / "cyclic_min.fst")
    self.assertEqual(pynini.minimize(m2), cyclic_min)

  def testWeightedAcyclicAcceptorMinimize(self):
    m3 = pynini.Fst.read(self.testdir / "m3.fst")
    weighted_acyclic_min = pynini.Fst.read(
        self.testdir / "weighted_acyclic_min.fst"
    )
    self.assertEqual(pynini.minimize(m3), weighted_acyclic_min)

  def testWeightedCyclicAcceptorMinimize(self):
    m4 = pynini.Fst.read(self.testdir / "m4.fst")
    weighted_cyclic_min = pynini.Fst.read(
        self.testdir / "weighted_cyclic_min.fst"
    )
    self.assertEqual(pynini.minimize(m4), weighted_cyclic_min)

  def testWeightedAcyclicTransducerMinimize(self):
    m5 = pynini.Fst.read(self.testdir / "m5.fst")
    transducer_acyclic_min = pynini.Fst.read(
        self.testdir / "transducer_acyclic_min.fst"
    )
    self.assertEqual(pynini.minimize(m5), transducer_acyclic_min)

  def testWeightedCyclicTransducerMinimize(self):
    m6 = pynini.Fst.read(self.testdir / "m6.fst")
    transducer_cyclic_min = pynini.Fst.read(
        self.testdir / "transducer_cyclic_min.fst"
    )
    self.assertEqual(pynini.minimize(m6), transducer_cyclic_min)


class MPdtComposeTest(absltest.TestCase):
  c1: pynini.Fst
  c2: pynini.Fst
  c3: pynini.Fst
  eparen: pynini.MPdtParentheses

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/extensions/mpdt/testdata",
    )
    cls.c1 = pynini.Fst.read(os.path.join(testdir, "c1.fst"))
    cls.c2 = pynini.Fst.read(os.path.join(testdir, "c2.fst"))
    cls.c3 = pynini.Fst.read(os.path.join(testdir, "c3.fst"))
    cls.eparen = pynini.MPdtParentheses.read(
        os.path.join(testdir, "eparen.triples")
    )

  def testMPdtCompose(self):
    result = pynini.mpdt_compose(self.c1, self.c2, self.eparen, left_mpdt=True)
    self.assertEqual(result, self.c3)


class MPdtExpandTest(absltest.TestCase):
  e1: pynini.Fst
  e2: pynini.Fst
  e3: pynini.Fst
  eparen: pynini.MPdtParentheses

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/extensions/mpdt/testdata",
    )
    cls.e1 = pynini.Fst.read(os.path.join(testdir, "e1.fst"))
    cls.e2 = pynini.Fst.read(os.path.join(testdir, "e2.fst"))
    cls.e3 = pynini.Fst.read(os.path.join(testdir, "e3.fst"))
    cls.eparen = pynini.MPdtParentheses.read(
        os.path.join(testdir, "eparen.triples")
    )

  def testRemoveParenthesesMPdtExpand(self):
    self.assertEqual(
        pynini.mpdt_expand(self.e1, self.eparen, keep_parentheses=False),
        self.e2,
    )

  def testKeepParenthesesMPdtExpand(self):
    self.assertEqual(
        pynini.mpdt_expand(self.e1, self.eparen, keep_parentheses=True), self.e3
    )


class MPdtReverseTest(absltest.TestCase):
  v1: pynini.Fst
  v2: pynini.Fst
  vparen: pynini.MPdtParentheses
  vparen_out: pynini.MPdtParentheses

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/extensions/mpdt/testdata",
    )
    cls.v1 = pynini.Fst.read(os.path.join(testdir, "v1.fst"))
    cls.v2 = pynini.Fst.read(os.path.join(testdir, "v2.fst"))
    cls.vparen = pynini.MPdtParentheses.read(
        os.path.join(testdir, "vparen.triples")
    )
    cls.vparen_out = pynini.MPdtParentheses.read(
        os.path.join(testdir, "vparen.triples.out")
    )

  def testMPdtReverse(self):
    v2_res, vparen_out_res = pynini.mpdt_reverse(self.v1, self.vparen)
    self.assertEqual(v2_res, self.v2)
    # Checks that the stack assignments also match.
    triples = zip(vparen_out_res, self.vparen_out)
    for (_, _, assign_res), (_, _, assign) in triples:
      self.assertEqual(assign_res, assign)


class MutableArcIteratorTest(absltest.TestCase):
  testdir: str

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/compile"
    )

  def testMutableArcIteratorAfterFstDeletion(self):
    f = pynini.Fst.read(os.path.join(self.testdir, "fst.compiled"))
    maiter = f.mutable_arcs(f.start())
    del f  # Should be garbage-collected immediately.
    self.assertFalse(maiter.done())


class OptimizeTest(absltest.TestCase):
  testdir: str

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "opengrm/operators/testdata"
    )

  def testOptimizeWeightedAcyclicAcceptor(self):
    o1 = pynini.Fst.read(os.path.join(self.testdir, "ofst1.fst"))
    o1_opt = pynini.Fst.read(os.path.join(self.testdir, "ofst1_opt.fst"))
    self.assertEqual(pynini.optimize(o1, True), o1_opt)

  def testOptimizeUnweightedCyclicAcceptor(self):
    o2 = pynini.Fst.read(os.path.join(self.testdir, "ofst2.fst"))
    o2_opt = pynini.Fst.read(os.path.join(self.testdir, "ofst2_opt.fst"))
    self.assertEqual(pynini.optimize(o2, True), o2_opt)

  def testOptimizeWeightedCyclicAcceptor(self):
    o3 = pynini.Fst.read(os.path.join(self.testdir, "ofst3.fst"))
    o3_opt = pynini.Fst.read(os.path.join(self.testdir, "ofst3_opt.fst"))
    self.assertEqual(pynini.optimize(o3, True), o3_opt)

  def testOptimizeWeightedCyclicFunctionalTransducer(self):
    o4 = pynini.Fst.read(os.path.join(self.testdir, "ofst4.fst"))
    o4_opt = pynini.Fst.read(os.path.join(self.testdir, "ofst4_opt.fst"))
    self.assertEqual(pynini.optimize(o4, True), o4_opt)

  def testOptimizeCyclicWeightedNonfunctionalTransducer(self):
    o5 = pynini.Fst.read(os.path.join(self.testdir, "ofst5.fst"))
    o5_opt = pynini.Fst.read(os.path.join(self.testdir, "ofst5_opt.fst"))
    self.assertEqual(pynini.optimize(o5, True), o5_opt)


class PdtComposeTest(absltest.TestCase):
  c1: pynini.Fst
  c2: pynini.Fst
  c3: pynini.Fst
  cparen: pynini.PdtParentheses

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/extensions/pdt/testdata"
    )
    cls.c1 = pynini.Fst.read(os.path.join(testdir, "c1.fst"))
    cls.c2 = pynini.Fst.read(os.path.join(testdir, "c2.fst"))
    cls.c3 = pynini.Fst.read(os.path.join(testdir, "c3.fst"))
    cls.cparen = pynini.PdtParentheses.read(
        os.path.join(testdir, "cparen.pairs")
    )

  def testLeftPdtPdtCompose(self):
    self.assertEqual(
        pynini.pdt_compose(self.c1, self.c2, self.cparen, left_pdt=True),
        self.c3,
    )

  def testRightPdtPdtCompose(self):
    self.assertEqual(
        pynini.pdt_compose(self.c2, self.c1, self.cparen, left_pdt=False),
        self.c3,
    )


class PdtExpandTest(absltest.TestCase):
  e1: pynini.Fst
  e2: pynini.Fst
  e3: pynini.Fst
  eparen: pynini.PdtParentheses

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/extensions/pdt/testdata"
    )
    cls.e1 = pynini.Fst.read(os.path.join(testdir, "e1.fst"))
    cls.e2 = pynini.Fst.read(os.path.join(testdir, "e2.fst"))
    cls.e3 = pynini.Fst.read(os.path.join(testdir, "e3.fst"))
    cls.eparen = pynini.PdtParentheses.read(
        os.path.join(testdir, "eparen.pairs")
    )

  def testRemoveParenthesesPdtExpand(self):
    self.assertEqual(
        pynini.pdt_expand(self.e1, self.eparen, keep_parentheses=False), self.e2
    )

  def testKeepParenthesesPdtExpand(self):
    self.assertEqual(
        pynini.pdt_expand(self.e1, self.eparen, keep_parentheses=True), self.e3
    )


class PdtReplaceTest(absltest.TestCase):
  r1: pynini.Fst
  r2: pynini.Fst
  r3: pynini.Fst
  r4: pynini.Fst
  r5: pynini.Fst
  rparen: pynini.PdtParentheses

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/extensions/pdt/testdata"
    )
    cls.r1 = pynini.Fst.read(os.path.join(testdir, "r1.fst"))
    cls.r2 = pynini.Fst.read(os.path.join(testdir, "r2.fst"))
    cls.r3 = pynini.Fst.read(os.path.join(testdir, "r3.fst"))
    cls.r4 = pynini.Fst.read(os.path.join(testdir, "r4.fst"))
    cls.r5 = pynini.Fst.read(os.path.join(testdir, "r5.fst"))
    cls.rparen = pynini.PdtParentheses.read(
        os.path.join(testdir, "rparen.pairs")
    )

  def testPdtReplace(self):
    pairs = [(3, self.r1), (4, self.r2), (5, self.r3), (6, self.r4)]
    fst, paren = pynini.pdt_replace(pairs)
    self.assertEqual(fst, self.r5)
    # Tests the parentheses object.
    self.assertEqual(len(paren), len(self.rparen))
    for p1, p2 in zip(paren, self.rparen):
      self.assertEqual(p1, p2)


class PdtReverseTest(absltest.TestCase):
  v1: pynini.Fst
  v2: pynini.Fst
  vparen: pynini.PdtParentheses

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/extensions/pdt/testdata"
    )
    cls.v1 = pynini.Fst.read(os.path.join(testdir, "v1.fst"))
    cls.v2 = pynini.Fst.read(os.path.join(testdir, "v2.fst"))
    cls.vparen = pynini.PdtParentheses.read(
        os.path.join(testdir, "vparen.pairs")
    )

  def testPdtReverse(self):
    self.assertEqual(pynini.pdt_reverse(self.v1, self.vparen), self.v2)


class ProjectTest(absltest.TestCase):
  p1: pynini.Fst
  p2: pynini.Fst
  p3: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/project"
    )
    cls.p1 = pynini.Fst.read(os.path.join(testdir, "p1.fst"))
    cls.p2 = pynini.Fst.read(os.path.join(testdir, "p2.fst"))
    cls.p3 = pynini.Fst.read(os.path.join(testdir, "p3.fst"))

  def testInputProject(self):
    self.assertEqual(pynini.project(self.p1, "input"), self.p2)

  def testOutputProject(self):
    self.assertEqual(pynini.project(self.p1, "output"), self.p3)

  def testStringParameterTypeErrorForProject(self):
    # Ensure that Thrax-style projections raise errors instead of silently doing
    # something unexpected.
    with self.assertRaises(TypeError):
      pynini.project(self.p1, True)  # pytype: disable=wrong-arg-types
    with self.assertRaises(TypeError):
      pynini.project(self.p1, False)  # pytype: disable=wrong-arg-types


class PruneTest(absltest.TestCase):
  p1: pynini.Fst
  p2: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/prune"
    )
    cls.p1 = pynini.Fst.read(os.path.join(testdir, "p1.fst"))
    cls.p2 = pynini.Fst.read(os.path.join(testdir, "p2.fst"))

  def testPrune(self):
    self.assertEqual(pynini.prune(self.p1, weight=0.5), self.p2)


class PushTest(absltest.TestCase):
  testdir: str
  p1: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/push"
    )
    cls.p1 = pynini.Fst.read(os.path.join(cls.testdir, "p1.fst"))

  def testToInitialWeightPush(self):
    p2 = pynini.Fst.read(os.path.join(self.testdir, "p2.fst"))
    self.assertEqual(pynini.push(self.p1, push_weights=True), p2)

  def testToFinalWeightPush(self):
    p3 = pynini.Fst.read(os.path.join(self.testdir, "p3.fst"))
    self.assertEqual(
        pynini.push(self.p1, push_weights=True, reweight_type="to_final"), p3
    )

  def testToInitialLabelPush(self):
    p4 = pynini.Fst.read(os.path.join(self.testdir, "p4.fst"))
    self.assertEqual(pynini.push(self.p1, push_labels=True), p4)

  def testToFinalLabelPush(self):
    p5 = pynini.Fst.read(os.path.join(self.testdir, "p5.fst"))
    self.assertEqual(
        pynini.push(self.p1, push_labels=True, reweight_type="to_final"), p5
    )


class RandequivalentTest(absltest.TestCase):
  e1: pynini.Fst
  e2: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/equivalent",
    )
    cls.e1 = pynini.Fst.read(os.path.join(testdir, "e1.fst"))
    cls.e2 = pynini.Fst.read(os.path.join(testdir, "e2.fst"))

  def testRandequivalent(self):
    self.assertTrue(pynini.randequivalent(self.e1, self.e2, npath=25, seed=212))


class RandgenTest(absltest.TestCase):
  r1: pynini.Fst
  r2: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/randgen"
    )
    cls.r1 = pynini.Fst.read(os.path.join(testdir, "r1.fst"))
    cls.r2 = pynini.Fst.read(os.path.join(testdir, "r2.fst"))

  def testRandgen(self):
    self.assertEqual(pynini.randgen(self.r1, seed=2), self.r2)


class RelabelTest(absltest.TestCase):
  testdir: str
  r1: pynini.Fst
  r4: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/relabel"
    )
    cls.r1 = pynini.Fst.read(os.path.join(cls.testdir, "r1.fst"))
    cls.r4 = pynini.Fst.read(os.path.join(cls.testdir, "r4.fst"))

  def testInputSymbolsRelabel(self):
    r2 = pynini.Fst.read(os.path.join(self.testdir, "r2.fst"))
    in2 = pynini.SymbolTable.read_text(os.path.join(self.testdir, "in2.map"))
    self.assertEqual(pynini.relabel_tables(self.r1, new_isymbols=in2), r2)

  def testOutputSymbolsRelabel(self):
    r3 = pynini.Fst.read(os.path.join(self.testdir, "r3.fst"))
    out3 = pynini.SymbolTable.read_text(os.path.join(self.testdir, "out3.map"))
    self.assertEqual(pynini.relabel_tables(self.r1, new_osymbols=out3), r3)

  def testInputPairsRelabel(self):
    r5 = pynini.Fst.read(os.path.join(self.testdir, "r5.fst"))
    pairs = [(1, 2), (2, 1)]
    self.assertEqual(pynini.relabel_pairs(self.r4, ipairs=pairs), r5)

  def testOutputPairsRelabel(self):
    r6 = pynini.Fst.read(os.path.join(self.testdir, "r6.fst"))
    pairs = [(1, 2), (2, 3), (3, 4), (4, 1)]
    self.assertEqual(pynini.relabel_pairs(self.r4, opairs=pairs), r6)


class ReplaceTest(absltest.TestCase):
  g1: pynini.Fst
  g2: pynini.Fst
  g3: pynini.Fst
  g4: pynini.Fst
  g_out: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/replace"
    )
    cls.g1 = pynini.Fst.read(os.path.join(testdir, "g1.fst"))
    cls.g2 = pynini.Fst.read(os.path.join(testdir, "g2.fst"))
    cls.g3 = pynini.Fst.read(os.path.join(testdir, "g3.fst"))
    cls.g4 = pynini.Fst.read(os.path.join(testdir, "g4.fst"))
    cls.g_out = pynini.Fst.read(os.path.join(testdir, "g_out.fst"))

  def testReplace(self):
    pairs = [(1, self.g1), (2, self.g2), (3, self.g3), (4, self.g4)]
    self.assertEqual(
        pynini.replace(pairs, call_arc_labeling="neither"), self.g_out
    )


class ReprTest(absltest.TestCase):

  # Helper.
  def EpsilonMachine(self):
    f = pynini.Fst()
    f.set_start(f.add_state())
    f.set_final(f.start())
    return f

  def testArcRepr(self):
    arc = pynini.Arc(97, 97, pynini.Weight.one("tropical"), 1)
    self.assertStartsWith(repr(arc), "<Arc at")

  def testArcIteratorRepr(self):
    f = self.EpsilonMachine()
    i = f.arcs(f.start())
    self.assertStartsWith(repr(i), "<_ArcIterator at")

  def testEncodeMapperRepr(self):
    e = pynini.EncodeMapper()
    self.assertStartsWith(repr(e), "<EncodeMapper at")

  def testFarRepr(self):
    tmpfile = os.path.join(absltest.get_default_test_tmpdir(), "test.far")
    try:
      with pynini.Far(tmpfile, mode="w") as f:
        self.assertStartsWith(repr(f), "<sttable Far")
    finally:
      os.remove(tmpfile)

  def testFstRepr(self):
    f = self.EpsilonMachine()
    self.assertStartsWith(repr(f), "<vector Fst")

  def testMPdtParentheses(self):
    m = pynini.MPdtParentheses()
    self.assertStartsWith(repr(m), "<MPdtParentheses")

  def testMutableArcIterator(self):
    f = self.EpsilonMachine()
    i = f.mutable_arcs(f.start())
    self.assertStartsWith(repr(i), "<_MutableArcIterator")

  def testPdtParentheses(self):
    p = pynini.PdtParentheses()
    self.assertStartsWith(repr(p), "<PdtParentheses")

  def testStateIterator(self):
    f = self.EpsilonMachine()
    i = f.states()
    self.assertStartsWith(repr(i), "<_StateIterator")

  def testStringPathIterator(self):
    f = self.EpsilonMachine()
    i = f.paths()
    self.assertStartsWith(repr(i), "<_StringPathIterator at")

  def testSymbolTable(self):
    s = pynini.SymbolTable("Gouda")
    self.assertStartsWith(repr(s), "<SymbolTable")

  def testWeight(self):
    w = pynini.Weight.one("tropical")
    self.assertStartsWith(repr(w), "<tropical Weight")


class ReweightTest(absltest.TestCase):
  testdir: str
  r1: pynini.Fst
  r2: pynini.Fst
  potentials: Iterable[pynini.Weight]

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/reweight"
    )
    cls.r1 = pynini.Fst.read(os.path.join(cls.testdir, "r1.fst"))
    cls.r2 = pynini.Fst.read(os.path.join(cls.testdir, "r2.fst"))
    wt = cls.r1.weight_type()
    cls.potentials = [
        pynini.Weight(wt, 2),
        pynini.Weight(wt, 3),
        pynini.Weight(wt, -1),
    ]

  def testToInitialReweight(self):
    r2 = pynini.Fst.read(os.path.join(self.testdir, "r2.fst"))
    self.assertEqual(pynini.reweight(self.r1, self.potentials), r2)

  def testToFinalReweight(self):
    r3 = pynini.Fst.read(os.path.join(self.testdir, "r3.fst"))
    self.assertEqual(
        pynini.reweight(self.r1, self.potentials, reweight_type="to_final"), r3
    )


class ReverseTest(absltest.TestCase):
  r1: pynini.Fst
  r2: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/reverse"
    )
    cls.r1 = pynini.Fst.read(os.path.join(testdir, "r1.fst"))
    cls.r2 = pynini.Fst.read(os.path.join(testdir, "r2.fst"))

  def testReverse(self):
    self.assertEqual(pynini.reverse(self.r1), self.r2)


class RmEpsilonTest(absltest.TestCase):
  r1: pynini.Fst
  r2: pynini.Fst
  r4: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/rmepsilon"
    )
    cls.r1 = pynini.Fst.read(os.path.join(testdir, "r1.fst"))
    cls.r2 = pynini.Fst.read(os.path.join(testdir, "r2.fst"))
    cls.r4 = pynini.Fst.read(os.path.join(testdir, "r4.fst"))

  def testRmEpsilon(self):
    self.assertEqual(pynini.rmepsilon(self.r1), pynini.rmepsilon(self.r2))

  def testRmEpsilonWithWeightThreshold(self):
    self.assertEqual(pynini.rmepsilon(self.r1, weight=1.0, nstate=10), self.r4)


class ShortestDistanceTest(absltest.TestCase):
  testdir: str
  sd2: pynini.Fst
  sd2_dst: list[str]
  sd2_rdst: list[str]

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/shortest-distance",
    )
    cls.sd2 = pynini.Fst.read(os.path.join(cls.testdir, "sd2.fst"))
    cls.sd2_dst = ["0", "3", "5", "7"]
    cls.sd2_rdst = ["10", "7", "7", "3"]

  def testForwardShortestDistance(self):
    sd2_dst_res = [str(i) for i in pynini.shortestdistance(self.sd2)]
    self.assertEqual(sd2_dst_res, self.sd2_dst)

  def testBackwardShortestDistance(self):
    sd2_rdst_res = [
        str(i) for i in pynini.shortestdistance(self.sd2, reverse=True)
    ]
    self.assertEqual(sd2_rdst_res, self.sd2_rdst)

  def testFifoQueueShortestDistance(self):
    sd2_dst_res = [
        str(i) for i in pynini.shortestdistance(self.sd2, queue_type="fifo")
    ]
    self.assertEqual(sd2_dst_res, self.sd2_dst)

  def testLifoQueueShortestDistance(self):
    sd2_dst_res = [
        str(i) for i in pynini.shortestdistance(self.sd2, queue_type="lifo")
    ]
    self.assertEqual(sd2_dst_res, self.sd2_dst)

  def testShortestFirstQueueShortestDistance(self):
    sd2_dst_res = [
        str(i) for i in pynini.shortestdistance(self.sd2, queue_type="shortest")
    ]
    self.assertEqual(sd2_dst_res, self.sd2_dst)

  def testStateOrderQueueShortestDistance(self):
    sd2_dst_res = [
        str(i) for i in pynini.shortestdistance(self.sd2, queue_type="state")
    ]
    self.assertEqual(sd2_dst_res, self.sd2_dst)

  def testTopOrderQueueShortestDistance(self):
    sd2_dst_res = [
        str(i) for i in pynini.shortestdistance(self.sd2, queue_type="top")
    ]
    self.assertEqual(sd2_dst_res, self.sd2_dst)


class ShortestPathTest(absltest.TestCase):
  testdir: str
  sp1: pynini.Fst
  sp5: pynini.Fst
  sp6: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/shortest-path",
    )
    cls.sp1 = pynini.Fst.read(os.path.join(cls.testdir, "sp1.fst"))
    cls.sp5 = pynini.Fst.read(os.path.join(cls.testdir, "sp5.fst"))
    cls.sp6 = pynini.Fst.read(os.path.join(cls.testdir, "sp6.fst"))

  def testOneBestShortestPath(self):
    sp2 = pynini.Fst.read(os.path.join(self.testdir, "sp2.fst"))
    self.assertEqual(pynini.shortestpath(self.sp1), sp2)

  def testKBestShortestPath(self):
    sp3 = pynini.Fst.read(os.path.join(self.testdir, "sp3.fst"))
    self.assertEqual(pynini.shortestpath(self.sp1, nshortest=4), sp3)

  def testKBestUniqueShortestPath(self):
    sp4 = pynini.Fst.read(os.path.join(self.testdir, "sp4.fst"))
    self.assertEqual(
        pynini.shortestpath(self.sp1, nshortest=4, unique=True), sp4
    )

  def testKBestPrunedShortestPath(self):
    sp9 = pynini.Fst.read(os.path.join(self.testdir, "sp9.fst"))
    self.assertEqual(
        pynini.shortestpath(self.sp5, nshortest=4, nstate=20, weight=1.0), sp9
    )

  def testFifoQueueShortestPath(self):
    self.assertEqual(pynini.shortestpath(self.sp5, queue_type="fifo"), self.sp6)

  def testLifoQueueShortestPath(self):
    self.assertEqual(pynini.shortestpath(self.sp5, queue_type="lifo"), self.sp6)

  def testShortestFirstQueueShortestPath(self):
    self.assertEqual(
        pynini.shortestpath(self.sp5, queue_type="shortest"), self.sp6
    )

  def testStateOrderQueueShortestPath(self):
    self.assertEqual(
        pynini.shortestpath(self.sp5, queue_type="state"), self.sp6
    )

  def testTopOrderQueueShortestPath(self):
    self.assertEqual(pynini.shortestpath(self.sp5, queue_type="top"), self.sp6)


class StateIteratorTest(absltest.TestCase):
  testdir: str

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/compile"
    )

  def testStateIteratorAfterFstDeletion(self):
    f = pynini.Fst.read(os.path.join(self.testdir, "fst.compiled"))
    size = f.num_states()
    siter = f.states()
    del f  # Should be garbage-collected immediately.
    self.assertEqual(list(siter), list(range(size)))


class SymbolTableIteratorTest(absltest.TestCase):
  sym_strings: list[str]

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.sym_strings = [
        "<epsilon>",
        "Sorry",
        "Cheddar",
        "Pont-l'Évêque",
        "Camembert",
    ]

  def testSymbolTableIterator(self):
    table = pynini.SymbolTable()
    for sym in self.sym_strings:
      table.add_symbol(sym)
    # `list(table)` calls `iter(table)` which invokes SymbolTable::iterator.
    self.assertEqual(list(table), list(enumerate(self.sym_strings)))

  def testSymbolTableIteratorNoCrashAfterSymbolTableDeletion(self):
    table = pynini.SymbolTable()
    for sym in self.sym_strings:
      table.add_symbol(sym)
    syms_iter = iter(table)
    del table
    # Here, since this is an owned SymbolTable and the iterator also maintains a
    # reference to it, the underlying SymbolTable will be valid still.
    self.assertEqual(list(syms_iter), list(enumerate(self.sym_strings)))

  def testSymbolTableIteratorNoCrashAfterFstSymbolTableDeletion(self):
    fst = pynini.cross(
        "My hovercraft is full of eels",
        "Mi aerodeslizador está lleno de anguilas",
    )
    table = pynini.SymbolTable()
    for sym in self.sym_strings:
      table.add_symbol(sym)
    fst.set_input_symbols(table)
    attached_syms = fst.input_symbols()
    syms_iter = iter(attached_syms)
    attached_syms_owned = attached_syms.copy()
    fst.set_input_symbols(None)  # Should GC attached syms immediately.
    with self.assertRaises(pynini.FstOpError):
      # Attempting to create an iterator of a SymbolTableView to a detached
      # SymbolTable raises FstOpError.
      list(attached_syms)
    with self.assertRaises(pynini.FstOpError):
      # A successfully created _SymbolTableIterator object maintains a reference
      # to its SymbolTable or SymbolTableView. Thus, it will crash in the same
      # way unless the user does an explicit copy.
      list(syms_iter)
    # An explicit copy will succeed.
    self.assertEqual(
        list(attached_syms_owned), list(enumerate(self.sym_strings))
    )

  def testSymbolTableIteratorNoCrashAfterFstDeletion(self):
    fst = pynini.cross(
        "My hovercraft is full of eels",
        "Mi aerodeslizador está lleno de anguilas",
    )
    table = pynini.SymbolTable()
    for sym in self.sym_strings:
      table.add_symbol(sym)
    fst.set_input_symbols(table)
    del table  # Should be garbage-collected immediately.
    attached_syms = fst.input_symbols()
    syms_iter = iter(attached_syms)
    attached_syms_owned = attached_syms.copy()
    del fst  # The underlying FST is ref-counted and thus shouldn't be GC'd.
    expected_symbols = list(enumerate(self.sym_strings))
    # # A successfully created _FstSymbolTableView extends the life of the
    # underlying FST. Thus, all these operations on SymbolTables attached to
    # deleted FSTs will succeed.
    self.assertEqual(list(attached_syms), expected_symbols)
    self.assertEqual(list(syms_iter), expected_symbols)
    # An explicitly copied SymbolTable will succeed as well.
    self.assertEqual(list(attached_syms_owned), expected_symbols)


class StatemapTest(absltest.TestCase):
  testdir: str

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.testdir = runfiles.test_src_path(
        "openfst/test/testdata/state-map"
    )

  def testArcSumStatemap(self):
    a1 = pynini.Fst.read(os.path.join(self.testdir, "a1.fst"))
    a2 = pynini.Fst.read(os.path.join(self.testdir, "a2.fst"))
    self.assertEqual(pynini.statemap(a1, "arc_sum"), a2)

  def testArcUniqueStatemap(self):
    b1 = pynini.Fst.read(os.path.join(self.testdir, "b1.fst"))
    b2 = pynini.Fst.read(os.path.join(self.testdir, "b2.fst"))
    self.assertEqual(pynini.statemap(b1, "arc_unique"), b2)


class StringTest(absltest.TestCase):
  """Tests string compilation and stringification."""

  cheese: str
  reply: str
  imported_cheese: str
  pynini.acceptor_props: pynini.FstProperties

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.cheese = "Red Leicester"
    cls.reply = "I'm afraid we're fresh out of Red Leicester sir"
    cls.imported_cheese = "Pont l'Evêque"
    cls.acceptor_props = (
        pynini.ACCEPTOR
        | pynini.I_DETERMINISTIC
        | pynini.O_DETERMINISTIC
        | pynini.I_LABEL_SORTED
        | pynini.O_LABEL_SORTED
        | pynini.UNWEIGHTED
        | pynini.ACYCLIC
        | pynini.INITIAL_ACYCLIC
        | pynini.TOP_SORTED
        | pynini.ACCESSIBLE
        | pynini.COACCESSIBLE
        | pynini.STRING
        | pynini.UNWEIGHTED_CYCLES
    )

  def testUnbracketedBytestringUnweightedAcceptorCompilation(self):
    cheese = pynini.accep(self.cheese)
    self.assertEqual(cheese, self.cheese)
    self.assertEqual(
        cheese.properties(self.acceptor_props, True), self.acceptor_props
    )

  def testUnbracketedBytestringUnweightedTransducerCompilation(self):
    exchange = pynini.cross(self.cheese, self.reply)
    exchange.project("input")
    exchange.rmepsilon()
    self.assertEqual(exchange, self.cheese)

  def testUnbracketedBytestringWeightedAcceptorCompilation(self):
    cheese = pynini.accep(self.cheese, weight=pynini.Weight.one("tropical"))
    self.assertEqual(cheese, self.cheese)
    self.assertEqual(
        cheese.properties(self.acceptor_props, True), self.acceptor_props
    )

  def testUnbracketedBytestringCastingWeightedAcceptorCompilation(self):
    cheese = pynini.accep(self.cheese, weight=0)
    self.assertEqual(cheese, self.cheese)
    self.assertEqual(
        cheese.properties(self.acceptor_props, True), self.acceptor_props
    )

  def testUnicodeBytestringAcceptorCompilation(self):
    cheese = pynini.accep(self.imported_cheese)
    self.assertEqual(cheese, self.imported_cheese)
    self.assertEqual(
        cheese.properties(self.acceptor_props, True), self.acceptor_props
    )

  def testAsciiUtf8AcceptorCompilation(self):
    cheese = pynini.accep(self.cheese, token_type="utf8")
    self.assertEqual(cheese, self.cheese)
    self.assertEqual(
        cheese.properties(self.acceptor_props, True), self.acceptor_props
    )

  def testEscapedBracketsBytestringAcceptorCompilation(self):
    ac = pynini.accep(r"[\[Camembert\] is a]\[cheese\]")
    # Should have 3 states accepting generated symbols, 8 accepting a byte,
    # and 1 final state.
    self.assertEqual(ac.num_states(), 12)

  def testEscapeMethodBytestringAcceptorCompilationIdentity(self):
    bracketed_examples = [
        r"[\[Camembert\] is a]\[cheese\]",
        # pylint: disable=anomalous-backslash-in-string
        "[\[Camembert\] is a]\[cheese\]",
        r"\[\[Camembert\] is a\]\[cheese\]",
        # pylint: disable=anomalous-backslash-in-string
        "\[\[Camembert\] is a\]\[cheese\]",
        r"[\[Camembert\] is a][cheese]",
        # pylint: disable=anomalous-backslash-in-string
        "[\[Camembert\] is a][cheese]",
        r"[[Camembert] is a][cheese]",
        "[[Camembert] is a][cheese]",
        r"Camembert is a cheese",
        "Camembert is a cheese",
        r"[ is a character",
        "[ is a character",
        r"\[ is an escaped character",
        # pylint: disable=anomalous-backslash-in-string
        "\[ is an escaped character",
        r"\\[ is a doubly escaped character",
        "\\[ is a doubly escaped character",
        r"\\\[ is a triply escaped character",
        # pylint: disable=anomalous-backslash-in-string
        "\\\[ is a triply escaped character",
        r"\\\\[ is a quadruply escaped character",
        "\\\\[ is a quadruply escaped character",
        r"This, that, and the other thing.",
        "This, that, and the other thing.",
        r"""Puncuation~!@#$%^&*()`-=[]\;',./_+{}|:"<>?.""",
        """Puncuation~!@#$%^&*()`-=[]\;',./_+{}|:"<>?.""",
        r"""Unicode: *Иöñéχıßþęη✝File*""",
        """Unicode: *Иöñéχıßþęη✝File*""",
        r"This string contains an escaped newline.\n",
        "This string contains a newline.\n",
        r"This string contains an escaped tab.\t",
        "This string contains a tab.\t",
        r"This string contains an escaped carriage return.\r",
        "This string contains a carriage return.\r",
        r"This string contains an escaped bell.\a",
        "This string contains a bell.\a",
        r"This string contains an escaped backspace.\b",
        "This string contains a backspace.\b",
        r"This string contains an escaped formfeed.\f",
        "This string contains a formfeed.\f",
        r"This string contains an escaped vertical tab.\v",
        "This string contains a vertical tab.\v",
        r"This string contains an escaped hex code \x6F",
        "This string contains an escaped hex code \x6F",
        r"This string contains an escaped octal code \o627",
        # pylint: disable=anomalous-backslash-in-string
        "This string contains an escaped octal code \o627",
        r"This string contains an escaped unicode codepoint \u200C",
        "This string contains an escaped unicode codepoint \u200C",
    ]

    for word in bracketed_examples:
      self.assertEqual(
          word,
          pynini.accep(pynini.escape(word), token_type="byte").string(
              token_type="byte"
          ),
      )
      self.assertEqual(
          word,
          pynini.accep(pynini.escape(word), token_type="utf8").string(
              token_type="utf8"
          ),
      )

  def testGarbageWeightAcceptorRaisesFstBadWeightError(self):
    with self.assertRaises(pynini.FstBadWeightError):
      pynini.accep(self.cheese, weight="nonexistent")

  def testGarbageArcTypeAcceptorRaisesFstArgError(self):
    with self.assertRaises(pynini.FstArgError):
      pynini.accep(self.cheese, arc_type="nonexistent")

  def testUnbalancedBracketsAcceptorRaisesFstStringCompilationError(self):
    with self.assertRaises(pynini.FstStringCompilationError):
      pynini.accep(self.cheese + "]")

  def testUnbalancedBracketsTransducerRaisesFstStringCompilationError(self):
    with self.assertRaises(pynini.FstStringCompilationError):
      pynini.cross(self.cheese, "[" + self.reply)

  def testCrossProductTransducerCompilation(self):
    cheese = pynini.accep(self.cheese)
    reply = pynini.accep(self.reply)
    exchange = pynini.cross(cheese, reply)
    exchange.project("input")
    exchange.rmepsilon()
    self.assertEqual(exchange, self.cheese)

  def testAsciiByteStringify(self):
    self.assertEqual(pynini.accep(self.cheese).string(), self.cheese)

  def testAsciiUtf8Stringify(self):
    self.assertEqual(
        pynini.accep(self.cheese, token_type="utf8").string("utf8"), self.cheese
    )

  def testUtf8ByteStringify(self):
    self.assertEqual(
        pynini.accep(self.imported_cheese).string(), self.imported_cheese
    )

  def testAsciiByteStringifyAfterSymbolTableDeletion(self):
    ac = pynini.accep(self.cheese)
    ac.set_output_symbols(None)
    self.assertEqual(ac.string(), self.cheese)

  def testUtf8Utf8Stringify(self):
    self.assertEqual(
        pynini.accep(self.imported_cheese, token_type="utf8").string("utf8"),
        self.imported_cheese,
    )

  def testUnicodeByteStringify(self):
    self.assertEqual(
        pynini.accep(self.imported_cheese).string(), self.imported_cheese
    )

  def testUnicodeUtf8Stringify(self):
    self.assertEqual(
        pynini.accep(self.imported_cheese, token_type="utf8").string("utf8"),
        self.imported_cheese,
    )

  def testByteEmptyStringify(self):
    self.assertEqual(
        pynini.accep("", token_type="byte").string(token_type="byte"), ""
    )

  def testUtf8EmptyStringify(self):
    self.assertEqual(
        pynini.accep("", token_type="utf8").string(token_type="utf8"), ""
    )

  def testSymsEmptyStringify(self):
    syms = pynini.SymbolTable()
    self.assertEqual(
        pynini.accep("", token_type=syms).string(token_type=syms), ""
    )

  def testUtf8StringifyAfterSymbolTableDeletion(self):
    ac = pynini.accep(self.imported_cheese, token_type="utf8")
    ac.set_output_symbols(None)
    self.assertEqual(ac.string("utf8"), self.imported_cheese)

  def testStringifyOnNonkStringFstRaisesFstOpError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.union(self.cheese, self.imported_cheese).string()

  def testCompositionOfStringAndLogArcWorks(self):
    cheese = "Greek Feta"
    self.assertEqual(cheese @ pynini.accep(cheese, arc_type="log"), cheese)

  def testCompositionOfLogArcAndStringWorks(self):
    cheese = "Tilsit"
    self.assertEqual(pynini.accep(cheese, arc_type="log") @ cheese, cheese)

  def testCompositionOfStringAndLog64ArcWorks(self):
    cheese = "Greek Feta"
    self.assertEqual(cheese @ pynini.accep(cheese, arc_type="log64"), cheese)

  def testCompositionOfLog64ArcAndStringWorks(self):
    cheese = "Tilsit"
    self.assertEqual(pynini.accep(cheese, arc_type="log64") @ cheese, cheese)

  def testLogWeightToStandardAcceptorRaisesFstStringCompilationError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.accep("Japanese Sage Derby", weight=pynini.Weight.one("log"))

  def testLog64WeightToLogAcceptorRaisesFstStringCompilationError(self):
    with self.assertRaises(pynini.FstOpError):
      pynini.accep(
          "Wensleydale", arc_type="log", weight=pynini.Weight.one("log64")
      )


class StringFileTest(absltest.TestCase):
  map_file: str

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "opengrm/string/testdata"
    )
    cls.map_file = os.path.join(testdir, "str.map")

  def ContainsMapping(self, istring, mapper, ostring):
    lattice = pynini.compose(istring, mapper, compose_filter="alt_sequence")
    lattice.project("output").rmepsilon().arcsort("olabel")
    lattice = pynini.compose(mapper, ostring, compose_filter="sequence")
    self.assertNotEqual(lattice.start(), pynini.NO_STATE_ID)

  def testByteToByteStringFile(self):
    mapper = pynini.string_file(pathlib.Path(self.map_file))
    self.ContainsMapping("[Bel Paese]", mapper, "Sorry")
    self.ContainsMapping("Cheddar", mapper, "Cheddar")
    self.ContainsMapping("Caithness", mapper, "Pont-l'Évêque")
    self.ContainsMapping("Pont-l'Évêque", mapper, "Camembert")

  def testByteToUtf8StringFile(self):
    utf8 = functools.partial(pynini.accep, token_type="utf8")
    mapper = pynini.string_file(self.map_file, output_token_type="utf8")
    self.ContainsMapping("[Bel Paese]", mapper, utf8("Sorry"))
    self.ContainsMapping("Cheddar", mapper, utf8("Cheddar"))
    self.ContainsMapping("Caithness", mapper, utf8("Pont-l'Évêque"))
    self.ContainsMapping("Pont-l'Évêque", mapper, utf8("Camembert"))

  def testUtf8ToUtf8StringFile(self):
    utf8 = functools.partial(pynini.accep, token_type="utf8")
    mapper = pynini.string_file(
        self.map_file, input_token_type="utf8", output_token_type="utf8"
    )
    self.ContainsMapping(utf8("[Bel Paese]"), mapper, utf8("Sorry"))
    self.ContainsMapping(utf8("Pont-l'Évêque"), mapper, utf8("Camembert"))

  def testByteToSymbolStringFile(self):
    syms = pynini.SymbolTable()
    syms.add_symbol("<epsilon>")
    syms.add_symbol("Sorry")
    syms.add_symbol("Cheddar")
    syms.add_symbol("Pont-l'Évêque")
    syms.add_symbol("Camembert")
    mapper = pynini.string_file(self.map_file, output_token_type=syms)
    symc = functools.partial(pynini.accep, token_type=syms)
    self.ContainsMapping("[Bel Paese]", mapper, symc("Sorry"))
    self.ContainsMapping("Pont-l'Évêque", mapper, symc("Camembert"))


class StringMapTest(absltest.TestCase):
  lines: list[tuple[str, ...]]

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    # In-Python version of str.map.
    cls.lines = [
        (
            "[Bel Paese]",
            "Sorry",
        ),
        ("Cheddar",),
        (
            "Caithness",
            "Pont-l'Évêque",
            ".666",
        ),
        (
            "Pont-l'Évêque",
            "Camembert",
        ),
    ]

  def ContainsMapping(self, istring, mapper, ostring):
    lattice = pynini.compose(istring, mapper, compose_filter="alt_sequence")
    lattice.project("output").rmepsilon().arcsort("olabel")
    lattice = pynini.compose(mapper, ostring, compose_filter="sequence")
    self.assertNotEqual(lattice.start(), pynini.NO_STATE_ID)

  def testByteToByteStringMap(self):
    mapper = pynini.string_map(self.lines)
    self.ContainsMapping("[Bel Paese]", mapper, "Sorry")
    self.ContainsMapping("Cheddar", mapper, "Cheddar")
    self.ContainsMapping("Caithness", mapper, "Pont-l'Évêque")
    self.ContainsMapping("Pont-l'Évêque", mapper, "Camembert")

  def testDictionaryStringMap(self):
    mydict = {
        self.lines[0][0]: self.lines[0][1],
        self.lines[1][0]: self.lines[1][0],
    }
    mapper = pynini.string_map(mydict.items())
    self.ContainsMapping("[Bel Paese]", mapper, "Sorry")

  def testByteToUtf8StringMap(self):
    mapper = pynini.string_map(self.lines, output_token_type="utf8")
    utf8 = functools.partial(pynini.accep, token_type="utf8")
    self.ContainsMapping("[Bel Paese]", mapper, utf8("Sorry"))
    self.ContainsMapping("Cheddar", mapper, utf8("Cheddar"))
    self.ContainsMapping("Caithness", mapper, utf8("Pont-l'Évêque"))
    self.ContainsMapping("Pont-l'Évêque", mapper, utf8("Camembert"))

  def testUtf8ToUtf8StringMap(self):
    mapper = pynini.string_map(
        self.lines, input_token_type="utf8", output_token_type="utf8"
    )
    utf8 = functools.partial(pynini.accep, token_type="utf8")
    self.ContainsMapping(utf8("[Bel Paese]"), mapper, utf8("Sorry"))
    self.ContainsMapping(utf8("Pont-l'Évêque"), mapper, utf8("Camembert"))

  def testByteToSymbolStringMap(self):
    syms = pynini.SymbolTable()
    syms.add_symbol("<epsilon>")
    syms.add_symbol("Sorry")
    syms.add_symbol("Cheddar")
    syms.add_symbol("Pont-l'Évêque")
    syms.add_symbol("Camembert")
    mapper = pynini.string_map(self.lines, output_token_type=syms)
    symc = functools.partial(pynini.accep, token_type=syms)
    self.ContainsMapping("[Bel Paese]", mapper, symc("Sorry"))
    self.ContainsMapping("Pont-l'Évêque", mapper, symc("Camembert"))


class StringPathIteratorTest(absltest.TestCase):
  triples: Iterable[tuple[str, str, str]]
  f: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.triples = [
        ("Bel Paese", "Sorry", "4"),
        (
            "Red Windsor",
            "Normally, sir, yes, but today the van broke down.",
            "3",
        ),
        ("Stilton", "Sorry", "2"),
    ]
    cls.f = pynini.string_map(cls.triples)

  def testStringPathIteratorIStrings(self):
    self.assertCountEqual(
        self.f.paths().istrings(), (t[0] for t in self.triples)
    )

  def testStringPathIteratorOStrings(self):
    self.assertCountEqual(
        self.f.paths().ostrings(), (t[1] for t in self.triples)
    )

  def testStringPathIteratorWeights(self):
    self.assertCountEqual(
        (str(w) for w in self.f.paths().weights()),
        (str(t[2]) for t in self.triples),
    )

  def testStringPathIteratorAfterFstDeletion(self):
    cheeses = ("Pipo Crem'", "Fynbo")
    f = pynini.union(*cheeses)
    sp = f.paths()
    del f  # Should be garbage-collected immediately.
    self.assertCountEqual(sp.ostrings(), cheeses)

  def testStringPathLabelsWithEpsilons(self):
    # Note that the Thompson construction for union connects the initial state
    # of the first FST to the initial state of the second FST with an
    # epsilon arc, a fact we take advantage of here.
    cheeses = ["Ilchester", "Limburger"]
    f = pynini.union(*cheeses)
    sp = f.paths()
    self.assertCountEqual(cheeses, sp.ostrings())


class DefaultTokenTypeContextManagerTest(parameterized.TestCase):
  syms: pynini.SymbolTable
  string_fst_map: dict[str, dict[str, pynini.Fst]]

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    cls.syms = pynini.SymbolTable()
    cls.syms.add_symbol("<epsilon>")
    cls.syms.add_symbol("a")
    cls.syms.add_symbol("µ")
    cls.syms.add_symbol("学")
    cls.string_fst_map = {
        "a": {
            "byte": pynini.accep("a", token_type="byte"),
            "utf8": pynini.accep("a", token_type="utf8"),
            "syms": pynini.accep("a", token_type=cls.syms),
        },
        "µ": {
            "byte": pynini.accep("µ", token_type="byte"),
            "utf8": pynini.accep("µ", token_type="utf8"),
            "syms": pynini.accep("µ", token_type=cls.syms),
        },
        "学": {
            "byte": pynini.accep("学", token_type="byte"),
            "utf8": pynini.accep("学", token_type="utf8"),
            "syms": pynini.accep("学", token_type=cls.syms),
        },
    }

  # We use these three strings in the below tests since the each character
  # differs in its UTF-8 representation, requiring 1, 2, and 3 bytes
  # respectively to represent, thus requiring 2, 3, and 4 states respectively to
  # represent in a byte-level FST. On the other hand, each is encoded using only
  # 2 states for both UTF-8 and SymbolTable FSTs, though with different
  # labelings. The UTF-8 FST has Unicode codepoint labels, and the SymbolTable
  # FST uses labels derived from the given SymbolTable.
  @parameterized.parameters(["a", "µ", "学"])
  def testDefaultTokenTypeByteImplicit(self, char: str):
    with pynini.default_token_type("byte"):
      f_implicit_byte = pynini.accep(char)
      paths_implicit_byte = f_implicit_byte.paths()
      self.assertEqual(paths_implicit_byte.ilabels(), list(char.encode()))
      self.assertEqual(paths_implicit_byte.olabels(), list(char.encode()))
      self.assertEqual(f_implicit_byte.num_states(), len(char.encode()) + 1)
      self.assertEqual(f_implicit_byte.string(), char)
      self.assertEqual(f_implicit_byte.string(token_type="byte"), char)

  @parameterized.parameters(["a", "µ", "学"])
  def testDefaultTokenTypeByteExplicit(self, char: str):
    with pynini.default_token_type("byte"):
      f_explicit_byte = pynini.accep(char, token_type="byte")
      paths_explicit_byte = f_explicit_byte.paths()
      self.assertEqual(paths_explicit_byte.ilabels(), list(char.encode()))
      self.assertEqual(paths_explicit_byte.olabels(), list(char.encode()))
      self.assertEqual(f_explicit_byte.num_states(), len(char.encode()) + 1)
      self.assertEqual(f_explicit_byte.string(), char)
      self.assertEqual(f_explicit_byte.string(token_type="byte"), char)

  @parameterized.parameters(["a", "µ", "学"])
  def testDefaultTokenTypeUTF8Implicit(self, char: str):
    with pynini.default_token_type("utf8"):
      f_implicit_utf8 = pynini.accep(char)
      paths_implicit_utf8 = f_implicit_utf8.paths()
      self.assertEqual(paths_implicit_utf8.ilabels(), [ord(char)])
      self.assertEqual(paths_implicit_utf8.olabels(), [ord(char)])
      self.assertEqual(f_implicit_utf8.num_states(), 2)
      self.assertEqual(f_implicit_utf8.string(), char)
      self.assertEqual(f_implicit_utf8.string(token_type="utf8"), char)

  @parameterized.parameters(["a", "µ", "学"])
  def testDefaultTokenTypeUTF8Explicit(self, char: str):
    with pynini.default_token_type("utf8"):
      f_explicit_utf8 = pynini.accep(char, token_type="utf8")
      paths_explicit_utf8 = f_explicit_utf8.paths()
      self.assertEqual(paths_explicit_utf8.ilabels(), [ord(char)])
      self.assertEqual(paths_explicit_utf8.olabels(), [ord(char)])
      self.assertEqual(f_explicit_utf8.num_states(), 2)
      self.assertEqual(f_explicit_utf8.string(), char)
      self.assertEqual(f_explicit_utf8.string(token_type="utf8"), char)

  @parameterized.parameters(["a", "µ", "学"])
  def testDefaultTokenTypeSymbolTableImplicit(self, char: str):
    with pynini.default_token_type(self.syms):
      f_implicit_syms = pynini.accep(char)
      paths_implicit_syms = f_implicit_syms.paths()
      self.assertEqual(paths_implicit_syms.ilabels(), [self.syms.find(char)])
      self.assertEqual(paths_implicit_syms.olabels(), [self.syms.find(char)])
      self.assertEqual(paths_implicit_syms.istring(), char)
      self.assertEqual(paths_implicit_syms.ostring(), char)
      self.assertEqual(f_implicit_syms.num_states(), 2)
      self.assertEqual(f_implicit_syms.string(), char)
      self.assertEqual(f_implicit_syms.string(token_type=self.syms), char)

  @parameterized.parameters(["a", "µ", "学"])
  def testDefaultTokenTypeSymbolTableExplicit(self, char: str):
    with pynini.default_token_type(self.syms):
      f_explicit_syms = pynini.accep(char, token_type=self.syms)
      paths_explicit_syms = f_explicit_syms.paths()
      self.assertEqual(paths_explicit_syms.ilabels(), [self.syms.find(char)])
      self.assertEqual(paths_explicit_syms.olabels(), [self.syms.find(char)])
      self.assertEqual(paths_explicit_syms.istring(), char)
      self.assertEqual(paths_explicit_syms.ostring(), char)
      self.assertEqual(f_explicit_syms.num_states(), 2)
      self.assertEqual(f_explicit_syms.string(), char)
      self.assertEqual(f_explicit_syms.string(token_type=self.syms), char)

  @parameterized.parameters(["a", "µ", "学"])
  def testNestedDefaultTokenTypeContextManagers_ZeroDeep(self, char: str):
    self.assertEqual(pynini.accep(char), self.string_fst_map[char]["byte"])

  @parameterized.parameters(["a", "µ", "学"])
  def testNestedDefaultTokenTypeContextManagers_OneDeep(self, char: str):
    with pynini.default_token_type("utf8"):
      self.assertEqual(pynini.accep(char), self.string_fst_map[char]["utf8"])

  @parameterized.parameters(["a", "µ", "学"])
  def testNestedDefaultTokenTypeContextManagers_TwoDeep(self, char: str):
    with pynini.default_token_type("utf8"):
      with pynini.default_token_type("byte"):
        self.assertEqual(pynini.accep(char), self.string_fst_map[char]["byte"])

  @parameterized.parameters(["a", "µ", "学"])
  def testNestedDefaultTokenTypeContextManagers_TwoDeepAfterInsidePop(
      self, char: str
  ):
    with pynini.default_token_type("utf8"):
      with pynini.default_token_type("byte"):
        pass
      self.assertEqual(pynini.accep(char), self.string_fst_map[char]["utf8"])

  @parameterized.parameters(["a", "µ", "学"])
  def testNestedDefaultTokenTypeContextManagers_TwoDeepAfterOutsidePop(
      self, char: str
  ):
    with pynini.default_token_type("utf8"):
      with pynini.default_token_type(self.syms):
        pass
    self.assertEqual(pynini.accep(char), self.string_fst_map[char]["byte"])

  @parameterized.parameters(["a", "µ", "学"])
  def testDefaultTokenTypeWithExplicitTokenTypeByte(self, char: str):
    with pynini.default_token_type("utf8"):
      self.assertEqual(
          pynini.accep(char, token_type="byte"),
          self.string_fst_map[char]["byte"],
      )

  @parameterized.parameters(["a", "µ", "学"])
  def testDefaultTokenTypeWithExplicitTokenTypeNone(self, char: str):
    with pynini.default_token_type("utf8"):
      self.assertEqual(
          pynini.accep(char, token_type=None), self.string_fst_map[char]["utf8"]
      )

  @parameterized.parameters(["a", "µ", "学"])
  def testDefaultTokenTypeWithExplicitTokenTypeSyms(self, char: str):
    with pynini.default_token_type("utf8"):
      self.assertEqual(
          pynini.accep(char, token_type=self.syms),
          self.string_fst_map[char]["syms"],
      )

  @parameterized.parameters(["a", "µ", "学"])
  def testDecoratedDefaultTokenTypeByte(self, char: str):

    @pynini.default_token_type("byte")
    def MyByteAcceptor(arg: str) -> pynini.Fst:
      return pynini.accep(arg)

    self.assertEqual(MyByteAcceptor(char), self.string_fst_map[char]["byte"])

  @parameterized.parameters(["a", "µ", "学"])
  def testDecoratedDefaultTokenTypeByteInsideContextManager(self, char: str):

    @pynini.default_token_type("byte")
    def MyByteAcceptor(arg: str) -> pynini.Fst:
      return pynini.accep(arg)

    with pynini.default_token_type("utf8"):
      # In an outer default_token_type setting, the decorator should still take
      # priority. This outer context manager should be ignored.
      self.assertEqual(MyByteAcceptor(char), self.string_fst_map[char]["byte"])

  # NOTE: Tests aren't parametrized over `token_type` since this would make
  # pytype not able to check it, and since Pynini is a Cython extension module,
  # having the tests also test typing is paramount.
  @parameterized.parameters(["a", "µ", "学"])
  def testDecoratedDefaultTokenTypeUTF8(self, char: str):

    @pynini.default_token_type("utf8")
    def MyUtf8Acceptor(arg: str) -> pynini.Fst:
      return pynini.accep(arg)

    self.assertEqual(MyUtf8Acceptor(char), self.string_fst_map[char]["utf8"])

  @parameterized.parameters(["a", "µ", "学"])
  def testDecoratedDefaultTokenTypeUTF8InsideContextManager(self, char: str):

    @pynini.default_token_type("utf8")
    def MyUtf8Acceptor(arg: str) -> pynini.Fst:
      return pynini.accep(arg)

    with pynini.default_token_type(self.syms):
      # In an outer default_token_type setting, the decorator should still take
      # priority. This outer context manager should be ignored.
      self.assertEqual(MyUtf8Acceptor(char), self.string_fst_map[char]["utf8"])

  @parameterized.parameters(["a", "µ", "学"])
  def testDecoratedDefaultTokenTypeSymbolTable(self, char: str):

    @pynini.default_token_type(self.syms)
    def MySymsAcceptor(arg: str) -> pynini.Fst:
      return pynini.accep(arg)

    self.assertEqual(MySymsAcceptor(char), self.string_fst_map[char]["syms"])

  @parameterized.parameters(["a", "µ", "学"])
  def testDecoratedDefaultTokenTypeSymbolTableInsideContextManager(
      self, char: str
  ):

    @pynini.default_token_type(self.syms)
    def MySymsAcceptor(arg: str) -> pynini.Fst:
      return pynini.accep(arg)

    with pynini.default_token_type("byte"):
      # In an outer default_token_type setting, the decorator should still take
      # priority. This outer context manager should be ignored.
      self.assertEqual(MySymsAcceptor(char), self.string_fst_map[char]["syms"])

  def testGarbageTokenTypeString(self):
    with self.assertRaises(pynini.FstArgError):
      with pynini.default_token_type("nonexistent"):  # pytype: disable=wrong-arg-types
        pass

  def testGarbageTokenTypeInt(self):
    with self.assertRaises(TypeError):
      with pynini.default_token_type(52):  # pytype: disable=wrong-arg-types
        pass


class SymbolTableTest(absltest.TestCase):

  def testPickleIO(self):
    f = pynini.SymbolTable()
    f.add_symbol("<epsilon>")
    f.add_symbol("Dorset Blue Vinney")
    g = pickle.loads(pickle.dumps(f))
    self.assertEqual(f.labeled_checksum(), g.labeled_checksum())


class SynchronizeTest(absltest.TestCase):
  s1: pynini.Fst
  s2: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/synchronize",
    )
    cls.s1 = pynini.Fst.read(os.path.join(testdir, "s1.fst"))
    cls.s2 = pynini.Fst.read(os.path.join(testdir, "s2.fst"))

  def testSynchronize(self):
    self.assertEqual(pynini.synchronize(self.s1), self.s2)


class TopsortTest(absltest.TestCase):
  t1: pynini.Fst
  t2: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/topsort"
    )
    cls.t1 = pynini.Fst.read(os.path.join(testdir, "t1.fst"))
    cls.t2 = pynini.Fst.read(os.path.join(testdir, "t2.fst"))

  def testTopsort(self):
    self.assertEqual(pynini.topsort(self.t1), self.t2)


class UnionTest(absltest.TestCase):
  u1: pynini.Fst
  u2: pynini.Fst
  u3: pynini.Fst

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    testdir = runfiles.test_src_path(
        "openfst/test/testdata/union"
    )
    cls.u1 = pynini.Fst.read(os.path.join(testdir, "u1.fst"))
    cls.u2 = pynini.Fst.read(os.path.join(testdir, "u2.fst"))
    cls.u3 = pynini.Fst.read(os.path.join(testdir, "u3.fst"))

  def testUnion(self):
    self.assertEqual(pynini.union(self.u1, self.u2), self.u3)

  def testUnionOperator(self):
    self.assertEqual(pynini.union(self.u1 | self.u2), self.u3)


class WeightTest(absltest.TestCase):
  delta: float
  tropical_zero: pynini.Weight
  tropical_half: pynini.Weight
  tropical_one: pynini.Weight
  log_zero: pynini.Weight
  log_half: pynini.Weight
  log_one: pynini.Weight
  log_one_half: pynini.Weight
  log_two: pynini.Weight
  log64_zero: pynini.Weight
  log64_half: pynini.Weight
  log64_one: pynini.Weight
  log64_one_half: pynini.Weight
  log64_two: pynini.Weight

  @classmethod
  def setUpClass(cls):
    super().setUpClass()
    half = -math.log(0.5)
    one_half = -math.log(1.5)
    two = -math.log(2)
    cls.delta = 1.0 / 1024.0
    cls.tropical_zero = pynini.Weight.zero("tropical")
    cls.tropical_half = pynini.Weight("tropical", half)
    cls.tropical_one = pynini.Weight.one("tropical")
    cls.log_zero = pynini.Weight.zero("log")
    cls.log_half = pynini.Weight("log", half)
    cls.log_one = pynini.Weight.one("log")
    cls.log_one_half = pynini.Weight("log", one_half)
    cls.log_two = pynini.Weight("log", two)
    cls.log64_zero = pynini.Weight.zero("log64")
    cls.log64_half = pynini.Weight("log64", half)
    cls.log64_one = pynini.Weight.one("log64")
    cls.log64_one_half = pynini.Weight("log64", one_half)
    cls.log64_two = pynini.Weight("log64", two)

  # Helper.

  def assertApproxEquals(self, w1, w2):
    self.assertAlmostEqual(float(w1), float(w2), delta=self.delta)

  # Tropical weights.

  def testTropicalZeroPlusZeroEqualsZero(self):
    zero = self.tropical_zero
    self.assertEqual(pynini.plus(zero, zero), zero)

  def testTropicalOnePlusOneEqualsOne(self):
    one = self.tropical_one
    self.assertEqual(pynini.plus(one, one), one)

  def testTropicalOnePlusZeroEqualsOne(self):
    one = self.tropical_one
    zero = self.tropical_zero
    self.assertEqual(pynini.plus(one, zero), one)
    self.assertEqual(pynini.plus(zero, one), one)

  def testTropicalHalfPlusHalfEqualsHalf(self):
    half = self.tropical_half
    self.assertEqual(pynini.plus(half, half), half)

  def testTropicalZeroTimesZeroEqualsZero(self):
    zero = self.tropical_zero
    self.assertEqual(pynini.times(zero, zero), zero)

  def testTropicalOneTimesOneEqualsOne(self):
    one = self.tropical_one
    self.assertEqual(pynini.times(one, one), one)

  def testTropicalOneTimesZeroEqualsZero(self):
    one = self.tropical_one
    zero = self.tropical_zero
    self.assertEqual(pynini.times(one, zero), zero)
    self.assertEqual(pynini.times(zero, one), zero)

  def testTropicalHalfTimesOneEqualsHalf(self):
    half = self.tropical_half
    one = self.tropical_one
    self.assertEqual(pynini.times(half, one), half)
    self.assertEqual(pynini.times(one, half), half)

  def testTropicalZeroDivideOneEqualsZero(self):
    zero = self.tropical_zero
    one = self.tropical_one
    self.assertEqual(pynini.divide(zero, one), zero)

  def testTropicalOneDivideZeroRaisesFstBadWeightError(self):
    zero = self.tropical_zero
    one = self.tropical_one
    with self.assertRaises(pynini.FstBadWeightError):
      pynini.divide(one, zero)

  def testTropicalZeroDivideZeroRaisesFstBadWeightError(self):
    zero = self.tropical_zero
    with self.assertRaises(pynini.FstBadWeightError):
      pynini.divide(zero, zero)

  def testTropicalOneDivideOneEqualsOne(self):
    one = self.tropical_one
    self.assertEqual(pynini.divide(one, one), one)

  def testTropicalOneToTheTenthPowerEqualsOne(self):
    one = self.tropical_one
    self.assertEqual(pynini.power(one, 10), one)

  def testTropicalZeroToTheZerothPowerEqualsOne(self):
    zero = self.tropical_zero
    one = self.tropical_one
    self.assertEqual(pynini.power(zero, 0), one)

  # Log weights.

  def testLogZeroPlusZeroEqualsZero(self):
    zero = self.log_zero
    self.assertEqual(pynini.plus(zero, zero), zero)

  def testLogOnePlusOneEqualsTwo(self):
    one = self.log_one
    two = self.log_two
    self.assertApproxEquals(pynini.plus(one, one), two)

  def testLogOnePlusZeroEqualsOne(self):
    one = self.log_one
    zero = self.log_zero
    self.assertEqual(pynini.plus(one, zero), one)
    self.assertEqual(pynini.plus(zero, one), one)

  def testLogHalfPlusHalfEqualsOneHalf(self):
    half = self.log_half
    one = self.log_one
    one_half = self.log_one_half
    self.assertApproxEquals(pynini.plus(half, one), one_half)

  def testLogZeroTimesZeroEqualsZero(self):
    zero = self.log_zero
    self.assertEqual(pynini.times(zero, zero), zero)

  def testLogOneTimesOneEqualsOne(self):
    one = self.log_one
    self.assertEqual(pynini.times(one, one), one)

  def testLogOneTimesZeroEqualsZero(self):
    one = self.log_one
    zero = self.log_zero
    self.assertEqual(pynini.times(one, zero), zero)
    self.assertEqual(pynini.times(zero, one), zero)

  def testLogHalfTimesOneEqualsHalf(self):
    half = self.log_half
    one = self.log_one
    self.assertEqual(pynini.times(half, one), half)
    self.assertEqual(pynini.times(one, half), half)

  def testLogZeroDivideOneEqualsZero(self):
    zero = self.log_zero
    one = self.log_one
    self.assertEqual(pynini.divide(zero, one), zero)

  def testLogOneDivideZeroRaisesBadWeightError(self):
    zero = self.log_zero
    one = self.log_one
    with self.assertRaises(pynini.FstBadWeightError):
      pynini.divide(one, zero)

  def testLogZeroDivideZeroRaisesFstBadWeightError(self):
    zero = self.log_zero
    with self.assertRaises(pynini.FstBadWeightError):
      pynini.divide(zero, zero)

  def testLogOneDivideOneEqualsOne(self):
    one = self.log_one
    self.assertEqual(pynini.divide(one, one), one)

  def testLogOneToTheTenthPowerEqualsOne(self):
    one = self.log_one
    self.assertEqual(pynini.power(one, 10), one)

  def testLogZeroToTheZerothPowerEqualsOne(self):
    zero = self.log_zero
    one = self.log_one
    self.assertEqual(pynini.power(zero, 0), one)

  # Log64 weights.

  def testLog64ZeroPlusZeroEqualsZero(self):
    zero = self.log64_zero
    self.assertEqual(pynini.plus(zero, zero), zero)

  def testLog64OnePlusOneEqualsTwo(self):
    one = self.log64_one
    two = self.log64_two
    self.assertApproxEquals(pynini.plus(one, one), two)

  def testLog64OnePlusZeroEqualsOne(self):
    one = self.log64_one
    zero = self.log64_zero
    self.assertEqual(pynini.plus(one, zero), one)
    self.assertEqual(pynini.plus(zero, one), one)

  def testLog64HalfPlusHalfEqualsOneHalf(self):
    half = self.log64_half
    one = self.log64_one
    one_half = self.log64_one_half
    self.assertApproxEquals(pynini.plus(half, one), one_half)

  def testLog64ZeroTimesZeroEqualsZero(self):
    zero = self.log64_zero
    self.assertEqual(pynini.times(zero, zero), zero)

  def testLog64OneTimesOneEqualsOne(self):
    one = self.log64_one
    self.assertEqual(pynini.times(one, one), one)

  def testLog64OneTimesZeroEqualsZero(self):
    one = self.log64_one
    zero = self.log64_zero
    self.assertEqual(pynini.times(one, zero), zero)
    self.assertEqual(pynini.times(zero, one), zero)

  def testLog64HalfTimesOneEqualsHalf(self):
    half = self.log64_half
    one = self.log64_one
    self.assertEqual(pynini.times(half, one), half)
    self.assertEqual(pynini.times(one, half), half)

  def testLog64ZeroDivideOneEqualsZero(self):
    zero = self.log64_zero
    one = self.log64_one
    self.assertEqual(pynini.divide(zero, one), zero)

  def testLog64OneDivideZeroRaisesFstWeightError(self):
    zero = self.log64_zero
    one = self.log64_one
    with self.assertRaises(pynini.FstBadWeightError):
      pynini.divide(one, zero)

  def testLog64ZeroDivideZeroRaiseFstBadWeightError(self):
    zero = self.log64_zero
    with self.assertRaises(pynini.FstBadWeightError):
      pynini.divide(zero, zero)

  def testLog64OneDivideOneEqualsOne(self):
    one = self.log64_one
    self.assertEqual(pynini.divide(one, one), one)

  def testLog64ToTheTenthPowerEqualsOne(self):
    one = self.log64_one
    self.assertEqual(pynini.power(one, 10), one)

  def testLog64ToTheZerothPowerEqualsOne(self):
    zero = self.log64_zero
    one = self.log64_one
    self.assertEqual(pynini.power(zero, 0), one)


class WorkedExampleTest(absltest.TestCase):

  def testWorkedExample(self):
    pairs = zip(string.ascii_lowercase, string.ascii_uppercase)
    self.upcaser = pynini.string_map(pairs).closure()
    self.downcaser = pynini.invert(self.upcaser)
    awords = "You do have some cheese do you".lower().split()
    for aword in awords:
      result = (aword @ self.upcaser).project("output").optimize()
      self.assertEqual(result, aword.upper())
    cheese = "Parmesan".lower()
    cascade = (
        cheese @ self.upcaser @ self.downcaser @ self.upcaser @ self.downcaser
    )
    self.assertEqual(cascade.string(), cheese)


if __name__ == "__main__":
  absltest.main()
