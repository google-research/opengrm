# Pynini: Grammar compilation in Python

## Introduction

Pynini (Gorman 2016) is a library for compiling a grammar of strings, regular
expressions, and context-dependent rewrite rules into _weighted finite-state
transducers_.

### Design

Pynini supports much of the functionality of [Thrax](http://thrax.opengrm.org),
but whereas Thrax is essentially a compiler for a domain-specific language,
Pynini is implemented as a Python extension module. This provides several
advantages:

*   Pynini users can exploit Python's rich tooling ecosystem, including logging
    and unit testing frameworks.
*   Pynini users can incorporate Thrax primitives like string compilation into
    executables.

Pynini is based on the [OpenFst library](http://openfst.org), a C++11 template
library for weighted finite-state transducers, and particularly its
[Python extension](http://python.openfst.org).

### Outline

This document covers the following areas:

1.  [formal preliminaries underlying finite-state transducers](#formal),
2.  [getting started with Pynini](#intro),
3.  [examples of Pynini in action](#examples),
4.  [API reference](#api), and
5.  [advanced topics](#advanced).

Those who are familiar with FSTs may prefer to jump straight to
[the second section](#intro). Or, for a quick start, you may also wish to read
our
[Pynini tutorial for O'Reilly](https://www.oreilly.com/ideas/how-to-get-superior-text-processing-in-python-with-pynini).

## Formal preliminaries <a id="formal"></a>

_Finite-state transducers_ (FSTs) are models of computation widely used for
speech and language processing at Google:

*   *Speech recognition*: language models, the pronunciation dictionaries, and
    the lattices produced by the acoustic model are all represented as FSTs;
    combined, these define the hypothesis space for the recognizer.
*   *Speech synthesis*: FSTs are used for _text normalization_, the phase of
    linguistic analysis that converts written text like "meet me at St.
    Catherine on Bergen St. @ 9:30" to a pronounceable utterance like "meet me
    at the Saint Catherine on Bergen Street at nine thirty".
*   *Natural language processing*: FSTs are used to represent sequence models
    such as those used for part of speech tagging, noun chunking, and named
    entity recognition.
*   *Information retrieval*: finite automata are used to detect dates, times,
    etc., in web text.

FSTs are less "expressive" than e.g., arbitrary C++ functions or neural
networks. However, the formal constraints that limit their expressivity
guarantee that FSTs can be efficiently combined, optimized, and searched.

### State machines

Finite automata are simply _state machines_ with a finite number of states. And,
a state machine is any devices whose behavior can be modeled using transitions
between a number of states. While our finite automata are implemented in
software, there are also hardware state machines, like the traditional gumball
machine (image credit: Wikimedia Commons):

![Kosher gumball machine (from Wikimedia commons)](https://upload.wikimedia.org/wikipedia/commons/thumb/c/cd/Kosher_gumballs.JPG/360px-Kosher_gumballs.JPG)

A working gumball machine can be in one of two states: either its coin slot
does, or does not, contain a coin. Initially the gumball machine has an empty
coin slot; and turning the knob has no effect on the state of the machine. But
once a coin is inserted into the coin slot, turning the knob dispenses a gumball
and empties the coin slot. If the knob is turned while the coin slot is vacant,
no change of state occurs. However, if the knob is turned while the coin slot is
occupied, a gumball is dispensed and the coin slot is cleared. We can represent
this schematic description of a gumball machine as a directed graph in which the
states are represented by circles and the transitions between
states&mdash;henceforth, _arcs_&mdash; are represented by arrows between states.
Each arc has a pair of labels representing the input consumed, and output
emitted, respectively, when that arc is traversed. By convention, the Greek
letter $\epsilon$ (_epsilon_) is used to represent null input and/or output
along an arc.

![gumball FST](gumball.dot)

The bold double-circle state labeled `0` simply indicates that that state is
"final", whereas the single-circle state `1` is non-final. Here, this encodes
the notion that no user would walk away from a gumball machine while there's
still a coin in the slot.

### Finite-state acceptors

In what follows, we draw heavily from Roark & Sproat 2008, chapter 1 and
interested readers are advised to consult that resource for further details.

#### Definition

_finite-state acceptors_ (or FSAs) are the simplest type of FST. Each FSA
represents a set of strings (henceforth, a _language_), similar to a regular
expression, as follows. An FSA consists of a five-tuple $(Q, S, F, \Sigma,
\delta)$ where:

1.  $Q$ is a finite set of states,
2.  $S$ is the set of _start_ states,
3.  $F$ is the set of _final_ states,
4.  $\Sigma$ is the alphabet, and
5.  $\delta : Q \times (\Sigma \cup \epsilon) \rightarrow Q$ is the
    transition relation.

A _path_ through an FSA $(Q, S, F, \Sigma, \delta)$ is simply a sequence of
transitions starting with a start state $s \in S$, taking only transitions
licensed by $\delta$, and terminating at a final state $f \in F$. More
formally, let a path be a sequence $P = p_o, p_1, \ldots, p_n$ where each
element $p \in P$ is a triple over $Q \times (\Sigma \cup \epsilon) \times
Q$. Each triple consists of a source state, a _label_ in the alphabet (or the
null label $\epsilon$), and a destination state. Then, a path is valid if
and only if all of the following conditions hold:

1.  The source state for $p_0$ is in $S$,
2.  $\delta(q_i, l_i) = q_{i + 1}$ for any $p_i = (q_i, l_i, q_{i + 1})
    \in P$, and
3.  the destination state for $p_n$ is in $F$.

The first condition requires that the path begin at a start state, the second
condition requires that all transitions be licensed by $\delta$ , and the
third condition requires that the final transition be to a final state.

Let the _string_ of a path $P = p_o, p_1, \ldots, p_n$ be the possibly empty
sequence of labels. For instance, if $P$ is $(s, l_0, q_1), (q_1, l_1,
q_2), (q_2, l_2, q_3)$, then the string of $P$ is simply $l_0, l_1, l_2
$. Then the _language_ described by an FSA is simply the union of all strings
of all its paths.

#### Example

The following FSA represents the Perl-style regular expression `ab*cd+e`:

![ab*cd+e FSA](abcde.dot)

Here, the bold circle labeled `0` indicates the start state, and the double
circle labeled `4` indicates the final state.

### Finite-state transducers

#### Definition

_Finite-state transducers_ (FSTs) are generalization of FSAs. In the normal case
of a _two-way_ transducer, $\delta$ is instead a relation from $Q \times
(\Sigma_i \cup \epsilon) \times (\Sigma_o \cup \epsilon)$ to $Q$, where $
\Sigma_i$ and $\Sigma_o$ are the input and output alphabets, respectively.
Paths through a FST are defined similarly to the definition given for FSAs
above, except that each path corresponds to a set of two strings, an input
string over $\Sigma_i^{*}$ and an output string over $\Sigma_o^{*}$.
Whereas FSAs describe sets of strings, FSTs describe relations _between_ sets of
strings.

When the relation described by an FST is such that each input string corresponds
to at most one output string, we say that the FST is _functional_.

#### Example

The following FST represents the relation `(a:a)(b:b)*(c:g)(d:f)+(e:e)`.

![ab*(c:g)(d:f)+e FST](abcgdfe.dot)

### Weighted finite-state transducers

#### Definition

_Weighted finite-state transducers_ (WFSTs) are generalizations of FSTs which
use an alternative definition of both $F$ and $\delta$ incorporating the
notion of _weights_. FST weights and their operations can be understood by first
defining the notion of _semiring_, which require us to first define the notion
_monoid_.

A monoid is a pair $(M, \cdot )$ where $M$ is a set and $\cdot$ is a
binary operation on $M$ such that:

1.  $\cdot$ is _closed_ over $M$: for all $a, b$ in $M$, $a
    \cdot b \in M$,
2.  there is an identity element $e \in M$ such that $a \cdot e = e \cdot
    a = a$ for all $a \in M$, and
3.  $\cdot$ is _associative_: for all $a, b, c \in M$, $(a \cdot b)
    \cdot c = a \cdot (b \cdot c)$.

Finally, a monoid $(M, \cdot )$ is _commutative_ if for all $a, b \in M
$, $a \cdot b = b \cdot a$.

Then, a semiring is a triple $(\mathbb{K}, \oplus, \otimes)$ such that:

1.  $(\mathbb{K}, \oplus)$ is a commutative monoid,
2.  $(\mathbb{K}, \otimes)$ is a monoid,
3.  for all $a, b, c \in \mathbb{K}$, $a \otimes (b \oplus c) = (a \otimes
    b) \oplus (a \otimes c)$, and
4.  for all $a \in \mathbb{K}$, $a \otimes \bar{0} = \bar{0} \otimes a =
    \bar{0}$ where $\bar{0}$ is the identity element for the monoid $
    (\mathbb{K}, \oplus)$.

In many cases, $\mathbb{K}$ is the set of real numbers, so a semiring can be
denoted simply by specifying the $(\oplus, \otimes)$ pair. The so-called
_real semiring_ is then simply $(+, \times)$.

When working with probabilities as weights, we often use the _tropical semiring_
$(min, +)$ and negative log probabilities, taking advantage of the
logarithmic identity $\log(x) + \log(y) = log(x y)$. The tropical semiring
(and the associated `standard` arc type) is the default semiring in Pynini.

At last, we can give the modified definitions for $F$ and $\delta$ for
WFSTs. Whereas for unweighted FSTs, $F$ is a set of final states, for WFSTs
$F$ is a set of pairs over $Q \times \mathbb{K}$, where the second
element is the _final weight_ for that state. And, the transition relation $
\delta$ for a WFST is from $Q \times (\Sigma_i \cup \epsilon) \times
(\Sigma_o \cup \epsilon) \times \mathbb{K}$ to $Q$. The definition of
paths is parallel to those for unweighted FSTs except that each element in the
path is also associated with a weight in $\mathbb{K}$.

#### Example

WFSTs are a natural representation for conditional probability distributions
from strings to strings. For example, consider a text normalization rule which
verbalizes `2:00` as `two` with $P = .2$ and as `two o'clock` with $P = .8
$. The following _probability_ (i.e., real-valued) _semiring_ WFST encodes this
distribution:

![time FST](time.dot)

### Conventions used in Pynini

*   Pynini represents all acceptors and transducers, weighted or unweighted, as
    WFSTs. Thus, for example, a weighted finite-state acceptor (WFSA) is
    represented as a WFST in which input and output labels match in all cases,
    and an unweighted finite-state transducer is represented by a WFST in which
    all weights are $\bar{1}$ or $\bar{0}$.
*   Pynini only permits one state to be designated the start state.
*   Pynini assigns a final weight to all states; a _nonfinal_ state is just one
    which has a final weight of $\bar{0}$.

## Getting started with Pynini <a id="intro"></a>

### Starting Pynini

To use Pynini in an application or library, create a
[`py_binary`](https://bazel.build/reference/be/python#py_binary)
or
[`py_library`](https://bazel.build/reference/be/python#py_library)
target with `"//third_party/py/opengrm/pynini"` as a dependency. Then, within
that target, import the module as follows:

```python {.good}
from opengrm.pynini import pynini
```

### Building FSTs

In Pynini, all FST objects are an instance of a class called `pynini.Fst`,
representing a mutable WFST. The user must specify the arc type at construction
time; by default, the `standard` arc type (and the associated tropical semiring)
is are used.

Pynini provides several functions for compiling strings into FSTs; we proceed to
review a few of these methods.

#### Constructing acceptors <a id="acceptor"></a>

The `acceptor` function compiles a (Unicode or byte) string into a deterministic
acceptor. The user may specify a final weight using the `weight` keyword
argument; by default, the final weight is $\bar{1}$. The user may also
specify the desired arc type using the `arc_type` keyword argument. The user may
also specify how the characters in the string are to be translated into labels
of the acceptor. By default (`token_type=None`), which means that the
`token_type` will be set according to the current default token_type which
defaults to `"byte"`; thus each arc in the acceptor corresponds to a byte in the
input string. The default `token_type` used when `token_type=None` can be
overridden using the `default_token_type` context manager.

```python {.good}
# UTF-8 bytestring, byte arcs.
print(pynini.accep("Pont l'Evêque"))
# Output below...
```

```none
0 1 P
1 2 o
2 3 n
3 4 t
4 5 <SPACE>
5 6 l
6 7 '
7 8 E
8 9 v
9 10  <0xc3>
10  11  <0xaa>
11  12  q
12  13  u
13  14  e
14
```

If the user specifies `token_type="utf8"` then each arc in the FST corresponds
to a Unicode codepoint.

```python {.good}
# UTF-8 bytestring, Unicode codepoint arcs.
python pynini.accep("Pont l'Evêque", token_type="utf8")
# Output below...
```

```none
0 1 P
1 2 o
2 3 n
3 4 t
4 5 <SPACE>
5 6 l
6 7 '
7 8 E
8 9 v
9 10  <0xea>
10  11  q
11  12  u
12  13  e
13
```

Sequences of characters enclosed by `[` and `]` receive special interpretations
in `byte` or `utf8` mode. Pynini first attempts to parse any such sequence as an
integer using the C standard library function
[`strtol`](http://www.cplusplus.com/reference/cstdlib/strtol/).

```python {.good}
assert pynini.accep("b[0x61][97]") == pynini.accep("baa")  # OK.
```

If this fails, then Pynini treats the sequence as a sequence of one or more
whitespace-delimited "generated" symbols, each of which is given a unique
identifier in the resulting FST's symbol table.

```python {.good}
x = pynini.accep("[It's not much of a cheese shop really]")
y = pynini.accep("[It's][not][much][of][a][cheese][shop][really]")
assert x == y   # OK.
```

A bracket character to be interpreted literally *must* be escaped.

```python {.good}
bracket = pynini.accep("\[")  # OK.
```

```python {.bad}
unused = pynini.accep("[")  # Not OK: Raises FstStringCompilationError.
```

To feed an arbitrary string into Pynini as a literal and ensure contained
brackets do not generated symbols, a user can utilize `escape`:

```python {.good}
def literal_string_fst(string: str) -> pynini.Fst:
    return pynini.accep(pynini.escape(string))
```

Finally, if the user specifies a `SymbolTable` as the `token_type`, then the
input string is parsed according and the FST labeled according to that table.
String parsing failures are logged and raise a `FstStringCompilationError`
exception from within the Python interpreter.

Nearly all FST operations permit a string to be passed in place of an `Fst`
argument; in that case, `pynini.accep` is used (with default arguments) to
compile the string into an FST.

#### Constructing transducers <a id="transducer"></a>

The `transducer` function creates a transducer from two FSTs, compiling string
arguments into FSTs if necessary. The result is a cross-product of the two input
FSTs, interpreted as acceptors. Specifically, the transducer is constructed by
mapping output arcs in the first FST to epsilon, mapping the input arcs in the
second FST to epsilon, then concatenating the two. In the case that both FSTs
are strings, the resulting transducer will simply contain the input and output
string converted into labels, and the shorter of the two will be padded with
epsilons.

As with `acceptor`, the user may specify a final weight using the `weight`
keyword argument; the final weight is $\bar{1}$. The user also specify the
desired arc type using the `arc_type` keyword argument. The user may also
specify how the characters in the input and/or output strings are to be
translated into labels of the transducer, if strings are passed in place of FST
arguments. By default (`token_type=None`), which means that the `token_type`
will be set according to the current default token_type which defaults to
`"byte"`; thus each arc corresponds to a byte in the input and/or output string.
The default `token_type` used when `token_type=None` can be overridden using the
`default_token_type` context manager.

Whereas `transducer` is used to construct the cross-product of two strings, the
union of many such string pair cross-products is compiled using the `string_map`
and `string_file` functions. The former takes an iterable of strings (or pairs
or triples of strings); the latter reads string pairs from a one-to-three column
[TSV file](https://en.wikipedia.org/wiki/Tab-separated_values). Both functions
use a compact prefix tree representation which can be used to represent a
(multi)map FST.

As with the above, the user may specify the desired arc type using the
`arc_type` keyword argument. And as with `transducer`, the user may also specify
how the characters in the input and/or output strings are to be translated into
labels of the transducer using the appropriate keyword arguments, and whether or
not symbol tables should be attached to the FST.

### Worked examples <a id="examples"></a>

The following examples, taken from Gorman 2016 and Gorman & Sproat 2016, show
features of Pynini in action.

#### Finnish vowel harmony

Koskenniemi (1983) provides a number of manually-compiled FSTs modeling Finnish
morphophonological patterns. One of these concerns the well-known pattern of
Finnish vowel harmony. Many Finnish suffixes have two allomorphs differing only
in the backness specification of their vowel. For example, the adessive suffix
is usually realized as _-llä_ [lːæː] except when the preceding stem contains one
of _u_, _o_, and _a_ and there is no intervening _y_, _ö_, or _ä_; in this case,
it is _-lla_ [lːɑː]. For example, _käde_ 'hand' has an adessive _kädellä_,
whereas _vero_ 'tax' forms the adessive _verolla_ because the nearest stem vowel
is _o_ (Ringen & Heinämäki 1999).

The following gives a Pynini function `make_adessive` which generates the
appropriate form of the noun stem. It first concatenates the stem with an
abstract representation of the suffix, and then composes the result with a
context-dependent rewrite rule `adessive_harmony`.

```python {.good}
back_vowel = pynini.union("u", "o", "a")
neutral_vowel = pynini.union("i", "e")
front_vowel = pynini.union("y", "ö", "ä")
vowel = pynini.union(back_vowel, neutral_vowel, front_vowel)
archiphoneme = pynini.union("A", "I", "E", "O", "U")
consonant = pynini.union("b", "c", "d", "f", "g", "h", "j", "k", "l", "m", "n",
                         "p", "q", "r", "s", "t", "v", "w", "x", "z")
sigma_star = pynini.union(vowel, consonant, archiphoneme).closure()
adessive = "llA"
intervener = pynini.union(consonant, neutral_vowel).closure()
adessive_harmony = (pynini.cdrewrite(pynini.cross("A", "a"),
                                     back_vowel + intervener, "", sigma_star) @
                    pynini.cdrewrite(pynini.cross("A", "ä"), "", "", sigma_star)
                   ).optimize()


def make_adessive(stem):
  return ((stem + adessive) @ adessive_harmony).string()
```

#### Pluralization rules

Consider an application where one is generating text such as, "The current
temperature in New York is 25 degrees". Typically one would do this from a
template such as the following:

```none
The current temperature in __ is __ degrees
```

This works fine if one fills in the first blank with the name of a location and
the second blank with a number. But what if it’s really cold in New York and the
thermometer shows 1° (Fahrenheit)? One does *not* want this:

```none {.bad}
The current temperature in New York is 1 degrees
```

So one needs to have rules that know how to _inflect_ the unit appropriately
given the context. The Unicode Consortium has some
[guidelines](http://www.unicode.org/cldr/charts/29/supplemental/language_plural_rules.html)
for this; they primarily specify how many “plural” forms different languages
have and in which circumstances one uses each. From a linguistic point of view
the specifications are sometimes nonsensical, but they are widely used and are
useful for adding support for new languages.

For handling degrees, we can assume that the default is the plural form, in
which case we must _singularize_ the plural form in certain contexts. For the
word _degrees_ and many other cases, it’s just a matter of stripping off the
final _s_, but for a word ending in _-ches_ (such as _inches_) one would want to
strip off the _es_, and for _feet_ and _pence_ the necessary changes are
irregular (_foot_, _penny_).

One could of course take care of this with a special purpose Python function,
but here we consider how simple this is in Pynini. We start by defining
`singular_map` to handle irregular cases, plus the general cases:

```python {.good}
singular_map = pynini.union(
    pynini.cross("feet", "foot"),
    pynini.cross("pence", "penny"),
    # Any sequence of bytes ending in "ches" strips the "es";
    # the last argument -1 is a "weight" that gives this analysis
    # a higher priority, if it matches the input.
    sigma_star + pynini.cross("ches", "ch", -1),
    # Any sequence of bytes ending in "s" strips the "s".
    sigma_star + pynini.cross("s", ""))
```

Then as before we can define a context-dependent rewrite rule that performs the
singularization just in case the unit is preceded by `1`. We define the right
context for the application of the rule rc as punctuation, space, or `[EOS]`, a
special symbol for the end-of-string.

```python {.good}
rc = pynini.union(".", ",", "!", ";", "?", " ", "[EOS]")
singularize = pynini.cdrewrite(singular_map, " 1 ", rc, sigma_star)
```

Then we can define a convenience function to apply this rule...

```python {.good}
def singularize(string):
  return pynini.shortestpath(
      pynini.compose(string.strip(), singularize)).string()
```

...and use it like so:

```none
>>> singularize("The current temperature in New York is 1 degrees")
"The current temperature in New York is 1 degree"
>>> singularize("That measures 1 feet")
"That measures 1 foot"
>>> singularize("That measures 1.2 feet")
"That measures 1.2 feet"
>>> singularize("That costs just 1 pence")
"That costs just 1 penny"
```

One can handle other languages by changing the rules appropriately&mdash;for
example, in Russian, which has
[more complex pluralization rules](http://www.unicode.org/cldr/charts/29/supplemental/language_plural_rules.html),
one needs several different forms, not just singular and plural&mdash;and by
changing the conditions under which the various rules apply.

#### T9 disambiguation

T9 (short for "Text on 9 keys"; Grover et al. 1998) is a patented predictive
text entry system. In T9, each character in the "plaintext" alphabet is assigned
to one of the 9 digit keys (0 is usually reserved to represent space) of the
traditional 3x4 touch-tone phone grid. For instance, the message _GO HOME_ is
entered as _4604663_. Since the resulting "ciphertext" may be highly
ambiguous&mdash;this sequence could also be read as _GO HOOD_ or as many
nonsensical expressions&mdash;a hybrid language model/lexicon is used for
decoding.

The following snippet implements T9 encoding and decoding in Pynini:

```python {.good}
LM = pynini.Fst.read("charlm.fst")
T9_ENCODER = pynini.string_file("t9.tsv").closure()
T9_DECODER = pynini.invert(T9_ENCODER)


def encode_string(plaintext):
  return (plaintext @ T9_ENCODER).string()


def k_best(ciphertext, k):
  lattice = (ciphertext @ T9_DECODER).project("output") @ LM
  return pynini.shortestpath(lattice, nshortest=k, unique=True)
```

The first line reads a language model (LM), represented as a WFSA. The second
line reads the encoder table from a TSV file: in this file, each line contains
an alphabetic character and the corresponding digit key. By computing the
concatenative closure of this map, one obtains an FST `T9_ENCODER` which can
encode arbitrarily-long plaintext strings. The `encode_string` function applies
this FST to arbitrary plaintext strings: _application_ here refers to
composition of a string with a transducer followed by output projection. The
`k_best` function first applies a ciphertext string&mdash;a bytestring of
digits&mdash;to the inverse of the encoder FST (`T9_DECODER`). This creates an
intermediate lattice of all possible plaintexts consistent with the T9
ciphertext. This is then scored with&mdash;that is, composed with&mdash;the
character LM. Finally, this function returns the $k$ highest probability
plaintexts in the lattice. For the following example, the highest probability
plaintext is in fact the correct one:

```python {.good}
pt = "THE SINGLE MOST POPULAR CHEESE IN THE WORLD"
ct = encode_string(pt)
print("CIPHERTEXT:", ct)
print("DECIPHERED PLAINTEXT:", k_best(ct, 1).string())
# Output below...
```

```none
CIPHERTEXT: 8430746453066780767852702433730460843096753
DECIPHERED PLAINTEXT: THE SINGLE MOST POPULAR CHEESE IN THE WORLD
```

## API reference <a id="api"></a>

This section covers classes (representing FSTs, weights, and the like), their
methods (representing various accessors as well as destructive FST algorithms),
and functions (representing constructive FST algorithms, weight arithmetic, and
the like).

### Objects <a id="objects"></a>

#### `Arc`

An FST arc is essentially a tuple of input label, output label, weight, and the
next state ID.

##### Constructor

```none
Arc(ilabel, olabel, weight, nextstate)

This class represents an arc while remaining agnostic about the underlying arc
type.  Attributes of the arc can be accessed or mutated, and the arc can be
copied.

Attributes:
  ilabel: The input label.
  olabel: The output label.
  weight: The arc weight.
  nextstate: The destination state for the arc.
```

##### Methods

(None.)

#### `_ArcIterator`

An arc iterator is used to iterate through arcs leaving some state in an FST; it
is normally constructed by calling the `arcs` method of an `Fst` object.

##### Constructor

```none
_ArcIterator(ifst, state)

This class is used for iterating over the arcs leaving some state of an FST. It supports the full C++ API, but users should just call the `arcs` method of an FST object and take advantage of the Pythonic API.
```

##### Methods

```none
done(self)

Indicates whether the iterator is exhausted or not.

Returns:
  True if the iterator is exhausted, False otherwise.
```

```none
next(self)

Advances the iterator.
```

```none
flags(self)

Returns the current iterator behavioral flags.

Returns:
  The current iterator behavioral flags as an integer.
```

```none
position(self)

Returns the position of the iterator.

Returns:
  The iterator's position, expressed as an integer.
```

```none
reset(self)

Resets the iterator to the initial position.
```

```none
seek(self, a)

Advance the iterator to a new position.


Args:
  a: The position to seek to.
```

```none
set_flags(self, flags, mask)

Sets the current iterator behavioral flags.

Args:
  flags: The properties to be set.
  mask: A mask to be applied to the `flags` argument before setting them.
```

```none
value(self)

Returns the current arc.

Returns:
  The current arc.
```

#### `EncodeMapper`

An encode mapper is used to "encode" and "decode" an FST; it is frequently used
to apply FST algorithms while viewing the input FST as unweighted, as an
acceptor, or both.

##### Constructor

```none
EncodeMapper(arc_type="standard", encode_labels=False, encode_weights=False)

Arc encoder class, wrapping EncodeMapperClass.

This class provides an object which can be used to encode or decode FST arcs.
This is most useful to convert an FST to an unweighted acceptor, on which
some FST operations are more efficient, and then decoding the FST afterwards.

To use an instance of this class to encode or decode a mutable FST, pass it
as the first argument to the FST instance methods `encode` and `decode`.

For implementational reasons, it is not currently possible to use an encoder
on disk to construct this class.

Args:
  arc_type: A string indicating the arc type.
  encode_labels: Should labels be encoded?
  encode_weights: Should weights be encoded?
```

##### Methods

```none
arc_type(self)

Returns a string indicating the arc type.
```

```none
flags(self)

Returns the encoder's flags.
```

```none
input_symbols(self)

Returns the encoder's input symbol table, or None if none is present.
```

```none
output_symbols(self)

Returns the encoder's output symbol table, or None if none is present.
```

```none
properties(self, mask)

Provides property bits.

This method provides user access to the properties of the encoder.

Args:
  mask: The property mask to be compared to the encoder's properties.

Returns:
  A 64-bit bitmask representing the requested properties.
```

```none
set_input_symbols(self, syms)

Sets the encoder's input symbol table.

Args:
  syms: A SymbolTable.
```

```none
set_output_symbols(self, syms)

Sets the encoder's output symbol table.

Args:
  syms: A SymbolTable.
```

```
weight_type(self)

Returns a string indicating the weight type.
```

#### `Far`

A FAR is an "FST archive": a file used to store multiple FSTs. Each FST in a FAR
is stored with a string key.

The `Far` object behaves similarly to a Python file handle: it can be opened for
reading, or for writing. When opened for reading, it behaves similarly to an
iterator. FSTs can be written or read using Python's `[]` syntax, as well as via
instance methods. A `Far` can also be used as part of a
[PEP-343 context guard](https://www.python.org/dev/peps/pep-0343/).

##### Constructor

```none
Far(filename, mode="r", arc_type="standard", far_type="default")

Pynini FAR ("Fst ARchive") object.

This class is used to either read FSTs from or write FSTs to a FAR. When
opening a FAR for writing, the user may also specify the desired arc type
and FAR type.

Args:
  filename: A string indicating the filename.
  mode: FAR IO mode; one of: "r" (open for reading), "w" (open for writing).
  arc_type: Desired arc type; this is ignored if the FAR is opened for
      reading.
  far_type: Desired FAR type; this is ignored if the FAR is opened for
      reading.
```

##### Methods

```none
arc_type(self)

Returns a string indicating the arc type.
```

```none
closed(self)

Indicates whether the FAR is closed for IO.
```

```none
far_type(self)

Returns a string indicating the FAR type.
```

```none
mode(self)

Returns a char indicating the FAR's current mode.
```

```none
name(self)

Returns the FAR's filename.
```

###### Methods when opened for reading (`mode="r"`)

```none
done(self)

Indicates whether the iterator is exhausted or not.

Returns:
  True if the iterator is exhausted, False otherwise.
```

```none
find(self, key)

Sets the current position to the first entry greater than or equal to the key (a
string) and indicates whether or not a match was found.

Args:
  key: A string key.

Returns:
  True if the key was found, False otherwise.
```

```none
get_fst(self)

Returns the FST at the current position.

Returns:
  A copy of the FST at the current position.
```

```none
get_key(self)

Returns the string key at the current position.

Returns:
  The string key at the current position.
```

```none
next(self)

Advances the iterator.
```

```none
reset(self)

Resets the iterator to the initial position.
```

###### Methods when opened for writing (`mode="w"`)

```none
add(self, key, fst)

Adds an FST to the FAR (when open for writing).

This methods adds an FST to the FAR which can be retrieved with the
specified string key.

This method is provided for compatibility with the C++ API only; most users
should use the Pythonic API.

Args:
  key: The string used to key the input FST.
  fst: The FST to write to the FAR.

Raises:
  FstOpError: Cannot invoke method in current mode.
  FstOpError: Incompatible or invalid arc type.
```

```none
close(self)

Closes the FAR and flushes to disk (when open for writing).

Raises:
  FstOpError: Cannot invoke method in current mode.
```

#### `Fst`

This class represents a mutable, expanded FST. In addition to its constructor,
it can be created using the [string compilation function](#acceptor) `acceptor`,
[the cross-product construction functions](#transducer) `transducer`,
`string_map`, and `string_file`, as well as by the various constructive
[algorithms](#algorithms).

##### Constructors

```none
Fst(arc_type="standard")

This class wraps a mutable FST and exposes all destructive methods.

Args:
  arc_type: An optional string indicating the arc type for the FST.
```

```none
Fst.read(filename)

Reads an FST from a file.

Args:
  filename: The string location of the input file.

Returns:
  An FST.

Raises:
  FstIOError: Read failed.
  FstOpError: Read-time conversion failed.
```

```none
Fst.from_pywrapfst(ifst)

Constructs a Pynini FST from a pywrapfst._Fst.

This class method converts an FST from the pywrapfst module (pywrapfst._Fst
or its subclass pywrapfst._MutableFst) to a Pynini.Fst. This is essentially
a downcasting operation which grants the object additional instance methods,
including an enhanced `closure`, `paths`, and `print`. The input FST is
not invalidated, but mutation of the input or output object while the other
is still in scope will trigger a deep copy.

Args:
  ifst: Input FST of type pywrapfst._Fst.

Returns:
  An FST.
```

##### Methods

Note that many methods here are "destructive" in that they mutate the underlying
FST. Wherever possible, such methods return `self` to allow
[method chaining](https://en.wikipedia.org/wiki/Method_chaining).

```none
add_arc(self, state, arc)

Adds a new arc to the FST and return self.

Args:
  state: The integer index of the source state.
  arc: The arc to add.

Returns:
  self.

Raises:
  FstIndexError: State index out of range.
  FstOpdexError: Incompatible or invalid weight type.
```

```none
add_state(self)

Adds a new state to the FST and returns the state ID.

Returns:
  The integer index of the new state.
```

```none
arc_type(self)

Returns a string indicating the arc type.
```

```none
arcs(self, state)

Returns an iterator over arcs leaving the specified state.

Args:
  state: The source state ID.

Returns:
  An _ArcIterator.
```

```none
arcsort(self, sort_type="ilabel")

Sorts arcs leaving each state of the FST.

This operation destructively sorts arcs leaving each state using either
input or output labels.

Args:
  sort_type: Either "ilabel" (sort arcs according to input labels) or
      "olabel" (sort arcs according to output labels).

Returns:
  self.

Raises:
  FstArgError: Unknown sort type.
```

See also:
[ArcSort algorithm](http://www.openfst.org/twiki/bin/view/FST/ArcSortDoc).

```none
closure(self, lower)
closure(self, lower, upper)

Computes concatenative closure.

This operation destructively converts the FST to its concatenative closure.
If A transduces string x to y with weight w, then the zero-argument form
`A.closure()` mutates A so that it transduces between empty strings with
weight 1, transduces string x to y with weight w, transduces xx to yy with
weight w \otimes w, string xxx to yyy with weight w \otimes w \otimes w
(and so on).

When called with two optional positive integer arguments, these act as
lower and upper bounds, respectively, for the number of cycles through the
original FST that the mutated FST permits. Therefore, `A.closure(0, 1)`
mutates A so that it permits 0 or 1 cycles; i.e., the mutated A transduces
between empty strings or transduces x to y.

When called with one optional positive integer argument, this argument
acts as the lower bound, with the upper bound implicitly set to infinity.
Therefore, `A.closure(1)` performs a mutation roughly equivalent to
`A.closure()` except that the former does not transduce between empty
strings.

The following are the equivalents for the closure-style syntax used in
Perl-style regular expressions:

Regexp:             This method:            Copy shortcuts:

/x?/                x.closure(0, 1)         x.ques
/x*/                x.closure()             x.star
/x+/                x.closure(1)            x.plus
/x{N}/              x.closure(N, N)         x ** N
/x{M,N}/            x.closure(M, N)
/x{N,}/             x.closure(N)
/x{,N}/             x.closure(0, N)

Args:
  lower: lower bound.
  upper: upper bound.

Returns:
  self.
```

See also:
[Closure algorithm](http://www.openfst.org/twiki/bin/view/FST/ClosureDoc),
[ConcatRange algorithm](/opengrm/operators/README.md#concat-range).

```none
concat(self, ifst)

Computes the concatenation (product) of two FSTs.

This operation destructively concatenates the FST with a second FST. If A
transduces string x to y with weight a and B transduces string w to v with
weight b, then their concatenation transduces string xw to yv with weight
a \otimes b.

Args:
  ifst: The second input Fst.

Returns:
  self.

Raises:
  FstOpError: Operation failed.
```

See also:
[Concat algorithm](http://www.openfst.org/twiki/bin/view/FST/ConcatDoc).

```none
connect(self)

Removes unsuccessful paths.

This operation destructively trims the FST, removing states and arcs that
are not part of any successful path.

Returns:
  self.
```

See also:
[Connect algorithm](http://www.openfst.org/twiki/bin/view/FST/ConnectDoc).

```none
copy(self)

Makes a copy of the FST.
```

```none
decode(self, encoder)

Decodes encoded labels and/or weights.

This operation reverses the encoding performed by `encode`.

Args:
  encoder: An EncodeMapper object used to encode the FST.

Returns:
  self.
```

See also:
[Encode and Decode algorithms](http://www.openfst.org/twiki/bin/view/FST/EncodeDecodeDoc).

```none
delete_arcs(self, state, n=0)

Deletes arcs leaving a particular state.

Args:
  state: The integer index of a state.
  n: An optional argument indicating how many arcs to be deleted. If this
      argument is omitted or passed as zero, all arcs from this state are
      deleted.

Returns:
  self.

Raises:
  FstIndexError: State index out of range.
```

```none
delete_states(self, states=None)

Deletes states.

Args:
  states: An optional iterable of integer indices of the states to be
      deleted. If this argument is omitted, all states are deleted.

Returns:
  self.

Raises:
  FstIndexError: State index out of range.

See also: `delete_arcs`.
```

```none
draw(self, filename, isymbols=None, osymbols=None, ssymbols=None,
     acceptor=False, title="", width=8.5, height=11, portrait=False,
     vertical=False, ranksep=0.4, nodesep=0.25, fontsize=14,
     precision=5, float_format="g", show_weight_one=False):

Writes out the FST in Graphviz text format.

This method writes out the FST in the dot graph description language. The
graph can be rendered using the `dot` executable provided by Graphviz.

Args:
  filename: The string location of the output dot/Graphviz file.
  isymbols: An optional symbol table used to label input symbols.
  osymbols: An optional symbol table used to label output symbols.
  ssymbols: An optional symbol table used to label states.
  acceptor: Should the figure be rendered in acceptor format if possible?
  title: An optional string indicating the figure title.
  width: The figure width, in inches.
  height: The figure height, in inches.
  portrait: Should the figure be rendered in portrait rather than
      landscape?
  vertical: Should the figure be rendered bottom-to-top rather than
      left-to-right?
  ranksep: The minimum separation separation between ranks, in inches.
  nodesep: The minimum separation between nodes, in inches.
  fontsize: Font size, in points.
  precision: Numeric precision for floats, in number of chars.
  float_format: One of: 'e', 'f' or 'g'.
  show_weight_one: Should weights equivalent to semiring One be printed?
```

See also:
[Printing, Drawing, and summarizing FSTs](http://openfst.org/twiki/bin/view/FST/FstQuickTour#Printing_Drawing_and_Summarizing).

```none
encode(self, encoder)

Encodes labels and/or weights.

This operation allows for the representation of a weighted transducer as a
weighted acceptor, an unweighted transducer, or an unweighted acceptor by
considering the pair (input label, output label), the pair (input label,
weight), or the triple (input label, output label, weight) as a single
label. Applying this operation mutates the EncodeMapper argument, which
can then be used to decode.

Args:
  encoder: An EncodeMapper object to be used as the encoder.

Returns:
  self.
```

See also:
[Encode and Decode algorithms](http://www.openfst.org/twiki/bin/view/FST/EncodeDecodeDoc).

```none
final(self, state)

Returns the final weight of a state.

Args:
  state: The integer index of a state.

Returns:
  The final Weight of that state.

Raises:
  FstIndexError: State index out of range.
```

```none
fst_type(self)

Returns a string indicating the FST type.
```

```none
input_symbols(self)

Returns the FST's input symbol table, or None if none is present.
```

```none
invert(self)

Inverts the FST's transduction.

This operation destructively inverts the FST's transduction by exchanging
input and output labels.

Returns:
  self.
```

See also:
[Invert algorithm](http://www.openfst.org/twiki/bin/view/FST/InvertDoc).

```none
minimize(self, delta=0.0009765625, allow_nondet=False)

Minimizes the FST.

This operation destructively performs the minimization of deterministic
weighted automata and transducers. If the input FST A is an acceptor, this
operation produces the minimal acceptor B equivalent to A, i.e. the
acceptor with a minimal number of states that is equivalent to A. If the
input FST A is a transducer, this operation internally builds an equivalent
transducer with a minimal number of states. However, this minimality is
obtained by allowing transition having strings of symbols as output labels,
this known in the litterature as a real-time transducer. Such transducers
are not directly supported by the library. This function will convert such
transducer by expanding each string-labeled transition into a sequence of
transitions. This will results in the creation of new states, hence losing
the minimality property.

Args:
  delta: Comparison/quantization delta.
  allow_nondet: Attempt minimization of non-deterministic FST?

Returns:
  self.
```

See also:
[Minimize algorithm](http://www.openfst.org/twiki/bin/view/FST/MinimizeDoc).

```none
mutable_arcs(self, state)

Returns a mutable iterator over arcs leaving the specified state.

Args:
  state: The source state ID.

Returns:
  A _MutableArcIterator.
```

```none
mutable_input_symbols(self)

Returns the FST's (mutable) input symbol table, or None if none is present.
```

```none
mutable_output_symbols(self)

Returns the FST's (mutable) output symbol table, or None if none is present.
```

```none
num_arcs(self, state)

Returns the number of arcs leaving a state.

Args:
  state: The integer index of a state.

Returns:
  The number of arcs leaving that state.

Raises:
  FstIndexError: State index out of range.
```

```none
num_input_epsilons(self, state)

Returns the number of arcs with epsilon input labels leaving a state.

Args:
  state: The integer index of a state.

Returns:
  The number of epsilon-input-labeled arcs leaving that state.

Raises:
  FstIndexError: State index out of range.
```

```none
num_output_epsilons(self, state)

Returns the number of arcs with epsilon output labels leaving a state.

Args:
  state: The integer index of a state.

Returns:
  The number of epsilon-output-labeled arcs leaving that state.

Raises:
  FstIndexError: State index out of range.
```

```none
num_states(self)

Returns the number of states.
```

```none
optimize(self, compute_props=False)

Performs a generic optimization of the FST.

This operation destructively optimizes the FST using epsilon-removal,
arc-sum mapping, determinization, and minimization (where possible). The
algorithm is as follows:

* If the FST is not (known to be) epsilon-free, perform epsilon-removal.
* Combine identically labeled multi-arcs and sum their weights.
* If the FST does not have idempotent weights, halt.
* If the FST is not (known to be) deterministic:
  - If the FST is a (known) acceptor:
    * If the FST is not (known to be) unweighted and/or acyclic, encode
      weights.
  - Otherwise, encode labels and, if the FST is not (known to be)
    unweighted, encode weights.
  - Determinize the FST.
* Minimize the FST.
* Decode the FST and combine identically-labeled multi-arcs and sum their
  weights, if the FST was previously encoded.

By default, FST properties are not computed if they are not already set.

This optimization may result in a reduction of size (due to epsilon-removal,
arc-sum mapping, and minimization) and possibly faster composition, but
determinization (a prerequisite of minimization) may result in an
exponential blowup in size in the worst case. Judicious use of optimization
is a bit of a black art.

Args:
  compute_props: Should unknown FST properties be computed to help choose
      appropriate optimizations?

Returns:
  self.
```

See also: [Optimize algorithm](/opengrm/operators/README.md#optimize).

```none
output_symbols(self)

Returns the FST's output symbol table, or None if none is present.
```

```none
paths(self, input_token_type=None, output_token_type=None)

Creates iterator over all string paths in an acyclic FST.

This method returns an iterator over all paths (represented as pairs of
strings and an associated path weight) through an acyclic FST. This
operation is only feasible when the FST is acyclic. Depending on the
requested token type, the arc labels along the input and output sides of a
path are interpreted as Unicode codepoints, raw bytes, or a
concatenation of string labels from a symbol table.

Args:
  input_token_type: An optional string indicating how the input strings are to
      be constructed from arc labels---one of: "byte" (interprets arc labels
      as raw bytes), "utf8" (interprets arc labels as Unicode code points),
      or "symbol" (interprets arc labels using the input symbol table).  If not
      set, or set to None, the value is set to the default token_type, which
      begins as "byte", but can be overridden for regions of code using the
      default_token_type context manager.
  output_token_type: An optional string indicating how the output strings are to
      be constructed from arc labels---one of: "byte" (interprets arc labels
      as raw bytes), "utf8" (interprets arc labels as Unicode code points),
      or "symbol" (interprets arc labels using the input symbol table).  If not
      set, or set to None, the value is set to the default token_type, which
      begins as "byte", but can be overridden for regions of code using the
      default_token_type context manager.

Raises:
  FstArgError: Unknown token type.
  FstArgError: FST is not acyclic.
```

See also: [_StringPathIterator algorithm](/opengrm/paths/README.md#string-paths).

```none
print(self, isymbols=None, osymbols=None, ssymbols=None, acceptor=False,
      show_weight_one=False, missing_sym="")

Produces a human-readable string representation of the FST.

This method generates a human-readable string representation of the FST.
The caller may optionally specify SymbolTables used to label input labels,
output labels, or state labels, respectively.

Args:
  isymbols: An optional symbol table used to label input symbols.
  osymbols: An optional symbol table used to label output symbols.
  ssymbols: An optional symbol table used to label states.
  acceptor: Should the FST be rendered in acceptor format if possible?
  show_weight_one: Should weights equivalent to semiring One be printed?
  missing_symbol: The string to be printed when symbol table lookup fails.

Returns:
  A formatted string representing the machine.
```

```none
project(self, project_type)

Converts the FST to an acceptor using input or output labels.

This operation destructively projects an FST onto its domain or range by
either copying each arc's input label to its output label (the default) or
vice versa.

Args:
  project_type: A string matching a known projection type; one of:
        "input", "output".

Returns:
  self.
```

See also:
[Project algorithm](http://www.openfst.org/twiki/bin/view/FST/ProjectDoc).

```none
properties(self, mask)

Provides property bits.

This method provides user access to the properties of the encoder.

Args:
  mask: The property mask to be compared to the encoder's properties.

Returns:
  A 64-bit bitmask representing the requested properties.
```

See also: [FST properties](#properties).

```none
prune(self, delta=0.0009765625, nstate=-1, weight=None)

Removes paths with weights below a certain threshold.

This operation deletes states and arcs in the input FST that do not belong
to a successful path whose weight is no more (w.r.t the natural semiring
order) than the threshold t \otimes-times the weight of the shortest path in
the input FST. Weights must be commutative and have the path property.

Args:
  delta: Comparison/quantization delta.
  nstate: State number threshold.
  weight: A Weight or weight string indicating the desired weight threshold
      below which paths are pruned; if omitted, no paths are pruned.

Returns:
  self.
```

See also: [Prune algorithm](http://www.openfst.org/twiki/bin/view/FST/PruneDoc).

```none
push(self, delta=0.0009765625, remove_total_weight=False,
     reweight_type="to_initial")

Pushes weights towards the initial or final states.

This operation destructively produces an equivalent transducer by pushing
the weights towards the initial state or toward the final states. When
pushing weights towards the initial state, the sum of the weight of the
outgoing transitions and final weight at any non-initial state is equal to
one in the resulting machine. When pushing weights towards the final states,
the sum of the weight of the incoming transitions at any state is equal to
one. Weights need to be left distributive when pushing towards the initial
state and right distributive when pushing towards the final states.

Args:
  delta: Comparison/quantization delta.
  remove_total_weight: If pushing weights, should the total weight be
      removed?
  reweight_type: Push towards initial or final states: a string matching a known
      reweight type: one of "to_initial", "to_final"

Returns:
  self.
```

See also: [Push algorithm](http://www.openfst.org/twiki/bin/view/FST/PushDoc).

```none
relabel_pairs(self, ipairs=None, opairs=None)

Replaces input and/or output labels using pairs of labels.

This operation destructively relabels the input and/or output labels of the
FST using pairs of the form (old_ID, new_ID); omitted indices are
identity-mapped.

Args:
  ipairs: An iterable containing (older index, newer index) integer pairs.
  opairs: An iterable containing (older index, newer index) integer pairs.

Returns:
  self.

Raises:
  FstArgError: No relabeling pairs specified.
```

See also:
[Relabel algorithm](http://www.openfst.org/twiki/bin/view/FST/RelabelDoc).

```none
relabel_tables(self, old_isymbols=None, new_isymbols=None,
               unknown_isymbol="", attach_new_isymbols=True,
               old_osymbols=None, new_osymbols=None,
               unknown_osymbol="", attach_new_osymbols=True)

Replaces input and/or output labels using SymbolTables.

This operation destructively relabels the input and/or output labels of the
FST using user-specified symbol tables; omitted symbols are identity-mapped.

Args:
  old_isymbols: The old SymbolTable for input labels, defaulting to the
      FST's input symbol table.
  new_isymbols: A SymbolTable used to relabel the input labels
  unknown_isymbol: Input symbol to use to relabel OOVs (if empty,
      OOVs raise an exception)
  attach_new_isymbols: Should new_isymbols be made the FST's input symbol
      table?
  old_osymbols: The old SymbolTable for output labels, defaulting to the
      FST's output symbol table.
  new_osymbols: A SymbolTable used to relabel the output labels.
  unknown_osymbol: Output symbol to use to relabel OOVs (if empty,
      OOVs raise an exception)
  attach_new_isymbols: Should new_osymbols be made the FST's output symbol
      table?

Returns:
  self.

Raises:
  FstArgError: No SymbolTable specified.
```

See also:
[Relabel algorithm](http://www.openfst.org/twiki/bin/view/FST/RelabelDoc).

```none
reserve_arcs(self, state, n)

Reserve n arcs at a particular state (best effort).

Args:
  state: The integer index of a state.
  n: The number of arcs to reserve.

Returns:
  self.

Raises:
  FstIndexError: State index out of range.
```

```none
reserve_states(self, n)

Reserve n states (best effort).

Args:
  n: The number of states to reserve.

Returns:
  self.
```

```none
reweight(self, potentials, to_final=False)

Reweights an FST using an iterable of potentials.

This operation destructively reweights an FST according to the potentials
and in the direction specified by the user. An arc of weight w, with an
origin state of potential p and destination state of potential q, is
reweighted by p^{-1} \otimes (w \otimes q) when reweighting towards the
initial state, and by (p \otimes w) \otimes q^{-1} when reweighting towards
the final states. The weights must be left distributive when reweighting
towards the initial state and right distributive when reweighting towards
the final states (e.g., TropicalWeight and LogWeight).

Args:
  potentials: An iterable of Weight or weight strings.
  to_final: Push towards final states?

Returns:
  self.
```

See also:
[Reweight algorithm](http://www.openfst.org/twiki/bin/view/FST/ReweightDoc).

```none
rmepsilon(self, connect=True, delta=0.0009765625, nstate=-1, weight=None)

Removes epsilon transitions.

This operation destructively removes epsilon transitions, i.e., those where
both input and output labels are epsilon) from an FST.

Args:
  connect: Should output be trimmed?
  delta: Comparison/quantization delta.
  nstate: State number threshold.
  weight: A Weight or weight string indicating the desired weight threshold
      below which paths are pruned; if omitted, no paths are pruned.

Returns:
  self.
```

See also:
[RmEpsilon algorithm](http://www.openfst.org/twiki/bin/view/FST/RmEpsilonDoc).

```none
set_final(self, state, weight)

Sets the final weight for a state.

Args:
  state: The integer index of a state.
  weight: A Weight or weight string indicating the desired final weight; if
      omitted, it is set to semiring One.

Raises:
  FstIndexError: State index out of range.
  FstOpError: Incompatible or invalid weight.
```

```none
set_input_symbols(self, syms)

Sets the input symbol table.

Passing None as a value will delete the input symbol table.

Args:
  syms: A SymbolTable.

Returns:
  self.
```

```none
set_output_symbols(self, syms)

Sets the output symbol table.

Passing None as a value will delete the output symbol table.

Args:
  syms: A SymbolTable.

Returns:
  self.
```

```none
set_properties(self, props, mask)

Sets the properties bits.

Args:
  props: The properties to be set.
  mask: A mask to be applied to the `props` argument before setting the
      FST's properties.

Returns:
  self.
```

```none
set_start(self, state)

Sets a state to be the initial state.

Args:
  state: The integer index of a state.

Returns:
  self.

Raises:
  FstIndexError: State index out of range.
```

```none
start(self)

Returns the start state.
```

```none
states(self)

Returns an iterator over all states in the FST.

Returns:
  A StateIterator object for the FST.
```

```none
string(self, token_type=None)

Creates a string from a string FST.

This method returns the string recognized by the FST as a Python byte or
Unicode string. This is only well-defined when the FST is an acceptor and a
"string" FST (meaning that the start state is numbered 0, and there is
exactly one transition from each state i to each state i + 1, there are no
other transitions, and the last state is final). Depending on the requested
token type, the arc labels are interpreted as a Unicode codepoints, raw bytes,
or as a concatenation of string labels from the output
symbol table.

The underlying routine reads only the output labels, so if the FST is
not an acceptor, it will be treated as the output projection of the FST.

Args:
  token_type: An optional string indicating how the string is to be constructed
      from arc labels---one of: "byte" (interprets arc labels as raw bytes),
      "utf8" (interprets arc labels as Unicode code points), or a
      SymbolTable. If not set, or set to None, the value is set to the
      default token_type, which begins as "byte", but can be overridden for
      regions of code using the default_token_type context manager.

Returns:
  The string corresponding to (an output projection) of the FST.

Raises:
  FstArgError: FST is not a string.
  FstArgError: Unknown token type.
```

Note: use `print(f)` as an alias for `print(f.print())`.

```none
topsort(self)

Sorts transitions by state IDs.

This operation destructively topologically sorts the FST, if it is acyclic;
otherwise it remains unchanged. Once sorted, all transitions are from lower
state IDs to higher state IDs

Returns:
   self.
```

See also:
[TopSort algorithm](http://www.openfst.org/twiki/bin/view/FST/TopSortDoc).

```none
union(self, ifst)

Computes the union (sum) of two FSTs.

This operation destructively computes the union (sum) of two FSTs. If A
transduces string x to y with weight a and B transduces string w to v with
weight b, then their union transduces x to y with weight a and w to v with
weight b.

Args:
  ifst: The second input FST.

Returns:
  self.

Raises:
  FstOpError: Operation failed.
```

See also: [Union algorithm](http://www.openfst.org/twiki/bin/view/FST/UnionDoc).

```none
verify(self)

Verifies that an FST's contents are sane.

Returns:
  True if the contents are sane, False otherwise.
```

Note: use this in combination with assertions in tests or library code.

```none
weight_type(self)

Provides the FST's weight type.

Returns:
  A string representing the weight type.
```

```none
write(self, filename)

Serializes FST to a file.

This method writes the FST to a file in a binary format.

Args:
  filename: The string location of the output file.

Raises:
  FstIOError: Write failed.
```

```none
write_to_string(self)

Serializes FST to a string.

Returns:
  A string.
```

#### `MPdtParentheses`

The MPDT parentheses class stores triples consisting of "push" and "pop"
parentheses arc labels and an associated stack label for a
[_multi-pushdown transducer_](http://openfst.org/twiki/bin/view/FST/MpdtExtension).

##### Constructor

```none
MPdtParentheses()

Multi-pushdown transducer parentheses class.

This class wraps a vector of pairs of arc labels in which the first label is
interpreted as a "push" stack operation and the second represents the
corresponding "pop" operation, and an equally sized vector which assigns each
pair to a stack. The library currently only permits two stacks (numbered 1 and
2) to be used.

A MPDT is expressed as an (Fst, MPdtParentheses) pair for the purposes of all
supported MPDT operations.
```

##### Methods

```none
add_triple(self, push, pop, assignment)

Adds a triple of (left parenthesis, right parenthesis, stack assignment)
triples to the object.

Args:
  push: An arc label to be interpreted as a "push" operation.
  pop: An arc label to be interpreted as a "pop" operation.
  assignment: An arc label indicating what stack the parentheses pair
      is assigned to.
```

```none
copy(self)

Makes a copy of this MPdtParentheses object.

Returns:
  A deep copy of the MPdtParentheses object.
```

```none
MPdtParentheses.read(filename)

Reads parentheses/assignment triples from a text file.

This class method creates a new MPdtParentheses object from a pairs of
integer labels in a text file.

Args:
  filename: The string location of the input file.

Returns:
  A new MPdtParentheses instance.

Raises:
  FstIOError: Read failed.
```

```none
write(self, filename)

Writes parentheses triples to a text file.

This method writes the MPdtParentheses object to a text file.

Args:
  filename: The string location of the output file.

Raises:
  FstIOError: Write failed.
```

#### `_MutableArcIterator`

A mutable arc iterator is used to iterate through arcs leaving some state in the
FST; it is constructed by calling the `mutable_arcs` method of an `Fst` object.

This iterator is invalidated if the underlying FST is mutated _by any other
means than invocations of the `set_value` method_.

##### Constructor

```none
_MutableArcIterator(ifst, state)

This class is used for iterating over the arcs leaving some state of an FST,
also permitting mutation of the current arc.
```

##### Methods

```none
done(self)

Indicates whether the iterator is exhausted or not.
```

```none
flags(self)

Returns the current iterator behavioral flags.

Returns:
  The current iterator behavioral flags as an integer.
```

```none
next(self)

Advances the iterator.
```

```none
position(self)

Returns the position of the iterator.

Returns:
  The iterator's position, expressed as an integer.
```

```none
reset(self)

Resets the iterator to the initial position.
```

```none
seek(self, a)

Advance the iterator to a new position.

Args:
  a: The position to seek to.
```

```none
set_flags(self, flags, mask)

Sets the current iterator behavioral flags.

Args:
  flags: The properties to be set.
  mask: A mask to be applied to the `flags` argument before setting them.
```

```none
set_value(self, arc)

Replace the current arc with a new arc.

Args:
  arc: The arc to replace the current arc with.
```

```none
value(self)

Returns the current arc.
```

#### `PdtParentheses`

The PDT parentheses class stores pairs of "push" and "pop" parentheses arc
labels for a
[_pushdown transducer_](http://openfst.org/twiki/bin/view/FST/PdtExtension).

##### Constructor

```none
PdtParentheses()

Pushdown transducer parentheses class.

This class wraps a vector of pairs of arc labels in which the first label is
interpreted as a "push" stack operation and the second represents the
corresponding "pop" operation. When efficiency is desired, the push and pop
indices should be contiguous.

A PDT is expressed as an (Fst, PdtParentheses) pair for the purposes of all
supported PDT operations.
```

##### Methods

```none
add_pair(self, push, pop)

Adds a pair of parentheses to the set.

Args:
  push: An arc label to be interpreted as a "push" operation.
  pop: An arc label to be interpreted as a "pop" operation.
```

```none
copy(self)

Makes a copy of this PdtParentheses object.

Returns:
  A deep copy of the PdtParentheses object.
```

```none
PdtParentheses.read(filename)

Reads parentheses pairs from a text file.

This class method creates a new PdtParentheses object from a pairs of
integer labels in a text file.

Args:
  filename: The string location of the input file.

Returns:
  A new PdtParentheses instance.

Raises:
  FstIOError: Read failed.
```

```none
write(self, filename)

Writes parentheses triples to a text file.

This method writes the MPdtParentheses object to a text file.

Args:
  filename: The string location of the output file.

Raises:
  FstIOError: Write failed.
```

#### `StateIterator`

An state iterator is used to iterate through state IDs in an FST; it is normally
constructed by calling the `states` method of an `Fst` object.

##### Constructor

```
StateIterator(ifst)

This class is used for iterating over the states in an FST.
```

##### Methods

```none
done(self)

Indicates whether the iterator is exhausted or not.

Returns:
  True if the iterator is exhausted, False otherwise.
```

```none
next(self)

Advances the iterator.
```

```none
reset(self)

Resets the iterator to the initial position.
```

```none
value(self)

Returns the current state index.
```

#### `_StringPathIterator` <a id="string-paths"></a>

The string paths object is used to iterate through paths of an
[acyclic](https://en.wikipedia.org/wiki/Directed_acyclic_graph) FST, in the
forms of input and output label sequences, input and output strings, and path
weights.

See also: [_StringPathIterator algorithm](/opengrm/paths/README.md#string-paths).

##### Constructor

```
_StringPathIterator(fst, input_token_type=None, output_token_type=None)

Iterator for string paths in acyclic FST.

This class provides an iterator over all paths (represented as pairs of
strings and an associated path weight) through an acyclic FST. This
operation is only feasible when the FST is acyclic. Depending on the
requested token type, the arc labels along the input and output sides of a
path are interpreted as Unicode codepoints, raw bytes, or a
concatenation of string labels from a symbol table. This class is normally
created by invoking the `paths` method of `Fst`.

Args:
  input_token_type: An optional string indicating how the input strings are to be
      constructed from arc labels---one of: "byte" (interprets arc labels
      as raw bytes), "utf8" (interprets arc labels as Unicode code points),
      or a SymbolTable. If not set, or set to None, the value is set to the
      default token_type, which begins as "byte", but can be overridden for
      regions of code using the default_token_type context manager.
  output_token_type: An optional string indicating how the output strings are to
      be constructed from arc labels---one of: "byte" (interprets arc labels
      as raw bytes), "utf8" (interprets arc labels as Unicode code points),
      or a SymbolTable. If not set, or set to None, the value is set to the
      default token_type, which begins as "byte", but can be overridden for
      regions of code using the default_token_type context manager.

Raises:
  FstArgError: Unknown token type.
  FstArgError: FST is not acyclic.
```

##### Methods

```none
done(self)

Indicates whether the iterator is exhausted or not.
```

```none
error(self)

Indicates whether the _StringPathIterator has encountered an error.

Returns:
  True if the _StringPathIterator is in an errorful state, False otherwise.
```

```none
ilabels(self)

Returns the input labels for the current path.

Returns:
  A list of input labels for the current path.
```

```none
istring(self)

Returns the current path's input string.

This method is provided for compatibility with the C++ API only; most users
should use the Pythonic API.

Returns:
  The path's input string.
```

```none
next(self)

Advances the iterator.
```

```none
istrings(self)

Generates all input strings in the FST.

This method returns a generator over all input strings in the path. The
caller is responsible for resetting the iterator if desired.

Yields:
  All input strings.
```

```none
ostrings(self)

Generates all output strings in the FST.

This method returns a generator over all output strings in the path. The
caller is responsible for resetting the iterator if desired.

Yields:
  All output strings.
```

```none
weights(self)

Generates all path weights in the FST.

This method returns a generator over all path weights. The caller is
responsible for resetting the iterator if desired.

Yields:
  All weights.
```

```none
olabels(self)

Returns the output labels for the current path.

Returns:
  A list of output labels for the current path.
```

```none
ostring(self)

Returns the current path's output string.

Returns:
  The path's output string.
```

```none
weight(self)

Returns the current path's total weight.

Returns:
  The path's Weight.
```

#### `SymbolTable`

A symbol table stores a bidirectional mapping between arc labels and "symbols"
(strings). They can be attached to FSTs or encoders, or iterated through using a
[`SymbolTableIterator`](#symbol-table-iterator).

##### Constructor

```none
SymbolTable(name="<unspecified>")

Mutable SymbolTable class.

This class wraps the library SymbolTable and exposes both const (i.e.,
access) and non-const (i.e., mutation) methods of wrapped object.
```

##### Methods

```none
add_symbol(self, symbol, key=-1)

Adds a symbol to the table and returns the index.

This method adds a symbol to the table. The caller can optionally
specify a non-negative integer index for the key.

Args:
  symbol: A symbol string.
  key: An index for the symbol (-1 is reserved for "no symbol requested").

Returns:
  The integer key of the new symbol.
```

```none
add_table(self, syms)

Adds another SymbolTable to this table.

This method merges another symbol table into the current table. All key
values will be offset by the current available key.

Args:
  syms: A SymbolTable to be merged with the current table.
```

```none
available_key(self)

Returns an integer indicating the next available key index in the table.
```

```none
checksum(self)

Returns a string indicating the label-agnostic MD5 checksum for the table.
```

```none
copy(self)

Returns a mutable copy of the SymbolTable.
```

```none
find(self, key)

Given a symbol or index, finds the other one.

This method returns the index associated with a symbol key, or the symbol
associated with a index key.

Args:
  key: Either a string or an index.

Returns:
  If key is a string, the associated index; if key is an integer, the
      associated symbol.

Raises:
  KeyError: Key not found.
```

```none
get_nth_key(self, pos)

Retrieves the integer index of the n-th key in the table.

Args:
  pos: The n-th key to retrieve.

Returns:
  The integer index of the n-th key.

Raises:
  KeyError: index not found.
```

```none
labeled_checksum(self)

Returns a string indicating the label-dependent MD5 checksum for the table.
```

```none
member(self, key)

Given a symbol or index, returns whether it is found in the table.

This method returns a boolean indicating whether the given symbol or index
is present in the table. If one intends to perform subsequent lookup, it is
better to simply call the find method, catching the KeyError.

Args:
  key: Either a string or an index.

Returns:
  Whether or not the key is present (as a string or a index) in the table.
```

```none
name(self)

Returns the symbol table's name.
```

```none
name(self)

Returns the symbol table's name.
```

```none
SymbolTable.read(filename)

Reads symbol table from binary file.

This class method creates a new SymbolTable from a symbol table binary file.

Args:
  filename: The string location of the input binary file.

Returns:
  A new SymbolTable instance.
```

```none
SymbolTable.read_text(filename)

Reads symbol table from text file.

This class method creates a new SymbolTable from a symbol table text file.

Args:
  filename: The string location of the input text file.
  allow_negative_labels: Should negative labels be allowed? (Not
      recommended; may cause conflicts).

Returns:
  A new SymbolTable instance.
```

```none
write(self, filename)

Serializes symbol table to a file.

This methods writes the SymbolTable to a file in binary format.

Args:
  filename: The string location of the output file.

Raises:
  FstIOError: Write failed.
```

```none
write_text(self, filename)

Writes symbol table to text file.

This method writes the SymbolTable to a file in human-readable format.

Args:
  filename: The string location of the output file.

Raises:
  FstIOError: Write failed.
```

#### `SymbolTableIterator` <a id="symbol-table-iterator"></a>

A symbol table iterator is used to iterate through label/string pairs stored in
a symbol table.

##### Constructor

```none
SymbolTableIterator(syms)

This class is used for iterating over a symbol table.
```

##### Methods

```none
done(self)

Indicates whether the iterator is exhausted or not.

Returns:
  True if the iterator is exhausted, False otherwise.
```

```none
next(self)

Advances the iterator.
```

```none
reset(self)

Resets the iterator to the initial position.
```

```none
symbol(self)

Returns the current symbol string.

This method returns the current symbol string at this point in the table.

Returns:
  A symbol string.
```

```none
value(self)

Returns the current integer index of the symbol.

Returns:
  An integer index.
```

#### `Weight`

The weight class is used to represent weights which can be attached to arcs or
final states in an FST. They also support semiring-dependent arithmetic
operations via external functions.

##### Constructors

```none
Weight(weight_type, weight_string)

FST weight class.

This class represents an FST weight. When passed as an argument to an FST
operation, it should have the weight type of the input FST(s) to said
operation.

Args:
  weight_type: A string indicating the weight type.
  weight_string: A string indicating the underlying weight.

Raises:
  FstArgError: Weight type not found.
  FstBadWeightError: Invalid weight.
```

```none
Weight.no_weight(weight_type)

Constructs a non-member weight in the semiring.
```

```none
Weight.one(weight_type)

Constructs semiring One.
```

```none
Weight.zero(weight_type)

Constructs semiring zero.
```

##### Methods

```none
copy(self)

Returns a copy of the Weight.
```

```none
type(self)

Returns a string indicating the weight type.
```

#### `Fst` algorithm functions <a id="algorithms"></a>

FSTs support the following constructive FST algorithm functions, which take one
or more `Fst` arguments and (usually) return a new `Fst`.

```none
arcmap(ifst, delta=0.0009765625, map_type="identity", weight=None)

Constructively applies a transform to all arcs and final states.

This operation transforms each arc and final state in the input FST using
one of the following:

  * identity: maps to self.
  * input_epsilon: replaces all input labels with epsilon.
  * invert: reciprocates all non-Zero weights.
  * output_epsilon: replaces all output labels with epsilon.
  * plus: adds a constant to all weights.
  * quantize: quantizes weights.
  * rmweight: replaces all non-Zero weights with 1.
  * superfinal: redirects final states to a new superfinal state.
  * times: right-multiplies a constant to all weights.
  * to_log: converts weights to the log semiring.
  * to_log64: converts weights to the log64 semiring.
  * to_standard: converts weights to the tropical ("standard") semiring.

Args:
  ifst: The input FST.
  delta: Comparison/quantization delta (ignored unless `map_type` is
      `quantize`).
  map_type: A string matching a known mapping operation (see above).
  weight: A Weight or weight string passed to the arc-mapper; this is ignored
      unless `map_type` is `plus` (in which case it defaults to semiring Zero)
      or `times` (in which case it defaults to semiring One).

Returns:
  An FST.

Raises:
  FstArgError: Unknown map type.
```

See also:
[ArcMap algorithm](http://www.openfst.org/twiki/bin/view/FST/ArcMapDoc).

```none
cdrewrite(tau, lambda, rho, sigma_star, direction="ltr", mode="obl")

Generates a transducer expressing a context-dependent rewrite rule.

This operation compiles a transducer representing a context-dependent
rewrite rule of the form

    phi -> psi / lambda __ rho

over a finite vocabulary. To apply the resulting transducer, simply compose
it with an input string or lattice.

Args:
  tau: A transducer representing phi -> psi.
  lambda: An unweighted acceptor representing the left context.
  rho: An unweighted acceptor representing the right context.
  sigma_star: A cyclic, unweighted acceptor representing the closure over the
      alphabet.
  direction: A string specifying the direction of rule application; one of:
      "ltr" (left-to-right application), "rtl" (right-to-left application),
      or "sim" (simultaneous application).
  mode: A string specifying the mode of rule application; one of: "obl"
      (obligatory application), "opt" (optional application).

Returns:
  An rewrite rule FST.

Raises:
  FstArgError: Unknown cdrewrite direction type.
  FstArgError: Unknown cdrewrite mode type.
  FstOpError: Operation failed.
```

```none
compose(ifst1, ifst2, compose_filter="auto", connect=True)

Constructively composes two FSTs.

This operation computes the composition of two FSTs. If A transduces string
x to y with weight a and B transduces y to z with weight b, then their
composition transduces string x to z with weight a \otimes b. The output
labels of the first transducer or the input labels of the second transducer
must be sorted (or otherwise support appropriate matchers).

Args:
  ifst1: The first input FST.
  ifst2: The second input FST.
  compose_filter: A string matching a known composition filter; one of:
      "alt_sequence", "auto", "match", "null", "sequence", "trivial".
  connect: Should output be trimmed?

Returns:
  A composed FST.
```

See also:
[Compose algorithm](http://www.openfst.org/twiki/bin/view/FST/ComposeDoc).

```none
determinize(ifst, delta=0.0009765625, det_type="functional", nstate=-1,
            subsequential_label=0, weight=None,
            incremental_subsequential_label=False)

Constructively determinizes a weighted FST.

This operations creates an equivalent FST that has the property that no
state has two transitions with the same input label. For this algorithm,
epsilon transitions are treated as regular symbols (cf. `rmepsilon`).

Args:
  ifst: The input FST.
  delta: Comparison/quantization delta.
  det_type: Type of determinization; one of: "functional" (input transducer is
      functional), "nonfunctional" (input transducer is not functional) and
      disambiguate" (input transducer is not functional but only keep the min
      of ambiguous outputs).
  nstate: State number threshold.
  subsequential_label: Input label of arc corresponding to residual final
      output when producing a subsequential transducer.
  weight: A Weight or weight string indicating the desired weight threshold
      below which paths are pruned; if omitted, no paths are pruned.
  increment_subsequential_label: Increment subsequential when creating
      several arcs for the residual final output at a given state.

Returns:
  An equivalent deterministic FST.

Raises:
  FstArgError: Unknown determinization type.
```

See also:
[Determinize algorithm](http://www.openfst.org/twiki/bin/view/FST/DeterminizeDoc).

```none
disambiguate(ifst, delta=0.0009765625, nstate=-1, subsequential_label=0,
             weight=None):

Constructively disambiguates a weighted transducer.

This operation disambiguates a weighted transducer. The result will be an
equivalent FST that has the property that no two successful paths have the
same input labeling. For this algorithm, epsilon transitions are treated as
regular symbols (cf. `rmepsilon`).

Args:
  ifst: The input FST.
  delta: Comparison/quantization delta.
  nstate: State number threshold.
  subsequential_label: Input label of arc corresponding to residual final
      output when producing a subsequential transducer.
  weight: A Weight or weight string indicating the desired weight threshold
      below which paths are pruned; if omitted, no paths are pruned.

Returns:
  An equivalent disambiguated FST.
```

See also:
[Disambiguate algorithm](http://www.openfst.org/twiki/bin/view/FST/DisambiguateDoc).

```none
difference(ifst1, ifst2, compose_filter="auto", connect=True)

Constructively computes the difference of two FSTs.

This operation computes the difference between two FSAs. Only strings that are
in the first automaton but not in second are retained in the result. The first
argument must be an acceptor; the second argument must be an unweighted,
epsilon-free, deterministic acceptor. The output labels of the first
transducer or the input labels of the second transducer must be sorted (or
otherwise support appropriate matchers).

Args:
  ifst1: The first input FST.
  ifst2: The second input FST.
  compose_filter: A string matching a known composition filter; one of:
      "alt_sequence", "auto", "match", "null", "sequence", "trivial".
  connect: Should the output FST be trimmed?

Returns:
  An FST representing the difference of the two FSTs.
```

See also:
[Difference algorithm](http://www.openfst.org/twiki/bin/view/FST/DifferenceDoc).

```none
epsnormalize(ifst, eps_norm_type="input")

Constructively epsilon-normalizes an FST.

This operation creates an equivalent FST that is epsilon-normalized. An
acceptor is epsilon-normalized if it is epsilon-removed (cf. `rmepsilon`).
A transducer is input epsilon-normalized if, in addition, along any path, all
arcs with epsilon input labels follow all arcs with non-epsilon input labels.
Output epsilon-normalized is defined similarly. The input FST must be
functional.

Args:
  ifst: The input FST.
    eps_norm_type: A string matching a known epsilon normalization type; one of:
          "input", "output".

Returns:
  An equivalent epsilon-normalized FST.
```

See also:
[EpsNormalize algorithm](http://openfst.org/twiki/bin/view/FST/EpsNormalizeDoc).

```none
equal(ifst1, ifst2, delta=0.0009765625)

Are two FSTs equal?

This function tests whether two FSTs have the same states with the same
numbering and the same transitions with the same labels and weights in the
same order.

Args:
  ifst1: The first input FST.
  ifst2: The second input FST.
  delta: Comparison/quantization delta.

Returns:
  True if the FSTs satisfy the above condition, else False.
```

See also: [Equal algorithm](http://openfst.org/twiki/bin/view/FST/EqualDoc).

```none
equivalent(ifst1, ifst2, delta=0.0009765625)

Are the two acceptors equivalent?

This operation tests whether two epsilon-free deterministic weighted
acceptors are equivalent, that is if they accept the same strings with the
same weights.

Args:
  ifst1: The first input FST.
  ifst2: The second input FST.
  delta: Comparison/quantization delta.

Returns:
  True if the FSTs satisfy the above condition, else False.

Raises:
  FstOpError: Equivalence test encountered error.
```

See also:
[Equivalent algorithm](http://www.openfst.org/twiki/bin/view/FST/EquivalentDoc).

```none
intersect(ifst1, ifst2, compose_filter="auto", connect=True)

Constructively intersects two FSTs.

This operation computes the intersection (Hadamard product) of two FSTs.
Only strings that are in both automata are retained in the result. The two
arguments must be acceptors. One of the arguments must be label-sorted (or
otherwise support appropriate matchers).

Args:
  ifst1: The first input FST.
  ifst2: The second input FST.
  compose_filter: A string matching a known composition filter; one of:
      "alt_sequence", "auto", "match", "null", "sequence", "trivial".
  connect: Should output be trimmed?

Returns:
  An intersected FST.
```

See also:
[Intersect algorithm](http://www.openfst.org/twiki/bin/view/FST/IntersectDoc).

```none
leniently_compose(ifst1, ifst2, compose_filter="auto", connect=True)

Constructively leniently-composes two FSTs.

This operation computes the lenient composition of two FSTs. The lenient
composition of two FSTs the priority union of their composition and the
left-hand side argument, where priority union is simply union in which the
left-hand side argument's relations have "priority" over the right-hand side
argument's relations.

Args:
  ifst: The input FST.
  sigma_star: A cyclic, unweighted acceptor representing the closure over the
      alphabet.
  compose_filter: A string matching a known composition filter; one of:
      "alt_sequence", "auto", "match", "null", "sequence", "trivial".
  connect: Should output be trimmed?

Returns:
  A leniently-composed FST.

Raises:
  FstOpError: Operation failed.
```

See also:
[LenientlyCompose algorithm](/opengrm/operators/README.md#leniently-compose).

```none
matches(ifst1, ifst2, compose_filter="auto")

Returns whether or not two FSTs "match" (have a non-empty composition).

This operation computes the composition of two FSTs, connects the result,
and then returns True iff the composition is non-empty (has a valid start
state); the resulting composition is then discarded. Normally the first
argument is a string and the second an acceptor, but many other sensible
configurations are possible.

Args:
  ifst1: The first input FST.
  ifst2: The second input FST.
  compose_filter: A string matching a known composition filter; one of:
      "alt_sequence", "auto", "match", "null", "sequence", "trivial".

Returns:
  True if the composition of ifst1 and ifst2 is non-null, else False.
```

See also:
[Compose algorithm](http://www.openfst.org/twiki/bin/view/FST/ComposeDoc).

```none
prune(ifst, delta=0.0009765625, nstate=-1, weight=None)

Constructively removes paths with weights below a certain threshold.

This operation deletes states and arcs in the input FST that do not belong
to a successful path whose weight is no more (w.r.t the natural semiring
order) than the threshold t \otimes-times the weight of the shortest path in
the input FST. Weights must be commutative and have the path property.

Args:
  ifst: The input FST.
  delta: Comparison/quantization delta.
  nstate: State number threshold.
  weight: A Weight or weight string indicating the desired weight threshold
      below which paths are pruned; if omitted, no paths are pruned.

Returns:
  A pruned FST.
```

See also: [Prune algorithm](http://openfst.org/twiki/bin/view/FST/PruneDoc).

```none
push(ifst, delta=0.0009765625, push_weights=False, push_labels=False,
     remove_common_affix=False, remove_total_weight=False, to_final=False)

Constructively pushes weights/labels towards initial or final states.

This operation produces an equivalent transducer by pushing the weights
and/or the labels towards the initial state or toward the final states.

When pushing weights towards the initial state, the sum of the weight of the
outgoing transitions and final weight at any non-initial state is equal to 1
in the resulting machine. When pushing weights towards the final states, the
sum of the weight of the incoming transitions at any state is equal to 1.
Weights need to be left distributive when pushing towards the initial state
and right distributive when pushing towards the final states.

Pushing labels towards the initial state consists in minimizing at every
state the length of the longest common prefix of the output labels of the
outgoing paths. Pushing labels towards the final states consists in
minimizing at every state the length of the longest common suffix of the
output labels of the incoming paths.

Args:
  ifst: The input FST.
  delta: Comparison/quantization delta.
  push_weights: Should weights be pushed?
  push_labels: Should labels be pushed?
  remove_common_affix: If pushing labels, should common prefix/suffix be
      removed?
  remove_total_weight: If pushing weights, should total weight be removed?
  to_final: Push towards final states?

Returns:
  An equivalent pushed FST.
```

See also: [Push algorithm](http://www.openfst.org/twiki/bin/view/FST/PushDoc).

```none
randgen(ifst, npath=1, seed=0, select="uniform", max_length=2147483647,
        weight=False, remove_total_weight=False)

Randomly generate successful paths in an FST.

This operation randomly generates a set of successful paths in the input FST.
This relies on a mechanism for selecting arcs, specified using the `select`
argument. The default selector, "uniform", randomly selects a transition
using a uniform distribution. The "log_prob" selector randomly selects a
transition w.r.t. the weights treated as negative log probabilities after
normalizing for the total weight leaving the state. In all cases, finality is
treated as a transition to a super-final state.

Args:
  ifst: The input FST.
  npath: The number of random paths to generate.
  seed: An optional seed value for random path generation; if zero, the
      current time and process ID is used.
  select: A string matching a known random arc selection type; one of:
      "uniform", "log_prob", "fast_log_prob".
  max_length: The maximum length of each random path.
  weighted: Should the output be weighted by path count?
  remove_total_weight: Should the total weight be removed (ignored when
      `weighted` is False)?

Returns:
  An FST containing one or more random paths.
```

See also:
[RandGen algorithm](http://www.openfst.org/twiki/bin/view/FST/RandGenDoc).

```none
replace(root, replacements, call_arc_labeling="neither",
        return_arc_labeling="neither", epsilon_on_replace=False,
        return_label=0)

Constructively replaces arcs in an FST with other FST(s).

This operation performs the dynamic replacement of arcs in one FST with
other FSTs, allowing for the definition of FSTs analogous to RTNs. The output
FST is the result of recursively replacing each arc in all input FSTs that
matches some "non-terminal" with a corresponding FST. More precisely, an arc
from state s to state d with nonterminal output label n in an input FST is
replaced by redirecting this "call" arc to the initial state of a copy of the
the replacement FST and then adding "return" arcs from each final state of
the replacement FST to d in the input FST. If there are cyclic dependencies
among the replacement rules, the resulting FST does not have a finite
expansion and an exception will be raised.

Args:
  root: The root FST.
  replacements: An iterable containing label/FST pairs. If the iterable
     implements .iteritems or .items, this is used to extract the pairs.
  call_arc_labeling: A string indicating which call arc labels should be
      non-epsilon. One of: "input" (default), "output", "both", "neither".
      This value is set to "neither" if epsilon_on_replace is True.
  return_arc_labeling: A string indicating which return arc labels should be
      non-epsilon. One of: "input", "output", "both", "neither" (default).
      This value is set to "neither" if epsilon_on_replace is True.
  epsilon_on_replace: Should call and return arcs be epsilon arcs? If True,
      this effectively overrides call_arc_labeling and return_arc_labeling,
      setting both to "neither".
  return_label: The integer label for return arcs.

Returns:
  A replaced FST.

Raises:
  KeyError: Nonterminal symbol not found.
  FstOpError: Operation failed.
```

See also
[Replace algorithm](http://www.openfst.org/twiki/bin/view/FST/ReplaceDoc).

```none
reverse(ifst, require_superinitial=True)

Constructively reverses an FST's transduction.

This operation reverses an FST. If A transduces string x to y with weight a,
then the reverse of A transduces the reverse of x to the reverse of y with
weight a.Reverse(). (Typically, a = a.Reverse() and Arc = RevArc, e.g.,
TropicalWeight and LogWeight.) In general, e.g., when the weights only form a
left or right semiring, the output arc type must match the input arc type.

Args:
  ifst: The input FST.
  require_superinitial: Should a superinitial state be created?

Returns:
  A reversed FST.
```

See also:
[Reverse algorithm](http://www.openfst.org/twiki/bin/view/FST/ReverseDoc).

```none
rmepsilon(ifst, connect=True, delta=0.0009765625, nstate=-1,
          queue_type="auto", reverse=False, weight=None)

Constructively removes epsilon transitions from an FST.

This operation removes epsilon transitions (those where both input and output
labels are epsilon) from an FST.

Args:
  ifst: The input FST.
  connect: Should output be trimmed?
  delta: Comparison/quantization delta.
  nstate: State number threshold.
  queue_type: A string matching a known queue type; one of: "auto", "fifo",
      "lifo", "shortest", "state", "top".
  reverse: Should epsilon transitions be removed in reverse order?
  weight: A string indicating the desired weight threshold; paths with
      weights below this threshold will be pruned.

Returns:
  An equivalent FST with no epsilon transitions.
```

See also:
[RmEpsilon algorithm](http://www.openfst.org/twiki/bin/view/FST/RmEpsilonDoc).

```none
shortestdistance(ifst, delta=0.0009765625, nstate=-1, queue_type="auto",
                 reverse=False)

Compute the shortest distance from the initial or final state.

This operation computes the shortest distance from the initial state (when
`reverse` is False) or from every state to the final state (when `reverse` is
True). The shortest distance from p to q is the \otimes-sum of the weights of
all the paths between p and q. The weights must be right (if `reverse` is
False) or left (if `reverse` is True) distributive, and k-closed (i.e., 1
\otimes x \otimes x^2 \otimes ... \otimes x^{k + 1} = 1 \otimes x \otimes x^2
\otimes ... \otimes x^k; e.g., TropicalWeight).

Args:
  ifst: The input FST.
  delta: Comparison/quantization delta.
  nstate: State number threshold (this is ignored if `reverse` is True).
  queue_type: A string matching a known queue type; one of: "auto", "fifo",
      "lifo", "shortest", "state", "top" (this is ignored if `reverse` is
      True).
  reverse: Should the reverse distance (from each state to the final state)
      be computed?

Returns:
  A list of Weight objects representing the shortest distance for each state.
```

See also:
[ShortestDistance algorithm](http://www.openfst.org/twiki/bin/view/FST/ShortestDistanceDoc).

```none
shortestpath(ifst, delta=0.0009765625, nshortest=1, nstate=-1,
             queue_type="auto", unique=False, weight=None)

Construct an FST containing the shortest path(s) in the input FST.

This operation produces an FST containing the n-shortest paths in the input
FST. The n-shortest paths are the n-lowest weight paths w.r.t. the natural
semiring order. The single path that can be read from the ith of at most n
transitions leaving the initial state of the resulting FST is the ith
shortest path. The weights need to be right distributive and have the path
property. They also need to be left distributive as well for n-shortest with
n > 1 (e.g., TropicalWeight).

Args:
  ifst: The input FST.
  delta: Comparison/quantization delta.
  nshortest: The number of paths to return.
  nstate: State number threshold.
  queue_type: A string matching a known queue type; one of: "auto", "fifo",
      "lifo", "shortest", "state", "top".
  unique: Should the resulting FST only contain distinct paths? (Requires
      the input FST to be an acceptor; epsilons are treated as if they are
      regular symbols.)
  weight: A Weight or weight string indicating the desired weight threshold
      below which paths are pruned; if omitted, no paths are pruned.

Returns:
  An FST containing the n-shortest paths.
```

See also:
[ShortestPath algorithm](http://www.openfst.org/twiki/bin/view/FST/ShortestPathDoc).

```none
state_map(ifst, map_type)

Constructively applies a transform to all states.

This operation transforms each state according to the requested map type.
Note that currently, only one state-mapping operation is supported.

Args:
  ifst: The input FST.
  map_type: A string matching a known mapping operation; one of: "arc_sum"
      (sum weights of identically-labeled multi-arcs), "arc_unique" (deletes
      non-unique identically-labeled multi-arcs).

Returns:
  An FST with states remapped.

Raises:
  FstArgError: Unknown map type.
```

See also:
[StateMap algorithm](http://www.openfst.org/twiki/bin/view/FST/StateMapDoc).

```none
synchronize(ifst)

Constructively synchronizes an FST.

This operation synchronizes a transducer. The result will be an equivalent
FST that has the property that during the traversal of a path, the delay is
either zero or strictly increasing, where the delay is the difference between
the number of non-epsilon output labels and input labels along the path. For
the algorithm to terminate, the input transducer must have bounded delay,
i.e., the delay of every cycle must be zero.

Args:
  ifst: The input FST.

Returns:
  An equivalent synchronized FST.
```

See also:
[Synchronize algorithm](http://www.openfst.org/twiki/bin/view/FST/SynchronizeDoc).

The following destructive methods of `Fst` also can be called as constructive
functions; like the above, these take one or more `Fst` arguments and return a
new `Fst`.

*   `arcsort`
*   `closure`
*   `concat`
*   `connect`
*   `decode` and `encode`
*   `invert`
*   `minimize`
*   `optimize`
*   `project`
*   `relabel_pairs`
*   `relabel_tables`
*   `reweight`
*   `topsort`

#### `Fst` binary operators

The following FST functions have binary operator aliases.

*   `compose`: `@`
*   `concat`: `+`
*   `closure`: `** n` (where `n` is an integer)
*   `union`: `|`

#### Context Managers

```none
default_token_type(token_type)
Override the default token_type used by Pynini functions and classes.

A context manager and context decorator to temporarily override the default
token_type used by Pynini functions and classes.

Args:
  token_type: A string indicating how the string is to be constructed from
      arc labels---one of: "byte" (interprets arc labels as raw bytes),
      "utf8" (interprets arc labels as Unicode code points), or a
      SymbolTable.

Returns:
  A context decorator that temporarily overrides the default token_type used
  by Pynini.
```

#### `Weight` arithmetic functions <a id="arithmetic"></a>

Weights of the same semiring also support the following arithmetic functions.
which return a new `Weight`.

```none
plus(lhs, rhs)

Computes the sum of two Weights in the same semiring.

This function computes lhs \oplus rhs, raising an exception if lhs and rhs
are not in the same semiring.

Args:
   lhs: left-hand side Weight.
   rhs: right-hand side Weight.

Returns:
  A Weight object.

Raises:
  FstArgError: Weight type not found (or not in same semiring).
  FstBadWeightError: Invalid weight.
```

```none
times(lhs, rhs)

Computes the product of two Weights in the same semiring.

This function computes lhs \otimes rhs, raising an exception if lhs and rhs
are not in the same semiring.

Args:
   lhs: left-hand side Weight.
   rhs: right-hand side Weight.

Returns:
  A Weight object.

Raises:
  FstArgError: Weight type not found (or not in same semiring).
  FstBadWeightError: Invalid weight.
```

```none
power(lhs, rhs)

Computes the iterated product of a weight.

Args:
   w: The weight.
   n: The power.

Returns:
  A Weight object.

Raises:
  FstArgError: Weight type not found (or not in same semiring).
  FstBadWeightError: Invalid weight.
```

```none
divide(lhs, rhs)

Computes the quotient of two Weights in the same semiring.

This function computes lhs \oslash rhs, raising an exception if lhs and rhs
are not in the same semiring. As there is no way to specify whether to use
left vs. right division, this assumes a commutative semiring in which these
are equivalent operations.

Args:
   lhs: left-hand side Weight.
   rhs: right-hand side Weight.

Returns:
  A Weight object.

Raises:
  FstArgError: Weight type not found (or not in same semiring).
  FstBadWeightError: Invalid weight.
```

### Exceptions <a id="exceptions"></a>

The following exceptions (all errors) are raised by Pynini:

*   `TypeError`: as used typically in python; used by Pynini to indicate that
    arguments are not the type expected by Pynini.
*   `FstError`: abstract base class for all exceptions raised by Pynini.
*   `FstArgError`: signals an error parsing input arguments (often in matching
    string arguments to underlying enum values).
*   `FstBadWeightError`: signals an error in constructing a weight.
*   `FstIndexError`: signals an attempt to access an out-of-range state ID.
*   `FstIOError`: signals an error in reading or writing from disk.
*   `FstOpError`: signals an error returned by an FST algorithm; usually, this
    is accompanied by a low-level logging statement.
*   `FstStringCompilationError`: signals an error compiling a string into an
    FST.

## Advanced topics <a id="advanced"></a>

### Getting the hang of iterators

The `Fst` class provides four types of iterators (plus `SymbolTableIterator`),
which are usually constructed by invoking the appropriate method of `Fst`. Some
examples are given below.

There are two ways to use the various iterators. First, they can be used
similarly to Python built-in iterators over file handles or containers by
placing them in a `for`-loop, but they also expose a lower-level interface used
as follows:

```python {.good}
while not iterator.done():
  # Do something with `iterator` in its current state.
  iterator.next()  # Advances the iterator.
```

Attempting to use an iterator which is "done" may result in a fatal error. To
return an iterator to the initial state, one can use the `reset` method.

The method `states` creates a `StateIterator`. When iterated over, it yields
state ID integers in strictly increasing order. For example, the following
function computes the number of arcs and states of an FST (a simple measure of
FST size).

```python {.good}
def size(f):
  """Computes the number of arcs and states in the FST."""
  return sum(1 + f.num_arcs(state) for state in f.states())
```

The method `arcs` returns an `_ArcIterator`. When iterated over, it yields all
arcs leaving some state of the FST. For example, the following function computes
the _in-degree_ (number of incoming states) for a given state ID.

```python {.good}
def indegree(f, state):
  """Computes in-degree of some state in an FST."""
  n = 0
  for s in f.states():
    for arc in f.arcs(s):
      n += (arc.nextstate == state)
  return n
```

The method `mutable_arcs` is similar to `arcs`, except that the
`_MutableArcIterator` it returns has a special method `set_value` which replaces
the arc at the iterator's current position. For example, the following function
changes input labels matching a certain label to epsilon (cf. the
`relabel_pairs` method).

```python {.good}
def map_ilabel_to_epsilon(f, ilabel):
  """Maps any input label matching `ilabel` to epsilon (0)."""
  for state in f.states():
    aiter = f.mutable_arcs(state)
    while not aiter.done():
      arc = aiter.value()
      if arc.ilabel == ilabel:
        arc.ilabel = 0
        aiter.set_value(arc)
      aiter.next()
  return f
```

The method `paths` returns a `_StringPathIterator` iterator. This provides
several ways to access the paths of an
[acyclic](https://en.wikipedia.org/wiki/Directed_acyclic_graph) FST. For each
path, one can access the input and output strings via the `istring` and
`ostring` methods, and the path weight via the `weight` method. One can also
access all input strings, all output strings, and all path weights using the
`istrings`, `ostrings`, and `weights` methods, which return generators; after
invoking any one of these methods, the caller must reset the iterator to reuse
it.

See in-module documentation strings for more information on iterators.

### FST properties <a id="properties"></a>

Each `Fst` has an associated set of stored properties which assert facts about
the FST. These can be queried by using the `properties` method. Some properties
are binary (either true or false), whereas others are ternary (true, false, or
unknown). The first argument to `properties` is a bitmask for the desired
property/properties, and the second argument indicates whether unknown
properties are to be computed.

Querying known properties is a constant time operation.

```python {.good}
# Optimizes the FST if it is *not known* to be deterministic.
if f.properties(pynini.I_DETERMINISTIC, False) != pynini.I_DETERMINISTIC:
  f.optimize()
```

Computing unknown properties is linear in the size of the FST in the worst case.

```python {.good}
# Optimizes the FST if it is not already deterministic and epsilon-free.
desired_props = pynini.I_DETERMINISTIC | pynini.NO_EPSILONS
if f.properties(desired_props, True) != desired_props:
  f.optimize()
```

An FST's properties can be set using the `set_properties` method.

For more information on FST properties, see the
[OpenFst docs](http://www.openfst.org/twiki/bin/view/FST/FstAdvancedUsage#Properties).

### Creating FSTs manually <a id="manual"></a>

`Fst` (which happens to be the constructor for the `Fst` class) creates an FST
with no states or arcs. Before this FST can be used, one must first add a state,
set it as initial, and set it or some other state as final.

```python {.good}
f = pynini.Fst()
assert not f.verify()  # OK.
state = f.add_state()
f.set_start(state)
f.set_final(state)
assert f.verify()      # OK.
```

In general, attempting to refer to a state before it has been added to the FST
will raise an `FstIndexError`.

```python {.bad}
f = pynini.Fst()
f.set_start(1)  # Not OK: Raises FstIndexError.
```

```python {.bad}
f = pynini.Fst()
f.set_final(1)  # Not OK: Raises FstIndexError.
```

```python {.bad}
f = pynini.Fst()
f.add_arc(1, Arc(65, 65, 0, f.start()))  # Not OK: Raises FstIndexError.
```

The one exception to this generalization is that one may add an arc to a
destination state not yet added to the FST, though the FST will not `verify`
until that state is added.

```python {.good}
f = pynini.Fst()
f.set_start(f.add_state())
f.add_arc(f.start(), pynini.Arc(65, 65, 0, 1))
assert not f.verify()  # OK, though arc has non-existent destination state ID.
state = f.add_state()  # OK, now that state exists.
f.set_final(state)
assert f.verify()      # OK.
```

### Pushdown transducers and multi-pushdown transducers

Pushdown transducers (PDTs) and multi-pushdown transducers (MPDTs) are two
less-commonly encountered generalizations of (W)FSTs.

PDTs represent the class of weighted context-free grammars, and are often used
to encode a set of phrase-structure grammar rules. A PDT can be thought of as an
WFST in which the machine's memory is augmented by a stack. In Pynini a PDT is
encoded as an WFST in which some transitions represent "push" or "pop"
operations (represented as open and close parentheses) rather than input and/or
output symbols. A path through the PDT is then valid if and only if it is a path
through the WFST and the open and close parenthese balance along that path.

MPDTs are a further generalization of PDTs in which there is more than one
stack. While these are computationally quite unconstrained, in speech and
language applications they are primarily used for modeling
[reduplication](https://en.wikipedia.org/wiki/Reduplication).

In Pynini, there is no one class representing a PDT or MPDT; rather they are
represented as a pair of an `Fst` and a table of parentheses labels. The
`PdtParentheses` object contains pairs of open/close parentheses labels, and the
`MPdtParentheses` object also contains an integer representing the identity of
the stack each parentheses pair is assigned to.

PDTs and MPDT support composition with an FST using the `pdt_compose` and
`mpdt_compose` functions, respectively. These functions take `Fst` arguments, a
parentheses table (`PdtParentheses` or `MPdtParentheses`, respectively), and a
specification of whether the right or the left argument should be interpreted as
a (M)PDT. Both functions return the FST component of an (M)PDT; the parentheses
table can be reused.

```python {.good}
(f, fparens) = ipdt
opdt = (pynini.pdt_compose(f, g, fparens), fparens)
```

The `pdt_expand` and `mpdt_expand` functions can be used to expand a PDT or MPDT
into an FST (assuming they have a finite expansion). They take an `Fst` and
parentheses table and return an `Fst`. The boolean `keep_parentheses` argument
species whether or not expansion should keep parentheses arcs; by default, they
are removed during expansion.

```python {.good}
(f, fparens) = ipdt
g = pynini.pdt_expand(f, fparens)
```

The `pdt_reverse` and `mpdt_reverse` functions can be used to reverse a PDT or
MPDT. Both take as arguments the two components of an (M)PDT. The `pdt_reverse`
function return the FST component of a PDT, whereas the `mpdt_function` also
returns a modified `MPdtParentheses` object.

```python {.good}
(f, fparen) = impdt
(g, gparens) = pynini.mpdt_reverse(f, fparens)
```

Finally, PDTs (but not MPDTs) provide a shortest-path operation using
`pdt_shortestpath`. Note that the result is an FST rather than a PDT.

```python {.good}
(f, fparens) = ipdt
sp = pynini.pdt_shortestpath(f, fparens)
```

## References

Gorman, Kyle. 2016. Pynini: A Python library for weighted finite-state grammar
compilation. In _Proceedings of the ACL Workshop on Statistical NLP and Weighted
Automata_, 75-80.

Gorman, Kyle, and Richard Sproat. 2016.
[How to get superior text processing in Python with Pynini](https://www.oreilly.com/ideas/how-to-get-superior-text-processing-in-python-with-pynini).
O'Reilly Ideas blog, accessed 11/16/16.

Grover, Dale L., Martin T. King, and Clifford A. Kushler. 1998. Reduced keyboard
disambiguating computer. US Patent 5,818,437.

Koskenniemi, Kimmo. 1983. Two-level morphology: A general computational model
for word-form recognition and production. Doctoral dissertation, University of
Helsinki.

Ringen, Catherine O. and Orvokki Heinämäki. 1999. Variation in Finnish vowel
harmony: An OT account. _Natural Language and Linguistic Theory_ 17(2): 303-337.

Roark, Brian, and Richard Sproat. 2007. _Computational approaches to syntax and
morphology_. Oxford: Oxford University Press.
