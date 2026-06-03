# FST rewriting

The rewrite manager's rewriting methods apply a single rule to an input string.
It only applies one rule at a time and (due to limitations in CLIF wrapping)
does not have complete support for PDT and MPDT rewriting.

The rule cascade's rewriting methods apply a cascade (ordered list) of rules to
an input string; this cascade is specified by calling the `set_rules` method. It
has full support for PDT and MPDT rewriting.

Both are initialized from a (standard-arc) FAR file, and both offer the
following methods:

*   `top_rewrite`: returns one optimal rewrite, raising a ValueError if
    composition fails; **Behavior in the case of ties is implementation-defined
    and may change at any time.**
*   `one_top_rewrite`: returns one optimal rewrite, raising a ValueError if
    composition fails or there is a tie. **This method protects against changes
    in the aforementioned implementation-defined behavior.**
*   `n_top_rewrites`: returns up to `n` top rewrites. **Behavior in the case of
    ties is implementation-defined and may change at any time.**
*   `top_rewrites`: returns all optimal rewrites (i.e., all rewrites with the
    same cost as the best rewrite. **Ordering is implementation-defined and may
    change at any time.**
*   `rewrites`: returns all rewrites. **Ordering is implementation-defined and
    may change at any time.**

Both also provide `matches*` methods which can be used to determine whether
rewrites produce any matching strings.
