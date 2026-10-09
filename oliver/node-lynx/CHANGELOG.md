# CHANGELOG

## 0.1.4
- [Breaking] OpenCard requires a host-configured template directory / URL prefix allowlist. The CLI requires repeatable `--allow-template` entries, independently of any initial page; templates load on demand.
- Add `--unsafe-allow-any-template` for explicit unrestricted OpenCard access, with a startup warning.
- This entry-point mitigation does not restrict CDP navigation, scripts, or subresource loading and does not provide a complete server-side request forgery or local-file-access boundary.

## 0.1.3
1. [Feature] Support evaluating JavaScript in a view's BTS runtime with `evaluateScript`.
2. [Feature] Support sharing a BTS runtime across views with the same non-empty `groupName`.
3. [Feature] Support synchronous local JavaScript loading from configurable `resourceRootPaths`.

## 0.1.2
1. [Feature] Support process-wide Lynx log level configuration through LynxEnv and the CLI.

## 0.1.1
1. [Feature] Set default clientInfo in node-lynx for debug router
2. [Feature] Support headless CDP input events through Input.emulateTouchFromMouseEvent.

## 0.1.0
1. [Feature] Migrate node-lynx to the embedder windowless headless renderer with Clay software rendering support.
