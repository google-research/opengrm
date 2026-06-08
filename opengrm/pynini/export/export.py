"""Helper classes for generating FAR files with Pynini."""

import os
from typing import Union

from absl import logging
from opengrm.pynini import pynini

_Filename = Union[str, os.PathLike]


class Exporter:
  """Helper class to collect and export FSTs into an FAR archive file.

  For example, the following code collects 2 FSTs and writes them upon
  destruction of the involved exporter instance.

    exporter = grm.Exporter(filename)
    exporter.export("FST1", fst1)
    exporter.export("FST2", fst2)

  Typically, instead of explicitly creating an Exporter, a client will use the
  exporter provided as the argument to the generator_main function.
  """

  def __init__(
      self,
      filename: _Filename,
      arc_type: str = "standard",
      far_type: pynini.FarType = "default",
  ) -> None:
    """Creates an exporter that writes a FAR archive file upon destruction.

    Args:
      filename: A string with the filename.
      arc_type: A string with the arc type; one of: "standard", "log", "log64".
      far_type: A string with the file type; one of: "default", "sstable",
        "sttable", "stlist".
    """
    logging.info("Setting up exporter for %r", filename)
    self._fsts = {}
    self._filename = os.fspath(filename)
    self._arc_type = arc_type
    self._far_type = far_type
    self._is_open = True

  def __setitem__(self, name: str, fst: pynini.Fst) -> None:
    """Register an FST under a given name to be saved into the FST archive.

    Args:
      name: A string with the name of the fst.
      fst: An FST to be stored.
    """
    assert self._is_open
    logging.info("Adding FST %r to archive %r", name, self._filename)
    self._fsts[name] = fst

  def close(self) -> None:
    """Writes the registered FSTs into the given file and closes it."""
    assert self._is_open
    logging.info("Writing FSTs into %r", self._filename)
    with pynini.Far(
        self._filename, "w", arc_type=self._arc_type, far_type=self._far_type
    ) as sink:
      for name in sorted(self._fsts):
        logging.info("Writing FST %r to %r", name, self._filename)
        sink[name] = self._fsts[name]
    logging.info("Writing FSTs to %r complete", self._filename)
    self._is_open = False
