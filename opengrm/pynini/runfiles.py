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

"""Utilities for working with resources and runfiles.

This module supports resolving resource paths in different environments:
1. Bazel/Blaze: Uses the official rules_python runfiles library.
2. CMake: Falls back to using the TEST_SRCDIR environment variable.
3. Direct Python Interpreter: Falls back to resolving paths relative to the
   current working directory (using absolute paths).
"""

import os

try:
  from python.runfiles import Runfiles  # pylint: disable=g-import-not-at-top
  _RESOURCES_ROOT = Runfiles.Create()
except ImportError:
  _RESOURCES_ROOT = None

_TEST_SRCDIR = os.environ.get("TEST_SRCDIR")


def _prepend_workspace(relpath: str) -> str:
  """Prepends Bzlmod repo workspace name to relative path."""
  if relpath.startswith("opengrm"):
    relpath = f"opengrm/{relpath}"
  elif relpath.startswith("openfst"):
    relpath = f"com_google_openfst/{relpath}"
  return relpath


def resource_path(relpath: str) -> str:
  """Returns full resource path constructed from relative path.

  Args:
    relpath: Path component relative to the resource root directory.

  Returns:
    Fully qualified path.
  """
  if _RESOURCES_ROOT:
    return _RESOURCES_ROOT.Rlocation(_prepend_workspace(relpath))
  if _TEST_SRCDIR:
    return os.path.join(_TEST_SRCDIR, relpath)
  return os.path.abspath(relpath)


def test_src_path(relpath: str, second_rel_path: str | None = None) -> str:
  """Expands test data path based on a relative path.

  Args:
    relpath: First component of the path relative to the test data directory.
    second_rel_path: Second component of the path relative to the first
      component (potentially empty).

  Returns:
    Fully qualified path in the runfiles directory.
  """
  return resource_path(
      os.path.join(relpath, second_rel_path) if second_rel_path else relpath
  )
