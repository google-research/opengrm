# Releasing Pynini wheels

This guide covers how Pynini `manylinux_2_28_x86_64` binary wheels and source
distributions (`sdist`) are built, tested, and published to TestPyPI and PyPI
without local Docker builds or developer-managed PyPI API tokens.

## Overview

On GitHub, `.github/workflows/wheels.yml` automates building, testing, and
publishing using `opengrm/pynini/Dockerfile` and `pyproject.toml`:

1.  **`warmup-cpp-cache`**: Compiles Python-independent C++ dependencies
    (Abseil, OpenFst, OpenGrm) once into the `wheel-building-env` image layer
    and transfers the BuildKit cache across shards via a run-scoped workflow
    artifact (`buildx-cache-cpp-x86_64`, 1-day retention) rather than cross-run
    GHA cache.
2.  **`build-and-test-wheels`**: Builds, repairs (`auditwheel`), and runs
    clean-container import smoke tests and Pynini's unit tests in parallel
    across all target Python runtimes (CPython 3.11–3.15 and PyPy 3.11).
3.  **`build-sdist` & `verify-dist`**: Builds the PEP 517 `sdist` with
    `uv build --sdist` and validates all collected wheels and the `sdist` using
    isolated `uvx` invocations of `auditwheel show` and `twine check --strict`
    (on `pynini-<version>` releases, also verifies artifact versions match the
    release tag).
4.  **`publish-testpypi` & `publish-pypi`**: On manual `workflow_dispatch`
    opt-in (on `main`) or `pynini-<version>` GitHub pre-releases, stages
    validated artifacts for [TestPyPI](https://test.pypi.org/project/pynini/);
    on published non-prerelease `pynini-<version>` GitHub Releases, stages
    validated artifacts for [PyPI](https://pypi.org/project/pynini/).

Pushes to `main` and pull requests touching Pynini, its OpenGrm C++
dependencies, or the wheel build configuration (`CMakeLists.txt`, `cmake/**`,
`pyproject.toml`, `.dockerignore`, `opengrm/pynini/**`,
`.github/workflows/wheels.yml`) run steps 1–3 automatically without publishing.

## Releasing a new version

Once one-time setup is complete, releasing a new version takes four steps:

1.  **Bump `__version__`**: Update `__version__` in `opengrm/pynini/pynini.pyx`
    (single source of truth; `pyproject.toml` reads it dynamically via regex)
    and land the commit on `main`.

2.  **(Optional) Dry-run on TestPyPI**:

    *   Open the [Pynini Wheels
        workflow](https://github.com/google-research/opengrm/actions/workflows/wheels.yml),
        click **Run workflow** on `main`, and check **`publish_to_testpypi`**
        (alternatively, publish a `pynini-<version>` GitHub pre-release).

    *   Smoke-test the uploaded wheel in a clean `x86_64` Linux environment
        (`--extra-index-url` resolves `absl-py` from PyPI):

        ```bash
        uv venv pynini-test
        uv pip install --python pynini-test/bin/python \
          --index-url https://test.pypi.org/simple/ \
          --extra-index-url https://pypi.org/simple/ \
          "pynini==<version>"
        pynini-test/bin/python -c \
          "import pynini, pywrapfst; print(pynini.__version__, pynini.accep('ok'))"
        ```

    *   Inspect the [TestPyPI project
        page](https://test.pypi.org/project/pynini/) to check package metadata,
        wheels, and README rendering.

    > WARNING: TestPyPI rejects re-uploading identical version filenames. If you
    > need to re-run a TestPyPI upload for the same `<version>`, delete the
    > existing release on TestPyPI first (or test with a pre-release suffix).

3.  **Publish to PyPI**: Create and publish a non-prerelease release on [GitHub
    Releases](https://github.com/google-research/opengrm/releases) using the tag
    format `pynini-<version>` (matching `__version__`). `wheels.yml`
    automatically builds and tests all 6 Python shards and the `sdist`, verifies
    the artifact version matches `<version>`, and publishes the release to PyPI
    (if `pypi` environment reviewers are configured, approve the deployment job
    when prompted).

4.  **Verify the release**: Inspect the [PyPI project
    page](https://pypi.org/project/pynini/) and confirm a clean install works on
    `x86_64` Linux:

    ```bash
    uv venv pynini-verify
    uv pip install --python pynini-verify/bin/python "pynini==<version>"
    pynini-verify/bin/python -c \
      "import pynini, pywrapfst; print(pynini.__version__, pynini.accep('ok'))"
    ```

## Local debugging and verification

To build, repair, run smoke and unit tests, and export `manylinux_2_28_x86_64`
wheels to `./wheelhouse` locally from an OpenGrm checkout root:

```bash
# All supported Python runtimes (CPython 3.11-3.15 and PyPy 3.11; default PYTHON_TAGS="*"):
docker buildx build -f opengrm/pynini/Dockerfile \
  --target=export-wheels \
  --output type=local,dest=./wheelhouse .

# Or a single Python runtime shard for faster iteration (e.g., CPython 3.12):
docker buildx build -f opengrm/pynini/Dockerfile \
  --target=export-wheels \
  --build-arg PYTHON_TAGS=cp312-cp312 \
  --output type=local,dest=./wheelhouse .
```

To validate the PEP 517 `sdist` and PyPI README metadata locally with `uv`:

```bash
uv build --sdist
uvx --from 'twine>=7,<8' twine check --strict dist/*.tar.gz
```

## Updating supported Python versions

When adding a new Python release or dropping an end-of-life runtime, update
these files together:

1.  **`opengrm/pynini/Dockerfile`**: Update the `rm -rf /opt/python/...` pruning
    list in the `common` stage.
2.  **`.github/workflows/wheels.yml`**: Update the `build-and-test-wheels`
    matrix and the minimum wheel count assertion (`test "${whl_count}" -ge ...`)
    in `verify-dist`.
3.  **`pyproject.toml`**: Update `requires-python` and
    `Programming Language :: Python :: ...` classifiers.
4.  **`README.md` and `opengrm/pynini/README.md`**: Update the documented
    supported Python version range.
