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

import os

from absl.testing import absltest
from opengrm.pynini import runfiles


class RunfilesTest(absltest.TestCase):

  def test_smoke(self):
    fst_file = runfiles.test_src_path(
        "openfst/test/testdata/determinize", "d1.fst"
    )
    self.assertTrue(os.path.exists(fst_file))
    grm_file = runfiles.test_src_path(
        "opengrm/string/testdata/str.map"
    )
    self.assertTrue(os.path.exists(grm_file))


if __name__ == "__main__":
  absltest.main()
