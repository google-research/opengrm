# Pynini: Grammar compilation in Python

        ,ggggggggggg,
       dP"""88""""""Y8,
       Yb,  88      `8b
        `"  88      ,8P                        gg                  gg
            88aaaad8P"                         ""                  ""
            88"""""  gg     gg   ,ggg,,ggg,    gg    ,ggg,,ggg,    gg
            88       I8     8I  ,8" "8P" "8,   88   ,8" "8P" "8,   88
            88       I8,   ,8I  I8   8I   8I   88   I8   8I   8I   88
            88      ,d8b, ,d8I ,dP   8I   Yb,_,88,_,dP   8I   Yb,_,88,_
            88      P""Y88P"8888P'   8I   `Y88P""Y88P'   8I   `Y88P""Y8
                          ,d8I'
                        ,dP'8I
                       ,8"  8I
                       I8   8I
                       `8, ,8I
                        `Y8P"

## Getting started

Pynini is a Python extension library for compiling, optimizing, and applying
grammar rules into weighted finite-state transducers (WFSTs), pushdown
transducers (PDTs), and multi-pushdown transducers (MPDTs) backed by
[OpenFst](https://www.openfst.org).

To get started, see [the docs](https://github.com/google-research/opengrm/blob/main/opengrm/pynini/docs/index.md) or [pynini.opengrm.org](https://pynini.opengrm.org).

For some more context, see
[How to get superior text processing in Python with Pynini](https://www.oreilly.com/ideas/how-to-get-superior-text-processing-in-python-with-pynini)
and
[Pynini: A Python library for weighted finite-state grammar compilation](http://openfst.cs.nyu.edu/twiki/pub/GRM/Pynini/pynini-paper.pdf).

## Open-source installation

Pynini supports Python 3.11–3.15:

*   **PyPI (`x86_64` Linux `manylinux_2_28` wheels and source distribution)**:
    `pip install pynini`
*   **Conda-Forge (Linux, macOS, and Windows)**:
    `conda install -c conda-forge pynini`
*   **From source**: Clone
    [`google-research/opengrm`](https://github.com/google-research/opengrm) and
    run `pip install .` from the repository root (requires a C++17 compiler,
    CMake >= 3.22, and Python >= 3.11 headers). Both top-level imports
    (`import pynini`, `import pywrapfst`) and monorepo imports
    (`from opengrm.pynini import pynini`, `from openfst import pywrapfst`) are
    supported.

## Citation

If you use Pynini in your research, please cite:

> K. Gorman. 2016.
> [Pynini: A Python library for weighted finite-state grammar compilation](http://openfst.cs.nyu.edu/twiki/pub/GRM/Pynini/pynini-paper.pdf).
> In *Proc. ACL Workshop on Statistical NLP and Weighted Automata*, pages 75–80.
