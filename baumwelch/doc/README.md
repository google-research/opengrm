# Baum-Welch training





## Training

The `BaumWelch` template class is the primary driver for training. It is
templated on arc (which indirectly determines the strategy used for count
collection during the E-step; see below) as well as an expectation table type
(which determines the strategy used for renormalization during the M-step; see
below).

Most users should instead use the `TrainBaumWelch` function. This function takes
as arguments FSTs representing the language model and channel model and a FAR
representing the ciphertext.

### Expectation tables and normalization strategies

This function also takes as an enum argument representing the desired
expectation table type. Three such types are available. The global expectation
table (`GLOBAL`) performs global normalization and is only valid when the
channel model is itself acyclic. The state expectation table (`STATE`)
normalizes so that all arcs leaving a state sum to semiring one. The input-label
expectation table (`ILABEL`) assumes that the channel model is a single-state
machine and normalizes so that the weights of all arcs leaving that single state
with the same input label sum to semiring one. The state/input-label expectation
table (`STATE_ILABEL`) normalizes so that the weights of all arcs leaving a
state with the same input label sum to semiring one; this is arguably the most
intuitive normalization strategy and so it used as the default.

### Semirings and count collection strategies

Training function is controlled in part by the arc type of the inputs.

In a semiring without the
[_path property_](http://www.openfst.org/twiki/bin/view/FST/FstWeightRequirements),
like the log semiring, the E-step sums over all state sequences and thus
performs true Baum-Welch training. In the case the input is a FST language model
with backoffs, attach a $$\phi$$-matcher to ensure proper interpretation.

In a semiring with the path property, like the tropical semiring, the E-step
sums over only the most likely state sequence, an approximation known as
_Viterbi training_. In the case the input is an FST language model with
backoffs, they can be encoded exactly using a $$\phi$$-matcher or approximated
using the default matcher.

In practice, Baum-Welch training is slightly slower than Viterbi training,
though somewhat more accurate (Jansche 2003).

### Options

Finally, `TrainBaumWelch` takes an struct representing options for training.
These define the maximum number of iterations for training, whether or not
_random starts_ (to be defined) are to be performed, whether or not _zero arcs_
(those arcs in the channel model which have zero expectations in the cascade)
are to be pruned beforehand, and a comparison/quantization delta to be used to
detect convergence.

Since expectation maximization training is only locally optimal, _random starts_
are used to avoid local minima by starting training from randomly weighted
channel models. For particularly difficult problems, it may be beneficial to
perform a very large number of random starts (e.g., Berg-Kilpatrick and Klein,
2013). In this library, we randomly initialize weights by sampling from the
uniform distribution (0, 1] in the real numbers, mapping these values onto the
appropriate values in the desired semiring, and then renormalizing.

### Default training regime

The default training regime is as follows:

1.  Prune zero arcs
2.  Train the initial ("flat start") channel model till convergence or 50
    iterations, whichever comes first
3.  Generate 10 random starts and then train them until convergence or 50
    iterations, whichever comes first
4.  Return the converged model with the highest posterior likelihood

## Decoding

The `DecodeBaumWelch` functions are the primary driver for decoding. Decoding
with the trained model is essentially a process of constructing a weighted
lattice followed by a shortest path computation.

In a semiring with the path property, decoding is simple:

1.  Compose the input, channel model, and output, and materialize the cascade.
2.  Compute the shortest path using the Viterbi algorithm.
3.  Input-project (for decipherment only), then remove epsilons and weights.

In a semiring without the path property, exact decoding is much more
challenging, and in fact we do not have an algorithm for the pair case since we
normally need to preserve the input-output correspondences. In the decipherment
case, it is possible to perform with a DFA lattice, but determinizing the entire
lattice is rarely feasible for interesting problems. Therefore we use the
following strategy:

1.  Compose the plaintext model, channel model, and ciphertext, and materialize
    the cascade, then input-project and remove epsilons to produce an acyclic,
    epsilon-free non-deterministic weighted acceptor (henceforth, the NFA).
2.  Compute $$\beta_n$$, the costs to the future in the NFA.
3.  Compute, on the fly, a deterministic acceptor (henceforth, the DFA);
    expansion of the DFA converts the NFA future costs $$\beta_n$$ to the DFA
    future costs $$\beta_d$$.
4.  Compute, on the fly, a mapping of the DFA to a semiring with the path
    property by converting the weights to tropical.
5.  Compute the shortest path of the on-the-fly tropical-semiring DFA using
    $$A^{*}$$ search (Hart et al. 1968) with $$\beta_d$$ as a heuristic, halting
    immediately after the shortest path is found.
6.  Convert the shortest path back to the input semiring and remove weights.

## Problems

### Pair modeling

In the pair scenario, we observe a set of paired input/output strings $$(i, o)$$
where $$i \in \mathcal{I}^{*}$$ and $$o \in \mathcal{O}^{*}$$ and wish to
construct a conditional model. This is useful for many monotonic
string-to-string transduction problems, such as grapheme-to-phoneme conversion
or abbreviation expansion.

#### Probabilistic formulation

We would like to estimate a conditional probability model over input-output
pairs, henceforth $$P(o \mid i)$$.

#### Finite-state formulation

We first construct FARs containing all $$i$$ and all $$o$$ pairs, respectively.

We then construct a model of the alignment $$ \Gamma \subseteq \mathcal{I}^{*}
\times \mathcal{O}^{*}$$. This serves as the initial (usually uniform) model for
$$P(i, o)$$; we henceforth refer to it as the _channel model_.

Given an estimate for the channel model, $$\hat{\Gamma}$$, we can compute the
best alignment by decoding

$$\mathrm{ShortestPath}\left[i \circ \hat{\Gamma} \circ o\right]$$

where $$\textrm{ShortestPath}$$ represents the Viterbi algorithm.

#### Pair language modeling

Once the alignment model $$\hat{\Gamma}$$ is estimated, we can use it to
construct a so-called _pair language model_ (Novak et al. 2012). This is like a
classic language model but the actual symbols represent pairs in $$\mathcal{I}
\times \mathcal{O}$$; unlike $$\hat{\Gamma}$$, it is a joint model. To construct
such a model:

1.  Compute the best $$i, o$$ alignments by decoding with $$\hat{\Gamma}$$.
2.  Rewrite the FSTs in the decoded alignments FAR as acceptors over a pair
    symbol vocabulary $$\mathcal{I} \times \mathcal{O}$$.
3.  Compute a higher-order language model (e.g., using the NGram tools), with
    standard smoothing and shrinking options, producing a weighted finite-state
    acceptor.
4.  Convert the weighted finite-state acceptor to a finite-state transducer by
    rewriting the "acceptor" $$\mathcal{I} \times \mathcal{O}$$ arcs with the corresponding
    "transducer" $$\mathcal{I} : \mathcal{O}$$ arcs.

Then the pair language model can be applied using composition and decoded using
the Viterbi algorithm.

### Decipherment

In the decipherment scenario, we observe a corpus of ciphertext $$c \in
\mathcal{C}^{*}$$. We imagine that there exists a corpus of plaintext $$p \in
\mathcal{P}^{*}$$ which was used to generate the ciphertext $$c$$. Our goal is
to _decode_ the ciphertext; i.e., to uncover the corresponding plaintext. We
assume that the ciphertext $$c$$ has been generated (i.e., encoded) by
$$\pi_o\left(p \circ \gamma\right)$$ where $$\pi_o$$ is the output projection
operator and $$\gamma$$ is the encoder (i.e., inverse key). We further assume
that the ciphertext can be recovered (i.e., decoded) using $$\pi_i\left(\gamma
\circ c\right)$$ where $$\pi_i$$ is the input projection operator. However, we
do not observe either $$\gamma$$, and must instead estimate it from data.

#### Probabilistic formulation

We would like to estimate a conditional probability model which predicts the
plaintext $$p$$ given $$c$$; i.e., $$P(p \mid c)$$. By Bayes' rule

$$P(p \mid c) \propto P(p) P(c \mid p).$$

That is; the conditional probability of the plaintext given observed ciphertext
is proportional to the product of the probability of the plaintext and the
conditional probability of the ciphertext given the plaintext; for any corpus of
ciphertext the denominator $$P(c)$$ is constant and thus can be ignored.

We then express decoding as

$$\hat{p} = \arg\max_p P(p) P(c \mid p).$$

#### Finite-state formulation

We first construct a probabilistic model $$\Lambda \subseteq \mathcal{P}^{*}$$,
a superset of the true plaintext $$p$$ and a model of $$P(p)$$. We henceforth
refer to this as the _language model_.

We then construct a model of the inverse keyspace, $$ \Gamma \subseteq
\mathcal{P}^{*} \times \mathcal{C}^{*}$$. This is a superset of the true encoder
relation $$\gamma$$ and serves as an initial (usually uniform) model for $$P(c
\mid p)$$; we henceforth refer to it as the _channel model_.

Given an estimate for the channel model, $$\hat{\Gamma}$$, we can express
decoding as

$$\mathrm{ShortestPath}\left[\pi_i\left(\Lambda \circ \hat{\Gamma}
\circ c\right)\right]$$

where $$\textrm{ShortestPath}$$ represents the Viterbi algorithm.

#### Sparsity penalties

Knight et al. (2006) suggest maximizing

$$\hat{p} = \arg\max_p P(p) P(c \mid p)^k$$

where $$k$$ is some real number $$> 1$$ during decoding. This strategy increases
the sparsity of the hypotheses generated by the trained model. This penalty can
be applied using `fstmap --map_type=power --power=$K` before decoding, if
desired.

## References

Berg-Kilpatrick, T., and Klein, D. 2013. Decipherment with a million random
restarts. In _EMNLP_, pages 874-878.

Dempster, A. P., Laird, N. M., and Rubin, D. B. 1977. Maximum likelihood from
incomplete data via the EM algorithm. _Journal of the Royal Statistical Society,
Series B_, 39(1): 1-38.

Hart, P. E., Nilsson, N. J., Raphael, B. 1968. A formal basis for the heuristic
determination of minimal cost paths. _IEEE Transactions on Systems Science and
Cybernetics_ 4(2): 100-107.

Jansche, M. 2003. Inference of string mappings for language technology. Doctoral
dissertation, Ohio State University.

Knight, K., Nair, A., Rashod, N., and Yamada, K. 2006. Unsupervised analysis for
decipherment problems. In _COLING_, pages 499-506.

Novak, J. R., Minematsu, N., and Hirose, K. 2012. WFST-based grapheme-to-phoneme
conversion: Open source tools for alignment, model-building and decoding. In
_FSMNLP_, pages 45-49.

