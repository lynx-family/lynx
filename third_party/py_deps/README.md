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
