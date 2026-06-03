# FST paths

This directory contains classes used to iterate over paths in an FST. They can
be accessed as C++ template classes, or in Python via Pynini.

*For further information, UTSL.*

## `fst::PathIterator`

This template class provides a basic iterator over paths. It is constructed from
a `const Fst<Arc> &` and an optional argument that specifies whether or not the
FST should be checked for acyclicity during construction.

The `ILabels()` and `OLabels()` methods return vectors of input and output
labels (respectively) of the current path, and the `Weight()` method returns the
weight of the current path.

This iterator:

*   is advanced using the `Next()` method,
*   is exhausted once the `Done()` returns `true`,
*   is reset using the `Reset()` method, and
*   signals an error using the `Error()` method.

The acyclity check should only be disabled when the caller can ensure finite
iteration (e.g., by knowing the FST is acyclic, or limiting the number of
iterated paths).

## `fst::StringPathIterator`

This template class is a subclass of `fst::PathIterator` which adds `IString()`
and `OString()` methods returning input and output strings. These methods use
the same string printing methods as `fst::PrintString` and the printing mode is
controlled by the enum `fst::TokenType`.

At construction time, users specify printing modes and/or symbol tables for the
input and output sides of the FST, and whether or not the FST should be checked
for acyclity.

This iterator:

*   is advanced using the `Next()` method,
*   is exhausted once the `Done()` returns `true`,
*   is reset using the `Reset()` method, and
*   signals an error using the `Error()` method.

The acyclity check should only be disabled when the caller can ensure finite
iteration (e.g., by knowing the FST is acyclic, or limiting the number of
iterated paths).

## `nlp.grm.language.pynini.StringPathIterator`

Pynini wraps `fst::script::StringPathIteratorClass` with the Python class
`StringPathIterator`. As in C++, this object can be constructed directly from an
FST argument, or it can be constructed using the `paths()` method of an
(acyclic) `nlp.grm.language.pynini.Fst` instance.

Both construction methods take token types for both input and output sides; for
the `TokenType::SYMBOL` token type, simply provide a
`nlp.grm.language.pynini.SymbolTable` argument.

This class provides the normal iterator methods (`next()`, `done()`, `reset()`,
and `error()`), the accessor methods (`ilabels()`, `istring()`, `olabels()`,
`ostring()`, `weight()`), and speciality iterators (`istrings()`, `ostrings()`,
and `weights()`; `items()` returns triples of input string, output string, and
weight).
