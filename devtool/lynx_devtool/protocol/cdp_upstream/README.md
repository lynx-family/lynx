# Chrome DevTools Protocol Schema

`protocol.json` is a generated, vendored Chrome DevTools Protocol schema used by
`devtool/lynx_devtool/protocol/scripts/update_cdp_metadata.py` to classify
locally implemented Lynx DevTool CDP methods as standard CDP or Lynx-extension
CDP.

Generate the file with `devtool/lynx_devtool/protocol/scripts/update_cdp_upstream.py`.
Normal CI must not fetch it from the network.

Current source:

- Repository: `https://github.com/ChromeDevTools/devtools-protocol`
- Commit: `f6ba0b18c0e98a82f1182c9ff4da4e2002a86319`
- Commit date: `2026-07-08T05:32:02Z`
- Upstream files:
  - `json/browser_protocol.json`
  - `json/js_protocol.json`
- License: see `LICENSE`.

The vendored `protocol.json` is built by concatenating the two upstream
`domains` arrays and keeping the shared protocol `version`.

To update from a Git revision:

```bash
tools/env.sh python3 devtool/lynx_devtool/protocol/scripts/update_cdp_upstream.py \
  --write \
  --revision <devtools-protocol-commit>
```

To update from local upstream files:

```bash
tools/env.sh python3 devtool/lynx_devtool/protocol/scripts/update_cdp_upstream.py \
  --write \
  --browser-protocol [BROWSER_PROTOCOL_JSON] \
  --js-protocol [JS_PROTOCOL_JSON]
```
