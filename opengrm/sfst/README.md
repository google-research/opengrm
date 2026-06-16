# SFst: Stochastic finite-state transducer library

SFst is a library for normalizing, sampling, combining, and approximating
stochastic (or probabilistic) finite-state transducers. It makes use of
functionality in the
[OpenFst library](https://github.com/google-research/openfst) to create and
manipulate language models encoded as weighted FSTs under two strict
properties:

1. **A canonical topology**: Supports explicit epsilon and failure backoff
   transitions (typically using `fst::kNoLabel`).
2. **A normalized distribution**: Assigns a mathematically exact negative log
   probability to paths leaving a state.

## Definitions

A **Canonical stochastic FST (SFST)** is a weighted finite-state transducer
(represented in OpenFst format) that satisfies the following conditions:

*   **Canonical topology**:
    *   The arcs at each state are sorted by input label.
    *   There is at most one failure (backoff) transition (typically labeled
        `fst::kNoLabel`) per state.
    *   There are no cycles consisting solely of failure transitions and/or
        epsilon transitions.
    *   No assumption is made of general determinism (unlike standard canonical
        n-gram models).
    *   Epsilon transitions are treated as regular symbols by failure
        transitions (each instance behaves as if uniquely labeled).
*   **Normalized distribution**:
    *   Assigns a mathematically exact negative log probability to each
        path leaving a state (weights sum to 1.0 in log semiring).

### Additional terminology

*   **Failure transitions (`phi_label`)**: Transitions traversed only when no
    regular matching transition exists for an input label. They do not consume
    input symbol tokens. Within a canonical SFST, regular epsilon transitions
    are treated by failure transitions as ordinary symbols.
*   **Epsilon transition (`<epsilon>`)**: A standard arc that consumes no input
    symbols. Within a canonical SFST, regular epsilon transitions are treated by
    failure transitions as ordinary consuming symbols.
*   **Backoff-complete FST**: An automaton containing structural failure arcs
    that provide complete fallback routing for unobserved symbol sequences.

## Core operations and binaries

Care must be taken that the input FSTs meet specified requirements (e.g.,
canonical, backoff-complete, or normalized). Binary commands typically verify
their input requirements are satisfied or raise an error, whereas C++ library
versions may omit checks for efficiency.

*   **`sfstapprox`**: Approximates a stochastic FST as a backoff-complete FST.
*   **`sfstcount`**: Counts paths from a stochastic FST with respect to a
    backoff-complete FST.
*   **`sfstinfo`**: Prints structural and topological summaries for canonical
    SFST models.
*   **`sfstintersect`**: Intersects two canonical stochastic FSAs.
*   **`sfstmerge`**: Performs in-memory linear interpolation or Bayesian
    merging of two language models, automatically calculating and normalizing
    state-posteriors and backoff weights.
*   **`sfstngramapprox`**: Approximates an arbitrary weighted FST into a
    canonical and normalized n-gram topology of a specified maximum order.
*   **`sfstngramcount`**: Counts n-grams from a FAR corpus, producing an FST
    with raw counts.
*   **`sfstngramprint`**: De-serializes a canonical SFST back into a gap-free,
    compliant ARPA format using a shortest word-path BFS traversal to extract
    histories.
*   **`sfstngramread`**: Compiles an ARPA-format language model file into a
    canonical, gap-filled stochastic FST.
*   **`sfstngramsymbols`**: Generates an OpenFst-style symbol table from a
    text corpus.
*   **`sfstnormalize`**: Gives an FST the weight distribution of a stochastic
    FST. Supports global normalization, local normalization, Kullback-Leibler
    (KL) minimization constraints (`--method=kl_min`), and standard
    backoff-summed normalization (`--method=summed`).
*   **`sfstperplexity`**: Evaluates the perplexity and out-of-vocabulary (OOV)
    rate of a dataset against an estimated language model.
*   **`sfstrandgen`**: Samples random paths from an SFST.
*   **`sfstshortestdistance`**: Computes the shortest distance taking failure
    transitions into account.
*   **`sfstshrink`**: Prunes n-gram models using list-based, count-based,
    Seymore-based, or exact Stolcke-based KL divergence thresholding.
*   **`sfstsmooth`**: Estimates and populates smoothed backoff language models.
    Supports absolute discounting, Katz, Kneser-Ney, Witten-Bell, and
    unsmoothed models.
*   **`sfsttopology`**: Constructs specific structured FST target topologies.
*   **`sfsttrim`**: Prunes useless (unreachable/dead-end) states and
    transitions in stochastic automata.

## Backoff label conventions

By default, this library utilizes **`fst::kNoLabel`** for its failure and
backoff arcs, ensuring clean separation between structural epsilons and
failure paths.

## Detailed quick tour

This tour is organized around the stages of n-gram model creation, modification,
and use, using Oscar Wilde's *The Importance of Being Earnest* as an example
corpus.

### 1. Corpus I/O

Text corpora are represented as binary finite-state archives (FAR), with one
automaton per sentence.

First, generate a symbol table for the text tokens in the input corpus:

```bash
sfstngramsymbols earnest.txt earnest.syms
```

By default, `sfstngramsymbols` creates symbol table entries for `<epsilon>`
and `<UNK>`.

Next, convert the text corpus to a binary FAR archive:

```bash
farcompilestrings --fst_type=compact --symbols=earnest.syms --keep_symbols earnest.txt earnest.far
```

You can print the FAR back to text:

```bash
farprintstrings earnest.far > earnest.txt.out
```

### 2. N-gram counting

Count n-grams from the FAR corpus. By default, the order is 3 (trigram). We can
specify 5-gram:

```bash
sfstngramcount --order=5 earnest.far earnest.cnts
```

This produces an FST with raw counts.

### 3. Parameter estimation (Smoothing)

Estimate and smooth the model. We use Katz smoothing as an example:

```bash
sfstsmooth --method=katz earnest.cnts earnest.mod
```

Other methods include `witten_bell`, `absolute`, `kneser_ney`, `presmoothed`,
and `unsmoothed`.

We can generate a random sentence from this model:

```bash
sfstrandgen earnest.mod > sentence.fst
farprintstrings sentence.fst
```

### 4. Model merging and pruning

To merge two count files or models (e.g., from different corpus splits):

```bash
sfstmerge model1.fst model2.fst merged.fst
```

To prune/shrink the model (e.g., using Stolcke pruning with a threshold):

```bash
sfstshrink --method=stolcke --theta=0.00015 earnest.mod earnest.pru
```

### 5. Model reading, printing, and info

To print the model in ARPA format:

```bash
sfstngramprint earnest.mod earnest.ARPA
```

To read an ARPA format back to FST:

```bash
sfstngramread earnest.ARPA earnest.mod.from_arpa
```

To print information about the model:

```bash
sfstinfo earnest.mod
```

### 6. Model evaluation

To evaluate the model on a test set (e.g., computing perplexity):

First, compile the test sentences:

```bash
echo -e "A HAND BAG\nBAG HAND A" | farcompilestrings --generate_keys=1 --symbols=earnest.syms --keep_symbols > eval.far
```

Then run perplexity:

```bash
sfstperplexity earnest.mod eval.far
```

## Model approximation

Model approximation allows you to take a model and project/approximate it
onto a target topology (e.g., a different model's topology).

### Idempotency check
Approximating a model onto its own topology should yield the same model:

```bash
sfstapprox earnest.mod earnest.mod > earnest.approx
sfstperplexity earnest.approx eval.far
```

Alternative two-step approximation (count then normalize):

```bash
sfstcount earnest.mod earnest.mod > earnest.approx_cnts
sfstnormalize --method=kl_min earnest.approx_cnts > earnest.approx2
sfstperplexity earnest.approx2 eval.far
```

### Test set-based target topology
Approximate the training model onto the topology of a test set model.
First, build a model for the test set (using `eval.far` as test set here):

```bash
sfstngramcount --order=5 eval.far eval.cnts
sfstsmooth --method=katz eval.cnts eval.mod
```

Now approximate the training model `earnest.mod` onto the test model
`eval.mod` topology:

```bash
sfstapprox earnest.mod eval.mod > earnest.eval_topology.approx
sfstperplexity earnest.eval_topology.approx eval.far
```

### Pruned target topology
Approximate a model onto a pruned model's topology:

```bash
sfstapprox earnest.mod earnest.pru > earnest.pru.approx
sfstperplexity earnest.pru.approx eval.far
```

## Using the C++ Library

To use SFst in C++, include `third_party/opengrm/sfst/sfstlib.h` (which includes
most headers) or specific headers like `third_party/opengrm/sfst/trim.h`,
`third_party/opengrm/sfst/smooth.h`, etc.

Link against `//third_party/opengrm/sfst:sfst`.

All classes and functions are in the `sfst` namespace.

### Core classes and functions

*   **`sfst::PhiAccess<Arc>`**: Computes and manages phi-inaccessible
    transitions. It requires an `ExpandedFst`.

    ```cpp
    #include "third_party/opengrm/sfst/trim.h"
    // ...
    sfst::PhiAccess<StdArc> phi_access(fst, phi_label);
    if (!phi_access.Error()) {
      // Use phi_access.Accessible(state), phi_access.ForbiddenPosition(), etc.
    }
    ```

*   **`sfst::Trimmer<Arc>`**: Trims transitions in stochastic automata.

    ```cpp
    #include "third_party/opengrm/sfst/trim.h"
    // ...
    sfst::Trimmer<StdArc> trimmer(&fst, phi_label, sfst::TRIM_NEEDED_FINAL);
    trimmer.PhiTrim();
    trimmer.Finalize();
    ```

*   **`sfst::Trim`**: Helper function for trimming.

    ```cpp
    #include "third_party/opengrm/sfst/trim.h"
    // ...
    bool success = sfst::Trim(&fst, phi_label, sfst::TRIM_NEEDED_FINAL);
    ```

*   **`sfst::Intersect`**: Intersects two canonical stochastic FSAs.

    ```cpp
    #include "third_party/opengrm/sfst/intersect.h"
    // ...
    StdVectorFst ofst;
    bool success = sfst::Intersect(ifst1, ifst2, &ofst, phi_label);
    ```

*   **Smoothing functions** (in `smooth.h`):
    *   `sfst::Katz(MutableFst<Arc> *fst, Label phi_label, int64_t bins)`
    *   `sfst::KneserNey(MutableFst<Arc> *fst, Label phi_label, double discount_D)`
    *   `sfst::AbsoluteDiscounting(MutableFst<Arc> *fst, Label phi_label, double discount_D)`
    *   `sfst::WittenBell(MutableFst<Arc> *fst, Label phi_label, double witten_bell_k)`

*   **Shrinking functions** (in `shrink.h`):
    *   `sfst::StolckeShrink(MutableFst<Arc> *fst, Label phi_label, double theta)`
    *   `sfst::SeymoreShrink(MutableFst<Arc> *fst, Label phi_label, double theta, double total_unigram_count)`

## Background reading and references

For general material on finite-state transducers and OpenFst, see the
[OpenFst background reading](https://github.com/google-research/openfst/blob/main/docs/background.md).
For API-level documentation, consult the
[documented source code](https://www.opengrm.org/doxygen/sfst/html/).

Allauzen, C., and Riley, M. 2018. [Algorithms for weighted automata with
failure transitions](https://www.opengrm.org/twiki/pub/GRM/SFstBackground/ciaa18.pdf).
In *Proceedings of the 23rd International Conference on Implementation and
Application of Automata*.

> **Note:** This paper makes two simplifying assumptions: (1) a successful
> path cannot end in a failure transition, and (2) there are no epsilon
> transitions when failure transitions are present. Neither limitation is
> required in this library (failure transitions treat epsilons as defined
> for a canonical FST).

Roark, B., Sproat, R., Allauzen, C., Riley, M., Sorensen, J., and Tai, T. 2012.
[The OpenGrm open-source finite-state grammar software libraries](https://aclanthology.org/P12-3011/).
In *Proceedings of the ACL 2012 System Demonstrations*, pages 61–66.

Suresh, A. T., Roark, B., Riley, M., and Schogol, V. 2019. [Distilling weighted
automata from arbitrary probabilistic models](https://www.opengrm.org/twiki/pub/GRM/SFstBackground/fsmnlp_approx.pdf).
In *Proceedings of the 14th International Conference on Finite-State Methods and
Natural Language Processing*.

*   [Longer version with proofs and additional experiments](https://www.opengrm.org/twiki/pub/GRM/SFstBackground/1905.08701.pdf).

## Download

*   **Current Release:**
    [sfst-1.2.1.tar.gz](https://www.opengrm.org/twiki/pub/GRM/SFstDownload/sfst-1.2.1.tar.gz)
    (requires C++17)

    *   **SHA-256:**
        `3da1473a45cb0cd4eda06a528808dc5fb1f5cfca659189a349aeb1f98018031c`

*   **Distribution Documentation:**
    *   [README](https://www.opengrm.org/twiki/pub/GRM/SFstDownload/README)
    *   [INSTALL](https://www.opengrm.org/twiki/bin/view/GRM/SfstDistInstall)
    *   [NEWS](https://www.opengrm.org/twiki/pub/GRM/SFstDownload/NEWS)
    *   [COPYING](https://www.opengrm.org/twiki/bin/view/GRM/SFstDistCopying)

*   **Older Releases:**
    *   [sfst-1.2.0.tar.gz](https://www.opengrm.org/twiki/pub/GRM/SFstDownload/sfst-1.2.0.tar.gz)
    *   [sfst-1.1.0.tar.gz](https://www.opengrm.org/twiki/pub/GRM/SFstDownload/sfst-1.1.0.tar.gz)
    *   [sfst-1.0.0.tar.gz](https://www.opengrm.org/twiki/pub/GRM/SFstDownload/sfst-1.0.0.tar.gz)
