# Vendored Python Dependencies

This directory contains the Python packages needed by the repository setup and
tooling paths that previously came from `tools/vpython_tools/requirements.txt`.

In the template-assembler repository this lives at `lynx/third_party/py_deps`.
When the `lynx/` directory is synced as the standalone Lynx repository, it lives
at `third_party/py_deps`.

The packages are loaded by adding this directory to `PYTHONPATH` from Lynx's
`tools/env.sh` / `tools/envsetup.*` scripts. Downstream repositories should
delegate to the Lynx env script instead of duplicating this path. Repository
setup no longer installs these packages from a network package index.

## Manual dependency updates

Run the update from the repository root. In template-assembler, set
`dest_dir=lynx/third_party/py_deps`; in the standalone Lynx repository, set
`dest_dir=third_party/py_deps`.

```bash
pkg="package-name==version"
dest_dir="lynx/third_party/py_deps"
workdir="$(mktemp -d)"

python3 -m pip download \
  --dest "$workdir/wheels" \
  --only-binary=:all: \
  --platform any \
  --implementation py \
  --python-version 3 \
  --abi none \
  "$pkg"

# This includes transitive dependencies. Every downloaded wheel must be pure
# Python. The command should print nothing.
find "$workdir/wheels" -type f ! -name '*-none-any.whl' -print

python3 -m pip install \
  --no-index \
  --find-links "$workdir/wheels" \
  --only-binary=:all: \
  --no-compile \
  --target "$workdir/site" \
  "$pkg"

# If updating existing packages, remove their old package directories and
# matching *.dist-info directories from "$dest_dir" before copying.
rsync -a "$workdir/site/" "$dest_dir/"
```

After copying, verify that no native or generated artifacts were vendored. These
commands should print nothing:

```bash
find "$dest_dir" -type f \
  \( -name '*.so' -o -name '*.pyd' -o -name '*.dll' -o -name '*.dylib' \
     -o -name '*.pyc' -o -name '*.pyo' \
     -o -name '*.c' -o -name '*.cc' -o -name '*.cpp' -o -name '*.h' \) \
  -print
find "$dest_dir" -type d -name '__pycache__' -print
```

Then run an import smoke test and update the full vendored package list below,
including any transitive dependencies:

```bash
tools/env.sh python3 -c "import package_name"
python3 -m pip list --path "$workdir/site" --format=freeze
```

Vendored packages:

- `beautifulsoup4==4.14.2`
- `bs4==0.0.2`
- `certifi==2026.1.4`
- `charset-normalizer==3.4.4`
- `doxmlparser==1.14.0`
- `idna==3.11`
- `jinja2==3.1.6`
- `json5==0.9.28`
- `markupsafe==3.0.3`
- `pyyaml==6.0.3`
- `qrcode==8.2`
- `requests==2.32.5`
- `six==1.17.0`
- `soupsieve==2.8`
- `typing-extensions==4.15.0`
- `urllib3==2.6.3`

Native extension artifacts (`*.so`, `*.pyd`, `*.dll`, `*.dylib`) are intentionally
not vendored here. `doxmlparser` falls back to Python's standard
`xml.etree.ElementTree` parser when `lxml` is unavailable; the Doxygen API
parser adds a small compatibility wrapper for lxml-only element attributes.
