# Baum-Welch training

## Training

Training is performed using the `TrainBaumWelch` function. It takes as arguments
a FAR or WFST representing the plaintext, and channel model and a FAR
representing the ciphertext.

### Normalization strategies

Training takes a template argument argument representing the desired expectation
table type. Two types are available. The state expectation table normalizes so
that all arcs leaving a state sum to semiring one. The state/input-label
expectation table normalizes so that the weights of all arcs leaving a state
with the same input label sum to semiring one; this is arguably the most
intuitive normalization strategy and so it used as the default.

### Semirings and count collection strategies

Training is controlled in part by the arc type of the inputs.

In a semiring without the
[_path property_](http://www.openfst.org/twiki/bin/view/FST/FstWeightRequirements),
like the log semiring, the E-step sums over all state sequences and thus
performs true Baum-Welch training. In the case the input is a FST language model
with backoffs, attach a $$\phi$$-matcher (`fstspecial --fst_type=phi`) to ensure
proper interpretation.

In a semiring with the path property, like the tropical semiring, the E-step
sums over only the most likely state sequence, an approximation known as
_Viterbi training_ (Brown et al. 1993:293). In the case the input is an FST
language model with backoffs, backoffs can be encoded exactly using a
$$\phi$$-matcher or approximated by $$\epsilon$$ using the default matcher. In
practice, Baum-Welch training is slightly slower than Viterbi training, though
somewhat more accurate (Jansche 2003).

### Options

`TrainBaumWelch` takes an struct representing options for training. These define
the size of the batch, the maximum number of iterations for training, the step
size reduction power $$\alpha$$, and the comparison/quantization $$\delta$$ used
to detect convergence.

Since expectation maximization training is only locally optimal, _random
(re)starts_ can be used to avoid local minima. For particularly difficult
problems, it may be beneficial to perform a very large number of random starts
(e.g., Berg-Kilpatrick and Klein 2013). In this library, we randomly initialize
weights by sampling from the uniform distribution $$(\texttt{kDelta}, 1]$$ in
the real numbers and then mapping these values onto the appropriate values in
the desired semiring. `RandomizeBaumWelch` can be used to generate random
restarts.

### Stepwise interpolation

The actual training method uses a form of stepwise interpolation due to Liang
and Klein (2009). At time $$k$$ a weight $$W_k$$ is given by

$$W_k = (1 - \nu_k) W_{k - 1} + \nu_k M_k$$

where $$\nu_k = (k - 2)^{-\alpha}$$, $$\alpha$$ is a fixed hyperparameter
controlling exponential decay of $$\nu_k$$, and $$M_k$$ is the maximized
expectation at time $$k$$.

## Decoding

`DecodeBaumWelch` is the primary driver for decoding. Decoding with the trained
model is essentially a process of constructing a weighted lattice followed by a
shortest path computation.

In a semiring with the path property, decoding is simple:

1.  Compose the input, channel model, and output, and materialize the cascade.
2.  Compute the shortest path using the Viterbi algorithm.
3.  Input-project (for decipherment only), then remove epsilons and weights.

In a semiring without the path property, exact decoding is much more
challenging, and in fact we do not have an algorithm for the pair case since we
normally need to preserve the input-output correspondences. In the decipherment
case, it is possible to compute the shortest string using a DFA lattice, but
determinizing the entire lattice is rarely feasible for interesting problems.
Therefore we use the following strategy:

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
    immediately after the shortest path is found. This gives us the shortest
    string.
6.  Convert the shortest string back to the input semiring and remove weights.

## Problems

### Pair modeling

In the pair scenario, we observe a set of paired input/output strings $$(i, o)$$
where $$i \in {I}^{*}$$ and $$o \in {O}^{*}$$ and wish to construct a
conditional model. This is useful for many monotonic string-to-string
transduction problems, such as grapheme-to-phoneme conversion or abbreviation
expansion.

#### Probabilistic formulation

We would like to estimate a conditional probability model over input-output
pairs, henceforth $$\textrm{P}(o \mid i)$$.

#### Finite-state formulation

We first construct FARs containing all $$i$$ and all $$o$$ pairs, respectively.

We then construct a model of the alignment $$ \Gamma \subseteq {I}^{*} \times
{O}^{*}$$. This serves as the initial (usually uniform) model for $$(i, o)$$; we
henceforth refer to it as the _channel model_.

Given an estimate for the channel model, $$\hat{\Gamma}$$, we can compute the
best alignment by decoding

$$\mathrm{ShortestPath}\left[i \circ \hat{\Gamma} \circ o\right]$$

where $$\textrm{ShortestPath}$$ represents the Viterbi algorithm.

#### Pair language modeling

Once the alignment model $$\hat{\Gamma}$$ is estimated, we can use it to
construct a so-called _pair language model_ (Novak et al. 2012, 2016). This is
much like a classic language model but the actual symbols represent pairs in
$${I} \times {O}$$; unlike $$\hat{\Gamma}$$, it is a joint model. To construct
such a model:

1.  Compute the best $$i, o$$ alignments by decoding with $$\hat{\Gamma}$$.
2.  Rewrite the FSTs in the decoded alignments FAR as acceptors over a pair
    symbol vocabulary $${I} \times {O}$$.
3.  Compute a higher-order language model (e.g., using the NGram tools), with
    standard smoothing and shrinking options, producing a weighted finite-state
    acceptor.
4.  Convert the weighted finite-state acceptor to a finite-state transducer by
    rewriting the "acceptor" $${I} \times {O}$$ arcs with the corresponding
    "transducer" $${I} : {O}$$ arcs.

Then the pair language model can be applied using composition and decoded using
the Viterbi algorithm.

### Decipherment

In the decipherment scenario, we observe a corpus of ciphertext $$c \in
{C}^{*}$$. We imagine that there exists a corpus of plaintext $$p \in {P}^{*}$$
which was used to generate the ciphertext $$c$$. Our goal is to _decode_ the
ciphertext; i.e., to uncover the corresponding plaintext. We assume that the
ciphertext $$c$$ has been generated (i.e., encoded) by $$\pi_o\left(p \circ
\gamma\right)$$ where $$\pi_o$$ is the output projection operator and $$\gamma$$
is the encoder (i.e., inverse key). We further assume that the ciphertext can be
recovered (i.e., decoded) using $$\pi_i\left(\gamma \circ c\right)$$ where
$$\pi_i$$ is the input projection operator. However, we do not observe either
$$\gamma$$, and must instead estimate it from data.

#### Probabilistic formulation

We would like to estimate a conditional probability model which predicts the
plaintext $$p$$ given $$c$$; i.e., $$\textrm{P}(p \mid c)$$. By Bayes' rule

$$\textrm{P}(p \mid c) \propto \textrm{P}(p) \textrm{P}(c \mid p).$$

That is; the conditional probability of the plaintext given observed ciphertext
is proportional to the product of the probability of the plaintext and the
conditional probability of the ciphertext given the plaintext; for any corpus of
ciphertext the denominator $$\textrm{P}(c)$$ is constant and thus can be
ignored.

We then express decoding as

$$\hat{p} = \arg\max_p \textrm{P}(p) \textrm{P}(c \mid p).$$

#### Finite-state formulation

We first construct a probabilistic model $$\Lambda \subseteq {P}^{*}$$, a
superset of the true plaintext $$p$$ and a model of $$(p)$$. We henceforth refer
to this as the _language model_.

We then construct a model of the inverse keyspace, $$ \Gamma \subseteq {P}^{*}
\times {C}^{*}$$. This is a superset of the true encoder relation $$\gamma$$ and
serves as an initial (usually uniform) model for $$(c \mid p)$$; we henceforth
refer to it as the _channel model_.

Given an estimate for the channel model, $$\hat{\Gamma}$$, we can express
decoding as

$$\mathrm{ShortestPath}\left[\pi_i\left(\Lambda \circ \hat{\Gamma}
\circ c\right)\right]$$

where $$\textrm{ShortestPath}$$ represents the Viterbi algorithm.

#### Sparsity penalties

Knight et al. (2006) suggest maximizing

$$\hat{p} = \arg\max_p \textrm{P}(p) \textrm{P}(c \mid p)^k$$

where $$k$$ is some real number $$> 1$$ during decoding. This strategy increases
the sparsity of the hypotheses generated by the trained model. This penalty can
be applied using `fstmap --map_type=power --power=$K` before decoding, if
desired.

## References

Berg-Kilpatrick, T., and Klein, D. 2013. Decipherment with a million random
restarts. In _Proceedings of the 2013 Conference on Empirical Methods in Natural
Language Processing_, pages 874-878. Seattle.

Brown, P., Della Pietra, S. A., Della Pietra, V. J., and Mercer, R. L. 1993. The
mathematics of statistical machine translation: parameter estimation.
_Computational Linguistics_ 19(2): 263-311.

Dempster, A. P., Laird, N. M., and Rubin, D. B. 1977. Maximum likelihood from
incomplete data via the EM algorithm. _Journal of the Royal Statistical Society,
Series B_, 39(1): 1-38.

Hart, P. E., Nilsson, N. J., Raphael, B. 1968. A formal basis for the heuristic
determination of minimal cost paths. _IEEE Transactions on Systems Science and
Cybernetics_ 4(2): 100-107.

Jansche, M. 2003. Inference of string mappings for language technology. Doctoral
dissertation, Ohio State University.

Knight, K., Nair, A., Rashod, N., and Yamada, K. 2006. Unsupervised analysis for
decipherment problems. _Proceedings of the COLING/ACL 2006 Main Conference
Poster Sessions_, pages 499-506. Sydney.

Liang, P., and Klein, D. 2009. Online EM for unsupervised models. In
_Proceedings of Human Language Technologies: The 2009 Annual Conference of the
North American Chapter of the Association for Computational Linguistics_, pages
611-619. Boulder.

Novak, J. R., Minematsu, N., and Hirose, K. 2012. WFST-based grapheme-to-phoneme
conversion: Open source tools for alignment, model-building and decoding.
_Proceedings of the 10th International Workshop on Finite State Methods and
Natural Language Processing_, pages 45-49. Donostia–San Sebastián.

Novak, J. R., Minematsu, N., and Hirose, K. 2016. Phonetisaurus: exploring
grapheme-to-phoneme conversion with joint n-gram models in the WFST framework.
_Natural Language Engineering_ 22(6): 907-938.

