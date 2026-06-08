"""Utilities for working with resources and runfiles."""

import os

from python.runfiles import Runfiles

_RESOURCES_ROOT = Runfiles.Create()


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
  return _RESOURCES_ROOT.Rlocation(_prepend_workspace(relpath))


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
