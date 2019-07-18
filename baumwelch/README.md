This is a C++ library (including associated binaries) which allows the user to
estimate the parameters of a discrete HMM against a language model using the
Baum-Welch algorithm (a special case of the expectation maximization
meta-algorithm). It uses OpenFst finite-state transducers (FSTs) and FST
archives (FARs) as inputs and outputs.

This library is primarily developed by [Kyle Gorman](mailto:kbg@google.com).

# Dependencies

This library depends on:

* A standards-compliant C++ 11 compiler (GCC \>= 4.8 or Clang \>= 700)
* The most recent version of [OpenFst](http://openfst.org) (at the time of
  writing, 1.7.3) built with the `far` and `script` extensions (i.e., built
  with `./configure --enable-grm`) and headers

# Installation instructions

This library uses GNU autotools so one can simply use the standard
`./configure; make; make install` conventions for compilation and installation.

# Binaries

In addition to library code, this package provides two binaries:

* `baumwelchtrain` trains an FST channel model given an FST language model
  and a FAR of ciphertext strings. The exact count-collection logic is
  determined by the semiring of the inputs: in the log semiring (or other
  non-idempotent semirings), it performs true Baum-Welch training, and in the
  tropical semiring (or other idempotent semirings) it performs Viterbi
  training, an approximation.
* `baumwelchdecode` decodes a FAR of ciphertext given an FST language model,
  a trained FST channel model. Once again, the exact decoding method is
  determined by the semiring of the inputs: in the log semiring (or other 
  non-path semirings) it performs forward-backward decoding using an A-star
  search strategy, and in the tropical semiring (or other path semirings) it
  performs Viterbi decoding.

# Documentation

See `doc/README.md`.

# License

This library is released under the Apache license. See `LICENSE` for more
information.

# Interested in contributing?

See `CONTRIBUTING` for more information.

# Mandatory disclaimer

This is not an official Google product.
