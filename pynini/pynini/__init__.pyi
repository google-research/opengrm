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
#
# Copyright 2016 and onwards Google, Inc.
#
# For general information on the Pynini grammar compilation library, see
# pynini.opengrm.org.

from _pywrapfst import Fst as _Fst
from _pywrapfst import MutableFst as _MutableFst
from _pywrapfst import VectorFst as _VectorFst
from _pywrapfst import Weight as _Weight
from _pywrapfst import SymbolTable as _SymbolTable
from _pywrapfst import SymbolTableView as _SymbolTableView

from _pywrapfst import FstArgError
from _pywrapfst import FstIOError
from _pywrapfst import FstOpError

## Typing imports.
from _pywrapfst import _FarFileModeFlag
from _pywrapfst import _ArcTypeFlag
from _pywrapfst import _FarTypeFlag
from _pywrapfst import _Filename
from _pywrapfst import WeightLike
from _pywrapfst import _Label
from _pywrapfst import _StateId
from _pywrapfst import _SortTypeFlag
from _pywrapfst import _QueueType
from _pywrapfst import _ComposeFilterFlag

from typing import Type, TypeVar, Union, Tuple, Any, Optional, List, Iterable, Iterator, Mapping

# Custom exceptions.
class FstStringCompilationError(FstArgError, ValueError): ...

# Custom types

FstLike = Union[Fst, str]
# TODO(wolfsonkin): Change to use a typing.Literal once Python 3.8 hits.
TokenType = Union[_SymbolTableView, str]

# Helper functions.

T = TypeVar("T", bound="Fst")
class Fst(_VectorFst):
  def __init__(self, arc_type: _ArcTypeFlag = ...): ...
  @classmethod
  def from_pywrapfst(cls: Type[T], fst: _Fst) -> T: ...
  @classmethod
  def read(cls: Type[T], filename: _Filename) -> T: ...
  @classmethod
  def read_from_string(cls: Type[T], state: bytes) -> T: ...
  def __reduce__(self) -> Union[str, Tuple[Any, ...]]: ...
  def paths(self,
            input_token_type: TokenType = ...,
            output_token_type: TokenType = ...) -> StringPathIterator: ...
  def string(self, token_type: TokenType = ...) -> str: ...
  # The following all override their definition in MutableFst.
  def copy(self: T) -> T: ...
  def closure(self: T, lower: int = ..., upper: int = ...) -> T: ...
  @property
  def plus(self) -> Fst: ...
  @property
  def ques(self) -> Fst: ...
  @property
  def star(self) -> Fst: ...
  def concat(self: T, fst2: FstLike) -> T: ...
  def optimize(self: T, compute_props: bool = ...) -> T: ...
  def union(self: T, *fsts2: FstLike) -> T: ...
  # Operator overloads.
  def __eq__(self, other: FstLike) -> bool: ...
  def __ne__(self, other: FstLike) -> bool: ...
  def __add__(self, other: FstLike) -> Fst: ...
  def __sub__(self, other: FstLike) -> Fst: ...
  def __matmul__(self, other: FstLike) -> Fst: ...
  def __or__(self, other: FstLike) -> Fst: ...
  # NOTE: Cython automatically generates the reversed overloads.
  def __req__(self, other: FstLike) -> bool: ...
  def __rne__(self, other: FstLike) -> bool: ...
  def __radd__(self, other: FstLike) -> Fst: ...
  def __rsub__(self, other: FstLike) -> Fst: ...
  def __rmatmul__(self, other: FstLike) -> Fst: ...
  def __ror__(self, other: FstLike) -> Fst: ...

# Functions for FST compilation.

def acceptor(astring: str,
             weight: Optional[WeightLike] = ...,
             arc_type: _ArcTypeFlag = ...,
             token_type: TokenType = ...) -> Fst: ...
def transducer(fst1: FstLike,
               fst2: FstLike,
               weight: Optional[WeightLike] = ...,
               arc_type: _ArcTypeFlag = ...,
               token_type: TokenType = ...) -> Fst: ...
def cdrewrite(
    tau: FstLike,
    lambda_: FstLike,
    rho: FstLike,
    sigma: FstLike,
    # TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
    direction: str = ...,
    # TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
    mode: str = ...
) -> Fst: ...
# TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
def leniently_compose(fst1: FstLike,
                      fst2: FstLike,
                      sigma: FstLike,
                      compose_filter: _ComposeFilterFlag = ...,
                      connect: bool = ...) -> Fst: ...
# TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
def matches(fst1: FstLike,
            fst2: FstLike,
            compose_filter: _ComposeFilterFlag = ...) -> bool: ...
def string_file(filename: _Filename,
                arc_type: _ArcTypeFlag = ...,
                input_token_type: TokenType = ...,
                output_token_type: TokenType = ...) -> Fst: ...
def string_map(lines: Union[Mapping[str, str],
                            Iterable[Iterable[str]]],
               arc_type: _ArcTypeFlag = ...,
               input_token_type: TokenType = ...,
               output_token_type: TokenType = ...) -> Fst: ...
def generated_symbols() -> SymbolTable: ...


# # Decorator for one-argument constructive FST operations.

# NOTE: These are copy-pasta from _pywrapfst.pyx but with
# `s/ifst: Fst/ifst: FstLike/` and `s/-> MutableFst/-> Fst/`.


def arcmap(
    ifst: FstLike,
    delta: float = ...,
    # TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
    map_type: str = ...,
    power: float = ...,
    weight: Optional[WeightLike] = ...) -> Fst: ...
def determinize(
    ifst: FstLike,
    delta: float = ...,
    # TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
    det_type: str = ...,
    nstate: _StateId = ...,
    subsequential_label: _Label = ...,
    weight: Optional[WeightLike] = ...,
    increment_subsequential_label: bool = ...) -> Fst: ...
def disambiguate(ifst: FstLike,
                 delta: float = ...,
                 nstate: _StateId = ...,
                 subsequential_label: _Label = ...,
                 weight: Optional[WeightLike] = ...) -> Fst: ...
def epsnormalize(ifst: FstLike, eps_norm_output: bool = ...) -> Fst: ...
def prune(ifst: FstLike,
          delta: float = ...,
          nstate: _StateId = ...,
          weight: Optional[WeightLike] = ...) -> Fst: ...
def push(ifst: FstLike,
         delta: float = ...,
         push_weights: bool = ...,
         push_labels: bool = ...,
         remove_common_affix: bool = ...,
         remove_total_weight: bool = ...,
         to_final: bool = ...) -> Fst: ...
def randgen(
    ifst: FstLike,
    npath: int = ...,
    # TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
    select: str = ...,
    max_length: int = ...,
    weighted: bool = ...,
    remove_total_weight: bool = ...,
    seed: int = ...) -> Fst: ...
def reverse(ifst: FstLike, require_superinitial: bool = ...) -> Fst: ...
def shortestpath(
    ifst: FstLike,
    delta: float = ...,
    nshortest: int = ...,
    nstate: _StateId = ...,
    queue_type: _QueueType = ...,
    unique: bool = ...,
    weight: Optional[WeightLike] = ...) -> Fst: ...
# TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
def statemap(ifst: FstLike, map_type: str) -> Fst: ...
def synchronize(ifst: FstLike) -> Fst: ...

# NOTE: This are copy-pasta from _pywrapfst.pyx but with
# `s/ifst: Fst/fst: FstLike/`.

def shortestdistance(
    ifst: FstLike,
    delta: float = ...,
    nstate: _StateId = ...,
    queue_type: _QueueType = ...,
    reverse: bool = ...) -> List[Weight]: ...

# # Two-argument constructive FST operations. If just one of the two FST
# # arguments has been compiled, the arc type of the compiled argument is used to
# # determine the arc type of the not-yet-compiled argument.

# NOTE: These are copy-pasta from _pywrapfst.pyx but with
# `s/ifst(\d): Fst/fst\1: FstLike/` and `s/-> MutableFst/-> Fst/`.


def compose(
    fst1: FstLike,
    fst2: FstLike,
    compose_filter: _ComposeFilterFlag = ...,
    connect: bool = ...) -> Fst: ...
def intersect(fst1: FstLike,
              fst2: FstLike,
              compose_filter: _ComposeFilterFlag = ...,
              connect: bool = ...) -> Fst: ...
# NOTE: This is copy-pasta from _pywrapfst.pyx but with
# `s/ifst(\d): Fst/fst\1: FstLike/` and `s/-> MutableFst/-> Fst/`.


def difference(
    fst1: FstLike,
    fst2: FstLike,
    compose_filter: _ComposeFilterFlag = ...,
    connect: bool = ...) -> Fst: ...
# # Simple comparison operations.

# NOTE: This is copy-pasta from _pywrapfst.pyx but with
# `s/ifst(\d): Fst/fst\1: FstLike/` and `s/-> MutableFst/-> Fst/`.

def equal(fst1: FstLike, fst2: FstLike, delta: float = ...) -> bool: ...
def equivalent(fst1: FstLike, fst2: FstLike, delta: float = ...) -> bool: ...
def isomorphic(fst1: FstLike, fst2: FstLike, delta: float = ...) -> bool: ...
def randequivalent(
    fst1: FstLike,
    fst2: FstLike,
    npath: int = ...,
    delta: float = ...,
    # TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
    select: str = ...,
    max_length: int = ...,
    seed: int = ...) -> bool: ...
############################################################

def concat(fst1: FstLike, fst2: FstLike) -> Fst: ...
def replace(
    pairs: Iterable[Tuple[int, Fst]],
    # TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
    call_arc_labeling: str = ...,
    # TODO(wolfsonkin): Use typing.Literal when Python 3.8 hits.
    return_arc_labeling: str = ...,
    epsilon_on_replace: bool = ...,
    return_label: _Label = ...) -> Fst: ...
def union(*fsts: FstLike) -> Fst: ...
# Pushdown transducer classes and operations.

class PdtParentheses:
  def __repr__(self) -> str: ...
  def __len__(self) -> int: ...
  def __iter__(self) -> Iterator[Tuple[int, int]]: ...
  def copy(self) -> PdtParentheses: ...
  def add_pair(self, push: int, pop: int) -> None: ...
  @classmethod
  def read(cls, filename: _Filename) -> PdtParentheses: ...
  def write(self, filename: _Filename) -> None: ...

def pdt_compose(fst1: FstLike,
                fst2: FstLike,
                parens: PdtParentheses,
                compose_filter: _ComposeFilterFlag = ...,
                left_pdt: bool = ...) -> Fst: ...
def pdt_expand(fst: FstLike,
               parens: PdtParentheses,
               connect: bool = ...,
               keep_parentheses: bool = ...,
               weight: Optional[WeightLike] = ...) -> Fst: ...
def pdt_replace(
    pairs: Iterable[Tuple[int, FstLike]],
    pdt_parser_type: str = ...,
    start_paren_labels: _Label = ...,
    left_paren_prefix: str = ...,
    right_paren_prefix: str = ...) -> Tuple[Fst, PdtParentheses]: ...
def pdt_reverse(fst: FstLike, parens: PdtParentheses) -> Fst: ...
def pdt_shortestpath(fst: FstLike,
                     parens: PdtParentheses,
                     queue_type: _QueueType = ...,
                     keep_parentheses: bool = ...,
                     path_gc: bool = ...) -> Fst: ...

# Multi-pushdown transducer classes and operations.
class MPdtParentheses(object):
  def __repr__(self) -> str: ...
  def __len__(self) -> int: ...
  def __iter__(self) -> Iterator[Tuple[_Label, _Label, _Label]]: ...
  def copy(self) -> MPdtParentheses: ...
  def add_triple(self, push: _Label, pop: _Label, assignment: _Label) -> None: ...
  @classmethod
  def read(cls, filename: _Filename) -> MPdtParentheses: ...
  def write(self, filename: _Filename) -> None: ...

def mpdt_compose(fst1: FstLike,
                 fst2: FstLike,
                 parens: MPdtParentheses,
                 compose_filter: _ComposeFilterFlag = ...,
                 left_mpdt: bool = ...) -> Fst: ...
def mpdt_expand(fst: FstLike,
                parens: MPdtParentheses,
                connect: bool = ...,
                keep_parentheses: bool = ...) -> Fst: ...
def mpdt_reverse(fst: FstLike,
                 parens: MPdtParentheses) -> Tuple[Fst, MPdtParentheses]: ...

class StringPathIterator:
  def __repr__(self) -> str: ...
  def __init__(self,
               fst: FstLike,
               input_token_type: TokenType = ...,
               output_token_type: TokenType = ...) -> None: ...
  def done(self) -> bool: ...
  def error(self) -> bool: ...
  def ilabels(self) -> List[_Label]: ...
  def olabels(self) -> List[_Label]: ...
  def istring(self) -> str: ...
  def istrings(self) -> Iterator[str]: ...
  def items(self) -> Iterator[Tuple[str, str, Weight]]: ...
  def next(self) -> None: ...
  def reset(self) -> None: ...
  def ostring(self) -> str: ...
  def ostrings(self) -> Iterator[str]: ...
  def weight(self) -> Weight: ...
  def weights(self) -> Iterator[Weight]: ...

class Far(object):
  def __init__(self,
               filename: _Filename,
               mode: _FarFileModeFlag = ...,
               arc_type: _ArcTypeFlag = ...,
               far_type: _FarTypeFlag = ...) -> None: ...
  def error(self) -> bool: ...
  # TODO(wolfsonkin): Maybe just return string.
  def arc_type(self) -> _ArcTypeFlag: ...
  def closed(self) -> bool: ...
  # TODO(wolfsonkin): Maybe just return string.
  # TODO(wolfsonkin): If we switch to typing.Literal, take into account that
  # this can return the literal "closed".
  def far_type(self) -> _FarTypeFlag: ...
  def mode(self) -> _FarFileModeFlag: ...
  def name(self) -> str: ...
  def done(self) -> bool: ...
  def find(self, key: str) -> bool: ...
  def get_fst(self) -> Fst: ...
  def get_key(self) -> str: ...
  def next(self) -> None: ...
  def reset(self) -> None: ...
  def __getitem__(self, key: str) -> Fst: ...
  def add(self, key: str, fst: Fst) -> None: ...
  # TODO(wolfsonkin): Make this support FstLikeing.
  def __setitem__(self, key: str, fst: Fst) -> None: ...
  def close(self) -> None: ...
  # Adds support for use as a PEP-343 context manager.
  def __enter__(self) -> Far: ...
  # TODO(wolfsonkin): Add typing to this.
  # See https://github.com/python/typeshed/blob/master/stdlib/2and3/builtins.pyi#L1664
  # for more detail.
  def __exit__(self, exc, value, tb): ...
## PYTHON IMPORTS.

# Classes from _pywrapfst.

from _pywrapfst import Arc
from _pywrapfst import ArcIterator
from _pywrapfst import EncodeMapper
from _pywrapfst import MutableArcIterator
from _pywrapfst import StateIterator
from _pywrapfst import SymbolTable
from _pywrapfst import Weight

# Exceptions not yet imported.
from _pywrapfst import FstBadWeightError
from _pywrapfst import FstIndexError

# FST constants.
from _pywrapfst import NO_LABEL
from _pywrapfst import NO_STATE_ID
from _pywrapfst import NO_SYMBOL

# FST properties.
from _pywrapfst import ACCEPTOR
from _pywrapfst import ACCESSIBLE
from _pywrapfst import ACYCLIC
from _pywrapfst import ADD_ARC_PROPERTIES
from _pywrapfst import ADD_STATE_PROPERTIES
from _pywrapfst import ADD_SUPERFINAL_PROPERTIES
from _pywrapfst import ARC_SORT_PROPERTIES
from _pywrapfst import BINARY_PROPERTIES
from _pywrapfst import COACCESSIBLE
from _pywrapfst import COPY_PROPERTIES
from _pywrapfst import CYCLIC
from _pywrapfst import DELETE_ARC_PROPERTIES
from _pywrapfst import DELETE_STATE_PROPERTIES
from _pywrapfst import EPSILONS
from _pywrapfst import ERROR
from _pywrapfst import EXPANDED
from _pywrapfst import EXTRINSIC_PROPERTIES
from _pywrapfst import FST_PROPERTIES
from _pywrapfst import I_DETERMINISTIC
from _pywrapfst import I_EPSILONS
from _pywrapfst import I_LABEL_INVARIANT_PROPERTIES
from _pywrapfst import I_LABEL_SORTED
from _pywrapfst import INITIAL_ACYCLIC
from _pywrapfst import INITIAL_CYCLIC
from _pywrapfst import INTRINSIC_PROPERTIES
from _pywrapfst import MUTABLE
from _pywrapfst import NEG_TRINARY_PROPERTIES
from _pywrapfst import NO_EPSILONS
from _pywrapfst import NO_I_EPSILONS
from _pywrapfst import NON_I_DETERMINISTIC
from _pywrapfst import NON_O_DETERMINISTIC
from _pywrapfst import NO_O_EPSILONS
from _pywrapfst import NOT_ACCEPTOR
from _pywrapfst import NOT_ACCESSIBLE
from _pywrapfst import NOT_COACCESSIBLE
from _pywrapfst import NOT_I_LABEL_SORTED
from _pywrapfst import NOT_O_LABEL_SORTED
from _pywrapfst import NOT_STRING
from _pywrapfst import NOT_TOP_SORTED
from _pywrapfst import NULL_PROPERTIES
from _pywrapfst import O_DETERMINISTIC
from _pywrapfst import O_EPSILONS
from _pywrapfst import O_LABEL_INVARIANT_PROPERTIES
from _pywrapfst import O_LABEL_SORTED
from _pywrapfst import POS_TRINARY_PROPERTIES
from _pywrapfst import RM_SUPERFINAL_PROPERTIES
from _pywrapfst import SET_ARC_PROPERTIES
from _pywrapfst import SET_FINAL_PROPERTIES
from _pywrapfst import SET_START_PROPERTIES
from _pywrapfst import STATE_SORT_PROPERTIES
from _pywrapfst import STRING
from _pywrapfst import TOP_SORTED
from _pywrapfst import TRINARY_PROPERTIES
from _pywrapfst import UNWEIGHTED
from _pywrapfst import UNWEIGHTED_CYCLES
from _pywrapfst import WEIGHTED
from _pywrapfst import WEIGHTED_CYCLES
from _pywrapfst import WEIGHT_INVARIANT_PROPERTIES

# Arc iterator properties.
from _pywrapfst import ARC_FLAGS
from _pywrapfst import ARC_I_LABEL_VALUE
from _pywrapfst import ARC_NEXT_STATE_VALUE
from _pywrapfst import ARC_NO_CACHE
from _pywrapfst import ARC_O_LABEL_VALUE
from _pywrapfst import ARC_VALUE_FLAGS
from _pywrapfst import ARC_WEIGHT_VALUE

# Encode mapper properties.
from _pywrapfst import ENCODE_FLAGS
from _pywrapfst import ENCODE_LABELS
from _pywrapfst import ENCODE_WEIGHTS

# NOTE: The following are copy-pasta from _pywrapfst.pyx but with
# `s/self: T/fst: FstLike/` and `s/-> T/-> Fst/`.

def arcsort(fst: FstLike, sort_type: _SortTypeFlag = ...) -> Fst: ...
def closure(fst: FstLike, lower: int = ..., upper: int = ...) -> Fst: ...
def connect(fst: FstLike) -> Fst: ...
def decode(fst: FstLike, mapper: EncodeMapper) -> Fst: ...
def encode(fst: FstLike, mapper: EncodeMapper) -> Fst: ...
def invert(fst: FstLike) -> Fst: ...
def minimize(fst: FstLike,
             delta: float = ...,
             allow_nondet: bool = ...) -> Fst: ...
def optimize(fst: FstLike, compute_props: bool = ...) -> Fst: ...
def project(fst: FstLike, project_output: bool = ...) -> Fst: ...
def relabel_pairs(
    fst: FstLike,
    ipairs: Optional[Iterable[Tuple[_Label, _Label]]] = ...,
    opairs: Optional[Iterable[Tuple[_Label, _Label]]] = ...) -> Fst: ...
def relabel_tables(fst: FstLike,
                   old_isymbols: Optional[_SymbolTableView] = ...,
                   new_isymbols: Optional[_SymbolTableView] = ...,
                   unknown_isymbol: str = ...,
                   attach_new_isymbols: bool = ...,
                   old_osymbols: Optional[_SymbolTableView] = ...,
                   new_osymbols: Optional[_SymbolTableView] = ...,
                   unknown_osymbol: str = ...,
                   attach_new_osymbols: bool = ...) -> Fst: ...
def reweight(fst: FstLike,
             potentials: Iterable[WeightLike],
             to_final: bool = ...) -> Fst: ...
def rmepsilon(fst: FstLike,
              queue_type: _QueueType = ...,
              connect: bool = ...,
              weight: Optional[WeightLike] = ...,
              nstate: _StateId = ...,
              delta: float = ...) -> Fst: ...
def topsort(fst: FstLike) -> Fst: ...
#############################################

from _pywrapfst import compact_symbol_table
from _pywrapfst import merge_symbol_table

from _pywrapfst import divide
from _pywrapfst import power
from _pywrapfst import plus
from _pywrapfst import times

# Single-char aliases for the biggest three functions.

a = acceptor
t = transducer
u = union

